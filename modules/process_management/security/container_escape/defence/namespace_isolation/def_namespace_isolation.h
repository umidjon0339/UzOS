#ifndef DEF_NAMESPACE_ISOLATION_H
#define DEF_NAMESPACE_ISOLATION_H

// A container is a process whose view of the system (PIDs, mounts, users,
// network) is narrowed by namespaces. UzOS has no namespaces or
// containers, so there is nothing to escape FROM; this documents the
// defence a real system needs. 1 = documented concept.
int def_namespaces_present(void);

#endif
