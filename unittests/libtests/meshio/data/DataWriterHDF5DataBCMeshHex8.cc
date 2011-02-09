// -*- C++ -*-
//
// ======================================================================
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
// ======================================================================
//

#include "DataWriterHDF5DataBCMeshHex8.hh"

#include <assert.h> // USES assert()

const char* pylith::meshio::DataWriterHDF5DataBCMeshHex8::_meshFilename = 
  "data/hex8.mesh";

const char* pylith::meshio::DataWriterHDF5DataBCMeshHex8::_bcLabel = 
  "top";

const char* pylith::meshio::DataWriterHDF5DataBCMeshHex8::_faultLabel = 
  "fault";
const int pylith::meshio::DataWriterHDF5DataBCMeshHex8::_faultId = 100;

const char* pylith::meshio::DataWriterHDF5DataBCMeshHex8::_timestepFilename = 
  "hex8_bc.h5";

const char* pylith::meshio::DataWriterHDF5DataBCMeshHex8::_vertexFilename = 
  "hex8_bc_vertex.h5";

const char* pylith::meshio::DataWriterHDF5DataBCMeshHex8::_cellFilename = 
  "hex8_bc_cell.h5";

const double pylith::meshio::DataWriterHDF5DataBCMeshHex8::_time = 1.0;

const char* pylith::meshio::DataWriterHDF5DataBCMeshHex8::_timeFormat = 
  "%3.1f";

const int pylith::meshio::DataWriterHDF5DataBCMeshHex8::_numVertexFields = 3;
const int pylith::meshio::DataWriterHDF5DataBCMeshHex8::_numVertices = 8;

const pylith::meshio::DataWriterData::FieldStruct
pylith::meshio::DataWriterHDF5DataBCMeshHex8::_vertexFields[] = {
  { "displacements", topology::FieldBase::VECTOR, 3 },
  { "pressure", topology::FieldBase::SCALAR, 1 },
  { "other", topology::FieldBase::OTHER, 2 },
};
const double pylith::meshio::DataWriterHDF5DataBCMeshHex8::_vertexField0[] = {
  1.1, 2.2, 3.3,
  4.4, 5.5, 6.6,
  7.7, 8.8, 9.9,
  10.1, 11.2, 12.3,
  1.2, 2.3, 3.4,
  4.5, 5.6, 6.7,
  7.8, 8.9, 9.0,
  10.2, 11.3, 12.4,
};
const double pylith::meshio::DataWriterHDF5DataBCMeshHex8::_vertexField1[] = {
  2.1, 3.2, 4.3, 5.4, 6.5, 7.6, 8.7, 9.8,
};
const double pylith::meshio::DataWriterHDF5DataBCMeshHex8::_vertexField2[] = {
  1.2, 2.3,
  3.4, 4.5,
  5.6, 6.7,
  7.8, 8.9,
  1.3, 2.4,
  3.5, 4.6,
  5.7, 6.8,
  7.9, 8.0,
};

const int pylith::meshio::DataWriterHDF5DataBCMeshHex8::_numCellFields = 3;
const int pylith::meshio::DataWriterHDF5DataBCMeshHex8::_numCells = 2;

const pylith::meshio::DataWriterData::FieldStruct
pylith::meshio::DataWriterHDF5DataBCMeshHex8::_cellFields[] = {
  { "traction", topology::FieldBase::VECTOR, 3 },
  { "pressure", topology::FieldBase::SCALAR, 1 },
  { "other", topology::FieldBase::TENSOR, 6 },
};
const double pylith::meshio::DataWriterHDF5DataBCMeshHex8::_cellField0[] = {
  1.1, 2.2, 3.3,
  4.4, 5.5, 6.6
};
const double pylith::meshio::DataWriterHDF5DataBCMeshHex8::_cellField1[] = {
  2.1, 3.2,
};
const double pylith::meshio::DataWriterHDF5DataBCMeshHex8::_cellField2[] = {
  1.2, 2.3, 3.4, 4.5, 5.6, 6.7,
  7.8, 8.9, 9.0, 10.1, 11.2, 12.3
};

pylith::meshio::DataWriterHDF5DataBCMeshHex8::DataWriterHDF5DataBCMeshHex8(void)
{ // constructor
  meshFilename = const_cast<char*>(_meshFilename);
  bcLabel = const_cast<char*>(_bcLabel);
  faultLabel = const_cast<char*>(_faultLabel);
  faultId = _faultId;

  timestepFilename = const_cast<char*>(_timestepFilename);
  vertexFilename = const_cast<char*>(_vertexFilename);
  cellFilename = const_cast<char*>(_cellFilename);

  time = _time;
  timeFormat = const_cast<char*>(_timeFormat);
  
  numVertexFields = _numVertexFields;
  numVertices = _numVertices;
  assert(3 == numVertexFields);
  vertexFieldsInfo = const_cast<DataWriterData::FieldStruct*>(_vertexFields);
  vertexFields[0] = const_cast<double*>(_vertexField0);
  vertexFields[1] = const_cast<double*>(_vertexField1);
  vertexFields[2] = const_cast<double*>(_vertexField2);

  numCellFields = _numCellFields;
  numCells = _numCells;
  assert(3 == numCellFields);
  cellFieldsInfo = const_cast<DataWriterData::FieldStruct*>(_cellFields);
  cellFields[0] = const_cast<double*>(_cellField0);
  cellFields[1] = const_cast<double*>(_cellField1);
  cellFields[2] = const_cast<double*>(_cellField2);
} // constructor

pylith::meshio::DataWriterHDF5DataBCMeshHex8::~DataWriterHDF5DataBCMeshHex8(void)
{}


// End of file