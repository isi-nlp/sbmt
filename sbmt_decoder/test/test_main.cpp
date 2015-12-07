////////////////////////////////////////////////////////////////////////////////
///
///  for those that are curious:
///  inside auto_unit_test.hpp is a main() function that only gets instantiated
///  if BOOST_AUTO_TEST_MAIN is defined.
///  in this main() function, all of the tests that are declared using 
///  BOOST_AUTO_TEST_CASE(my_test) in the test_*.cpp files in this directory are
///  loaded into a test suite and executed.
///
////////////////////////////////////////////////////////////////////////////////
#define DEBUG_PRINT_MAIN
#include <graehl/shared/debugprint.hpp>
#define  BOOST_AUTO_TEST_MAIN
# undef BOOST_TEST_DYN_LINK
#include <boost/test/auto_unit_test.hpp>
