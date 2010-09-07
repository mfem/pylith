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
// Copyright (c) 2010 University of California, Davis
//
// See COPYING for license information.
//
// ----------------------------------------------------------------------
//

#include <portinfo>

#include "TestFieldsNewMesh.hh" // Implementation of class methods

#include "pylith/topology/Mesh.hh" // USES Mesh
#include "pylith/topology/Field.hh" // USES Field
#include "pylith/topology/FieldsNew.hh" // USES FieldsNew

#include "pylith/meshio/MeshIOAscii.hh" // USES MeshIOAscii

// ----------------------------------------------------------------------
CPPUNIT_TEST_SUITE_REGISTRATION( pylith::topology::TestFieldsNewMesh );

// ----------------------------------------------------------------------
typedef pylith::topology::FieldsNew<pylith::topology::Mesh> FieldsNewMesh;
typedef pylith::topology::Mesh::RealSection RealSection;
typedef pylith::topology::Mesh::SieveMesh SieveMesh;

// ----------------------------------------------------------------------
void
pylith::topology::TestFieldsNewMesh::setUp(void)
{ // setUp
  _mesh = new Mesh;
  meshio::MeshIOAscii importer;
  importer.filename("data/tri3.mesh");
  importer.read(_mesh);
} // setUp

// ----------------------------------------------------------------------
void
pylith::topology::TestFieldsNewMesh::tearDown(void)
{ // tearDown
  delete _mesh; _mesh = 0;
} // tearDown

// ----------------------------------------------------------------------
// Test constructor.
void
pylith::topology::TestFieldsNewMesh::testConstructor(void)
{ // testConstructor
  CPPUNIT_ASSERT(0 != _mesh);
  FieldsNewMesh fields(*_mesh);
} // testConstructor
 
// ----------------------------------------------------------------------
// Test hasField().
void
pylith::topology::TestFieldsNewMesh::testHasField(void)
{ // testHasField
  CPPUNIT_ASSERT(0 != _mesh);
  FieldsNewMesh fields(*_mesh);

  fields.add("field A", "velocity", 3, FieldBase::VECTOR);
  
  CPPUNIT_ASSERT_EQUAL(true, fields.hasField("field A"));
  CPPUNIT_ASSERT_EQUAL(false, fields.hasField("field B"));
  CPPUNIT_ASSERT_EQUAL(false, fields.hasField("field C"));

  fields.add("field B", "displacement", 3, FieldBase::VECTOR);

  CPPUNIT_ASSERT_EQUAL(true, fields.hasField("field A"));
  CPPUNIT_ASSERT_EQUAL(true, fields.hasField("field B"));
  CPPUNIT_ASSERT_EQUAL(false, fields.hasField("field C"));

} // testHasField

// ----------------------------------------------------------------------
// Test add().
void
pylith::topology::TestFieldsNewMesh::testAdd(void)
{ // testAdd
  CPPUNIT_ASSERT(0 != _mesh);
  FieldsNewMesh fields(*_mesh);
  
  fields.add("field A", "velocity", 3, FieldBase::VECTOR);
  fields.add("field B", "displacement", 4, FieldBase::OTHER, 2.0, true);

  const size_t size = 2;
  CPPUNIT_ASSERT_EQUAL(size, fields._fields.size());
} // testAdd

#if 0
// ----------------------------------------------------------------------
// Test add(domain).
void
pylith::topology::TestFieldsNewMesh::testAddDomain(void)
{ // testAddDomain
  const int fiberDim = 3;

  CPPUNIT_ASSERT(0 != _mesh);
  FieldsNewMesh fields(*_mesh);
  
  const char* label = "field";
  fields.add(label, "velocity", Field<Mesh>::VERTICES_FIELD, fiberDim);
  const size_t size = 1;
  CPPUNIT_ASSERT_EQUAL(size, fields._fields.size());

  Field<Mesh>& field = fields.get(label);
  const ALE::Obj<RealSection>& section = field.section();
  CPPUNIT_ASSERT(!section.isNull());
  const ALE::Obj<SieveMesh>& sieveMesh = _mesh->sieveMesh();
  CPPUNIT_ASSERT(!sieveMesh.isNull());
  const ALE::Obj<SieveMesh::label_sequence>& vertices = 
    sieveMesh->depthStratum(0);
  CPPUNIT_ASSERT(!vertices.isNull());
  field.allocate();
  for (SieveMesh::label_sequence::iterator v_iter=vertices->begin();
       v_iter != vertices->end();
       ++v_iter)
    CPPUNIT_ASSERT_EQUAL(fiberDim, section->getFiberDimension(*v_iter));
} // testAddDomain
#endif

// ----------------------------------------------------------------------
// Test get().
void
pylith::topology::TestFieldsNewMesh::testGet(void)
{ // testGet
  CPPUNIT_ASSERT(0 != _mesh);
  FieldsNewMesh fields(*_mesh);

  fields.add("field A", "velocity", 3, FieldBase::VECTOR);
  fields.add("field B", "displacement", 4, FieldBase::OTHER, 2.0, true);
  fields.allocate(FieldBase::VERTICES_FIELD);

  const Field<Mesh>& fieldA = fields.get("field A");
  CPPUNIT_ASSERT_EQUAL(std::string("velocity"), std::string(fieldA.label()));
  CPPUNIT_ASSERT_EQUAL(FieldBase::VECTOR,
		       fieldA.vectorFieldType());
  CPPUNIT_ASSERT_EQUAL(1.0, fieldA.scale());
  CPPUNIT_ASSERT_EQUAL(false, fieldA.addDimensionOkay());

  const Field<Mesh>& fieldB = fields.get("field B");
  CPPUNIT_ASSERT_EQUAL(std::string("displacement"), 
		       std::string(fieldB.label()));
  CPPUNIT_ASSERT_EQUAL(FieldBase::OTHER,
		       fieldB.vectorFieldType());
  CPPUNIT_ASSERT_EQUAL(2.0, fieldB.scale());
  CPPUNIT_ASSERT_EQUAL(true, fieldB.addDimensionOkay());
} // testGet

// ----------------------------------------------------------------------
// Test get() const.
void
pylith::topology::TestFieldsNewMesh::testGetConst(void)
{ // testGetConst
  CPPUNIT_ASSERT(0 != _mesh);
  FieldsNewMesh fields(*_mesh);

  fields.add("field A", "velocity", 3, FieldBase::VECTOR);
  fields.add("field B", "displacement", 4, FieldBase::OTHER, 2.0, true);
  fields.allocate(FieldBase::VERTICES_FIELD);

  const Field<Mesh>& fieldA = fields.get("field A");
  CPPUNIT_ASSERT_EQUAL(std::string("velocity"), std::string(fieldA.label()));
  CPPUNIT_ASSERT_EQUAL(FieldBase::VECTOR,
		       fieldA.vectorFieldType());
  CPPUNIT_ASSERT_EQUAL(1.0, fieldA.scale());
  CPPUNIT_ASSERT_EQUAL(false, fieldA.addDimensionOkay());

  const Field<Mesh>& fieldB = fields.get("field B");
  CPPUNIT_ASSERT_EQUAL(std::string("displacement"), 
		       std::string(fieldB.label()));
  CPPUNIT_ASSERT_EQUAL(FieldBase::OTHER,
		       fieldB.vectorFieldType());
  CPPUNIT_ASSERT_EQUAL(2.0, fieldB.scale());
  CPPUNIT_ASSERT_EQUAL(true, fieldB.addDimensionOkay());
} // testGetConst

// ----------------------------------------------------------------------
// Test mesh().
void
pylith::topology::TestFieldsNewMesh::testMesh(void)
{ // testMesh
  CPPUNIT_ASSERT(0 != _mesh);
  FieldsNewMesh fields(*_mesh);

  const Mesh& mesh = fields.mesh();
} // testMesh

// ----------------------------------------------------------------------
// Test fieldNames() const.
void
pylith::topology::TestFieldsNewMesh::testFieldNames(void)
{ // testFieldNames
  const int numFieldsE = 2;
  const char* namesE[2] = {
    "field A",
    "field B"
  };

  CPPUNIT_ASSERT(0 != _mesh);
  FieldsNewMesh fields(*_mesh);

  fields.add("field A", "velocity", 3, FieldBase::VECTOR);
  fields.add("field B", "displacement", 4, FieldBase::OTHER, 2.0, true);

  int numFields = 0;
  std::string* names = 0;
  fields.fieldNames(&numFields, &names);
  
  CPPUNIT_ASSERT_EQUAL(numFieldsE, numFields);
  
  for (int i=0; i < numFields; ++i)
    CPPUNIT_ASSERT_EQUAL(std::string(namesE[i]), names[i]);

  delete[] names; names = 0;
} // testFieldNames


// End of file 