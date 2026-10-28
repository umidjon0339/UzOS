#ifndef DEF_VM_ISOLATION_H
#define DEF_VM_ISOLATION_H

// A hypervisor escape breaks OUT of a VM into the host. UzOS runs AS the
// guest under QEMU, so "escaping" would mean attacking QEMU itself -
// a real exploit we do not write. 1 marks this as a documented concept;
// the real defences (hypervisor patches, least privilege, VM isolation)
// live in the host, not the guest.
int def_guest_stays_in_its_box(void);

#endif
