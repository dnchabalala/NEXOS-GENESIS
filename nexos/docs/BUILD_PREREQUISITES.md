# NexOS build prerequisites

The NexOS kernel itself needs NASM and Python 3. Building the upstream
NetSurf source graph adds the following host-only tools and libraries:

* `gperf` — generates libhubbub's static HTML tree-builder tables from
  `element-type.gperf` during the development build.
* A C compiler, `make`, Git, and the system development packages selected by
  the NetSurf port configuration.

`gperf` is never copied into the ISO and is not a NexOS runtime interface.
The `make netsurf-config` target checks for it before installing the locked
NetSurf configuration. The source fetch target remains usable without it;
the check belongs at the point where libhubbub generation is requested.
