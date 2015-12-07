# Process this file with autoconf to produce a configure script.
# note:  ./bootstrap has been provided to call the autotools in the right order

AC_PREREQ(2.57)
AC_INIT(decoder-utilities, 0.1, pust@isi.edu)

AC_CONFIG_AUX_DIR(config)

AC_CANONICAL_SYSTEM
AC_ARG_PROGRAM

AM_INIT_AUTOMAKE

AC_DISABLE_FAST_INSTALL

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

# Checks for libraries.
boost_version_string="1.33.0"
AX_BOOST_VERSION_CURRENT($boost_version_string)
if test $ax_boost_version_current = "no"; then
    AC_MSG_ERROR([this project requires a version of boost >= $boost_version_string])
fi

AX_BOOST_REGEX
if test $ax_boost_regex = "no"; then
    AC_MSG_ERROR([could not build against boost::regex.]
                 [if you think you have this library, try setting --with-boost-suffix, ]
                 [and make sure the compiler/linker/loader can find headers/libs])
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

SBMT_RULEREADER
if test $sbmt_rulereader = "no"; then
    AC_MSG_ERROR([Headers and/or libs for RuleReader were not found.  ]
                 [Please install RuleReader to somewhere we can find it.  ]  
                 [An installable RuleReader can be found at ]
                 [https://nlg0.isi.edu/svn/sbmt/trunk/RuleReader ]
                 [If you have installed it to a non-standard location, set ]
                 [CPPFLAGS = -I$header-location , ]
                 [LDFLAGS  = -L$lib-location , and ]
                 [LD_LIBRARY_PATH = $lib-location to find headers/libs.])
fi

SBMT_LIBSBMT
if test $sbmt_libsbmt = "no"; then
    AC_MSG_ERROR([Headers and/or libs for libsbmt were not found.  ]
                 [Please install libsbmt to somewhere we can find it.  ]  
                 [libsbmt can be found at ]
                 [https://nlg0.isi.edu/svn/sbmt/trunk/sbmt_decoder ]
                 [If you have installed it to a non-standard location, set ]
                 [CPPFLAGS = -I$header-location , ]
                 [LDFLAGS  = -L$lib-location , and ]
                 [LD_LIBRARY_PATH = $lib-location to find headers/libs.])
fi

AC_ARG_ENABLE(ngram,
              AC_HELP_STRING([--disable-ngram],
                             [disabling forces decoder to be trigram only]),
              if test "x$enableval" = "xno"; then
                AC_DEFINE(MINI_JUST_TRIGRAM,1)
                AC_MSG_CHECKING([MINI_JUST_TRIGRAM=1])
              elif test "x$enableval" = "xyes"; then
                AC_DEFINE(MINI_JUST_TRIGRAM,0)
                AC_MSG_CHECKING([MINI_JUST_TRIGRAM=0])
              fi,
              AC_DEFINE(MINI_JUST_TRIGRAM,0)
              AC_MSG_CHECKING([MINI_JUST_TRIGRAM=0])
              )

AC_OUTPUT(Makefile)

