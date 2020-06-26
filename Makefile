ifneq (${KERNELRELEASE},)

# kbuild part of makefile
include Kbuild

else
# default makefile
#include Makefile.cmake
include Makefile.kernel

endif
