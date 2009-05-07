// -*- C++ -*-
//
// ======================================================================
//
//                           Brad T. Aagaard
//                        U.S. Geological Survey
//
// {LicenseText}
//
// ======================================================================
//

#include "DataWriterVTKDataMatMeshTri3.hh"

#include <assert.h> // USES assert()

const char* pylith::meshio::DataWriterVTKDataMatMeshTri3::_meshFilename = 
  "data/tri3.mesh";

const char* pylith::meshio::DataWriterVTKDataMatMeshTri3::_cellsLabel = 
  "material-id";
const int pylith::meshio::DataWriterVTKDataMatMeshTri3::_labelId = 0;

const char* pylith::meshio::DataWriterVTKDataMatMeshTri3::_faultLabel = 
  "fault";
const int pylith::meshio::DataWriterVTKDataMatMeshTri3::_faultId = 100;

const char* pylith::meshio::DataWriterVTKDataMatMeshTri3::_timestepFilename = 
  "tri3_mat.vtk";

const char* pylith::meshio::DataWriterVTKDataMatMeshTri3::_vertexFilename = 
  "tri3_mat_vertex.vtk";

const char* pylith::meshio::DataWriterVTKDataMatMeshTri3::_cellFilename = 
  "tri3_mat_cell.vtk";

const double pylith::meshio::DataWriterVTKDataMatMeshTri3::_time = 1.0;

const char* pylith::meshio::DataWriterVTKDataMatMeshTri3::_timeFormat = 
  "%3.1f";

const int pylith::meshio::DataWriterVTKDataMatMeshTri3::_numVertexFields = 3;
const int pylith::meshio::DataWriterVTKDataMatMeshTri3::_numVertices = 8;

const pylith::meshio::DataWriterVTKData::FieldStruct
pylith::meshio::DataWriterVTKDataMatMeshTri3::_vertexFields[] = {
  { "displacements", topology::FieldBase::VECTOR, 2 },
  { "pressure", topology::FieldBase::SCALAR, 1 },
  { "other", topology::FieldBase::OTHER, 2 },
};
const double pylith::meshio::DataWriterVTKDataMatMeshTri3::_vertexField0[] = {
  1.1, 2.2,
  3.3, 4.4,
  5.5, 6.6,
  7.7, 8.8,
  9.9, 10.0,
  11.1, 12.2,
  13.3, 14.4,
  15.5, 16.6,
};
const double pylith::meshio::DataWriterVTKDataMatMeshTri3::_vertexField1[] = {
  2.1, 3.2, 4.3, 5.4, 6.5, 7.6, 8.7, 9.8
};
const double pylith::meshio::DataWriterVTKDataMatMeshTri3::_vertexField2[] = {
  1.2, 2.3,
  3.4, 4.5,
  5.6, 6.7,
  7.8, 8.9,
  9.0, 10.1,
  11.2, 12.3,
  13.4, 14.5,
  15.6, 16.7
};

const int pylith::meshio::DataWriterVTKDataMatMeshTri3::_numCellFields = 3;
const int pylith::meshio::DataWriterVTKDataMatMeshTri3::_numCells = 1;

const pylith::meshio::DataWriterVTKData::FieldStruct
pylith::meshio::DataWriterVTKDataMatMeshTri3::_cellFields[] = {
  { "traction", topology::FieldBase::VECTOR, 2 },
  { "pressure", topology::FieldBase::SCALAR, 1 },
  { "other", topology::FieldBase::TENSOR, 3 },
};
const double pylith::meshio::DataWriterVTKDataMatMeshTri3::_cellField0[] = {
  1.1, 2.2,
};
const double pylith::meshio::DataWriterVTKDataMatMeshTri3::_cellField1[] = {
  2.1,
};
const double pylith::meshio::DataWriterVTKDataMatMeshTri3::_cellField2[] = {
  1.2, 2.3, 3.4,
};

pylith::meshio::DataWriterVTKDataMatMeshTri3::DataWriterVTKDataMatMeshTri3(void)
{ // constructor
  meshFilename = const_cast<char*>(_meshFilename);
  cellsLabel = const_cast<char*>(_cellsLabel);
  labelId = _labelId;
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
  vertexFieldsInfo = const_cast<DataWriterVTKData::FieldStruct*>(_vertexFields);
  vertexFields[0] = const_cast<double*>(_vertexField0);
  vertexFields[1] = const_cast<double*>(_vertexField1);
  vertexFields[2] = const_cast<double*>(_vertexField2);

  numCellFields = _numCellFields;
  numCells = _numCells;
  assert(3 == numCellFields);
  cellFieldsInfo = const_cast<DataWriterVTKData::FieldStruct*>(_cellFields);
  cellFields[0] = const_cast<double*>(_cellField0);
  cellFields[1] = const_cast<double*>(_cellField1);
  cellFields[2] = const_cast<double*>(_cellField2);
} // constructor

pylith::meshio::DataWriterVTKDataMatMeshTri3::~DataWriterVTKDataMatMeshTri3(void)
{}


// End of file