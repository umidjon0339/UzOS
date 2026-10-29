# container_escape -> namespace_isolation  (concept)

**Attack (concept).** Break out of a container - a process boxed in by
namespaces, cgroups and dropped capabilities - to reach the host, via a
leaked capability, a shared mount, or a kernel bug.

**Defence (concept).** Namespaces narrow what a container sees (PIDs,
mounts, users, network); cgroups cap its resources; dropped capabilities
limit what it can do. UzOS has one flat system with none of these, so
there is nothing to escape - this records what a real system needs.
