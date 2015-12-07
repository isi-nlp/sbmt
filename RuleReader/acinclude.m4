AC_DEFUN([AX_BOOST_VERSION_NUMBER],[
    boost_lib_version_req=$1
    boost_lib_version_req_shorten=`expr $boost_lib_version_req : '\([[0-9]]*\.[[0-9]]*\)'`
    boost_lib_version_req_major=`expr $boost_lib_version_req : '\([[0-9]]*\)'`
    boost_lib_version_req_minor=`expr $boost_lib_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
    boost_lib_version_req_sub_minor=`expr $boost_lib_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
    if test "x$boost_lib_version_req_sub_minor" = "x" ;
        then boost_lib_version_req_sub_minor="0"     
    fi
    ax_boost_version_number=`expr $boost_lib_version_req_major \* 100000 \+  $boost_lib_version_req_minor \* 100 \+ $boost_lib_version_req_sub_minor`
])

# if boost exceeds version string sets boost_version_current="yes" otherwise "no"
AC_DEFUN([AX_BOOST_VERSION_CURRENT],[
    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS
    AC_MSG_CHECKING(whether or not version of boost headers exceed $1)
    AX_BOOST_VERSION_NUMBER($1)
    ax_boost_version_current="no"
    AC_COMPILE_IFELSE(
        AC_LANG_PROGRAM([[#include <boost/version.hpp>]
                             [#include <boost/static_assert.hpp>]],
                            [[BOOST_STATIC_ASSERT(BOOST_VERSION >= $ax_boost_version_number);]]
        ), ax_boost_version_current="yes", ax_boost_version_current="no")
    AC_MSG_RESULT($ax_boost_version_current)
])

# sets boost_suffix_str if called
AC_DEFUN([AX_BOOST_SUFFIX],[
    AC_ARG_WITH([boost-suffix],
                   AC_HELP_STRING([--with-boost-suffix=(gcc gcc-d gcc-mt mingw mingw-d mgw-mt etc)],
                      [If your boost libraries use a target suffix in name specify it with this argument]),
                   [ax_boost_suffix=$withval],
                   [ax_boost_suffix=""]
    )
])

AC_DEFUN([AX_BOOST_LIBNAME],[
    AC_REQUIRE([AX_BOOST_SUFFIX])
    if test "x$ax_boost_suffix" = "x"; then
        ax_boost_libname="boost_$1"
    else ax_boost_libname="boost_$1-$ax_boost_suffix"
    fi
])

AC_DEFUN([AX_BOOST_UNIT_TEST_FRAMEWORK],[
    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS
    AC_REQUIRE([AX_BOOST_SUFFIX])
    AX_BOOST_LIBNAME(unit_test_framework)
    ax_boost_unit_test_framework="no"
    AC_MSG_CHECKING(can i compile a boost unit test and link against $ax_lib)
    save_LIBS=$LIBS
    LIBS="$save_LIBS -l$ax_boost_libname"

    AC_TRY_RUN([#define BOOST_AUTO_TEST_MAIN
                  #include <boost/test/auto_unit_test.hpp>

                  BOOST_AUTO_TEST_CASE(foo_test)
                  {
                     BOOST_CHECK(true);
                  }],
                 [boost_unit_test_framework_lib=$ax_boost_libname 
                  ax_boost_unit_test_framework="yes"],
                 [boost_unit_test_framework_lib=""
                  ax_boost_unit_test_framework="no"]
    )
    LIBS=$save_LIBS 

    AC_MSG_RESULT($ax_boost_unit_test_framework)
    if test $ax_boost_unit_test_framework="yes"; then
        AC_DEFINE(HAVE_BOOST_UNIT_TEST_FRAMEWORK,,[define if the Boost Unit Test Framework is available])
        AC_SUBST(boost_unit_test_framework_lib)
    fi
    AC_LANG_RESTORE
])dnl

