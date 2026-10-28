# hypervisor_escape -> vm_isolation  (concept)

**Attack (concept).** Break out of the virtual machine into the host,
usually through a bug in an emulated device or instruction. UzOS runs
**as the guest** under QEMU, so an escape would mean attacking QEMU
itself - a real exploit, out of scope for a teaching OS. Nothing is
launched.

**Defence (concept).** The boundary is kept by the host: hypervisor
patches, least privilege for the VM process, and strict device models.
A guest cannot and should not implement its own escape.
