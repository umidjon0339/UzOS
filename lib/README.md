# lib — small C library for the kernel

The kernel is built with `-ffreestanding`, so there is no standard C
library. Anything the kernel needs from it, we write here. Each
part has its own folder:

```
lib/
├── string/   string.h  string.c   memset, memcpy, strlen, strcmp, strcpy, strncpy, strrchr
├── stdlib/   stdlib.h  stdlib.c   atoi, strtoul
├── format/   format.h  format.c   format(): a small snprintf (%s %c %d %u %x, widths)
└── args/     args.h    args.c     arg_next: split a command line into words
```

Code in `lib/` depends on nothing else in the kernel, so any part of
the OS can use it.
