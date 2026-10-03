# SDK host source and profile provenance

The host checkers, object exporters, package programs and profile definitions
were developed in Mainframe Lab and extracted into this SDK under the root
MIT licence. The selected `vmkernel` component source is also maintained here.
They are edited in `src/` and retained by Git history.

`archive/` preserves historical PDPCLIB and PDLD repair patches used by the
older bootstrap packaging route. It is not an upstream product source tree or
a source for new implementation changes. Maintained PDPCLIB and Classic Linker
source belongs to the separate z/PDOS repository. GCC and newlib have their
own component records; official binutils 2.47 is checked and built unchanged
from its upstream archive by the producer.
