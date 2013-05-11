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
// Copyright (c) 2010-2013 University of California, Davis
//
// See COPYING for license information.
//
// ======================================================================
//

#include <portinfo>
#include <stdexcept>

#include "Mesh.hh" // implementation of class methods

#include "spatialdata/geocoords/CoordSys.hh" // USES CoordSys
#include "spatialdata/units/Nondimensional.hh" // USES Nondimensional
#include "pylith/utils/array.hh" // USES scalar_array
#include "pylith/utils/petscfwd.h" // USES PetscVec
#include "pylith/utils/error.h" // USES PYLITH_CHECK_ERROR

#include <stdexcept> // USES std::runtime_error
#include <sstream> // USES std::ostringstream
#include <cassert> // USES assert()

// ----------------------------------------------------------------------
// Default constructor
pylith::topology::Mesh::Mesh(void) :
  _newMesh(NULL),
  _numNormalCells(0), _numCohesiveCells(0), _numNormalVertices(0), _numShadowVertices(0), _numLagrangeVertices(0),
  _coordsys(0),
  _comm(PETSC_COMM_WORLD),
  _debug(false)
{ // constructor
} // constructor

// ----------------------------------------------------------------------
// Default constructor
pylith::topology::Mesh::Mesh(const int dim,
			     const MPI_Comm& comm) :
  _newMesh(NULL),
  _numNormalCells(0), _numCohesiveCells(0), _numNormalVertices(0), _numShadowVertices(0), _numLagrangeVertices(0),
  _coordsys(0),
  _comm(comm),
  _debug(false)
{ // constructor
  PYLITH_METHOD_BEGIN;

  createDMMesh(dim);
  PYLITH_METHOD_END;
} // constructor

// ----------------------------------------------------------------------
// Default destructor
pylith::topology::Mesh::~Mesh(void)
{ // destructor
  deallocate();
} // destructor

// ----------------------------------------------------------------------
// Deallocate PETSc and local data structures.
void
pylith::topology::Mesh::deallocate(void)
{ // deallocate
  PYLITH_METHOD_BEGIN;

  delete _coordsys; _coordsys = 0;
  PetscErrorCode err = DMDestroy(&_newMesh);PYLITH_CHECK_ERROR(err);

  PYLITH_METHOD_END;
} // deallocate
  
// ----------------------------------------------------------------------
// Create DMPlex mesh.
void
pylith::topology::Mesh::createDMMesh(const int dim)
{ // createDMMesh
  PYLITH_METHOD_BEGIN;

  PetscErrorCode err;
  err = DMDestroy(&_newMesh);PYLITH_CHECK_ERROR(err);
  err = DMCreate(_comm, &_newMesh);PYLITH_CHECK_ERROR(err);
  err = DMSetType(_newMesh, DMPLEX);PYLITH_CHECK_ERROR(err);
  err = DMPlexSetDimension(_newMesh, dim);PYLITH_CHECK_ERROR(err);
  err = PetscObjectSetName((PetscObject) _newMesh, "domain");PYLITH_CHECK_ERROR(err);

  PYLITH_METHOD_END;
} // createDMMesh

// ----------------------------------------------------------------------
// Set coordinate system.
void
pylith::topology::Mesh::coordsys(const spatialdata::geocoords::CoordSys* cs)
{ // coordsys
  PYLITH_METHOD_BEGIN;

  delete _coordsys; _coordsys = (0 != cs) ? cs->clone() : 0;
  if (0 != _coordsys)
    _coordsys->initialize();

  PYLITH_METHOD_END;
} // coordsys

// ----------------------------------------------------------------------
// Return the names of all vertex groups.
void
pylith::topology::Mesh::groups(int* numNames, 
			       char*** names) const
{ // groups
  PYLITH_METHOD_BEGIN;

  assert(numNames);
  assert(names);
  *numNames = 0;
  *names = 0;

  if (_newMesh) {
    PetscErrorCode err = 0;

    PetscInt numLabels = 0;
    err = DMPlexGetNumLabels(_newMesh, &numLabels);PYLITH_CHECK_ERROR(err);

    *numNames = numLabels;
    *names = new char*[numLabels];
    for (int iLabel=0; iLabel < numLabels; ++iLabel) {
      const char* namestr = NULL;
      err = DMPlexGetLabelName(_newMesh, iLabel, &namestr);PYLITH_CHECK_ERROR(err);
      // Must return char* that SWIG can deallocate.
      const char len = strlen(namestr);
      char* newName = 0;
      if (len > 0) {
	newName = new char[len+1];
	strncpy(newName, namestr, len+1);
      } else {
	newName = new char[1];
	newName[0] ='\0';
      } // if/else
      (*names)[iLabel] = newName;
    } // for
  } // if

  PYLITH_METHOD_END;
} // groups

// ----------------------------------------------------------------------
// Return the names of all vertex groups.
int
pylith::topology::Mesh::groupSize(const char *name)
{ // groupSize
  PYLITH_METHOD_BEGIN;

  assert(_newMesh);

  PetscErrorCode err = 0;

  PetscBool hasLabel = PETSC_FALSE;
  err = DMPlexHasLabel(_newMesh, name, &hasLabel);PYLITH_CHECK_ERROR(err);
  if (!hasLabel) {
    std::ostringstream msg;
    msg << "Cannot get size of group '" << name
	<< "'. Group missing from mesh.";
    throw std::runtime_error(msg.str());
  } // if

  PetscInt size = 0;
  err = DMPlexGetLabelSize(_newMesh, name, &size);PYLITH_CHECK_ERROR(err);

  PYLITH_METHOD_RETURN(size);
} // groupSize


// ----------------------------------------------------------------------
// Nondimensionalize the finite-element mesh.
void 
pylith::topology::Mesh::nondimensionalize(const spatialdata::units::Nondimensional& normalizer)
{ // initialize
  PYLITH_METHOD_BEGIN;

  PetscVec coordVec, coordDimVec;
  const PylithScalar lengthScale = normalizer.lengthScale();
  PetscErrorCode err;

  assert(_newMesh);
  err = DMGetCoordinatesLocal(_newMesh, &coordVec);PYLITH_CHECK_ERROR(err);assert(coordVec);
  // There does not seem to be an advantage to calling nondimensionalize()
  err = VecScale(coordVec, 1.0/lengthScale);PYLITH_CHECK_ERROR(err);
  err = DMPlexSetScale(_newMesh, PETSC_UNIT_LENGTH, lengthScale);PYLITH_CHECK_ERROR(err);

  PYLITH_METHOD_END;
} // nondimensionalize


// End of file 
