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
// Copyright (c) 2010-2012 University of California, Davis
//
// See COPYING for license information.
//
// ======================================================================
//

#include <portinfo>

#include "ElasticityExplicitLgDeform.hh" // implementation of class methods

#include "Quadrature.hh" // USES Quadrature
#include "CellGeometry.hh" // USES CellGeometry

#include "pylith/materials/ElasticMaterial.hh" // USES ElasticMaterial
#include "pylith/topology/Field.hh" // USES Field
#include "pylith/topology/SolutionFields.hh" // USES SolutionFields
#include "pylith/topology/Jacobian.hh" // USES Jacobian

#include "pylith/utils/array.hh" // USES scalar_array
#include "pylith/utils/macrodefs.h" // USES CALL_MEMBER_FN
#include "pylith/utils/lapack.h" // USES LAPACKdgesvd

#include "petscmat.h" // USES PetscMat
#include "spatialdata/geocoords/CoordSys.hh" // USES CoordSys
#include "spatialdata/units/Nondimensional.hh" // USES Nondimendional

#include "pylith/utils/petscerror.h" // USES CHECK_PETSC_ERROR
#include <cassert> // USES assert()
#include <stdexcept> // USES std::runtime_error

//#define PRECOMPUTE_GEOMETRY

// ----------------------------------------------------------------------
// Constructor
pylith::feassemble::ElasticityExplicitLgDeform::ElasticityExplicitLgDeform(void) :
  _dtm1(-1.0),
  _normViscosity(0.1)
{ // constructor
} // constructor

// ----------------------------------------------------------------------
// Destructor
pylith::feassemble::ElasticityExplicitLgDeform::~ElasticityExplicitLgDeform(void)
{ // destructor
  deallocate();
} // destructor
  
// ----------------------------------------------------------------------
// Deallocate PETSc and local data structures.
void
pylith::feassemble::ElasticityExplicitLgDeform::deallocate(void)
{ // deallocate
  IntegratorElasticityLgDeform::deallocate();
} // deallocate
  
// ----------------------------------------------------------------------
// Set time step for advancing from time t to time t+dt.
void
pylith::feassemble::ElasticityExplicitLgDeform::timeStep(const PylithScalar dt)
{ // timeStep
  if (_dt != -1.0)
    _dtm1 = _dt;
  else
    _dtm1 = dt;
  _dt = dt;
  assert(_dt == _dtm1); // For now, don't allow variable time step
  if (0 != _material)
    _material->timeStep(_dt);
} // timeStep

// ----------------------------------------------------------------------
// Set normalized viscosity for numerical damping.
void
pylith::feassemble::ElasticityExplicitLgDeform::normViscosity(const PylithScalar viscosity)
{ // normViscosity
  if (viscosity < 0.0) {
    std::ostringstream msg;
    msg << "Normalized viscosity (" << viscosity << ") must be nonnegative.";
    throw std::runtime_error(msg.str());
  } // if

  _normViscosity = viscosity;
} // normViscosity

// ----------------------------------------------------------------------
// Set flag for setting constraints for total field solution or
// incremental field solution.
void
pylith::feassemble::ElasticityExplicitLgDeform::useSolnIncr(const bool flag)
{ // useSolnIncr
  if (!flag)
    throw std::logic_error("Non-incremental solution not supported for "
			   "explicit time integration of elasticity "
			   "equation.");
} // useSolnIncr

// ----------------------------------------------------------------------
// Integrate constributions to residual term (r) for operator.
void
pylith::feassemble::ElasticityExplicitLgDeform::integrateResidual(
			  const topology::Field<topology::Mesh>& residual,
			  const PylithScalar t,
			  topology::SolutionFields* const fields)
{ // integrateResidual
  /// Member prototype for _elasticityResidualXD()
  typedef void (pylith::feassemble::ElasticityExplicitLgDeform::*elasticityResidual_fn_type)
    (const scalar_array&, const scalar_array&);

  assert(0 != _quadrature);
  assert(0 != _material);
  assert(0 != _logger);
  assert(0 != fields);

  const int setupEvent = _logger->eventId("ElIR setup");
  const int geometryEvent = _logger->eventId("ElIR geometry");
  const int computeEvent = _logger->eventId("ElIR compute");
  const int restrictEvent = _logger->eventId("ElIR restrict");
  const int stateVarsEvent = _logger->eventId("ElIR stateVars");
  const int stressEvent = _logger->eventId("ElIR stress");
  const int updateEvent = _logger->eventId("ElIR update");

  _logger->eventBegin(setupEvent);

  // Get cell geometry information that doesn't depend on cell
  const int numQuadPts = _quadrature->numQuadPts();
  const scalar_array& quadWts = _quadrature->quadWts();
  assert(quadWts.size() == numQuadPts);
  const int numBasis = _quadrature->numBasis();
  const int spaceDim = _quadrature->spaceDim();
  const int cellDim = _quadrature->cellDim();
  const int tensorSize = _material->tensorSize();
  /** :TODO:
   *
   * If cellDim and spaceDim are different, we need to transform
   * displacements into cellDim, compute action, and transform result
   * back into spaceDim. We get this information from the Jacobian and
   * inverse of the Jacobian.
   */
  if (cellDim != spaceDim)
    throw std::logic_error("Integration for cells with spatial dimensions "
			   "different than the spatial dimension of the "
			   "domain not implemented yet.");

  // Set variables dependent on dimension of cell
  totalStrain_fn_type calcTotalStrainFn;
  elasticityResidual_fn_type elasticityResidualFn;
  if (1 == cellDim) {
    elasticityResidualFn = 
      &pylith::feassemble::ElasticityExplicitLgDeform::_elasticityResidual1D;
    calcTotalStrainFn = 
      &pylith::feassemble::IntegratorElasticityLgDeform::_calcTotalStrain1D;
  } else if (2 == cellDim) {
    elasticityResidualFn = 
      &pylith::feassemble::ElasticityExplicitLgDeform::_elasticityResidual2D;
    calcTotalStrainFn = 
      &pylith::feassemble::IntegratorElasticityLgDeform::_calcTotalStrain2D;
  } else if (3 == cellDim) {
    elasticityResidualFn = 
      &pylith::feassemble::ElasticityExplicitLgDeform::_elasticityResidual3D;
    calcTotalStrainFn = 
      &pylith::feassemble::IntegratorElasticityLgDeform::_calcTotalStrain3D;
  } else
    assert(0);

  // Allocate vectors for cell values.
  scalar_array deformCell(numQuadPts*spaceDim*spaceDim);
  scalar_array strainCell(numQuadPts*tensorSize);
  scalar_array gravVec(spaceDim);
  scalar_array quadPtsGlobal(numQuadPts*spaceDim);

  // Get cell information
  DM              dmMesh = fields->mesh().dmMesh();
  IS              cellIS;
  const PetscInt *cells;
  PetscInt        numCells;
  PetscErrorCode  err;

  assert(dmMesh);
  err = DMComplexGetStratumIS(dmMesh, "material-id", _material->id(), &cellIS);CHECK_PETSC_ERROR(err);
  err = ISGetSize(cellIS, &numCells);CHECK_PETSC_ERROR(err);
  err = ISGetIndices(cellIS, &cells);CHECK_PETSC_ERROR(err);

  // Get sections
  topology::Field<topology::Mesh>& acc = fields->get("acceleration(t)");
  PetscSection accSection = acc.petscSection();
  Vec          accVec     = acc.localVector();
  assert(accSection);assert(accVec);

#if 0 // Numerical damping not yet implemented
  scalar_array velCell(numBasis*spaceDim);
  const ALE::Obj<RealSection>& velSection = 
    fields->get("velocity(t)").section();
  assert(!velSection.isNull());
  RestrictVisitor velVisitor(*velSection, velCell.size(), &velCell[0]);
  scalar_array dispAdjCell(numBasis*spaceDim);
#else
  scalar_array dispTCell(numBasis*spaceDim);
#endif

  topology::Field<topology::Mesh>& dispT = fields->get("disp(t)");
  PetscSection dispTSection = dispT.petscSection();
  Vec          dispTVec     = dispT.localVector();
  assert(dispTSection);assert(dispTVec);

  PetscSection residualSection = residual.petscSection();
  Vec          residualVec     = residual.localVector();

  scalar_array coordinatesCell(numBasis*spaceDim);
  PetscSection coordSection;
  Vec          coordVec;
  err = DMComplexGetCoordinateSection(dmMesh, &coordSection);CHECK_PETSC_ERROR(err);
  err = DMComplexGetCoordinateVec(dmMesh, &coordVec);CHECK_PETSC_ERROR(err);
  assert(coordSection);assert(coordVec);

  assert(0 != _normalizer);
  const PylithScalar lengthScale = _normalizer->lengthScale();
  const PylithScalar gravityScale = 
    _normalizer->pressureScale() / (_normalizer->lengthScale() *
				    _normalizer->densityScale());

  const PylithScalar dt = _dt;
  assert(_normViscosity > 0.0);
  assert(dt > 0);
  const PylithScalar viscosity = dt*_normViscosity;

  _logger->eventEnd(setupEvent);
  _logger->eventBegin(computeEvent);

  // Loop over cells
  for(PetscInt c = 0; c < numCells; ++c) {
    const PetscInt cell = cells[c];
    // Compute geometry information for current cell
#if defined(PRECOMPUTE_GEOMETRY)
    _quadrature->retrieveGeometry(cell);
#else
    const PetscScalar *coords;
    PetscInt           coordsSize;
    err = DMComplexVecGetClosure(dmMesh, coordSection, coordVec, cell, &coordsSize, &coords);CHECK_PETSC_ERROR(err);
    for(PetscInt i = 0; i < coordsSize; ++i) {coordinatesCell[i] = coords[i];}
    _quadrature->computeGeometry(coordinatesCell, cell);
    err = DMComplexVecRestoreClosure(dmMesh, coordSection, coordVec, cell, &coordsSize, &coords);CHECK_PETSC_ERROR(err);
#endif

    // Get state variables for cell.
    _material->retrievePropsAndVars(cell);

    // Reset element vector to zero
    _resetCellVector();

    // Restrict input fields to cell
    const PetscScalar *accArray, *velArray, *dispTArray;
    PetscInt           accSize,   velSize,   dispTSize;
    err = DMComplexVecGetClosure(dmMesh, accSection,   accVec,   cell, &accSize,   &accArray);CHECK_PETSC_ERROR(err);
#if 0 // Numerical damping not yet implemented.
    err = DMComplexVecGetClosure(dmMesh, velSection,   velVec,   cell, &velSize,   &velArray);CHECK_PETSC_ERROR(err);
    assert(velSize   == accSize);
#endif
    err = DMComplexVecGetClosure(dmMesh, dispTSection, dispTVec, cell, &dispTSize, &dispTArray);CHECK_PETSC_ERROR(err);
    assert(dispTSize == accSize);

    // Get cell geometry information that depends on cell
    const scalar_array& basis = _quadrature->basis();
    const scalar_array& basisDeriv = _quadrature->basisDeriv();
    const scalar_array& jacobianDet = _quadrature->jacobianDet();
    const scalar_array& quadPtsNondim = _quadrature->quadPts();

    // Compute body force vector if gravity is being used.
    if (0 != _gravityField) {
      const spatialdata::geocoords::CoordSys* cs = fields->mesh().coordsys();
      assert(0 != cs);
      
      // Get density at quadrature points for this cell
      const scalar_array& density = _material->calcDensity();

      quadPtsGlobal = quadPtsNondim;
      _normalizer->dimensionalize(&quadPtsGlobal[0], quadPtsGlobal.size(),
				  lengthScale);

      // Compute action for element body forces
      spatialdata::spatialdb::SpatialDB* db = _gravityField;
      for (int iQuad=0; iQuad < numQuadPts; ++iQuad) {
	const int err = db->query(&gravVec[0], gravVec.size(),
					     &quadPtsGlobal[0], spaceDim, cs);
	if (err)
	  throw std::runtime_error("Unable to get gravity vector for point.");
	_normalizer->nondimensionalize(&gravVec[0], gravVec.size(), 
				       gravityScale);
	const PylithScalar wt = quadWts[iQuad] * jacobianDet[iQuad] * density[iQuad];
	for (int iBasis=0, iQ=iQuad*numBasis;
	     iBasis < numBasis; ++iBasis) {
	  const PylithScalar valI = wt*basis[iQ+iBasis];
	  for (int iDim=0; iDim < spaceDim; ++iDim) {
	    _cellVector[iBasis*spaceDim+iDim] += valI*gravVec[iDim];
	  } // for
	} // for
      } // for
      PetscLogFlops(numQuadPts*(2+numBasis*(1+2*spaceDim)));
    } // if

    // Compute action for inertial terms
    const scalar_array& density = _material->calcDensity();
    for (int iQuad=0; iQuad < numQuadPts; ++iQuad) {
      const PylithScalar wt = 
	quadWts[iQuad] * jacobianDet[iQuad] * density[iQuad];
      for (int iBasis=0; iBasis < numBasis; ++iBasis) {
        const PylithScalar valI = wt*basis[iQuad*numBasis+iBasis];
        for (int jBasis=0; jBasis < numBasis; ++jBasis) {
          const PylithScalar valIJ = valI * basis[iQuad*numBasis+jBasis];
          for (int iDim=0; iDim < spaceDim; ++iDim)
            _cellVector[iBasis*spaceDim+iDim] -= 
	      valIJ * accArray[jBasis*spaceDim+iDim];
        } // for
      } // for
    } // for
    PetscLogFlops(numQuadPts*(2+numBasis*(1+numBasis*(2*spaceDim))));

#if 0 // Numerical damping not yet implemented. Is small strain
      // formulation compatible with numerical damping?

    // Numerical damping. Compute displacements adjusted by velocity
    // times normalized viscosity.
    for(PetscInt i = 0; i < dispTSize; ++i) {dispAdjCell[i] = dispTArray[i] + viscosity * velArray[i];}
    err = DMComplexVecRestoreClosure(dmMesh, velSection,   velVec,   cell, &velSize,   &velArray);CHECK_PETSC_ERROR(err);
#else
    for(PetscInt i = 0; i < dispTSize; ++i) {dispTCell[i] = dispTArray[i];}
#endif
    err = DMComplexVecRestoreClosure(dmMesh, accSection,   accVec,   cell, &accSize,   &accArray);CHECK_PETSC_ERROR(err);
    err = DMComplexVecRestoreClosure(dmMesh, dispTSection, dispTVec, cell, &dispTSize, &dispTArray);CHECK_PETSC_ERROR(err);

    // Compute B(transpose) * sigma, first computing strains
    _calcDeformation(&deformCell, basisDeriv, coordinatesCell, dispTCell, numBasis, numQuadPts, spaceDim);
    calcTotalStrainFn(&strainCell, deformCell, numQuadPts);
    const scalar_array& stressCell = _material->calcStress(strainCell, true);

    CALL_MEMBER_FN(*this, elasticityResidualFn)(stressCell, dispTCell);

    // Assemble cell contribution into field
    err = DMComplexVecSetClosure(dmMesh, residualSection, residualVec, cell, &_cellVector[0], ADD_VALUES);CHECK_PETSC_ERROR(err);
  } // for
  err = ISRestoreIndices(cellIS, &cells);CHECK_PETSC_ERROR(err);
  err = ISDestroy(&cellIS);CHECK_PETSC_ERROR(err);

  _logger->eventEnd(computeEvent);
} // integrateResidual

// ----------------------------------------------------------------------
// Integrate constributions to residual term (r) for operator.
void
pylith::feassemble::ElasticityExplicitLgDeform::integrateResidualLumped(
			  const topology::Field<topology::Mesh>& residual,
			  const PylithScalar t,
			  topology::SolutionFields* const fields)
{ // integrateResidualLumped
  /// Member prototype for _elasticityResidualXD()
  typedef void (pylith::feassemble::ElasticityExplicitLgDeform::*elasticityResidual_fn_type)
    (const scalar_array&, const scalar_array&);

  assert(0 != _quadrature);
  assert(0 != _material);
  assert(0 != _logger);
  assert(0 != fields);

  const int setupEvent = _logger->eventId("ElIR setup");
  const int geometryEvent = _logger->eventId("ElIR geometry");
  const int computeEvent = _logger->eventId("ElIR compute");
  const int restrictEvent = _logger->eventId("ElIR restrict");
  const int stateVarsEvent = _logger->eventId("ElIR stateVars");
  const int stressEvent = _logger->eventId("ElIR stress");
  const int updateEvent = _logger->eventId("ElIR update");

  _logger->eventBegin(setupEvent);

  // Get cell geometry information that doesn't depend on cell
  const int numQuadPts = _quadrature->numQuadPts();
  const scalar_array& quadWts = _quadrature->quadWts();
  assert(quadWts.size() == numQuadPts);
  const int numBasis = _quadrature->numBasis();
  const int spaceDim = _quadrature->spaceDim();
  const int cellDim = _quadrature->cellDim();
  const int tensorSize = _material->tensorSize();
  /** :TODO:
   *
   * If cellDim and spaceDim are different, we need to transform
   * displacements into cellDim, compute action, and transform result
   * back into spaceDim. We get this information from the Jacobian and
   * inverse of the Jacobian.
   */
  if (cellDim != spaceDim)
    throw std::logic_error("Integration for cells with spatial dimensions "
			   "different than the spatial dimension of the "
			   "domain not implemented yet.");

  // Set variables dependent on dimension of cell
  totalStrain_fn_type calcTotalStrainFn;
  elasticityResidual_fn_type elasticityResidualFn;
  if (1 == cellDim) {
    elasticityResidualFn = 
      &pylith::feassemble::ElasticityExplicitLgDeform::_elasticityResidual1D;
    calcTotalStrainFn = 
      &pylith::feassemble::IntegratorElasticityLgDeform::_calcTotalStrain1D;
  } else if (2 == cellDim) {
    elasticityResidualFn = 
      &pylith::feassemble::ElasticityExplicitLgDeform::_elasticityResidual2D;
    calcTotalStrainFn = 
      &pylith::feassemble::IntegratorElasticityLgDeform::_calcTotalStrain2D;
  } else if (3 == cellDim) {
    elasticityResidualFn = 
      &pylith::feassemble::ElasticityExplicitLgDeform::_elasticityResidual3D;
    calcTotalStrainFn = 
      &pylith::feassemble::IntegratorElasticityLgDeform::_calcTotalStrain3D;
  } else
    assert(0);

  // Allocate vectors for cell values.
  scalar_array deformCell(numQuadPts*spaceDim*spaceDim);
  scalar_array strainCell(numQuadPts*tensorSize);
  scalar_array gravVec(spaceDim);
  scalar_array quadPtsGlobal(numQuadPts*spaceDim);

  // Get cell information
  DM              dmMesh = fields->mesh().dmMesh();
  IS              cellIS;
  const PetscInt *cells;
  PetscInt        numCells;
  PetscErrorCode  err;

  assert(dmMesh);
  err = DMComplexGetStratumIS(dmMesh, "material-id", _material->id(), &cellIS);CHECK_PETSC_ERROR(err);
  err = ISGetSize(cellIS, &numCells);CHECK_PETSC_ERROR(err);
  err = ISGetIndices(cellIS, &cells);CHECK_PETSC_ERROR(err);

  // Get sections
  topology::Field<topology::Mesh>& acc = fields->get("acceleration(t)");
  PetscSection accSection = acc.petscSection();
  Vec          accVec     = acc.localVector();
  assert(accSection);assert(accVec);

#if 0 // Numerical damping not yet implemented
  scalar_array dispAdjCell(numBasis*spaceDim);
  topology::Field<topology::Mesh>& vel = fields->get("velocity(t)");
  PetscSection velSection = vel.petscSection();
  Vec          velVec     = vel.localVector();
  assert(velSection);assert(velVec);
#else
  scalar_array dispTCell(numBasis*spaceDim);
#endif
  
  topology::Field<topology::Mesh>& dispT = fields->get("disp(t)");
  PetscSection dispTSection = dispT.petscSection();
  Vec          dispTVec     = dispT.localVector();
  assert(dispTSection);assert(dispTVec);

  PetscSection residualSection = residual.petscSection();
  Vec          residualVec     = residual.localVector();

  scalar_array coordinatesCell(numBasis*spaceDim);
  PetscSection coordSection;
  Vec          coordVec;
  err = DMComplexGetCoordinateSection(dmMesh, &coordSection);CHECK_PETSC_ERROR(err);
  err = DMComplexGetCoordinateVec(dmMesh, &coordVec);CHECK_PETSC_ERROR(err);
  assert(coordSection);assert(coordVec);

  assert(0 != _normalizer);
  const PylithScalar lengthScale = _normalizer->lengthScale();
  const PylithScalar gravityScale = 
    _normalizer->pressureScale() / (_normalizer->lengthScale() *
				    _normalizer->densityScale());

  // Get parameters used in integration.
  scalar_array valuesIJ(numBasis);

  _logger->eventEnd(setupEvent);
  _logger->eventBegin(computeEvent);

  // Loop over cells
  for(PetscInt c = 0; c < numCells; ++c) {
    const PetscInt cell = cells[c];
    // Compute geometry information for current cell
#if defined(PRECOMPUTE_GEOMETRY)
    _quadrature->retrieveGeometry(cell);
#else
    const PetscScalar *coords;
    PetscInt           coordsSize;
    err = DMComplexVecGetClosure(dmMesh, coordSection, coordVec, cell, &coordsSize, &coords);CHECK_PETSC_ERROR(err);
    for(PetscInt i = 0; i < coordsSize; ++i) {coordinatesCell[i] = coords[i];}
    _quadrature->computeGeometry(coordinatesCell, cell);
    err = DMComplexVecRestoreClosure(dmMesh, coordSection, coordVec, cell, &coordsSize, &coords);CHECK_PETSC_ERROR(err);
#endif

    // Get state variables for cell.
    _material->retrievePropsAndVars(cell);

    // Reset element vector to zero
    _resetCellVector();

    // Restrict input fields to cell
    const PetscScalar *accArray, *velArray, *dispTArray;
    PetscInt           accSize,   velSize,   dispTSize;
    err = DMComplexVecGetClosure(dmMesh, accSection,   accVec,   cell, &accSize,   &accArray);CHECK_PETSC_ERROR(err);
#if 0 // Numerical damping not yet implemented.
    err = DMComplexVecGetClosure(dmMesh, velSection,   velVec,   cell, &velSize,   &velArray);CHECK_PETSC_ERROR(err);
    assert(velSize   == accSize);
#endif
    err = DMComplexVecGetClosure(dmMesh, dispTSection, dispTVec, cell, &dispTSize, &dispTArray);CHECK_PETSC_ERROR(err);
    assert(dispTSize == accSize);

    // Get cell geometry information that depends on cell
    const scalar_array& basis = _quadrature->basis();
    const scalar_array& basisDeriv = _quadrature->basisDeriv();
    const scalar_array& jacobianDet = _quadrature->jacobianDet();
    const scalar_array& quadPtsNondim = _quadrature->quadPts();

    // Compute body force vector if gravity is being used.
    if (0 != _gravityField) {
      const spatialdata::geocoords::CoordSys* cs = fields->mesh().coordsys();
      assert(0 != cs);
      
      // Get density at quadrature points for this cell
      const scalar_array& density = _material->calcDensity();

      quadPtsGlobal = quadPtsNondim;
      _normalizer->dimensionalize(&quadPtsGlobal[0], quadPtsGlobal.size(),
				  lengthScale);

      // Compute action for element body forces
      spatialdata::spatialdb::SpatialDB* db = _gravityField;
      for (int iQuad=0; iQuad < numQuadPts; ++iQuad) {
	const int err = db->query(&gravVec[0], gravVec.size(),
					     &quadPtsGlobal[0], spaceDim, cs);
	if (err)
	  throw std::runtime_error("Unable to get gravity vector for point.");
	_normalizer->nondimensionalize(&gravVec[0], gravVec.size(), 
				       gravityScale);
	const PylithScalar wt = quadWts[iQuad] * jacobianDet[iQuad] * density[iQuad];
	for (int iBasis=0, iQ=iQuad*numBasis;
	     iBasis < numBasis; ++iBasis) {
	  const PylithScalar valI = wt*basis[iQ+iBasis];
	  for (int iDim=0; iDim < spaceDim; ++iDim) {
	    _cellVector[iBasis*spaceDim+iDim] += valI*gravVec[iDim];
	  } // for
	} // for
      } // for
      PetscLogFlops(numQuadPts*(2+numBasis*(1+2*spaceDim)));
    } // if

    // Compute action for inertial terms
    const scalar_array& density = _material->calcDensity();
    valuesIJ = 0.0;
    for (int iQuad = 0; iQuad < numQuadPts; ++iQuad) {
      const PylithScalar wt = quadWts[iQuad] * jacobianDet[iQuad] * density[iQuad];
      const int iQ = iQuad * numBasis;
      PylithScalar valJ = 0.0;
      for (int jBasis = 0; jBasis < numBasis; ++jBasis)
	valJ += basis[iQ + jBasis];
      valJ *= wt;
      for (int iBasis = 0; iBasis < numBasis; ++iBasis)
	valuesIJ[iBasis] += basis[iQ + iBasis] * valJ;
    } // for
    for (int iBasis = 0; iBasis < numBasis; ++iBasis)
      for (int iDim = 0; iDim < spaceDim; ++iDim)
	_cellVector[iBasis*spaceDim+iDim] -= valuesIJ[iBasis] *
	  accArray[iBasis*spaceDim+iDim];
    PetscLogFlops(numQuadPts*(4+numBasis*3));

#if 0 // Numerical damping not yet implemented. Is small strain
      // formulation compatible with numerical damping?

    // Numerical damping. Compute displacements adjusted by velocity
    // times normalized viscosity.
    for(PetscInt i = 0; i < dispTSize; ++i) {dispAdjCell[i] = dispTArray[i] + viscosity * velArray[i];}
    err = DMComplexVecRestoreClosure(dmMesh, velSection,   velVec,   cell, &velSize,   &velArray);CHECK_PETSC_ERROR(err);
#else
    for(PetscInt i = 0; i < dispTSize; ++i) {dispTCell[i] = dispTArray[i];}
#endif
    err = DMComplexVecRestoreClosure(dmMesh, accSection,   accVec,   cell, &accSize,   &accArray);CHECK_PETSC_ERROR(err);
    err = DMComplexVecRestoreClosure(dmMesh, dispTSection, dispTVec, cell, &dispTSize, &dispTArray);CHECK_PETSC_ERROR(err);

    // Compute B(transpose) * sigma, first computing strains
    _calcDeformation(&deformCell, basisDeriv, coordinatesCell, dispTCell, numBasis, numQuadPts, spaceDim);
    calcTotalStrainFn(&strainCell, deformCell, numQuadPts);
    const scalar_array& stressCell = _material->calcStress(strainCell, true);

    CALL_MEMBER_FN(*this, elasticityResidualFn)(stressCell, dispTCell);
    
    // Assemble cell contribution into field
    err = DMComplexVecSetClosure(dmMesh, residualSection, residualVec, cell, &_cellVector[0], ADD_VALUES);CHECK_PETSC_ERROR(err);
  } // for
  err = ISRestoreIndices(cellIS, &cells);CHECK_PETSC_ERROR(err);
  err = ISDestroy(&cellIS);CHECK_PETSC_ERROR(err);

  _logger->eventEnd(computeEvent);
} // integrateResidualLumped

// ----------------------------------------------------------------------
// Compute matrix associated with operator.
void
pylith::feassemble::ElasticityExplicitLgDeform::integrateJacobian(
					topology::Jacobian* jacobian,
					const PylithScalar t,
					topology::SolutionFields* fields)
{ // integrateJacobian
  assert(0 != _quadrature);
  assert(0 != _material);
  assert(0 != jacobian);
  assert(0 != fields);

  const int setupEvent = _logger->eventId("ElIJ setup");
  const int geometryEvent = _logger->eventId("ElIJ geometry");
  const int computeEvent = _logger->eventId("ElIJ compute");
  const int restrictEvent = _logger->eventId("ElIJ restrict");
  const int stateVarsEvent = _logger->eventId("ElIJ stateVars");
  const int updateEvent = _logger->eventId("ElIJ update");

  _logger->eventBegin(setupEvent);

  // Get cell geometry information that doesn't depend on cell
  const int numQuadPts = _quadrature->numQuadPts();
  const scalar_array& quadWts = _quadrature->quadWts();
  assert(quadWts.size() == numQuadPts);
  const int numBasis = _quadrature->numBasis();
  const int spaceDim = _quadrature->spaceDim();
  const int cellDim = _quadrature->cellDim();
  const int tensorSize = _material->tensorSize();
  if (cellDim != spaceDim)
    throw std::logic_error("Don't know how to integrate elasticity " \
			   "contribution to Jacobian matrix for cells with " \
			   "different dimensions than the spatial dimension.");

  // Get cell information
  DM              dmMesh = fields->mesh().dmMesh();
  IS              cellIS;
  const PetscInt *cells;
  PetscInt        numCells;
  PetscErrorCode  err;

  assert(dmMesh);
  err = DMComplexGetStratumIS(dmMesh, "material-id", _material->id(), &cellIS);CHECK_PETSC_ERROR(err);
  err = ISGetSize(cellIS, &numCells);CHECK_PETSC_ERROR(err);
  err = ISGetIndices(cellIS, &cells);CHECK_PETSC_ERROR(err);

  // Get sections
  PetscSection solnSection = fields->solution().petscSection();
  Vec          solnVec     = fields->solution().localVector();

  scalar_array coordinatesCell(numBasis*spaceDim);
  PetscSection coordSection;
  Vec          coordVec;
  err = DMComplexGetCoordinateSection(dmMesh, &coordSection);CHECK_PETSC_ERROR(err);
  err = DMComplexGetCoordinateVec(dmMesh, &coordVec);CHECK_PETSC_ERROR(err);
  assert(coordSection);assert(coordVec);

  // Get sparse matrix
  const PetscMat jacobianMat = jacobian->matrix();
  assert(0 != jacobianMat);

  // Get parameters used in integration.
  const PylithScalar dt = _dt;
  const PylithScalar dt2 = dt*dt;
  assert(dt > 0);

  _logger->eventEnd(setupEvent);
  _logger->eventBegin(computeEvent);

  // Loop over cells
  for(PetscInt c = 0; c < numCells; ++c) {
    const PetscInt cell = cells[c];
    // Compute geometry information for current cell
#if defined(PRECOMPUTE_GEOMETRY)
    _quadrature->retrieveGeometry(cell);
#else
    const PetscScalar *coords;
    PetscInt           coordsSize;
    err = DMComplexVecGetClosure(dmMesh, coordSection, coordVec, cell, &coordsSize, &coords);CHECK_PETSC_ERROR(err);
    for(PetscInt i = 0; i < coordsSize; ++i) {coordinatesCell[i] = coords[i];}
    _quadrature->computeGeometry(coordinatesCell, cell);
    err = DMComplexVecRestoreClosure(dmMesh, coordSection, coordVec, cell, &coordsSize, &coords);CHECK_PETSC_ERROR(err);
#endif

    // Get state variables for cell.
    _material->retrievePropsAndVars(cell);

    // Reset element matrix to zero
    _resetCellMatrix();

    // Get cell geometry information that depends on cell
    const scalar_array& basis = _quadrature->basis();
    const scalar_array& jacobianDet = _quadrature->jacobianDet();

    // Get material physical properties at quadrature points for this cell
    const scalar_array& density = _material->calcDensity();

    // Compute Jacobian for inertial terms
    for (int iQuad=0; iQuad < numQuadPts; ++iQuad) {
      const PylithScalar wt = 
	quadWts[iQuad] * jacobianDet[iQuad] * density[iQuad] / dt2;
      for (int iBasis=0, iQ=iQuad*numBasis; iBasis < numBasis; ++iBasis) {
        const PylithScalar valI = wt*basis[iQ+iBasis];
        for (int jBasis=0; jBasis < numBasis; ++jBasis) {
          const PylithScalar valIJ = valI * basis[iQ+jBasis];
          for (int iDim=0; iDim < spaceDim; ++iDim) {
            const int iBlock = (iBasis*spaceDim + iDim) * (numBasis*spaceDim);
            const int jBlock = (jBasis*spaceDim + iDim);
            _cellMatrix[iBlock+jBlock] += valIJ;
          } // for
        } // for
      } // for
    } // for
    PetscLogFlops(numQuadPts*(3+numBasis*(1+numBasis*(1+spaceDim))));
    
    // Assemble cell contribution into PETSc matrix.
    err = DMComplexMatSetClosure(dmMesh, solnSection, PETSC_NULL, jacobianMat, cell, &_cellMatrix[0], ADD_VALUES);
    CHECK_PETSC_ERROR_MSG(err, "Update to PETSc Mat failed.");
  } // for
  err = ISRestoreIndices(cellIS, &cells);CHECK_PETSC_ERROR(err);
  err = ISDestroy(&cellIS);CHECK_PETSC_ERROR(err);

  _needNewJacobian = false;
  _material->resetNeedNewJacobian();

  _logger->eventEnd(computeEvent);
} // integrateJacobian

// ----------------------------------------------------------------------
// Compute matrix associated with operator.
void
pylith::feassemble::ElasticityExplicitLgDeform::integrateJacobian(
			    topology::Field<topology::Mesh>* jacobian,
			    const PylithScalar t,
			    topology::SolutionFields* fields)
{ // integrateJacobian
  assert(0 != _quadrature);
  assert(0 != _material);
  assert(0 != jacobian);
  assert(0 != fields);

  const int setupEvent = _logger->eventId("ElIJ setup");
  const int geometryEvent = _logger->eventId("ElIJ geometry");
  const int computeEvent = _logger->eventId("ElIJ compute");
  const int restrictEvent = _logger->eventId("ElIJ restrict");
  const int stateVarsEvent = _logger->eventId("ElIJ stateVars");
  const int updateEvent = _logger->eventId("ElIJ update");

  _logger->eventBegin(setupEvent);

  // Get cell geometry information that doesn't depend on cell
  const int numQuadPts = _quadrature->numQuadPts();
  const scalar_array& quadWts = _quadrature->quadWts();
  assert(quadWts.size() == numQuadPts);
  const int numBasis = _quadrature->numBasis();
  const int spaceDim = _quadrature->spaceDim();
  const int cellDim = _quadrature->cellDim();
  const int tensorSize = _material->tensorSize();
  if (cellDim != spaceDim)
    throw std::logic_error("Don't know how to integrate elasticity " \
			   "contribution to Jacobian matrix for cells with " \
			   "different dimensions than the spatial dimension.");

  // Get cell information
  DM              dmMesh = fields->mesh().dmMesh();
  IS              cellIS;
  const PetscInt *cells;
  PetscInt        numCells;
  PetscErrorCode  err;

  assert(dmMesh);
  err = DMComplexGetStratumIS(dmMesh, "material-id", _material->id(), &cellIS);CHECK_PETSC_ERROR(err);
  err = ISGetSize(cellIS, &numCells);CHECK_PETSC_ERROR(err);
  err = ISGetIndices(cellIS, &cells);CHECK_PETSC_ERROR(err);

  // Get parameters used in integration.
  const PylithScalar dt = _dt;
  const PylithScalar dt2 = dt*dt;
  assert(dt > 0);

  // Get sections
  PetscSection jacSection = jacobian->petscSection();
  Vec          jacVec     = jacobian->localVector();

  scalar_array coordinatesCell(numBasis*spaceDim);
  PetscSection coordSection;
  Vec          coordVec;
  err = DMComplexGetCoordinateSection(dmMesh, &coordSection);CHECK_PETSC_ERROR(err);
  err = DMComplexGetCoordinateVec(dmMesh, &coordVec);CHECK_PETSC_ERROR(err);
  assert(coordSection);assert(coordVec);

  _logger->eventEnd(setupEvent);
  _logger->eventBegin(computeEvent);

  // Loop over cells
  for(PetscInt c = 0; c < numCells; ++c) {
    const PetscInt cell = cells[c];
    // Compute geometry information for current cell
#if defined(PRECOMPUTE_GEOMETRY)
    _quadrature->retrieveGeometry(cell);
#else
    const PetscScalar *coords;
    PetscInt           coordsSize;
    err = DMComplexVecGetClosure(dmMesh, coordSection, coordVec, cell, &coordsSize, &coords);CHECK_PETSC_ERROR(err);
    for(PetscInt i = 0; i < coordsSize; ++i) {coordinatesCell[i] = coords[i];}
    _quadrature->computeGeometry(coordinatesCell, cell);
    err = DMComplexVecRestoreClosure(dmMesh, coordSection, coordVec, cell, &coordsSize, &coords);CHECK_PETSC_ERROR(err);
#endif

    // Get state variables for cell.
    _material->retrievePropsAndVars(cell);

    // Reset element matrix to zero
    _resetCellMatrix();

    // Get cell geometry information that depends on cell
    const scalar_array& basis = _quadrature->basis();
    const scalar_array& jacobianDet = _quadrature->jacobianDet();

    // Get material physical properties at quadrature points for this cell
    const scalar_array& density = _material->calcDensity();

    // Compute Jacobian for inertial terms
    for (int iQuad=0; iQuad < numQuadPts; ++iQuad) {
      const PylithScalar wt = 
	quadWts[iQuad] * jacobianDet[iQuad] * density[iQuad] / dt2;
      for (int iBasis=0, iQ=iQuad*numBasis; iBasis < numBasis; ++iBasis) {
        const PylithScalar valI = wt*basis[iQ+iBasis];
        for (int jBasis=0; jBasis < numBasis; ++jBasis) {
          const PylithScalar valIJ = valI * basis[iQ+jBasis];
          for (int iDim=0; iDim < spaceDim; ++iDim) {
            const int iBlock = (iBasis*spaceDim + iDim) * (numBasis*spaceDim);
            const int jBlock = (jBasis*spaceDim + iDim);
            _cellMatrix[iBlock+jBlock] += valIJ;
          } // for
        } // for
      } // for
    } // for
    PetscLogFlops(numQuadPts*(3+numBasis*(1+numBasis*(1+spaceDim))));
    _lumpCellMatrix();
    
    // Assemble cell contribution into lumped matrix.
    err = DMComplexVecSetClosure(dmMesh, jacSection, jacVec, cell, &_cellVector[0], ADD_VALUES);CHECK_PETSC_ERROR(err);
  } // for
  err = ISRestoreIndices(cellIS, &cells);CHECK_PETSC_ERROR(err);
  err = ISDestroy(&cellIS);CHECK_PETSC_ERROR(err);

  _needNewJacobian = false;
  _material->resetNeedNewJacobian();

  _logger->eventEnd(computeEvent);
} // integrateJacobian


// End of file 
