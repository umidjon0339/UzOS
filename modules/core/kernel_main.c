/*
 * kernel_main.c - Where the kernel starts in C.
 *
 * boot/boot.s calls kernel_main() with a stack and the multiboot info.
 * kernel_main() brings up the OS one layer at a time; each layer only
 * uses the layers set up before it. Then it becomes the shell.
 *
 * Every step prints a boot log line (ui/core/bootlog):
 *
 *     [  OK  ] Physical memory           127 MB RAM, 30340 frames free
 *
 * Read kernel_main() at the bottom first: it is the table of contents.
 */

#include <stdint.h>

#include "multiboot.h"      // boot/

#include "gdt.h"            // arch/x86/
#include "idt.h"

#include "cpu_info.h"       // CPU management
#include "cpu_faults.h"
#include "tsc.h"

#include "vga.h"            // device drivers
#include "palette.h"
#include "font.h"
#include "pic.h"
#include "timer.h"
#include "keyboard.h"
#include "mouse.h"
#include "rtc.h"

#include "pmm.h"            // memory
#include "vmm.h"
#include "kheap.h"

#include "scheduler.h"      // processes
#include "syscall_handler.h"
#include "sys_sleep.h"

#include "registry.h"       // security
#include "catalog.h"
#include "integrity.h"

#include "ramdisk.h"        // filesystem
#include "modules/filesystem_management/core/procfs/procfs.h"     // /proc (procfs_init)

#include "modules/network_management/core/pci/pci.h"       // network: pci_init
#include "modules/network_management/core/e1000/e1000.h"   // e1000_init, e1000_register_eth0
#include "modules/network_management/core/loopback/loopback.h"   // loopback_init
#include "modules/network_management/core/dhcp/dhcp.h"           // dhcp_start, dhcp_lease
#include "modules/network_management/core/netdev/netdev.h"       // netdev_by_name
#include "modules/network_management/core/inet/inet.h"           // inet_format, inet_prefix
#include "modules/network_management/core/dns/dns.h"             // dns_init, dns_set_nameserver

#include "users.h"          // users
#include "session.h"

#include "ui.h"             // look and feel
#include "bootlog.h"
#include "logo.h"
#include "statusbar.h"
#include "format.h"

#include "shell.h"          // shell and the commands each module adds
#include "shell_commands.h"
#include "mem_commands.h"
#include "proc_commands.h"
#include "fs_commands.h"
#include "io_commands.h"
#include "cpu_commands.h"
#include "user_commands.h"
#include "device_commands.h"
#include "ui_commands.h"
#include "security_commands.h"
#include "modules/network_management/commands/net_commands.h"   // register_net_commands

#define KEY_POLL_MS 10          // the shell looks for a key every timer tick
#define DHCP_BOOT_WAIT_MS 3000  // how long boot waits for a DHCP answer

static char detail[64];     // the text after a boot log line

/* ---------- 1. CPU ---------- */

static void setup_cpu(void) {
    boot_begin("CPU");                  // who it is and what it can do (cpuid)
    cpu_info_init();
    format(detail, sizeof(detail), "%s, family %u model %u, %d features",
           cpu_info()->vendor, cpu_info()->family, cpu_info()->model,
           cpu_feature_present_count());
    boot_ok(detail);

    boot_begin("GDT and TSS");          // segments + the ring 0 stack
    init_gdt();
    boot_ok("kernel and user segments");

    boot_begin("IDT");                  // where each interrupt goes
    init_idt();
    boot_ok("256 interrupt vectors");

    boot_begin("CPU exceptions");       // what a fault means (cpu_management)
    cpu_faults_init();
    boot_ok("a ring 3 fault kills only that process");

    // IRQ 0-15 start on vectors 0-15, which clash with CPU exceptions.
    // Move them to 32-47.
    boot_begin("PIC");
    pic_remap(0x20, 0x28);
    boot_ok("IRQ 0-15 -> vectors 32-47");
}

/* ---------- 2. Devices ---------- */

static void setup_devices(void) {
    boot_begin("Console");              // already running: vga_init()
    format(detail, sizeof(detail), "%s, font %s, theme %s", vga_display()->name,
           font_name(font_current()), palette_current()->name);
    boot_ok(detail);

    boot_begin("PIT timer");
    init_timer(TIMER_HZ);
    format(detail, sizeof(detail), "%u ticks per second", timer_frequency());
    boot_ok(detail);

    boot_begin("PS/2 keyboard");
    init_keyboard();
    boot_ok("IRQ 1");

    boot_begin("PS/2 mouse");
    if (init_mouse())
        boot_ok("IRQ 12, wheel scrolls the console");
    else
        boot_skip("no mouse found: the shell works without it");

    boot_begin("Real-time clock");
    rtc_time_t now;
    if (rtc_read(&now)) {
        format(detail, sizeof(detail), "%d-%02d-%02d %02d:%02d:%02d", now.year,
               now.month, now.day, now.hour, now.minute, now.second);
        boot_ok(detail);
    } else {
        boot_skip("no valid date: the clock shows --:--");
    }
}

/* ---------- 3. Memory ---------- */

static void setup_memory(uint32_t multiboot_magic, multiboot_info_t *mb_info) {
    // The PMM needs the bootloader's memory map to know which RAM is free.
    boot_begin("Physical memory");
    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC)
        boot_fail("not started by a multiboot bootloader: no memory map");
    pmm_init(mb_info);
    format(detail, sizeof(detail), "%u MB RAM, %u frames free",
           pmm_get_max_blocks() * (PAGE_SIZE / 1024) / 1024, pmm_get_free_blocks());
    boot_ok(detail);

    boot_begin("Paging");
    vmm_init();
    boot_ok("on; the NULL page is unmapped");

    boot_begin("Kernel heap");
    kheap_init();
    format(detail, sizeof(detail), "%u MB for kmalloc", KHEAP_SIZE / (1024 * 1024));
    boot_ok(detail);
}

/* ---------- 4. Security ---------- */

static void setup_security(void) {
    boot_begin("Security lab");         // every module's attack/defence pairs
    catalog_init();
    format(detail, sizeof(detail), "%d attacks, %d defences",
           registry_attack_count(), registry_defence_count());
    boot_ok(detail);

    // Measure the kernel while it is surely still the one that was built.
    boot_begin("Kernel integrity");
    integrity_init();
    const integrity_region_t *code = integrity_region(0), *constants = integrity_region(1);
    format(detail, sizeof(detail), "SHA-256 of %u KB code, %u KB constants",
           (uint32_t)(code->end - code->start) / 1024,
           (uint32_t)(constants->end - constants->start) / 1024);
    boot_ok(detail);
}

/* ---------- 5. Shell ---------- */

static void setup_shell(void) {
    boot_begin("Shell");
    shell_init();

    // Each module adds its own commands.
    register_shell_commands();      // help, clear, echo, calc, snake, ...
    register_mem_commands();        // meminfo, heapinfo, pagewalk, memtest
    register_proc_commands();       // kill, spawn, exec, sleep, testuser
    register_fs_commands();         // ls, cd, cat, mkdir, ...
    register_io_commands();         // ioinfo
    register_cpu_commands();        // lscpu, ps, top, htop, nice, chrt, perf, ...
    register_user_commands();       // whoami, id, users, ...
    register_device_commands();     // date, devices, theme, mode, ...
    register_ui_commands();         // uzfetch, statusbar
    register_security_commands();   // seclab
    register_net_commands();        // lspci, ...
    format(detail, sizeof(detail), "%d commands", shell_command_count());
    boot_ok(detail);
}

/* ---------- 6. Filesystem ---------- */

static void setup_filesystem(void) {
    boot_begin("RAM disk");
    ramdisk_init();                 // in-memory disk with the root "/"
    boot_ok("mounted at /");

    boot_begin("/proc");            // live kernel files (filesystem_management/core/procfs)
    procfs_init();
    boot_ok("cpuinfo, stat, <pid>/status");

    // Needs the filesystem: every user gets a home directory.
    boot_begin("Users");
    users_init();
    format(detail, sizeof(detail), "%d accounts, passwords as salted SHA-256",
           users_count());
    boot_ok(detail);
}

/* ---------- 7. Processes ---------- */

static void setup_processes(void) {
    boot_begin("Scheduler");
    scheduler_init();               // idle task; kernel_main becomes a process too
    scheduler_start();              // the timer now drives task switches
    boot_ok("nice -> levels 1-7, FIFO/RR, aging");

    session_init();                 // kernel_main is the shell: its uid is "who you are"
}

/* ---------- 8. System calls ---------- */

static void setup_syscalls(void) {
    boot_begin("System calls");
    init_syscalls();
    boot_ok("int 0x80");
}

/* ---------- 9. Network ---------- */

static void setup_network(void) {
    boot_begin("PCI bus");          // find the network card (network_management/core/pci)
    format(detail, sizeof(detail), "%d devices", pci_init());
    boot_ok(detail);

    boot_begin("Network card");     // the e1000 driver (network_management/core/e1000)
    if (e1000_init()) {
        uint8_t mac[6];
        e1000_mac(mac);
        format(detail, sizeof(detail), "e1000, IRQ %u, MAC %02x:%02x:%02x:%02x:%02x:%02x",
               e1000_irq(), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        boot_ok(detail);
    } else {
        boot_skip("no e1000 card: only the loopback network works");
    }

    boot_begin("Interfaces");       // lo always, eth0 if the card works (core/netdev)
    loopback_init();
    boot_ok(e1000_register_eth0() ? "lo 127.0.0.1, eth0" : "lo 127.0.0.1");

    dns_init();                     // /etc/hosts, /etc/resolv.conf
}

// Needs interrupts and the timer: the answers arrive by IRQ.
static void setup_dhcp(void) {
    netdev_t *eth0 = netdev_by_name("eth0");
    if (!eth0)
        return;
    boot_begin("DHCP");                 // ask the network for an address (core/dhcp)
    dhcp_start(eth0);
    const dhcp_lease_t *lease = dhcp_lease();
    for (int ms = 0; ms < DHCP_BOOT_WAIT_MS && lease->state != DHCP_BOUND; ms += 10)
        sys_sleep(10);
    if (lease->state == DHCP_BOUND) {
        char ip[INET_ADDR_TEXT], router[INET_ADDR_TEXT];
        inet_format(lease->ip, ip);
        inet_format(lease->router, router);
        format(detail, sizeof(detail), "eth0 %s/%d, gateway %s", ip, inet_prefix(lease->netmask), router);
        boot_ok(detail);
        if (lease->dns)
            dns_set_nameserver(lease->dns);     // /etc/resolv.conf
    } else {
        boot_skip("no answer yet: try 'dhclient' later");
    }
}

/* ---------- Entry point ---------- */

void kernel_main(uint32_t multiboot_magic, multiboot_info_t *mb_info) {
    vga_init();                     // screen first, so every step can report
    logo_print_banner();

    setup_cpu();
    setup_devices();
    setup_memory(multiboot_magic, mb_info);
    setup_security();
    setup_shell();
    setup_filesystem();
    setup_processes();
    setup_syscalls();
    setup_network();

    // Everything is ready: let hardware interrupts in.
    boot_begin("Interrupts");
    asm volatile("sti");
    boot_ok("on (sti)");

    // Needs the timer ticking: count CPU cycles over 10 ticks.
    boot_begin("CPU clock");
    tsc_init();
    if (tsc_mhz()) {
        format(detail, sizeof(detail), "%u MHz; rdtsc is kernel-only (CR4.TSD)", tsc_mhz());
        boot_ok(detail);
    } else {
        boot_skip("no TSC: clock speed unknown");
    }

    setup_dhcp();

    // The status bar's daemon is a process, so it comes last.
    boot_begin("Status bar");
    statusbar_start(scheduler_get_current_process()->pid);
    boot_ok("statusd updates it twice a second");

    vga_print_str("\n");
    ui_info(0, "UzOS is ready. Log in, then type 'help' for the commands.");
    session_login_prompt();         // nobody uses the shell without a password
    shell_print_prompt();

    // From now on kernel_main is the shell: wait for a key, handle it.
    // Waiting means SLEEPING, not spinning: then the scheduler knows the
    // shell does not need the CPU, and its time counts as idle (cpustat).
    while (1) {
        char c = keyboard_pop_key();
        if (c)
            shell_handle_key(c);
        else
            sys_sleep(KEY_POLL_MS);
    }
}
