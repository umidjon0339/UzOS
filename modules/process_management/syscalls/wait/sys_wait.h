#ifndef SYS_WAIT_H
#define SYS_WAIT_H
/* sys_wait.h - wait(): block until any child finishes; return its pid. */
#define SYS_WAIT 62         // EAX=62
int sys_wait(void);
#endif
