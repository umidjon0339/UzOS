#ifndef DEF_ENV_SANITIZE_H
#define DEF_ENV_SANITIZE_H

// Does the environment entry 'entry' reach the new program? It must be
// NAME=value (NAME: letters, digits, '_'), printable, and NAME must not
// be one that changes how a program is LOADED or RUN (LD_PRELOAD,
// LD_LIBRARY_PATH, IFS, BASH_ENV, ...). execve asks this for every
// entry. 1 = kept, 0 = dropped (logged).
int def_env_keep(const char *entry);

#endif
