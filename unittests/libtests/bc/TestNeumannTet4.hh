// -*- C++ -*-
//
// ----------------------------------------------------------------------
//
//                           Brad T. Aagaard
//                        U.S. Geological Survey
//
// {LicenseText}
//
// ----------------------------------------------------------------------
//

/**
 * @file unittests/libtests/bc/TestNeumannTet4.hh
 *
 * @brief C++ TestNeumann object.
 *
 * C++ unit testing for Neumann for mesh with 3-D tet cells.
 */

#if !defined(pylith_bc_testneumanntet4_hh)
#define pylith_bc_testneumanntet4_hh

#include "TestNeumann.hh" // ISA TestNeumann

/// Namespace for pylith package
namespace pylith {
  namespace bc {
    class TestNeumannTet4;
  } // bc
} // pylith

/// C++ unit testing for Neumann for mesh with 3-D tet cells.
class pylith::bc::TestNeumannTet4 : public TestNeumann
{ // class TestNeumann

  // CPPUNIT TEST SUITE /////////////////////////////////////////////////
  CPPUNIT_TEST_SUB_SUITE( TestNeumannTet4, TestNeumann );
  CPPUNIT_TEST( testInitialize );
  CPPUNIT_TEST( testIntegrateResidual );
  CPPUNIT_TEST_SUITE_END();

  // PUBLIC METHODS /////////////////////////////////////////////////////
public :

  /// Setup testing data.
  void setUp(void);

}; // class TestNeumannTet4

#endif // pylith_bc_neumanntet4_hh


// End of file 
