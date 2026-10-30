#ifndef SYS_IOCTL_H
#define SYS_IOCTL_H
/*
 * sys_ioctl.h - ioctl(fd, cmd, arg): "do this device-specific thing".
 *
 * The catch-all for everything read/write cannot say. UzOS answers the
 * networking ones (Linux's numbers), on a socket descriptor:
 *
 *   FIONREAD      bytes waiting to be read            (arg: int*)
 *   FIONBIO       non-blocking on/off                 (arg: int*)
 *   SIOCGIF...    an interface's address, netmask, flags, MTU, MAC, index
 *   SIOCSIF...    change them (needs CAP_NET_ADMIN)   (arg: ifreq_t*)
 *
 * That is how the classic `ifconfig` talks to the kernel.
 */
#include <stdint.h>
#define SYS_IOCTL 165       // EAX=165, EBX=fd, ECX=cmd, EDX=arg

#define FIONREAD       0x541B
#define FIONBIO        0x5421
#define SIOCGIFFLAGS   0x8913
#define SIOCSIFFLAGS   0x8914
#define SIOCGIFADDR    0x8915
#define SIOCSIFADDR    0x8916
#define SIOCGIFNETMASK 0x891B
#define SIOCSIFNETMASK 0x891C
#define SIOCGIFMTU     0x8921
#define SIOCSIFMTU     0x8922
#define SIOCGIFHWADDR  0x8927
#define SIOCGIFINDEX   0x8933

#define IFNAMSIZ 16

// Linux's struct ifreq (32 bytes): a name, then ONE of these values.
typedef struct {
    char name[IFNAMSIZ];
    union {
        struct { uint16_t family; uint16_t port; uint32_t addr; uint8_t zero[8]; } addr;  // sockaddr_in
        struct { uint16_t family; uint8_t data[14]; } hwaddr;                             // the MAC in data[0..5]
        int16_t flags;
        int32_t value;                                                                    // MTU, index
    } u;
} ifreq_t;

// The answer (0 for most), or -errno: -ENOTTY (not a socket / unknown
// cmd), -ENODEV (no such interface), -EPERM (needs CAP_NET_ADMIN).
int sys_ioctl(int fd, uint32_t cmd, void *arg);
#endif
