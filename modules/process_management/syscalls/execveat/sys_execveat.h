#ifndef SYS_EXECVEAT_H
#define SYS_EXECVEAT_H
/*
 * sys_execveat.h - execveat(dirfd, path, argv, envp, flags): like
 * execve(), but 'path' is resolved relative to the directory 'dirfd'
 * (or is a file descriptor itself). Same STUB status as execve(): UzOS
 * has no in-place image replacement. Returns -1.
 */
#define SYS_EXECVEAT 68     // EAX=68, EBX=dirfd, ECX=path, EDX=argv, ESI=envp, EDI=flags
int sys_execveat(int dirfd, const char *path, char *const argv[],
                 char *const envp[], int flags);
#endif
