#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT(itg-binarizer, 1.0, pust@isi.edu)
AC_CONFIG_SRCDIR([binal.cc])
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE

AC_DISABLE_FAST_INSTALL

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
AC_PROG_INSTALL


# Checks for libraries.
SBMT_RULEREADER
if test $sbmt_rulereader = "no"; then
    AC_MSG_ERROR([Headers and/or libs for RuleReader were not found.  ]
                 [Please install RuleReader to somewhere we can find it.  ]  
                 [An installable RuleReader can be found at ]
                 [https://nlg0.isi.edu/svn/sbmt/trunk/RuleReader ]
                 [If you have installed it to a non-standard location, set ]
                 [CPPFLAGS, LDFLAGS, and LD_LIBRARY_PATH to find headers/libs.])
fi

AX_BOOST_PROGRAM_OPTIONS
if test $ax_boost_program_options = "no"; then
    AC_MSG_ERROR([could not build against boost::program_options.]
                 [if you think you have this library, try setting --with-boost-suffix, ]
                 [and make sure the compiler/linker/loader can find headers/libs])
fi

AX_BOOST_FILESYSTEM
if test $ax_boost_filesystem = "no"; then
    AC_MSG_ERROR([could not build against boost::filesystem.]
                 [if you think you have this library, try setting --with-boost-suffix, ]
                 [and make sure the compiler/linker/loader can find headers/libs])
fi

# Checks for header files.


# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE

AC_OUTPUT(Makefile)
