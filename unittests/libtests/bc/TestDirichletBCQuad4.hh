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
 * @file unittests/libtests/bc/TestDirichletBCQuad4.hh
 *
 * @brief C++ TestDirichletBC object.
 *
 * C++ unit testing for DirichletBC for mesh with 1-D line cells.
 */

#if !defined(pylith_bc_testdirichletbcquad4_hh)
#define pylith_bc_testdirichletbcquad4_hh

#include "TestDirichletBC.hh" // ISA TestDirichletBC

/// Namespace for pylith package
namespace pylith {
  namespace bc {
    class TestDirichletBCQuad4;
  } // bc
} // pylith

/// C++ unit testing for DirichletBC for mesh with 2-D quad cells.
class pylith::bc::TestDirichletBCQuad4 : public TestDirichletBC
{ // class TestDirichletBC

  // CPPUNIT TEST SUITE /////////////////////////////////////////////////
  CPPUNIT_TEST_SUB_SUITE( TestDirichletBCQuad4, TestDirichletBC );
  CPPUNIT_TEST( testInitialize );
  CPPUNIT_TEST( testSetConstraintSizes );
  CPPUNIT_TEST( testSetConstraints );
  CPPUNIT_TEST( testSetField );
  CPPUNIT_TEST_SUITE_END();

  // PUBLIC METHODS /////////////////////////////////////////////////////
public :

  /// Setup testing data.
  void setUp(void);

}; // class TestDirichletBCQuad4

#endif // pylith_bc_dirichletbcquad4_hh


// End of file 