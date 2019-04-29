// -*- C++ -*-
//
// ----------------------------------------------------------------------
//
// Brad T. Aagaard, U.S. Geological Survey
// Charles A. Williams, GNS Science
// Matthew G. Knepley, University of Chicago
//
// This code was developed as part of the Computational Infrastructure
// for Geodynamics (http://geodynamics.org).
//
// Copyright (c) 2010-2017 University of California, Davis
//
// See COPYING for license information.
//
// ----------------------------------------------------------------------
//

/**
 * @file unittests/libtests/topology/TestSubMesh.hh
 *
 * @brief C++ unit testing for Mesh.
 */

#if !defined(pylith_topology_testsubmesh_hh)
#define pylith_topology_testsubmesh_hh

// Include directives ----------------------------------------------------------
#include <cppunit/extensions/HelperMacros.h>

#include "pylith/topology/topologyfwd.hh" // forward declarations
#include "pylith/utils/types.hh" // USES PylithScalar

// Forward declarations --------------------------------------------------------
/// Namespace for pylith package
namespace pylith {
    namespace topology {
        class TestSubMesh;
        class TestSubMesh_Data;
    } // topology
} // pylith

// TestSubMesh -----------------------------------------------------------------
/// C++ unit testing for Mesh.
class pylith::topology::TestSubMesh : public CppUnit::TestFixture {
    // CPPUNIT TEST SUITE //////////////////////////////////////////////////////
    CPPUNIT_TEST_SUITE(TestSubMesh);

    CPPUNIT_TEST(testConstructor);
    CPPUNIT_TEST(testConstructorMesh);
    CPPUNIT_TEST(testAccessors);
    CPPUNIT_TEST(testSizes);

    CPPUNIT_TEST_SUITE_END();

    // PUBLIC METHODS //////////////////////////////////////////////////////////
public:

    /// Setup testing data.
    void setUp(void);

    /// Deallocate testing data.
    void tearDown(void);

    /// Test constructor.
    void testConstructor(void);

    /// Test constructor w/mesh.
    void testConstructorMesh(void);

    /// Test coordsys(), debug(), comm().
    void testAccessors(void);

    /// Test dimension(), numCorners(), numVertices(), numCells(),
    void testSizes(void);

    // PROTECTED METHODS ///////////////////////////////////////////////////////
protected:

    // Build lower dimension mesh.
    void _buildMesh(void);

    // PROTECTED MEMBERS ///////////////////////////////////////////////////////
protected:

    TestSubMesh_Data* _data; ///< Data for testing.
    Mesh* _mesh; ///< Mesh holding lower dimension mesh.
    Mesh* _submesh; ///< Test subject, lower dimension mesh.

}; // class TestSubMesh

// TestSubMesh_Data-------------------------------------------------------------
class pylith::topology::TestSubMesh_Data {
    // PUBLIC METHODS //////////////////////////////////////////////////////////
public:

    /// Constructor
    TestSubMesh_Data(void);

    /// Destructor
    ~TestSubMesh_Data(void);

    // PUBLIC MEMBERS //////////////////////////////////////////////////////////
public:

    // GENERAL, VALUES DEPEND ON TEST CASE

    /// @defgroup Domain mesh information.
    /// @{
    int cellDim; ///< Cell dimension (matches space dimension).
    int numVertices; ///< Number of vertices.
    int numCells; ///< Number of cells.
    int numCorners; ///< Number of vertices per cell.
    int* cells; ///< Array of vertices in cells [numCells*numCorners].
    PylithScalar* coordinates; ///< Coordinates of vertices [numVertices*cellDim].
    const char* label; ///< Label of group associated with submesh.
    int groupSize; ///< Number of vertices in group.
    int* groupVertices; ///< Array of vertices in group.
    /// @}

    /// @defgroup SubMesh information.
    /// @{
    int submeshNumCorners; ///< Number of vertices per cell.
    int submeshNumVertices; ///< Number of vertices in submesh.
    int* submeshVertices; ///< Vertices in submesh.
    int submeshNumCells; ///< Number of cells in submesh.
    int* submeshCells; ///< Array of vertices in cells [submeshNumCells*submeshNumCorners].
    /// @}

}; // TestSubMesh_Data

#endif // pylith_topology_testsubmesh_hh

// End of file
