#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT(RuleReader, 1.0, wwang@usc.edu)

AC_CONFIG_AUX_DIR(config)

AC_CANONICAL_SYSTEM
AC_ARG_PROGRAM

AM_INIT_AUTOMAKE

AC_DISABLE_FAST_INSTALL

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

AX_BOOST_UNIT_TEST_FRAMEWORK
if test $ax_boost_unit_test_framework = "no"; then
    AC_MSG_WARN([could not build against boost unit test framework; you will not be able to 'make check']
                    [if you think you have this library, try setting --with-boost-suffix])
fi

AC_OUTPUT(Makefile src/Makefile test/Makefile include/Makefile)
