/*
 * sys_ioctl.c - ioctl() for sockets and network interfaces.
 *
 * Changing an interface goes through the same kernel calls as `ip`:
 * netdev_set_address (which also fixes the routes), netdev_set_up.
 *
 * Uses: socket_from_fd / socket_pending_bytes (core/socket), netdev_*
 *       (core/netdev), caps_caller_has (security_management), htonl /
 *       ntohl (core/inet), scheduler, memcpy.
 */
#include "modules/network_management/syscalls/ioctl/sys_ioctl.h"            // this file
#include "modules/network_management/core/socket/socket.h"                  // socket_from_fd, socket_pending_bytes
#include "modules/network_management/core/netdev/netdev.h"                  // netdev_by_name, netdev_set_*
#include "modules/network_management/core/inet/inet.h"                      // htonl, ntohl, NET_E*
#include "modules/security_management/core/caps/caps.h"                     // caps_caller_has, CAP_NET_ADMIN
#include "modules/process_management/core/process/process.h"                  // process_caller_uid
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process
#include "lib/string/string.h"                                              // memcpy, memset

#define NET_ENOTTY 25           // "inappropriate ioctl for this device"
#define NET_ENODEV 19           // no such interface
#define MTU_MIN    68
#define MTU_MAX    1500

static int interface_ioctl(uint32_t cmd, ifreq_t *req) {
    char name[IFNAMSIZ];
    memcpy(name, req->name, IFNAMSIZ);
    name[IFNAMSIZ - 1] = '\0';
    netdev_t *dev = netdev_by_name(name);
    if (!dev)
        return -NET_ENODEV;
    int setting = cmd == SIOCSIFFLAGS || cmd == SIOCSIFADDR || cmd == SIOCSIFNETMASK || cmd == SIOCSIFMTU;
    if (setting && !caps_caller_has(CAP_NET_ADMIN))
        return -NET_EPERM;

    uint32_t lock = netdev_lock();
    int r = 0;
    switch (cmd) {
    case SIOCGIFFLAGS:
        req->u.flags = (int16_t)(dev->flags | (netdev_running(dev) ? 0x40 : 0));   // 0x40 = IFF_RUNNING
        break;
    case SIOCSIFFLAGS:
        netdev_set_up(dev, req->u.flags & NETDEV_UP);
        if ((dev->flags & NETDEV_PROMISC) != (req->u.flags & NETDEV_PROMISC))
            r = netdev_set_promisc(dev, (req->u.flags & NETDEV_PROMISC) != 0, process_caller_uid());
        break;
    case SIOCGIFADDR:
    case SIOCGIFNETMASK:
        memset(&req->u.addr, 0, sizeof(req->u.addr));
        req->u.addr.family = AF_INET;
        req->u.addr.addr = htonl(cmd == SIOCGIFADDR ? dev->ip : dev->netmask);
        break;
    case SIOCSIFADDR:           // a new address keeps the old mask (or /24 if it had none)
        netdev_set_address(dev, ntohl(req->u.addr.addr), dev->netmask ? dev->netmask : 0xFFFFFF00);
        break;
    case SIOCSIFNETMASK:
        netdev_set_address(dev, dev->ip, ntohl(req->u.addr.addr));
        break;
    case SIOCGIFMTU:
        req->u.value = dev->mtu;
        break;
    case SIOCSIFMTU:
        if (req->u.value < MTU_MIN || req->u.value > MTU_MAX)
            r = -NET_EINVAL;
        else
            dev->mtu = (uint16_t)req->u.value;
        break;
    case SIOCGIFHWADDR:
        memset(&req->u.hwaddr, 0, sizeof(req->u.hwaddr));
        req->u.hwaddr.family = 1;               // ARPHRD_ETHER
        memcpy(req->u.hwaddr.data, dev->mac, 6);
        break;
    case SIOCGIFINDEX:
        req->u.value = dev->index;
        break;
    default:
        r = -NET_ENOTTY;
    }
    netdev_unlock(lock);
    return r;
}

int sys_ioctl(int fd, uint32_t cmd, void *arg) {
    process_t *self = scheduler_get_current_process();
    socket_t *s = socket_from_fd(self, fd);
    if (!s)
        return -NET_ENOTTY;                     // only sockets answer here
    switch (cmd) {
    case FIONREAD:
        *(int *)arg = socket_pending_bytes(s);
        return 0;
    case FIONBIO:
        s->nonblock = *(const int *)arg != 0;
        return 0;
    default:
        return interface_ioctl(cmd, (ifreq_t *)arg);
    }
}
