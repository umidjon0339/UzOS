# process_injection -> process_isolation  (concept)

**Attack (concept).** Write code or data into a running process's memory
(ptrace `POKETEXT`, `/proc/<pid>/mem`) and make it execute.

**Defence (process_isolation).** The real barrier is a separate address
space per process. UzOS walls ring 3 off from the kernel (the
`PAGE_USER` bit) but its kernel-mode tasks share one address space, so a
full demo is a concept here - the lesson is what complete isolation
requires.
