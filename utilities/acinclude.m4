AC_DEFUN([AC_CXX_NAMESPACES],
[AC_CACHE_CHECK(whether the compiler implements namespaces,
                   ac_cv_cxx_namespaces,
                   [AC_LANG_SAVE
                    AC_LANG_CPLUSPLUS
                    AC_TRY_COMPILE([namespace Outer { namespace Inner { int i = 0; }}],
                    [using namespace Outer::Inner; return i;],
                    ac_cv_cxx_namespaces=yes, ac_cv_cxx_namespaces=no)
                    AC_LANG_RESTORE
                   ]
 )
if test "$ac_cv_cxx_namespaces" = yes; then
  AC_DEFINE(HAVE_NAMESPACES,,[define if the compiler implements namespaces])
fi
])

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
    AC_REQUIRE([AC_CXX_NAMESPACES])
    AX_BOOST_LIBNAME(unit_test_framework)
    ax_boost_unit_test_framework="no"
    AC_MSG_CHECKING(can i compile with boost::unit_test_framework and link against $ax_boost_libname)
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

AC_DEFUN([AX_BOOST_HASH],[
    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS
    ax_boost_hash="no"
    AC_COMPILE_IFELSE(
        AC_LANG_PROGRAM(
            [[#include <boost/functional/hash.hpp>]]
          , [[boost::hash<int> hasher; int val = hasher(12345); return 0;]]
        )
      , [ax_boost_hash="yes"]
      , [ax_boost_hash="no"]
    )
    if test ax_boost_hash = "yes"; then
        AC_DEFINE(HAVE_BOOST_HASH,,[define if the boost::hash library is available])
    fi
    AC_LANG_RESTORE
])

AC_DEFUN([AX_BOOST_REGEX],[
    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS
    AC_REQUIRE([AX_BOOST_SUFFIX])
    AC_REQUIRE([AC_CXX_NAMESPACES])
    AX_BOOST_LIBNAME(regex)
    ax_boost_regex="no"
    AC_MSG_CHECKING(can i compile with boost::regex and link against $ax_boost_libname)
    save_LIBS=$LIBS
    LIBS="$save_LIBS -l$ax_boost_libname"

    AC_TRY_RUN([#include <boost/regex.hpp>
                int main() { boost::regex r; return 0; }]
               ,
               [boost_regex_lib=$ax_boost_libname 
                ax_boost_regex="yes"],
               [boost_regex_lib=""
                ax_boost_regex="no"]
    )
    LIBS=$save_LIBS 

    AC_MSG_RESULT($ax_boost_regex)
    if test $ax_boost_regex="yes"; then
        AC_DEFINE(HAVE_BOOST_REGEX,,[define if boost::regex is available])
        AC_SUBST(boost_regex_lib)
    fi
    AC_LANG_RESTORE
])dnl

AC_DEFUN([AX_BOOST_PROGRAM_OPTIONS],[
    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS
    AC_REQUIRE([AX_BOOST_SUFFIX])
    AC_REQUIRE([AC_CXX_NAMESPACES])
    AX_BOOST_LIBNAME(program_options)
    ax_boost_program_options="no"
    AC_MSG_CHECKING(can i compile with boost::program_options and link against $ax_boost_libname)
    save_LIBS=$LIBS
    LIBS="$save_LIBS -l$ax_boost_libname"

    AC_TRY_RUN([#include <boost/program_options.hpp>
                int main() { boost::program_options::options_description d(""); return 0; }]
               ,
               [boost_program_options_lib=$ax_boost_libname 
                ax_boost_program_options="yes"],
               [boost_program_options_lib=""
                ax_boost_program_options="no"]
    )
    LIBS=$save_LIBS 

    AC_MSG_RESULT($ax_boost_program_options)
    if test $ax_boost_program_options="yes"; then
        AC_DEFINE(HAVE_BOOST_PROGRAM_OPTIONS,,[define if boost::program_options is available])
        AC_SUBST(boost_program_options_lib)
    fi
    AC_LANG_RESTORE
])dnl

AC_DEFUN([AX_BOOST_FILESYSTEM],[
    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS
    AC_REQUIRE([AX_BOOST_SUFFIX])
    AC_REQUIRE([AC_CXX_NAMESPACES])
    AX_BOOST_LIBNAME(filesystem)
    ax_boost_filesystem="no"
    AC_MSG_CHECKING(can i compile with boost::filesystem and link against $ax_boost_libname)
    save_LIBS=$LIBS
    LIBS="$save_LIBS -l$ax_boost_libname"

    AC_TRY_RUN([#include <boost/filesystem/path.hpp>
                int main() { boost::filesystem::path p("/"); return 0; }]
               ,
               [boost_filesystem_lib=$ax_boost_libname 
                ax_boost_filesystem="yes"],
               [boost_filesystem_lib=""
                ax_boost_filesystem="no"]
    )
    LIBS=$save_LIBS 

    AC_MSG_RESULT($ax_boost_filesystem)
    if test $ax_boost_filesystem="yes"; then
        AC_DEFINE(HAVE_BOOST_FILESYSTEM,,[define if boost::filesystem is available])
        AC_SUBST(boost_filesystem_lib)
    fi
    AC_LANG_RESTORE
])dnl


AC_DEFUN([AX_BOOST_SERIALIZATION],[
    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS
    AC_REQUIRE([AX_BOOST_SUFFIX])
    AC_REQUIRE([AC_CXX_NAMESPACES])
    AX_BOOST_LIBNAME(serialization)
    ax_boost_serialization="no"
    AC_MSG_CHECKING(can i compile with boost::serialization and link against $ax_boost_libname)
    save_LIBS=$LIBS
    LIBS="$save_LIBS -l$ax_boost_libname"

    AC_TRY_RUN([#include <boost/archive/text_oarchive.hpp>
                #include <sstream>
                using namespace std;
                using namespace boost;
                using namespace boost::archive;
                int main() { stringstream str; text_oarchive oa(str); return 0; }]
               ,
               [boost_serialization_lib=$ax_boost_libname 
                ax_boost_serialization="yes"],
               [boost_serialization_lib=""
                ax_boost_serialization="no"]
    )
    LIBS=$save_LIBS 

    AC_MSG_RESULT($ax_boost_serialization)
    if test $ax_boost_serialization="yes"; then
        AC_DEFINE(HAVE_BOOST_SERIALIZATION,,[define if boost::serialization is available])
        AC_SUBST(boost_serialization_lib)
    fi
    AC_LANG_RESTORE
])dnl

AC_DEFUN([AX_BOOST_SIGNALS],[
    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS
    AC_REQUIRE([AX_BOOST_SUFFIX])
    AC_REQUIRE([AC_CXX_NAMESPACES])
    AX_BOOST_LIBNAME(signals)
    ax_boost_signals="no"
    AC_MSG_CHECKING(can i compile with boost::signals and link against $ax_boost_libname)
    save_LIBS=$LIBS
    LIBS="$save_LIBS -l$ax_boost_libname"

    AC_TRY_RUN([#include <boost/signals.hpp>
                #include <iostream>
                struct foo { void operator()() {std::cout << "signals work";} };
                int main() { 
                boost::signal<void (void)> sig;
                sig.connect(foo());
                sig();
                return 0; }]
               ,
               [boost_signals_lib=$ax_boost_libname 
                ax_boost_signals="yes"],
               [boost_signals_lib=""
                ax_boost_signals="no"]
    )
    LIBS=$save_LIBS 

    AC_MSG_RESULT($ax_boost_signals)
    if test $ax_boost_signals="yes"; then
        AC_DEFINE(HAVE_BOOST_SIGNALS,,[define if boost::signals is available])
        AC_SUBST(boost_signals_lib)
    fi
    AC_LANG_RESTORE
])dnl

AC_DEFUN([SBMT_RULEREADER],[
    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS
    sbmt_rulereader="no"
    save_LIBS=$LIBS
    LIBS="$LIBS -lRuleReader"
    AC_MSG_CHECKING(can i find and use RuleReader headers/libs)
    AC_TRY_RUN(
        [#include <RuleReader/Rule.h>
         int main() { ns_RuleReader::Rule R; return 0; }]
      , sbmt_rulereader="yes"
      , sbmt_rulereader="no"
    )
    LIBS=$save_LIBS
    if test $sbmt_rulereader="yes"; then
        AC_DEFINE(HAVE_RULEREADER,,[define if RuleReader is available])
    fi
    AC_MSG_RESULT($sbmt_rulereader)
    AC_LANG_RESTORE
])

AC_DEFUN([SBMT_LIBSBMT],[
    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS
    sbmt_libsbmt="no"
    save_LIBS=$LIBS
    LIBS="$LIBS -lsbmt"
    AC_MSG_CHECKING(can i find and use libsbmt)
    AC_TRY_RUN(
        [#include <sbmt/token/fat_token.hpp>
         int main() { sbmt::fat_token tok = sbmt::tag("NP"); return 0; }]
      , sbmt_libsbmt="yes"
      , sbmt_libsbmt="no"
    )
    LIBS=$save_LIBS
    if test $sbmt_libsbmt="yes"; then
        AC_DEFINE(HAVE_LIBSBMT,1,[define if libsbmt is available])
    fi
    AC_MSG_RESULT($sbmt_libsbmt)
    AC_LANG_RESTORE
])

AC_DEFUN([AC_CXX_GNU_CXX_HASH],[
AC_CACHE_CHECK(whether the compiler supports __gnu_cxx::hash_map and __gnu_cxx::hash_set,
ac_cxx_gnu_cxx_hash,
[AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_COMPILE([
    #include <ext/hash_map>
    #include <ext/hash_set>
    using __gnu_cxx::hash_map; 
    using __gnu_cxx::hash_set;
 ],
 [],
 ac_cxx_gnu_cxx_hash=yes, ac_cxx_gnu_cxx_hash=no)
 AC_LANG_RESTORE
])
if test "$ac_cxx_gnu_cxx_hash" = yes; then
  AC_DEFINE(
    HAVE_GNU_CXX_HASH,,
    [define if the compiler supports __gnu_cxx::hash_map and __gnu_cxx::hash_set]
  )
fi
])

AC_DEFUN([AC_CXX_STD_HASH],[
AC_CACHE_CHECK(whether the compiler supports std::hash_map and std::hash_set from ext/,
ac_cxx_std_hash,
[AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_COMPILE([
    #include <ext/hash_map>
    #include <ext/hash_set>
    using std::hash_map; 
    using std::hash_set;
 ],
 [],
 ac_cxx_std_hash=yes, ac_cxx_std_hash=no)
 AC_LANG_RESTORE
])
if test "$ac_cxx_std_hash" = yes; then
  AC_DEFINE(
    HAVE_STD_HASH,,
    [define if the compiler supports std::hash_map and std::hash_set]
  )
fi
])

