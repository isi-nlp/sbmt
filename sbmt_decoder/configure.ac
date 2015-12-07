# Process this file with autoconf to produce a configure script.
# note:  ./bootstrap has been provided to call the autotools in the right order


AC_PREREQ(2.57)
AC_INIT(sbmt, 0.1, pust@isi.edu)

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

AC_DEFINE(LM_NO_COMMON_LIB,1,)
AC_DEFINE(LM_NO_QT,1,)

# Checks for libraries.
boost_version_string="1.33.0"
AX_BOOST_VERSION_CURRENT($boost_version_string)
if test $ax_boost_version_current = "no"; then
    AC_MSG_ERROR([this project requires a version of boost >= $boost_version_string])
fi

AX_BOOST_UNIT_TEST_FRAMEWORK
if test $ax_boost_unit_test_framework = "no"; then
    AC_MSG_WARN([could not build against boost unit test framework; you will not be able to 'make check']
                    [if you think you have this library, try setting --with-boost-suffix])
fi

AX_BOOST_REGEX
if test $ax_boost_regex = "no"; then
    AC_MSG_ERROR([could not build against boost::regex.]
                 [if you think you have this library, try setting --with-boost-suffix, ]
                 [and make sure the compiler/linker/loader can find headers/libs])
fi

AX_BOOST_SIGNALS
if test $ax_boost_signals != "yes"; then
AC_MSG_ERROR([could not build against boost::signals.]
             [if you think you have this library, try setting --with-boost-suffix, ]
             [and make sure the compiler/linker/loader can find headers/libs])
fi

AX_BOOST_SERIALIZATION
if test $ax_boost_serialization != "yes"; then
AC_MSG_ERROR([could not build against boost::serialization.]
             [if you think you have this library, try setting --with-boost-suffix, ]
             [and make sure the compiler/linker/loader can find headers/libs])
fi

AX_BOOST_THREAD
if test $ax_boost_thread != "yes"; then
AC_MSG_ERROR([could not build against boost::thread.]
             [if you think you have this library, try setting --with-boost-suffix, ]
             [and make sure the compiler/linker/loader can find headers/libs.  ]
             [note: if using boost-suffix, it is likely you need to attach -mt])
fi

AX_BOOST_HASH
if test $ax_boost_hash != "yes"; then
    AC_MSG_ERROR([this project requires the boost::hash library])
fi

# Additional boost headers we need (should be redundant by the version 1.33 check)
# put non boost headers you need elsewhere
AC_CHECK_HEADERS([boost/multi_index_container.hpp \
                  boost/multi_index/hashed_index.hpp \
                  boost/operators.hpp],,
AC_MSG_ERROR(
[[not all expected boost headers could be found, but your boost version looks up-to-date.]
 [please check that none of the boost headers are missing on your system.  if everything] 
 [looks okay, please contact pust@isi.edu]]
))

SBMT_RULEREADER
if test $sbmt_rulereader = "no"; then
    AC_MSG_ERROR([Headers and/or libs for RuleReader were not found.  ]
                 [Please install RuleReader to somewhere we can find it.  ]  
                 [An installable RuleReader can be found at ]
                 [https://nlg0.isi.edu/svn/sbmt/trunk/RuleReader ]
                 [If you have installed it to a non-standard location, set ]
                 [CPPFLAGS, LDFLAGS, and LD_LIBRARY_PATH to find headers/libs.])
fi
     
# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_CXX_GNU_CXX_HASH
AC_CXX_STD_HASH

# Checks for library functions.
AC_HEADER_STDC


AC_OUTPUT(Makefile \
          3rdparty/Makefile \
          3rdparty/lw/Makefile \
          3rdparty/lw/Shared/Makefile \
          3rdparty/lw/Shared/Common/Makefile \
          3rdparty/lw/Shared/Core/Makefile \
          3rdparty/lw/Shared/Core/LangModel/Makefile \
          3rdparty/lw/Horse/Makefile \
          3rdparty/lw/Horse/App/Makefile \
          3rdparty/lw/Horse/App/LangModel/Makefile \
          src/Makefile \
          test/Makefile \
          include/Makefile )

