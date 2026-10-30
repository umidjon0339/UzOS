#ifndef SYS_WAITID_H
#define SYS_WAITID_H
/* sys_waitid.h - waitid(idtype, id, infop, options): wait for a child chosen by kind of id. */
#include <stdint.h>
#define SYS_WAITID 64       // EAX=64, EBX=idtype, ECX=id, EDX=infop (or 0), ESI=options
#define P_ALL   0           // any child
#define P_PID   1           // the child whose pid == id
#define P_PGID  2           // any child in process group id
#define P_PIDFD 3           // the child named by pidfd 'id'
#define WEXITED 4           // wait for children that ended (the only kind UzOS has)
#define WNOHANG_ID 1        // WNOHANG

// siginfo_t as waitid fills it (the first fields of Linux's).
typedef struct {
    int32_t  signo;         // SIGCHLD
    int32_t  error;
    int32_t  code;          // CLD_EXITED 1, CLD_KILLED 2, CLD_DUMPED 3
    uint32_t pid;
    uint32_t uid;
    int32_t  status;        // exit code, or the signal
} waitid_info_t;

int sys_waitid(int idtype, uint32_t id, waitid_info_t *infop, uint32_t options);
#endif
