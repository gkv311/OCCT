# libpng

include (FindPkgConfig)
if (NOT PKG_CONFIG_FOUND)
  message (FATAL_ERROR "pkg-config not found")
endif()
pkg_check_modules (LIBPNG libpng16 REQUIRED IMPORTED_TARGET)
if (NOT LIBPNG_FOUND)
  message(FATAL_ERROR "libpng16 development libraries cannot be found")
endif()
