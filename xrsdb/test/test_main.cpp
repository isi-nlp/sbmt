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

# define  BOOST_TEST_MAIN 
# ifdef BOOST_TEST_DYN_LINK
# warning "BOOST_TEST_DYN_LINK being unset"
# undef BOOST_TEST_DYN_LINK
# endif
# include <boost/test/unit_test.hpp>

