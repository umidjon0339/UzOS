/*
 * cmd_ps.c - `ps`: a snapshot of the processes.
 *
 *   ps                  your processes:             PID TTY TIME CMD
 *   ps -e   (or -A)     every process
 *   ps -f               full: UID PID PPID C STIME TTY TIME CMD
 *   ps -L               one row per THREAD, with its LWP (thread id)
 *   ps aux              BSD style: USER PID %CPU %MEM VSZ RSS TTY STAT START TIME COMMAND
 *   ps -eo pid,comm,%cpu            choose the columns (ps -o help lists them)
 *   ps -p 7,8   ps -u guest         only these processes / this user's
 *   ps -e --sort=-%cpu              sorted, '-' = biggest first
 *
 * Without -L, ps shows one row per process: a thread group's other
 * threads (clone(CLONE_THREAD)) are hidden behind their leader.
 * Names go through def_safe_text, so a process name can never break the
 * table (the ui defence safe_text, attack output_spoof).
 *
 * Uses: the process table and the idle task (process_management),
 *       priority_* (core/priority), policy_short (core/policy),
 *       affinity_current_cpu (core/affinity), thread_count (in this file),
 *       users_name / users_find_name (user_management), pmm_get_max_blocks
 *       (memory_management), timer_* / rtc_* (device_management),
 *       def_safe_text (ui defence safe_text), ui_* (ui), arg_next /
 *       format / string (lib).
 */

#include "modules/cpu_management/commands/ps/cmd_ps.h"              // this command
#include "modules/cpu_management/core/priority/priority.h"          // priority_effective
#include "modules/cpu_management/core/policy/policy.h"              // policy_short, policy_is_realtime
#include "modules/cpu_management/core/affinity/affinity.h"          // affinity_current_cpu
#include "modules/process_management/core/scheduler/scheduler.h"    // process table, idle task
#include "modules/user_management/core/users/users.h"               // users_name, users_find_name
#include "modules/memory_management/core/pmm/pmm.h"                 // pmm_get_max_blocks
#include "modules/device_management/timer/timer.h"                  // timer_ticks, timer_frequency
#include "modules/device_management/rtc/rtc.h"                      // rtc_read
#include "modules/ui/security/defence/safe_text/def_safe_text.h"    // def_safe_text
#include "modules/ui/core/style/ui.h"                               // ui_*
#include "lib/args/args.h"                                          // arg_next
#include "lib/format/format.h"                                      // format
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/string/string.h"                                      // strcmp, strncmp, strlen

#define MAX_COLUMNS   16
#define SCREEN_WIDTH  79
#define STACK_KB      (PROCESS_STACK_SIZE / 1024)

/* ---------- The columns ps knows ---------- */

typedef enum {
    F_PID, F_PPID, F_LWP, F_TGID, F_NLWP, F_UID, F_USER, F_C, F_PCPU, F_PMEM,
    F_PRI, F_NI, F_CLS, F_RTPRIO, F_PSR, F_STAT, F_S, F_TIME, F_ETIME, F_STIME,
    F_TTY, F_VSZ, F_RSS, F_LVL, F_EFF, F_WAIT, F_COMM, F_COUNT
} field_t;

static const struct {
    const char *names;      // what -o accepts, '|' between synonyms
    const char *header;
    int width;              // 0 = the rest of the line (the command)
    int left;               // left-aligned
    const char *meaning;
} fields[F_COUNT] = {
    [F_PID]    = { "pid",              "PID",     7, 0, "process (thread group) id" },
    [F_PPID]   = { "ppid",             "PPID",    7, 0, "parent's pid" },
    [F_LWP]    = { "lwp|tid|spid",     "LWP",     7, 0, "thread id (gettid)" },
    [F_TGID]   = { "tgid",             "TGID",    7, 0, "thread group id (getpid)" },
    [F_NLWP]   = { "nlwp|thcount",     "NLWP",    4, 0, "threads in the group" },
    [F_UID]    = { "uid",              "UID",     5, 0, "user id" },
    [F_USER]   = { "user|euser",       "USER",    8, 1, "user name" },
    [F_C]      = { "c",                "C",       3, 0, "CPU % (integer)" },
    [F_PCPU]   = { "%cpu|pcpu",        "%CPU",    5, 0, "CPU % in the last second" },
    [F_PMEM]   = { "%mem|pmem",        "%MEM",    5, 0, "memory % (its stack)" },
    [F_PRI]    = { "pri",              "PRI",     3, 0, "priority: 19-nice, 40+rt for real-time" },
    [F_NI]     = { "ni|nice",          "NI",      3, 0, "nice value (- for real-time)" },
    [F_CLS]    = { "cls|class|policy", "CLS",     3, 1, "policy: TS FF RR B IDL" },
    [F_RTPRIO] = { "rtprio",           "RTPRIO",  6, 0, "real-time priority (- if none)" },
    [F_PSR]    = { "psr",              "PSR",     3, 0, "CPU it runs on" },
    [F_STAT]   = { "stat",             "STAT",    4, 1, "state + < (nice<0) N (nice>0) l (threads)" },
    [F_S]      = { "s|state",          "S",       1, 1, "state: R S Z" },
    [F_TIME]   = { "time|cputime",     "TIME",    8, 0, "CPU time hh:mm:ss" },
    [F_ETIME]  = { "etime",            "ELAPSED", 8, 0, "time since it started" },
    [F_STIME]  = { "stime|start",      "STIME",   5, 0, "clock time it started" },
    [F_TTY]    = { "tty|tt",           "TTY",     6, 1, "terminal: tty1, ? for kernel" },
    [F_VSZ]    = { "vsz",              "VSZ",     5, 0, "virtual KiB (its stack)" },
    [F_RSS]    = { "rss",              "RSS",     5, 0, "resident KiB (its stack)" },
    [F_LVL]    = { "lvl",              "LVL",     3, 0, "UzOS level 1-7 (from nice)" },
    [F_EFF]    = { "eff",              "EFF",     3, 0, "UzOS level + aging" },
    [F_WAIT]   = { "wait",             "WAIT",    5, 0, "ticks waited READY (aging)" },
    [F_COMM]   = { "comm|cmd|args|command|ucmd", "CMD", 0, 1, "process name" },
};

/* ---------- Small helpers (only ps uses them) ---------- */

// Threads alive in thread group 'tgid' (every clone(CLONE_THREAD) child
// has its parent's tgid). Only this file needs it, so it lives here.
static int thread_count(uint32_t tgid) {
    process_t *table = process_get_table();
    int n = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].tgid == tgid && table[i].state != PROCESS_STATE_FREE &&
            table[i].state != PROCESS_STATE_ZOMBIE)
            n++;
    return n;
}


static int matches(const char *names, const char *word) {
    int len = strlen(word);
    while (*names) {
        if (strncmp(names, word, len) == 0 && (names[len] == '|' || names[len] == '\0'))
            return 1;
        while (*names && *names != '|')
            names++;
        if (*names == '|')
            names++;
    }
    return 0;
}

static int field_find(const char *word) {
    for (int f = 0; f < F_COUNT; f++)
        if (matches(fields[f].names, word))
            return f;
    return -1;
}

static char state_letter(const process_t *p) {
    switch (p->state) {
        case PROCESS_STATE_RUNNING:
        case PROCESS_STATE_READY:    return 'R';
        case PROCESS_STATE_SLEEPING: return 'S';
        case PROCESS_STATE_ZOMBIE:   return 'Z';
        default:                     return '?';
    }
}

static void clock_text(uint32_t seconds, char *out, int size) {
    format(out, size, "%02u:%02u:%02u", seconds / 3600, seconds / 60 % 60, seconds % 60);
}

// The wall-clock time (HH:MM) a process started.
static void start_text(const process_t *p, char *out, int size) {
    rtc_time_t now;
    uint32_t hz = timer_frequency() ? timer_frequency() : 1;
    if (!rtc_read(&now)) {
        format(out, size, "?");
        return;
    }
    int32_t ago = (timer_ticks - p->start_tick) / hz;
    int32_t day_s = now.hour * 3600 + now.minute * 60 + now.second - ago;
    while (day_s < 0)
        day_s += 24 * 3600;
    format(out, size, "%02d:%02d", day_s / 3600, day_s / 60 % 60);
}

// The text of field 'f' for process 'p'.
static void field_text(int f, const process_t *p, char *out, int size) {
    uint32_t hz = timer_frequency() ? timer_frequency() : 1;
    uint32_t cpu_s = (p->user_ticks + p->kernel_ticks) / hz;
    uint32_t total_kb = pmm_get_max_blocks() * (PAGE_SIZE / 1024);
    int rt = policy_is_realtime(p->policy);

    switch (f) {
        case F_PID:    format(out, size, "%u", p->tgid ? p->tgid : p->pid); break;
        case F_PPID:   format(out, size, "%u", p->ppid); break;
        case F_LWP:    format(out, size, "%u", p->pid); break;
        case F_TGID:   format(out, size, "%u", p->tgid); break;
        case F_NLWP:   format(out, size, "%d", thread_count(p->tgid ? p->tgid : p->pid)); break;
        case F_UID:    format(out, size, "%u", p->uid); break;
        case F_USER:   users_name(p->uid, out, size); break;
        case F_C:      format(out, size, "%u", p->cpu_percent); break;
        case F_PCPU:   format(out, size, "%u.0", p->cpu_percent); break;
        case F_PMEM: {
            uint32_t tenths = total_kb ? STACK_KB * 1000 / total_kb : 0;
            format(out, size, "%u.%u", tenths / 10, tenths % 10);
            break;
        }
        case F_PRI:    format(out, size, "%d", rt ? 40 + p->rt_priority : 19 - p->nice); break;
        case F_NI:     format(out, size, rt ? "-" : "%d", p->nice); break;
        case F_CLS:    format(out, size, "%s", policy_short(p->policy)); break;
        case F_RTPRIO: format(out, size, rt ? "%d" : "-", p->rt_priority); break;
        case F_PSR:    format(out, size, "%d", affinity_current_cpu()); break;
        case F_STAT:
            format(out, size, "%c%s%s", state_letter(p),
                   p->nice < 0 || rt ? "<" : p->nice > 0 ? "N" : "",
                   thread_count(p->tgid ? p->tgid : p->pid) > 1 ? "l" : "");
            break;
        case F_S:      format(out, size, "%c", state_letter(p)); break;
        case F_TIME:   clock_text(cpu_s, out, size); break;
        case F_ETIME: {
            uint32_t s = (timer_ticks - p->start_tick) / hz;
            format(out, size, "%02u:%02u", s / 60, s % 60);
            break;
        }
        case F_STIME:  start_text(p, out, size); break;
        case F_TTY:    format(out, size, "%s", p->is_system ? "?" : "tty1"); break;
        case F_VSZ:
        case F_RSS:    format(out, size, "%u", STACK_KB); break;
        case F_LVL:    format(out, size, "%d", p->priority); break;
        case F_EFF:    format(out, size, "%d", priority_effective(p)); break;
        case F_WAIT:   format(out, size, "%u", p->waited_ticks); break;
        default:       def_safe_text(p->name, out, size - 1); break;
    }
}

// A number to sort by (text fields sort by their first letters).
static int32_t sort_key(int f, const process_t *p) {
    char text[32];
    switch (f) {
        case F_PCPU: case F_C: return p->cpu_percent;
        case F_TIME:   return p->user_ticks + p->kernel_ticks;
        case F_NI:     return p->nice;
        case F_PRI:    return policy_is_realtime(p->policy) ? 40 + p->rt_priority : 19 - p->nice;
        case F_PPID:   return p->ppid;
        case F_LWP:    return p->pid;
        case F_UID:    return p->uid;
        case F_EFF:    return priority_effective(p);
        case F_WAIT:   return p->waited_ticks;
        case F_ETIME: case F_STIME: return -(int32_t)p->start_tick;
        case F_USER: case F_COMM: case F_CLS: case F_STAT: case F_S: case F_TTY:
            field_text(f, p, text, sizeof(text));
            return (text[0] << 16) | (text[1] << 8) | text[2];
        default:       return p->tgid ? p->tgid : p->pid;
    }
}

/* ---------- Choosing and printing ---------- */

typedef struct {
    int columns[MAX_COLUMNS];
    int column_count;
    int all;                    // -e: every process (else only the caller's user)
    int threads;                // -L: every thread
    uint32_t pids[8];           // -p: only these
    int pid_count;
    int uid;                    // -u: only this user (-1 = any)
    int sort_field;             // -1 = table order
    int sort_descending;
} ps_options_t;

static void set_columns(ps_options_t *o, const int *list, int n) {
    for (int i = 0; i < n && i < MAX_COLUMNS; i++)
        o->columns[i] = list[i];
    o->column_count = n;
}

static int wanted(const ps_options_t *o, const process_t *p, uint32_t my_uid) {
    if (p->state == PROCESS_STATE_FREE)
        return 0;
    if (!o->threads && p->tgid && p->tgid != p->pid && process_get_by_pid(p->tgid))
        return 0;                           // a thread, hidden behind its leader
    if (o->pid_count) {
        for (int i = 0; i < o->pid_count; i++)
            if (o->pids[i] == p->pid || o->pids[i] == p->tgid)
                return 1;
        return 0;
    }
    if (o->uid >= 0)
        return p->uid == (uint32_t)o->uid;
    return o->all || p->uid == my_uid;
}

static void spaces(int n) {
    for (int i = 0; i < n; i++)
        vga_print_str(" ");
}

#define COMMAND_WIDTH 15     // the command column when it is not the last one

static int width_of(int f) {
    return fields[f].width ? fields[f].width : COMMAND_WIDTH;
}

// One cell: padded to the column's width, right-aligned unless 'left'.
static void print_cell(int f, const char *text, int last) {
    int pad = width_of(f) - (int)strlen(text);
    if (!fields[f].left)
        spaces(pad);
    vga_print_str(text);
    if (last)
        return;
    if (fields[f].left)
        spaces(pad);
    vga_print_str(" ");
}

static void print_table(const ps_options_t *o) {
    process_t *table = process_get_table();
    process_t *rows[MAX_PROCESSES];
    int count = 0;
    uint32_t my_uid = process_caller_uid();

    for (int i = 0; i < MAX_PROCESSES; i++)
        if (wanted(o, &table[i], my_uid))
            rows[count++] = &table[i];

    // Insertion sort: at most 64 rows.
    if (o->sort_field >= 0) {
        for (int i = 1; i < count; i++) {
            process_t *p = rows[i];
            int32_t key = sort_key(o->sort_field, p);
            int j = i - 1;
            while (j >= 0 && (o->sort_descending ? sort_key(o->sort_field, rows[j]) < key
                                                 : sort_key(o->sort_field, rows[j]) > key)) {
                rows[j + 1] = rows[j];
                j--;
            }
            rows[j + 1] = p;
        }
    }

    ui_color(UI_TITLE);
    for (int c = 0; c < o->column_count; c++)
        print_cell(o->columns[c], fields[o->columns[c]].header, c == o->column_count - 1);
    vga_print_str("\n");
    ui_reset();

    for (int r = 0; r < count; r++) {
        int used = 0;
        for (int c = 0; c < o->column_count; c++) {
            int f = o->columns[c];
            char text[48];
            int last = c == o->column_count - 1;
            int room = last ? SCREEN_WIDTH - used : COMMAND_WIDTH;
            field_text(f, rows[r], text, fields[f].width ? (int)sizeof(text)
                                                         : (room > 1 ? room + 1 : 2));
            print_cell(f, text, last);
            used += width_of(f) + 1;
        }
        vga_print_str("\n");
    }
}

static void print_field_help(void) {
    for (int f = 0; f < F_COUNT; f++) {
        ui_color(UI_LINK);
        ui_printf("%-28s", fields[f].names);
        ui_color(UI_DIM);
        ui_printf("%s\n", fields[f].meaning);
    }
    ui_reset();
}

// "pid,comm,%cpu" -> columns. 0 if a name is unknown.
static int parse_fields(ps_options_t *o, const char *list) {
    char name[24];
    int n = 0;
    o->column_count = 0;
    while (*list) {
        n = 0;
        while (*list && *list != ',' && n < (int)sizeof(name) - 1)
            name[n++] = *list++;
        name[n] = '\0';
        if (*list == ',')
            list++;
        int f = field_find(name);
        if (f < 0 || o->column_count >= MAX_COLUMNS)
            return 0;
        o->columns[o->column_count++] = f;
    }
    return o->column_count > 0;
}

static void parse_pids(ps_options_t *o, const char *list) {
    while (*list && o->pid_count < 8) {
        o->pids[o->pid_count++] = atoi(list);
        while (*list && *list != ',')
            list++;
        if (*list == ',')
            list++;
    }
}

void cmd_ps(const char *args) {
    static const int plain[] = { F_PID, F_TTY, F_TIME, F_COMM };
    static const int full[]  = { F_UID, F_PID, F_PPID, F_C, F_STIME, F_TTY, F_TIME, F_COMM };
    static const int bsd[]   = { F_USER, F_PID, F_PCPU, F_PMEM, F_VSZ, F_RSS, F_TTY,
                                 F_STAT, F_STIME, F_TIME, F_COMM };
    ps_options_t o = { .all = 0, .threads = 0, .uid = -1, .sort_field = -1 };
    int is_full = 0, custom = 0, is_bsd = 0;
    char word[64];

    set_columns(&o, plain, 4);
    for (args = arg_next(args, word, sizeof(word)); word[0];
         args = arg_next(args, word, sizeof(word))) {
        if (strcmp(word, "aux") == 0 || strcmp(word, "ax") == 0) {
            is_bsd = o.all = 1;
        } else if (strncmp(word, "--sort=", 7) == 0) {
            const char *name = word + 7;
            o.sort_descending = *name == '-';
            if (*name == '-' || *name == '+')
                name++;
            o.sort_field = field_find(name);
        } else if (word[0] == '-' && word[1] != '-') {
            // Options may be glued: -ef, -eo, -eLf
            for (int i = 1; word[i]; i++) {
                char c = word[i];
                if (c == 'e' || c == 'A') {
                    o.all = 1;
                } else if (c == 'f') {
                    is_full = 1;
                } else if (c == 'L') {
                    o.threads = 1;
                } else if (c == 'o' || c == 'p' || c == 'u') {
                    char value[64];
                    const char *v = word + i + 1;   // -o pid,comm  or  -opid,comm
                    if (!*v) {
                        args = arg_next(args, value, sizeof(value));
                        v = value;
                    }
                    if (c == 'o') {
                        if (strcmp(v, "help") == 0) {
                            print_field_help();
                            return;
                        }
                        if (!parse_fields(&o, v)) {
                            ui_error("ps", "unknown column (ps -o help lists them)");
                            return;
                        }
                        custom = 1;
                    } else if (c == 'p') {
                        parse_pids(&o, v);
                    } else {
                        const user_t *u = users_find_name(v);
                        o.uid = u ? (int)u->uid : atoi(v);
                    }
                    break;                  // the rest of the word was the value
                } else {
                    ui_usage("ps [-e] [-f] [-L] [aux] [-o FIELDS|help] [-p PIDS] [-u USER] [--sort=-FIELD]");
                    return;
                }
            }
        } else {
            ui_usage("ps [-e] [-f] [-L] [aux] [-o FIELDS|help] [-p PIDS] [-u USER] [--sort=-FIELD]");
            return;
        }
    }

    if (!custom) {
        if (is_bsd)
            set_columns(&o, bsd, 11);
        else if (is_full)
            set_columns(&o, full, 8);
        if (o.threads) {                    // -L: the thread id after PID (and NLWP with -f)
            int at = is_bsd ? 2 : is_full ? 3 : 1;
            for (int i = o.column_count; i > at; i--)
                o.columns[i] = o.columns[i - 1];
            o.columns[at] = F_LWP;
            o.column_count++;
        }
    }
    print_table(&o);
}
