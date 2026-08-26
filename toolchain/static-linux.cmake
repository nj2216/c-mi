set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_C_COMPILER gcc CACHE FILEPATH "Static toolchain C compiler")
set(CMAKE_CXX_COMPILER g++ CACHE FILEPATH "Static toolchain C++ compiler")

set(CMI_STATIC_PREFIX "/opt/cmi-static" CACHE PATH "Static dependency prefix")
set(CMAKE_PREFIX_PATH "${CMI_STATIC_PREFIX}/qt;${CMI_STATIC_PREFIX}" CACHE STRING "Static package prefixes")
# Prefer static archives, but fall back to shared libraries for things like
# GL/X11 that never ship a static archive (GL must dlopen the vendor driver
# at runtime, so it can't be linked statically anyway).
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".so")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static-libgcc -static-libstdc++")

set(PKG_CONFIG_USE_STATIC_LIBS TRUE)
set(ENV{PKG_CONFIG_PATH} "${CMI_STATIC_PREFIX}/lib/pkgconfig:${CMI_STATIC_PREFIX}/share/pkgconfig")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
