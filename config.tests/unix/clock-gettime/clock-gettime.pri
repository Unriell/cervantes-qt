# clock_gettime() is implemented in librt on these systems
linux-*|glibc-*|hpux-*|solaris-*:LIBS_PRIVATE *= -lrt
