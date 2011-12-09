#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([kytea], [0.4.0], [kytea@phontron.com])
AC_CONFIG_SRCDIR([src/lib/kytea.cpp])
AC_CONFIG_HEADERS([src/include/kytea/config.h])
AC_CONFIG_FILES([
    Makefile
    src/Makefile
    src/include/Makefile
    src/lib/Makefile
    src/lib/liblinear/Makefile
    src/lib/liblinear/blas/Makefile
    src/bin/Makefile
    src/test/Makefile
    src/api/Makefile
    data/Makefile
])

# disable shared libraries
AC_PROG_LIBTOOL

# Intialize automake
AM_INIT_AUTOMAKE([-Wall])

# set CFLAGS and CXXFLAGS for maximal optimization
if test -n "${CXXFLAGS}"; then
  user_set_cxxflags=yes
fi
AC_PROG_CXX
if test X$user_set_cxxflags != Xyes; then
  CXXFLAGS="-g -Wall -O3"
fi
if test -n "${CFLAGS}"; then
  user_set_cflags=yes
fi
AC_PROG_CC
if test X$user_set_cflags != Xyes; then
  CFLAGS="-g -Wall -O3"
fi


# Checks for features.
AC_ARG_ENABLE(quantize,
  [  --enable-quantize       Quantize the model, resulting in smaller but possibly less accurate models)],
  [], [enable_quantize=yes])
if test "x$enable_quantize" == xno; then
    AC_DEFINE([DISABLE_QUANTIZE], [1], [Disable quantizing])
else
    AC_DEFINE([DISABLE_QUANTIZE], [0], [Enable quantizing])
fi


# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# Check to make sure that we have unordered_map
AC_LANG([C++])
AC_CHECK_HEADERS([tr1/unordered_map])
AC_CHECK_HEADERS([ext/hash_map])

AC_OUTPUT
