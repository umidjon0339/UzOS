#ifndef DEF_ORPHAN_REAPING_H
#define DEF_ORPHAN_REAPING_H

// On Linux an orphan (its parent died) is re-parented to init (pid 1) so
// someone still reaps it. UzOS does the same job a simpler way: a global
// reaper collects EVERY zombie by its state, whether or not its parent
// is still alive. 1 if a process in state 'is_zombie' will be collected
// regardless of 'parent_alive' - so an orphan is never leaked.
int def_orphan_is_collected(int is_zombie, int parent_alive);

#endif
