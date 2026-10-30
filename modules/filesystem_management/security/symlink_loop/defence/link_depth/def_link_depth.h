#ifndef DEF_LINK_DEPTH_H
#define DEF_LINK_DEPTH_H

#define LINK_HOPS_MAX 8         // symbolic links followed in one path walk (Linux: 40)

// The walk is about to follow its 'hops'-th symbolic link. Too many is a
// loop (a -> b -> a) or a chain built to waste time: ELOOP. vfs_lookup
// asks this before every link it follows. 1 = go on, 0 = refused (logged).
int def_link_hop_ok(int hops);

#endif
