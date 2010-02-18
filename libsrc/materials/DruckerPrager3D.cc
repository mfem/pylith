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

#include <portinfo>

#include "DruckerPragerEP3D.hh" // implementation of object methods

#include "Metadata.hh" // USES Metadata

#include "pylith/utils/array.hh" // USES double_array

#include "spatialdata/units/Nondimensional.hh" // USES Nondimensional

#include "petsc.h" // USES PetscLogFlops

#include <cmath> // USES fabs()
#include <cassert> // USES assert()
#include <cstring> // USES memcpy()
#include <sstream> // USES std::ostringstream
#include <stdexcept> // USES std::runtime_error

// ----------------------------------------------------------------------
namespace pylith {
  namespace materials {
    namespace _DruckerPragerEP3D{

      /// Dimension of material.
      const int dimension = 3;

      /// Number of entries in stress/strain tensors.
      const int tensorSize = 6;

      /// Number of entries in derivative of elasticity matrix.
      const int numElasticConsts = 36;

      /// Number of physical properties.
      const int numProperties = 6;

      /// Physical properties.
      const Metadata::ParamDescription properties[] = {
	{ "density", 1, pylith::topology::FieldBase::SCALAR },
	{ "mu", 1, pylith::topology::FieldBase::SCALAR },
	{ "lambda", 1, pylith::topology::FieldBase::SCALAR },
	{ "alpha_yield", 1, pylith::topology::FieldBase::SCALAR },
	{ "beta", 1, pylith::topology::FieldBase::SCALAR },
	{ "alpha_flow", 1, pylith::topology::FieldBase::SCALAR }
      };

      // Values expected in properties spatial database
      const int numDBProperties = 6;
      const char* dbProperties[] = {"density", "vs", "vp" ,
				    "friction-angle",
				    "cohesion",
				    "dilatation-angle"};

      /// Number of state variables.
      const int numStateVars = 1;

      /// State variables.
      const Metadata::ParamDescription stateVars[] = {
	{ "plastic_strain", tensorSize, pylith::topology::FieldBase::TENSOR }
      };

      // Values expected in state variables spatial database.
      const int numDBStateVars = 6;
      const char* dbStateVars[] = { "plastic-strain-xx",
				    "plastic-strain-yy",
				    "plastic-strain-zz",
				    "plastic-strain-xy",
				    "plastic-strain-yz",
				    "plastic-strain-xz"
      };

    } // _DruckerPragerEP3D
  } // materials
} // pylith

// Indices of physical properties.
const int pylith::materials::DruckerPragerEP3D::p_density = 0;

const int pylith::materials::DruckerPragerEP3D::p_mu = 
  pylith::materials::DruckerPragerEP3D::p_density + 1;

const int pylith::materials::DruckerPragerEP3D::p_lambda = 
  pylith::materials::DruckerPragerEP3D::p_mu + 1;

const int pylith::materials::DruckerPragerEP3D::p_alphaYield = 
  pylith::materials::DruckerPragerEP3D::p_lambda + 1;

const int pylith::materials::DruckerPragerEP3D::p_beta = 
  pylith::materials::DruckerPragerEP3D::p_alphaYield + 1;

const int pylith::materials::DruckerPragerEP3D::p_alphaFlow = 
  pylith::materials::DruckerPragerEP3D::p_beta + 1;

// Indices of property database values (order must match dbProperties).
const int pylith::materials::DruckerPragerEP3D::db_density = 0;

const int pylith::materials::DruckerPragerEP3D::db_vs = 
  pylith::materials::DruckerPragerEP3D::db_density + 1;

const int pylith::materials::DruckerPragerEP3D::db_vp = 
  pylith::materials::DruckerPragerEP3D::db_vs + 1;

const int pylith::materials::DruckerPragerEP3D::db_frictionAngle = 
  pylith::materials::DruckerPragerEP3D::db_vp + 1;

const int pylith::materials::DruckerPragerEP3D::db_cohesion = 
  pylith::materials::DruckerPragerEP3D::db_frictionAngle + 1;

const int pylith::materials::DruckerPragerEP3D::db_dilatationAngle = 
  pylith::materials::DruckerPragerEP3D::db_cohesion + 1;

// Indices of state variables.
const int pylith::materials::DruckerPragerEP3D::s_plasticStrain = 0;

// Indices of state variable database values (order must match dbStateVars).
const int pylith::materials::DruckerPragerEP3D::db_plasticStrain = 0;

// ----------------------------------------------------------------------
// Default constructor.
pylith::materials::DruckerPragerEP3D::DruckerPragerEP3D(void) :
  ElasticMaterial(_DruckerPragerEP3D::dimension,
		  _DruckerPragerEP3D::tensorSize,
		  _DruckerPragerEP3D::numElasticConsts,
		  Metadata(_DruckerPragerEP3D::properties,
			   _DruckerPragerEP3D::numProperties,
			   _DruckerPragerEP3D::dbProperties,
			   _DruckerPragerEP3D::numDBProperties,
			   _DruckerPragerEP3D::stateVars,
			   _DruckerPragerEP3D::numStateVars,
			   _DruckerPragerEP3D::dbStateVars,
			   _DruckerPragerEP3D::numDBStateVars)),
  _calcElasticConstsFn(0),
  _calcStressFn(0),
  _updateStateVarsFn(0)
{ // constructor
  useElasticBehavior(true);
} // constructor

// ----------------------------------------------------------------------
// Destructor.
pylith::materials::DruckerPragerEP3D::~DruckerPragerEP3D(void)
{ // destructor
} // destructor

// ----------------------------------------------------------------------
// Set whether elastic or inelastic constitutive relations are used.
void
pylith::materials::DruckerPragerEP3D::useElasticBehavior(const bool flag)
{ // useElasticBehavior
  if (flag) {
    _calcStressFn = 
      &pylith::materials::DruckerPragerEP3D::_calcStressElastic;
    _calcElasticConstsFn = 
      &pylith::materials::DruckerPragerEP3D::_calcElasticConstsElastic;
    _updateStateVarsFn = 
      &pylith::materials::DruckerPragerEP3D::_updateStateVarsElastic;

  } else {
    _calcStressFn = 
      &pylith::materials::DruckerPragerEP3D::_calcStressElastoplastic;
    _calcElasticConstsFn = 
      &pylith::materials::DruckerPragerEP3D::_calcElasticConstsElastoplastic;
    _updateStateVarsFn = 
      &pylith::materials::DruckerPragerEP3D::_updateStateVarsElastoplastic;
  } // if/else
} // useElasticBehavior

// ----------------------------------------------------------------------
// Compute properties from values in spatial database.
void
pylith::materials::DruckerPragerEP3D::_dbToProperties(
				double* const propValues,
				const double_array& dbValues) const
{ // _dbToProperties
  assert(0 != propValues);
  const int numDBValues = dbValues.size();
  assert(_DruckerPragerEP3D::numDBProperties == numDBValues);

  const double density = dbValues[db_density];
  const double vs = dbValues[db_vs];
  const double vp = dbValues[db_vp];
  const double frictionAngle = dbValues[db_frictionAngle];
  const double cohesion = dbValues[db_cohesion];
  const double dilatationAngle = dbValues[db_dilatationAngle];
 
  if (density <= 0.0 || vs <= 0.0 || vp <= 0.0 || frictionAngle < 0.0
      || cohesion <= 0.0 || dilatationAngle < 0.0
      || frictionAngle < dilatationAngle) {
    std::ostringstream msg;
    msg << "Spatial database returned illegal value for physical "
	<< "properties.\n"
	<< "density: " << density << "\n"
	<< "vp: " << vp << "\n"
	<< "vs: " << vs << "\n"
	<< "frictionAngle: " << frictionAngle << "\n"
	<< "cohesion: " << cohesion << "\n"
	<< "dilatationAngle: " << dilatationAngle << "\n";
    throw std::runtime_error(msg.str());
  } // if

  const double mu = density * vs*vs;
  const double lambda = density * vp*vp - 2.0*mu;
  const double alphaYield =
    2.0 * sin(frictionAngle)/(sqrt(3.0) * (3.0 - sin(frictionAngle)));
  const double beta =
    6.0 * cohesion *
    cos(frictionAngle)/(sqrt(3.0) * (3.0 - sin(frictionAngle)));
  const double alphaFlow =
    2.0 * sin(dilatationAngle)/(sqrt(3.0) * (3.0 - sin(dilatationAngle)));

  if (lambda <= 0.0) {
    std::ostringstream msg;
    msg << "Attempted to set Lame's constant lambda to nonpositive value.\n"
	<< "density: " << density << "\n"
	<< "vp: " << vp << "\n"
	<< "vs: " << vs << "\n";
    throw std::runtime_error(msg.str());
  } // if
  assert(mu > 0);

  propValues[p_density] = density;
  propValues[p_mu] = mu;
  propValues[p_lambda] = lambda;
  propValues[p_alphaYield] = alphaYield;
  propValues[p_cohesion] = cohesion;
  propValues[p_alphaFlow] = alphaFlow;

  PetscLogFlops(28);
} // _dbToProperties

// ----------------------------------------------------------------------
// Nondimensionalize properties.
void
pylith::materials::DruckerPragerEP3D::_nondimProperties(double* const values,
					         const int nvalues) const
{ // _nondimProperties
  assert(0 != _normalizer);
  assert(0 != values);
  assert(nvalues == _numPropsQuadPt);

  const double densityScale = _normalizer->densityScale();
  const double pressureScale = _normalizer->pressureScale();

  values[p_density] = 
    _normalizer->nondimensionalize(values[p_density], densityScale);
  values[p_mu] = 
    _normalizer->nondimensionalize(values[p_mu], pressureScale);
  values[p_lambda] = 
    _normalizer->nondimensionalize(values[p_lambda], pressureScale);
  values[p_beta] = 
    _normalizer->nondimensionalize(values[p_beta],
				   pressureScale);

  PetscLogFlops(4);
} // _nondimProperties

// ----------------------------------------------------------------------
// Dimensionalize properties.
void
pylith::materials::DruckerPragerEP3D::_dimProperties(double* const values,
						      const int nvalues) const
{ // _dimProperties
  assert(0 != _normalizer);
  assert(0 != values);
  assert(nvalues == _numPropsQuadPt);

  const double densityScale = _normalizer->densityScale();
  const double pressureScale = _normalizer->pressureScale();

  values[p_density] = 
    _normalizer->dimensionalize(values[p_density], densityScale);
  values[p_mu] = 
    _normalizer->dimensionalize(values[p_mu], pressureScale);
  values[p_lambda] = 
    _normalizer->dimensionalize(values[p_lambda], pressureScale);
  values[p_beta] = 
    _normalizer->dimensionalize(values[p_beta], pressureScale);

  PetscLogFlops(4);
} // _dimProperties

// ----------------------------------------------------------------------
// Compute initial state variables from values in spatial database.
void
pylith::materials::DruckerPragerEP3D::_dbToStateVars(
				double* const stateValues,
				const double_array& dbValues) const
{ // _dbToStateVars
  assert(0 != stateValues);
  const int numDBValues = dbValues.size();
  assert(_DruckerPragerEP3D::numDBStateVars == numDBValues);

  const int totalSize = _tensorSize;
  assert(totalSize == _numVarsQuadPt);
  assert(totalSize == numDBValues);
  memcpy(&stateValues[s_plasticStrain], &dbValues[db_plasticStrain],
	 _tensorSize*sizeof(double));

  PetscLogFlops(0);
} // _dbToStateVars

// ----------------------------------------------------------------------
// Nondimensionalize state variables.
void
pylith::materials::DruckerPragerEP3D::_nondimStateVars(double* const values,
						const int nvalues) const
{ // _nondimStateVars
  assert(0 != _normalizer);
  assert(0 != values);
  assert(nvalues == _numVarsQuadPt);

  PetscLogFlops(0);
} // _nondimStateVars

// ----------------------------------------------------------------------
// Dimensionalize state variables.
void
pylith::materials::DruckerPragerEP3D::_dimStateVars(double* const values,
					     const int nvalues) const
{ // _dimStateVars
  assert(0 != _normalizer);
  assert(0 != values);
  assert(nvalues == _numVarsQuadPt);

  PetscLogFlops(0);
} // _dimStateVars

// ----------------------------------------------------------------------
// Compute density at location from properties.
void
pylith::materials::DruckerPragerEP3D::_calcDensity(double* const density,
					    const double* properties,
					    const int numProperties,
					    const double* stateVars,
					    const int numStateVars)
{ // _calcDensity
  assert(0 != density);
  assert(0 != properties);
  assert(_numPropsQuadPt == numProperties);

  density[0] = properties[p_density];
} // _calcDensity

// ----------------------------------------------------------------------
// Get stable time step for implicit time integration.
double
pylith::materials::DruckerPragerEP3D::_stableTimeStepImplicit(
				  const double* properties,
				  const int numProperties,
				  const double* stateVars,
				  const int numStateVars) const
{ // _stableTimeStepImplicit
  assert(0 != properties);
  assert(_numPropsQuadPt == numProperties);
  assert(0 != stateVars);
  assert(_numVarsQuadPt == numStateVars);
  // It's unclear what to do for an elasto-plastic material, which has no
  // inherent time scale. For now, just set dtStable to a large value.
  const double dtStable = 1.0e10;
  PetscLogFlops(0);
  return dtStable;
} // _stableTimeStepImplicit

// ----------------------------------------------------------------------
// Compute stress tensor at location from properties as an elastic
// material.
void
pylith::materials::DruckerPragerEP3D::_calcStressElastic(
				         double* const stress,
					 const int stressSize,
					 const double* properties,
					 const int numProperties,
					 const double* stateVars,
					 const int numStateVars,
					 const double* totalStrain,
					 const int strainSize,
					 const double* initialStress,
					 const int initialStressSize,
					 const double* initialStrain,
					 const int initialStrainSize,
					 const bool computeStateVars)
{ // _calcStressElastic
  assert(0 != stress);
  assert(_DruckerPragerEP3D::tensorSize == stressSize);
  assert(0 != properties);
  assert(_numPropsQuadPt == numProperties);
  assert(0 != stateVars);
  assert(_numVarsQuadPt == numStateVars);
  assert(0 != totalStrain);
  assert(_DruckerPragerEP3D::tensorSize == strainSize);
  assert(0 != initialStress);
  assert(_DruckerPragerEP3D::tensorSize == initialStressSize);
  assert(0 != initialStrain);
  assert(_DruckerPragerEP3D::tensorSize == initialStrainSize);

  const double mu = properties[p_mu];
  const double lambda = properties[p_lambda];
  const double mu2 = 2.0 * mu;

  const double e11 = totalStrain[0] - initialStrain[0];
  const double e22 = totalStrain[1] - initialStrain[1];
  const double e33 = totalStrain[2] - initialStrain[2];
  const double e12 = totalStrain[3] - initialStrain[3];
  const double e23 = totalStrain[4] - initialStrain[4];
  const double e13 = totalStrain[5] - initialStrain[5];
  
  const double traceStrainTpdt = e11 + e22 + e33;
  const double s123 = lambda * traceStrainTpdt;

  stress[0] = s123 + mu2*e11 + initialStress[0];
  stress[1] = s123 + mu2*e22 + initialStress[1];
  stress[2] = s123 + mu2*e33 + initialStress[2];
  stress[3] = mu2 * e12 + initialStress[3];
  stress[4] = mu2 * e23 + initialStress[4];
  stress[5] = mu2 * e13 + initialStress[5];

  PetscLogFlops(25);
} // _calcStressElastic

// ----------------------------------------------------------------------
// Compute stress tensor at location from properties as an elastoplastic
// material.
void
pylith::materials::DruckerPragerEP3D::_calcStressElastoplastic(
					double* const stress,
					const int stressSize,
					const double* properties,
					const int numProperties,
					const double* stateVars,
					const int numStateVars,
					const double* totalStrain,
					const int strainSize,
					const double* initialStress,
					const int initialStressSize,
					const double* initialStrain,
					const int initialStrainSize,
					const bool computeStateVars)
{ // _calcStressElastoplastic
  assert(0 != stress);
  assert(_DruckerPragerEP3D::tensorSize == stressSize);
  assert(0 != properties);
  assert(_numPropsQuadPt == numProperties);
  assert(0 != stateVars);
  assert(_numVarsQuadPt == numStateVars);
  assert(0 != totalStrain);
  assert(_DruckerPragerEP3D::tensorSize == strainSize);
  assert(0 != initialStress);
  assert(_DruckerPragerEP3D::tensorSize == initialStressSize);
  assert(0 != initialStrain);
  assert(_DruckerPragerEP3D::tensorSize == initialStrainSize);

  const int tensorSize = _tensorSize;
  const double mu = properties[p_mu];
  const double lambda = properties[p_lambda];
    
  // We need to compute the plastic strain increment if state variables are
  // from previous time step.
  if (computeStateVars) {

    const double alphaYield = properties[p_alphaYield];
    const double beta = properties[p_beta];
    const double alphaFlow = properties[p_alphaFlow];
    const double mu2 = 2.0 * mu;
    const double bulkModulus = lambda + mu2/3.0;
    const double ae = 1.0/mu2;
    const double am = 1.0/(3.0 * bulkModulus);

    const double plasticStrainT[] = {stateVars[s_plasticStrain],
				     stateVars[s_plasticStrain + 1],
				     stateVars[s_plasticStrain + 2],
				     stateVars[s_plasticStrain + 3],
				     stateVars[s_plasticStrain + 4],
				     stateVars[s_plasticStrain + 5]};
    const double meanPlasticStrainT = (plasticStrainT[0] +
				       plasticStrainT[1] +
				       plasticStrainT[2])/3.0;
    const double devPlasticStrainT[] = { plasticStrainT[0] - meanPlasticStrainT,
					 plasticStrainT[1] - meanPlasticStrainT,
					 plasticStrainT[2] - meanPlasticStrainT,
					 plasticStrainT[3],
					 plasticStrainT[4],
					 plasticStrainT[5]};

    const double diag[] = { 1.0, 1.0, 1.0, 0.0, 0.0, 0.0 };

    // Initial stress values
    const double meanStressInitial = (initialStress[0] +
				      initialStress[1] +
				      initialStress[2])/3.0;
    const double devStressInitial[] = { initialStress[0] - meanStressInitial,
					initialStress[1] - meanStressInitial,
					initialStress[2] - meanStressInitial,
					initialStress[3],
					initialStress[4],
					initialStress[5] };

    // Initial strain values
    const double meanStrainInitial = (initialStrain[0] +
				      initialStrain[1] +
				      initialStrain[2])/3.0;
    const double devStrainInitial[] = { initialStrain[0] - meanStrainInitial,
					initialStrain[1] - meanStrainInitial,
					initialStrain[2] - meanStrainInitial,
					initialStrain[3],
					initialStrain[4],
					initialStrain[5] };

    // Values for current time step
    const double e11 = totalStrain[0];
    const double e22 = totalStrain[1];
    const double e33 = totalStrain[2];
    const double meanStrainTpdt = (e11 + e22 + e33)/3.0;
    const double meanStrainPPTpdt = meanStrainTpdt - meanPlasticStrainT -
      meanStrainInitial;

    const double strainPPTpdt[] =
      { totalStrain[0] - meanStrainTpdt - devPlasticStrainT[0] -
	devStrainInitial[0],
	totalStrain[1] - meanStrainTpdt - devPlasticStrainT[1] -
	devStrainInitial[1],
	totalStrain[2] - meanStrainTpdt - devPlasticStrainT[2] -
	devStrainInitial[2],
	totalStrain[3] - devPlasticStrainT[3] - devStrainInitial[3],
	totalStrain[4] - devPlasticStrainT[4] - devStrainInitial[4],
	totalStrain[5] - devPlasticStrainT[5] - devStrainInitial[5] };

    // Compute trial elastic stresses and yield function to see if yield should
    // occur.
    const double trialDevStress[] = { strainPPTpdt[0]/ae + devStressInitial[0],
				      strainPPTpdt[1]/ae + devStressInitial[1],
				      strainPPTpdt[2]/ae + devStressInitial[2],
				      strainPPTpdt[3]/ae + devStressInitial[3],
				      strainPPTpdt[4]/ae + devStressInitial[4],
				      strainPPTpdt[5]/ae + devStressInitial[5]};
    const double trialMeanStress = meanStrainPPTpdt/am + meanStressInitial;
    const double yieldFunction = 3.0* alphaYield * trialMeanStress +
      _scalarProduct(trialDevStress, trialDevStress) - beta;
    PetscLogFlops(74);

    // If yield function is greater than zero, compute elastoplastic stress.
    if (yieldFunction >= 0.0) {
      const double devStressInitialProd = 
	_scalarProduct(devStressInitial, devStressInitial);
      const double strainPPTpdtProd =
	_scalarProduct(strainPPTpdt, strainPPTpdt);
      const double d = sqrt(ae * ae * devStressInitialProd +
			    2.0 * ae *
			    _scalarProduct(devStressInitial, strainPPTpdt) +
			    strainPPTpdtProd);
      plasticMult = 2.0 * ae * am * (3.0 * alphaYield * meanStrainPPTpdt/am +
				     d/(sqrt(2.0) * ae) - beta)/
	(6.0 * alphaYield * alphaFlow * ae + am);
      const double meanStressTpdt =
	(meanStrainPPTpdt - plasticMult * alphaFlow)/am + meanStressInitial;
      double deltaDevPlasticStrain = 0.0;
      double devStressTpdt = 0.0;
      for (int iComp=0; iComp < tensorSize; ++iComp) {
	deltaDevPlasticStrain = plasticMult *(strainPPTpdt[iComp] +
					      ae * devStressInitial[iComp])/
	  (sqrt(2.0) * d);
	devStressTpdt = (strainPPTpdt[iComp] - deltaDevPlasticStrain)/ae +
	  devStressInitial[iComp];
	stress[iComp] = devStressTpdt + diag[iComp] * meanStressTpdt;
      } // for

    PetscLogFlops(62 + 11 * tensorSize);

    } else {
      // No plastic strain.
      const double meanStressTpdt = meanStrainPPTpdt/am + meanStressInitial;
      stress[0] = strainPPTpdt[0]/ae + devStressInitial[0] + meanStressTpdt; 
      stress[1] = strainPPTpdt[1]/ae + devStressInitial[1] + meanStressTpdt; 
      stress[2] = strainPPTpdt[2]/ae + devStressInitial[2] + meanStressTpdt; 
      stress[3] = strainPPTpdt[3]/ae + devStressInitial[3]; 
      stress[4] = strainPPTpdt[4]/ae + devStressInitial[4]; 
      stress[5] = strainPPTpdt[5]/ae + devStressInitial[5]; 
    } // if

    // If state variables have already been updated, the plastic strain for the
    // time step has already been computed.
  } else {
    const double mu2 = 2.0 * mu;
    const double plasticStrainTpdt[] = {stateVars[s_plasticStrain],
					stateVars[s_plasticStrain + 1],
					stateVars[s_plasticStrain + 2],
					stateVars[s_plasticStrain + 3],
					stateVars[s_plasticStrain + 4],
					stateVars[s_plasticStrain + 5]};

    const double e11 = totalStrain[0] - plasticStrainTpdt[0] - initialStrain[0];
    const double e22 = totalStrain[1] - plasticStrainTpdt[1] - initialStrain[1];
    const double e33 = totalStrain[2] - plasticStrainTpdt[2] - initialStrain[2];
    const double e12 = totalStrain[3] - plasticStrainTpdt[3] - initialStrain[3];
    const double e23 = totalStrain[4] - plasticStrainTpdt[4] - initialStrain[4];
    const double e13 = totalStrain[5] - plasticStrainTpdt[5] - initialStrain[5];

    const double traceStrainTpdt = e11 + e22 + e33;
    const double s123 = lambda * traceStrainTpdt;

    stress[0] = s123 + mu2 * e11 + initialStress[0];
    stress[1] = s123 + mu2 * e22 + initialStress[1];
    stress[2] = s123 + mu2 * e33 + initialStress[2];
    stress[3] = mu2 * e12 + initialStress[3];
    stress[4] = mu2 * e23 + initialStress[4];
    stress[5] = mu2 * e13 + initialStress[5];

    PetscLogFlops(31);

  } // else

} // _calcStressElastoplastic

// ----------------------------------------------------------------------
// Compute derivative of elasticity matrix at location from properties.
void
pylith::materials::DruckerPragerEP3D::_calcElasticConstsElastic(
				         double* const elasticConsts,
					 const int numElasticConsts,
					 const double* properties,
					 const int numProperties,
					 const double* stateVars,
					 const int numStateVars,
					 const double* totalStrain,
					 const int strainSize,
					 const double* initialStress,
					 const int initialStressSize,
					 const double* initialStrain,
					 const int initialStrainSize)
{ // _calcElasticConstsElastic
  assert(0 != elasticConsts);
  assert(_DruckerPragerEP3D::numElasticConsts == numElasticConsts);
  assert(0 != properties);
  assert(_numPropsQuadPt == numProperties);
  assert(0 != stateVars);
  assert(_numVarsQuadPt == numStateVars);
  assert(0 != totalStrain);
  assert(_DruckerPragerEP3D::tensorSize == strainSize);
  assert(0 != initialStress);
  assert(_DruckerPragerEP3D::tensorSize == initialStressSize);
  assert(0 != initialStrain);
  assert(_DruckerPragerEP3D::tensorSize == initialStrainSize);
 
  const double mu = properties[p_mu];
  const double lambda = properties[p_lambda];

  const double mu2 = 2.0 * mu;
  const double lambda2mu = lambda + mu2;

  elasticConsts[ 0] = lambda2mu; // C1111
  elasticConsts[ 1] = lambda; // C1122
  elasticConsts[ 2] = lambda; // C1133
  elasticConsts[ 3] = 0; // C1112
  elasticConsts[ 4] = 0; // C1123
  elasticConsts[ 5] = 0; // C1113
  elasticConsts[ 6] = lambda; // C2211
  elasticConsts[ 7] = lambda2mu; // C2222
  elasticConsts[ 8] = lambda; // C2233
  elasticConsts[ 9] = 0; // C2212
  elasticConsts[10] = 0; // C2223
  elasticConsts[11] = 0; // C2213
  elasticConsts[12] = lambda; // C3311
  elasticConsts[13] = lambda; // C3322
  elasticConsts[14] = lambda2mu; // C3333
  elasticConsts[15] = 0; // C3312
  elasticConsts[16] = 0; // C3323
  elasticConsts[17] = 0; // C3313
  elasticConsts[18] = 0; // C1211
  elasticConsts[19] = 0; // C1222
  elasticConsts[20] = 0; // C1233
  elasticConsts[21] = mu2; // C1212
  elasticConsts[22] = 0; // C1223
  elasticConsts[23] = 0; // C1213
  elasticConsts[24] = 0; // C2311
  elasticConsts[25] = 0; // C2322
  elasticConsts[26] = 0; // C2333
  elasticConsts[27] = 0; // C2312
  elasticConsts[28] = mu2; // C2323
  elasticConsts[29] = 0; // C2313
  elasticConsts[30] = 0; // C1311
  elasticConsts[31] = 0; // C1322
  elasticConsts[32] = 0; // C1333
  elasticConsts[33] = 0; // C1312
  elasticConsts[34] = 0; // C1323
  elasticConsts[35] = mu2; // C1313

  PetscLogFlops(2);
} // _calcElasticConstsElastic

// ----------------------------------------------------------------------
// Compute derivative of elasticity matrix at location from properties
// as an elastoplastic material.
void
pylith::materials::DruckerPragerEP3D::_calcElasticConstsElastoplastic(
				         double* const elasticConsts,
					 const int numElasticConsts,
					 const double* properties,
					 const int numProperties,
					 const double* stateVars,
					 const int numStateVars,
					 const double* totalStrain,
					 const int strainSize,
					 const double* initialStress,
					 const int initialStressSize,
					 const double* initialStrain,
					 const int initialStrainSize)
{ // _calcElasticConstsElastoplastic
  assert(0 != elasticConsts);
  assert(_DruckerPragerEP3D::numElasticConsts == numElasticConsts);
  assert(0 != properties);
  assert(_numPropsQuadPt == numProperties);
  assert(0 != stateVars);
  assert(_numVarsQuadPt == numStateVars);
  assert(0 != totalStrain);
  assert(_DruckerPragerEP3D::tensorSize == strainSize);
  assert(0 != initialStress);
  assert(_DruckerPragerEP3D::tensorSize == initialStressSize);
  assert(0 != initialStrain);
  assert(_DruckerPragerEP3D::tensorSize == initialStrainSize);

  // Duplicate functionality of _calcStressElastoplastic
  // Get properties
  const int tensorSize = _tensorSize;
  const double mu = properties[p_mu];
  const double lambda = properties[p_lambda];
  const double alphaYield = properties[p_alphaYield];
  const double beta = properties[p_beta];
  const double alphaFlow = properties[p_alphaFlow];
  const double mu2 = 2.0 * mu;
  const double bulkModulus = lambda + mu2/3.0;
  const double ae = 1.0/mu2;
  const double am = 1.0/(3.0 * bulkModulus);
  
  // Get state variables from previous time step
  const double plasticStrainT[] = {stateVars[s_plasticStrain],
				   stateVars[s_plasticStrain + 1],
				   stateVars[s_plasticStrain + 2],
				   stateVars[s_plasticStrain + 3],
				   stateVars[s_plasticStrain + 4],
				   stateVars[s_plasticStrain + 5]};
  const double meanPlasticStrainT = (plasticStrainT[0] +
				     plasticStrainT[1] +
				     plasticStrainT[2])/3.0;
  const double devPlasticStrainT[] = { plasticStrainT[0] - meanPlasticStrainT,
				       plasticStrainT[1] - meanPlasticStrainT,
				       plasticStrainT[2] - meanPlasticStrainT,
				       plasticStrainT[3],
				       plasticStrainT[4],
				       plasticStrainT[5]};

  const double diag[] = { 1.0, 1.0, 1.0, 0.0, 0.0, 0.0 };

  // Initial stress values
  const double meanStressInitial = (initialStress[0] +
				    initialStress[1] +
				    initialStress[2])/3.0;
  const double devStressInitial[] = { initialStress[0] - meanStressInitial,
				      initialStress[1] - meanStressInitial,
				      initialStress[2] - meanStressInitial,
				      initialStress[3],
				      initialStress[4],
				      initialStress[5] };

  // Initial strain values
  const double meanStrainInitial = (initialStrain[0] +
				    initialStrain[1] +
				    initialStrain[2])/3.0;
  const double devStrainInitial[] = { initialStrain[0] - meanStrainInitial,
				      initialStrain[1] - meanStrainInitial,
				      initialStrain[2] - meanStrainInitial,
				      initialStrain[3],
				      initialStrain[4],
				      initialStrain[5] };

  // Values for current time step
  const double e11 = totalStrain[0];
  const double e22 = totalStrain[1];
  const double e33 = totalStrain[2];
  const double meanStrainTpdt = (e11 + e22 + e33)/3.0;
  const double meanStrainPPTpdt = meanStrainTpdt - meanPlasticStrainT -
    meanStrainInitial;
  
  const double strainPPTpdt[] =
    { totalStrain[0] - meanStrainTpdt - devPlasticStrainT[0] -
      devStrainInitial[0],
      totalStrain[1] - meanStrainTpdt - devPlasticStrainT[1] -
      devStrainInitial[1],
      totalStrain[2] - meanStrainTpdt - devPlasticStrainT[2] -
      devStrainInitial[2],
      totalStrain[3] - devPlasticStrainT[3] - devStrainInitial[3],
      totalStrain[4] - devPlasticStrainT[4] - devStrainInitial[4],
      totalStrain[5] - devPlasticStrainT[5] - devStrainInitial[5] };
  
  // Compute trial elastic stresses and yield function to see if yield should
  // occur.
  const double trialDevStress[] = { strainPPTpdt[0]/ae + devStressInitial[0],
				    strainPPTpdt[1]/ae + devStressInitial[1],
				    strainPPTpdt[2]/ae + devStressInitial[2],
				    strainPPTpdt[3]/ae + devStressInitial[3],
				    strainPPTpdt[4]/ae + devStressInitial[4],
				    strainPPTpdt[5]/ae + devStressInitial[5]};
  const double trialMeanStress = meanStrainPPTpdt/am + meanStressInitial;
  const double yieldFunction = 3.0* alphaYield * trialMeanStress +
    _scalarProduct(trialDevStress, trialDevStress) - beta;
  PetscLogFlops(74);
  
  // If yield function is greater than zero, compute elastoplastic stress and
  // corresponding tangent matrix.
  if (yieldFunction >= 0.0) {
    const double devStressInitialProd = 
      _scalarProduct(devStressInitial, devStressInitial);
    const double strainPPTpdtProd =
      _scalarProduct(strainPPTpdt, strainPPTpdt);
    const double d = sqrt(ae * ae * devStressInitialProd +
			  2.0 * ae *
			  _scalarProduct(devStressInitial, strainPPTpdt) +
			  strainPPTpdtProd);
    plasticMult = 2.0 * ae * am * (3.0 * alphaYield * meanStrainPPTpdt/am +
				   d/(sqrt(2.0) * ae) - beta)/
      (6.0 * alphaYield * alphaFlow * ae + am);
    const double meanStressTpdt =
      (meanStrainPPTpdt - plasticMult * alphaFlow)/am + meanStressInitial;
    double deltaDevPlasticStrain = 0.0;
    double devStressTpdt = 0.0;
    for (int iComp=0; iComp < tensorSize; ++iComp) {
      deltaDevPlasticStrain = plasticMult *(strainPPTpdt[iComp] +
					    ae * devStressInitial[iComp])/
	(sqrt(2.0) * d);
      devStressTpdt = (strainPPTpdt[iComp] - deltaDevPlasticStrain)/ae +
	devStressInitial[iComp];
      stress[iComp] = devStressTpdt + diag[iComp] * meanStressTpdt;
    } // for

    // Define some constants and vectors
    const double dDdEPrime[] = {
      (ae * devStressInitial[0] + strainPPTpdt[0])/d,
      (ae * devStressInitial[1] + strainPPTpdt[1])/d,
      (ae * devStressInitial[2] + strainPPTpdt[2])/d,
      2.0 * (ae * devStressInitial[0] + strainPPTpdt[0])/d,
      2.0 * (ae * devStressInitial[0] + strainPPTpdt[0])/d,
      2.0 * (ae * devStressInitial[0] + strainPPTpdt[0])/d};
    const double const1 = 2.0 * ae * am/
      (6.0 * alphaYield * alphaFlow * ae + am);
    const double const2 = 3.0 * alphaYield/am;
    const double const3 = 1.0/(sqrt(2.0) * ae);
    const double dLambdadEPrime[] = {
      const1 * (-1.5 * const2 + const3 * dDdEPrime[0]),
      const1 * (-1.5 * const2 + const3 * dDdEPrime[1]),
      const1 * (-1.5 * const2 + const3 * dDdEPrime[2]),
      const1 * (                const3 * dDdEPrime[3]),
      const1 * (                const3 * dDdEPrime[4]),
      const1 * (                const3 * dDdEPrime[5])};


    PetscLogFlops(62 + 11 * tensorSize);

    } else {
      // No plastic strain.
      const double meanStressTpdt = meanStrainPPTpdt/am + meanStressInitial;
      stress[0] = strainPPTpdt[0]/ae + devStressInitial[0] + meanStressTpdt; 
      stress[1] = strainPPTpdt[1]/ae + devStressInitial[1] + meanStressTpdt; 
      stress[2] = strainPPTpdt[2]/ae + devStressInitial[2] + meanStressTpdt; 
      stress[3] = strainPPTpdt[3]/ae + devStressInitial[3]; 
      stress[4] = strainPPTpdt[4]/ae + devStressInitial[4]; 
      stress[5] = strainPPTpdt[5]/ae + devStressInitial[5]; 
    } // if

    // If state variables have already been updated, the plastic strain for the
    // time step has already been computed.
  } else {
    const double mu2 = 2.0 * mu;
    const double plasticStrainTpdt[] = {stateVars[s_plasticStrain],
					stateVars[s_plasticStrain + 1],
					stateVars[s_plasticStrain + 2],
					stateVars[s_plasticStrain + 3],
					stateVars[s_plasticStrain + 4],
					stateVars[s_plasticStrain + 5]};

    const double e11 = totalStrain[0] - plasticStrainTpdt[0] - initialStrain[0];
    const double e22 = totalStrain[1] - plasticStrainTpdt[1] - initialStrain[1];
    const double e33 = totalStrain[2] - plasticStrainTpdt[2] - initialStrain[2];
    const double e12 = totalStrain[3] - plasticStrainTpdt[3] - initialStrain[3];
    const double e23 = totalStrain[4] - plasticStrainTpdt[4] - initialStrain[4];
    const double e13 = totalStrain[5] - plasticStrainTpdt[5] - initialStrain[5];

    const double traceStrainTpdt = e11 + e22 + e33;
    const double s123 = lambda * traceStrainTpdt;

    stress[0] = s123 + mu2 * e11 + initialStress[0];
    stress[1] = s123 + mu2 * e22 + initialStress[1];
    stress[2] = s123 + mu2 * e33 + initialStress[2];
    stress[3] = mu2 * e12 + initialStress[3];
    stress[4] = mu2 * e23 + initialStress[4];
    stress[5] = mu2 * e13 + initialStress[5];

    PetscLogFlops(31);

  } // else
  const int tensorSize = _tensorSize;

  const double mu = properties[p_mu];
  const double lambda = properties[p_lambda];
  const double referenceStrainRate = properties[p_referenceStrainRate];
  const double referenceStress = properties[p_referenceStress];
  const double powerLawExp = properties[p_powerLawExponent];
    
  // State variables.
  const double visStrainT[] = {stateVars[s_viscousStrain],
			       stateVars[s_viscousStrain + 1],
			       stateVars[s_viscousStrain + 2],
			       stateVars[s_viscousStrain + 3],
			       stateVars[s_viscousStrain + 4],
			       stateVars[s_viscousStrain + 5]};
  const double stressT[] = {stateVars[s_stress],
			    stateVars[s_stress + 1],
			    stateVars[s_stress + 2],
			    stateVars[s_stress + 3],
			    stateVars[s_stress + 4],
			    stateVars[s_stress + 5]};

  const double mu2 = 2.0 * mu;
  const double lamPlusMu = lambda + mu;
  const double bulkModulus = lambda + mu2/3.0;
  const double ae = 1.0/mu2;
  const double diag[] = { 1.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
    
  // Need to figure out how time integration parameter alpha is going to be
  // specified.  It should probably be specified in the problem definition and
  // then used only by the material types that use it.  For now we are setting
  // it to 0.5, which should probably be the default value.
  const double alpha = 0.5;
  const double explicitFac = 1.0 - alpha;
  const double timeFac = _dt * explicitFac;
    
  /// Initial state.
  // Initial stress values.
  const double meanStressInitial = (initialStress[0] +
				    initialStress[1] +
				    initialStress[2])/3.0;
  const double devStressInitial[] = { initialStress[0] - meanStressInitial,
				      initialStress[1] - meanStressInitial,
				      initialStress[2] - meanStressInitial,
				      initialStress[3],
				      initialStress[4],
				      initialStress[5] };
  const double stressInvar2Initial = 0.5 *
    _scalarProduct(devStressInitial, devStressInitial);

  // Initial strain values.
  const double meanStrainInitial = (initialStrain[0] +
				    initialStrain[1] +
				    initialStrain[2])/3.0;
  
  /// Values for current time step
  const double e11 = totalStrain[0];
  const double e22 = totalStrain[1];
  const double e33 = totalStrain[2];
  const double meanStrainTpdt = (e11 + e22 + e33)/3.0 - meanStrainInitial;
  const double meanStressTpdt = 3.0 * bulkModulus * meanStrainTpdt;
  
  // Note that I use the initial strain rather than the deviatoric initial
  // strain since otherwise the initial mean strain would get used twice.
  
  const double strainPPTpdt[] =
    { totalStrain[0] - meanStrainTpdt - visStrainT[0] - initialStrain[0],
      totalStrain[1] - meanStrainTpdt - visStrainT[1] - initialStrain[1],
      totalStrain[2] - meanStrainTpdt - visStrainT[2] - initialStrain[2],
      totalStrain[3] - visStrainT[3] - initialStrain[3],
      totalStrain[4] - visStrainT[4] - initialStrain[4],
      totalStrain[5] - visStrainT[5] - initialStrain[5] };
  const double strainPPInvar2Tpdt = 0.5 *
    _scalarProduct(strainPPTpdt, strainPPTpdt);
  
  // Values for previous time step
  const double meanStressT = (stressT[0] + stressT[1] + stressT[2])/3.0;
  const double devStressT[] = { stressT[0] - meanStressT,
				stressT[1] - meanStressT,
				stressT[2] - meanStressT,
				stressT[3],
				stressT[4],
				stressT[5] };
  const double stressInvar2T = 0.5 * _scalarProduct(devStressT, devStressT);
  const double effStressT = sqrt(stressInvar2T);
    
  // Finish defining parameters needed for root-finding algorithm.
  const double b = strainPPInvar2Tpdt +
    ae * _scalarProduct(strainPPTpdt, devStressInitial) +
    ae * ae * stressInvar2Initial;
  const double c = (_scalarProduct(strainPPTpdt, devStressT) +
		    ae * _scalarProduct(devStressT, devStressInitial)) *
    timeFac;
  const double d = timeFac * effStressT;

  PetscLogFlops(92);

  // If b = c = d = 0, the effective stress is zero and the elastic constants
  // will be the same as for the elastic case. Otherwise, compute the tangent
  // matrix using the effective stress function algorithm.
  if (b == 0.0 && c == 0.0 && d == 0.0) {
    _calcElasticConstsElastic(elasticConsts,
			      numElasticConsts,
			      properties,
			      numProperties,
			      stateVars,
			      numStateVars,
			      totalStrain,
			      strainSize,
			      initialStress,
			      initialStressSize,
			      initialStrain,
			      initialStrainSize);
  } else {
    const double stressScale = mu;
  
    // Put parameters into a struct and call root-finding algorithm.
    _effStressParams.ae = ae;
    _effStressParams.b = b;
    _effStressParams.c = c;
    _effStressParams.d = d;
    _effStressParams.alpha = alpha;
    _effStressParams.dt = _dt;
    _effStressParams.effStressT = effStressT;
    _effStressParams.powerLawExp = powerLawExp;
    _effStressParams.referenceStrainRate = referenceStrainRate;
    _effStressParams.referenceStress = referenceStress;
    
    const double effStressInitialGuess = effStressT;
    
    const double effStressTpdt =
      EffectiveStress::calculate<DruckerPragerEP3D>(effStressInitialGuess,
					     stressScale, this);
  
    // Compute quantities at intermediate time tau used to compute values at
    // end of time step.
    const double effStressTau = (1.0 - alpha) * effStressT +
      alpha * effStressTpdt;
    const double gammaTau = referenceStrainRate *
      pow((effStressTau/referenceStress),
	  (powerLawExp - 1.0))/referenceStress;
    const double a = ae + alpha * _dt * gammaTau;
    const double factor1 = 1.0/a;
    const double factor2 = timeFac * gammaTau;
    const double devStressTpdt[] = {
      factor1 *
      (strainPPTpdt[0] - factor2 * devStressT[0] + ae * devStressInitial[0]),
      factor1 *
      (strainPPTpdt[1] - factor2 * devStressT[1] + ae * devStressInitial[1]),
      factor1 *
      (strainPPTpdt[2] - factor2 * devStressT[2] + ae * devStressInitial[2]),
      factor1 *
      (strainPPTpdt[3] - factor2 * devStressT[3] + ae * devStressInitial[3]),
      factor1 *
      (strainPPTpdt[4] - factor2 * devStressT[4] + ae * devStressInitial[4]),
      factor1 *
      (strainPPTpdt[5] - factor2 * devStressT[5] + ae * devStressInitial[5])};
    const double devStressTau[] = {
      alpha * devStressT[0] + explicitFac * devStressTpdt[0],
      alpha * devStressT[1] + explicitFac * devStressTpdt[1],
      alpha * devStressT[2] + explicitFac * devStressTpdt[2],
      alpha * devStressT[3] + explicitFac * devStressTpdt[3],
      alpha * devStressT[4] + explicitFac * devStressTpdt[4],
      alpha * devStressT[5] + explicitFac * devStressTpdt[5]};
    const double factor3 = 0.5 * referenceStrainRate * _dt * alpha *
      (powerLawExp - 1.0) *
      pow((effStressTau/referenceStress), (powerLawExp - 2.0))/
      (referenceStress * referenceStress * effStressTpdt);

    // Compute deviatoric derivatives
    const double dStress11dStrain11 = 1.0/
      (a + devStressTau[0] * devStressTpdt[0] * factor3);
    const double dStress22dStrain22 = 1.0/
      (a + devStressTau[1] * devStressTpdt[1] * factor3);
    const double dStress33dStrain33 = 1.0/
      (a + devStressTau[2] * devStressTpdt[2] * factor3);
    const double dStress12dStrain12 = 1.0/
      (a + 2.0 * devStressTau[3] * devStressTpdt[3] * factor3);
    const double dStress23dStrain23 = 1.0/
      (a + 2.0 * devStressTau[4] * devStressTpdt[4] * factor3);
    const double dStress13dStrain13 = 1.0/
      (a + 2.0 * devStressTau[5] * devStressTpdt[5] * factor3);
    
    /// Compute tangent matrix.
    // Form elastic constants.
    elasticConsts[ 0] = bulkModulus + 2.0 * dStress11dStrain11/3.0;  // C1111
    elasticConsts[ 1] = bulkModulus -       dStress11dStrain11/3.0;  // C1122
    elasticConsts[ 2] = elasticConsts[ 1]; // C1133
    elasticConsts[ 3] = 0.0;  // C1112
    elasticConsts[ 4] = 0.0;  // C1123
    elasticConsts[ 5] = 0.0;  // C1113
    elasticConsts[ 6] = bulkModulus + 2.0 * dStress22dStrain22/3.0;  // C2222
    elasticConsts[ 7] = bulkModulus -       dStress22dStrain22/3.0;  // C2233
    elasticConsts[ 8] = 0.0;  // C2212
    elasticConsts[ 9] = 0.0;  // C2223
    elasticConsts[10] = 0.0;  // C2213
    elasticConsts[11] = bulkModulus + 2.0 * dStress33dStrain33/3.0;  // C3333
    elasticConsts[12] = 0.0;  // C3312
    elasticConsts[13] = 0.0;  // C3323
    elasticConsts[14] = 0.0;  // C3313
    elasticConsts[15] = dStress12dStrain12;  // C1212
    elasticConsts[16] = 0.0;  // C1223
    elasticConsts[17] = 0.0;  // C1213
    elasticConsts[18] = dStress23dStrain23;  // C2323
    elasticConsts[19] = 0.0;  // C2313
    elasticConsts[20] = dStress13dStrain13;  // C1313
    
    PetscLogFlops(114);
  } // else
} // _calcElasticConstsElastoplastic

// ----------------------------------------------------------------------
// Update state variables.
void
pylith::materials::DruckerPragerEP3D::_updateStateVarsElastic(
				    double* const stateVars,
				    const int numStateVars,
				    const double* properties,
				    const int numProperties,
				    const double* totalStrain,
				    const int strainSize,
				    const double* initialStress,
				    const int initialStressSize,
				    const double* initialStrain,
				    const int initialStrainSize)
{ // _updateStateVarsElastic
  assert(0 != stateVars);
  assert(_numVarsQuadPt == numStateVars);
  assert(0 != properties);
  assert(_numPropsQuadPt == numProperties);
  assert(0 != totalStrain);
  assert(_DruckerPragerEP3D::tensorSize == strainSize);
  assert(0 != initialStress);
  assert(_DruckerPragerEP3D::tensorSize == initialStressSize);
  assert(0 != initialStrain);
  assert(_DruckerPragerEP3D::tensorSize == initialStrainSize);

  for (int iComp=0; iComp < _tensorSize; ++iComp) {
    stateVars[s_plasticStrain+iComp] = 0.0;
  } // for

  _needNewJacobian = true;
} // _updateStateVarsElastic

// ----------------------------------------------------------------------
// Update state variables.
void
pylith::materials::DruckerPragerEP3D::_updateStateVarsElastoplastic(
				    double* const stateVars,
				    const int numStateVars,
				    const double* properties,
				    const int numProperties,
				    const double* totalStrain,
				    const int strainSize,
				    const double* initialStress,
				    const int initialStressSize,
				    const double* initialStrain,
				    const int initialStrainSize)
{ // _updateStateVarsElastoplastic
  assert(0 != stateVars);
  assert(_numVarsQuadPt == numStateVars);
  assert(0 != properties);
  assert(_numPropsQuadPt == numProperties);
  assert(0 != totalStrain);
  assert(_DruckerPragerEP3D::tensorSize == strainSize);
  assert(0 != initialStress);
  assert(_DruckerPragerEP3D::tensorSize == initialStressSize);
  assert(0 != initialStrain);
  assert(_DruckerPragerEP3D::tensorSize == initialStrainSize);

  const int stressSize = _tensorSize;

  // For now, we are duplicating the functionality of _calcStressElastoplastic,
  // since otherwise we would have to redo a lot of calculations.

  const int tensorSize = _tensorSize;
  const double mu = properties[p_mu];
  const double lambda = properties[p_lambda];
  const double alphaYield = properties[p_alphaYield];
  const double beta = properties[p_beta];
  const double alphaFlow = properties[p_alphaFlow];
  const double mu2 = 2.0 * mu;
  const double bulkModulus = lambda + mu2/3.0;
  const double ae = 1.0/mu2;
  const double am = 1.0/(3.0 * bulkModulus);

  const double plasticStrainT[] = {stateVars[s_plasticStrain],
				   stateVars[s_plasticStrain + 1],
				   stateVars[s_plasticStrain + 2],
				   stateVars[s_plasticStrain + 3],
				   stateVars[s_plasticStrain + 4],
				   stateVars[s_plasticStrain + 5]};
  const double meanPlasticStrainT = (plasticStrainT[0] +
				     plasticStrainT[1] +
				     plasticStrainT[2])/3.0;
  const double devPlasticStrainT[] = { plasticStrainT[0] - meanPlasticStrainT,
				       plasticStrainT[1] - meanPlasticStrainT,
				       plasticStrainT[2] - meanPlasticStrainT,
				       plasticStrainT[3],
				       plasticStrainT[4],
				       plasticStrainT[5]};

  const double diag[] = { 1.0, 1.0, 1.0, 0.0, 0.0, 0.0 };

  // Initial stress values
  const double meanStressInitial = (initialStress[0] +
				    initialStress[1] +
				    initialStress[2])/3.0;
  const double devStressInitial[] = { initialStress[0] - meanStressInitial,
				      initialStress[1] - meanStressInitial,
				      initialStress[2] - meanStressInitial,
				      initialStress[3],
				      initialStress[4],
				      initialStress[5] };

  // Initial strain values
  const double meanStrainInitial = (initialStrain[0] +
				    initialStrain[1] +
				    initialStrain[2])/3.0;
  const double devStrainInitial[] = { initialStrain[0] - meanStrainInitial,
				      initialStrain[1] - meanStrainInitial,
				      initialStrain[2] - meanStrainInitial,
				      initialStrain[3],
				      initialStrain[4],
				      initialStrain[5] };

  // Values for current time step
  const double e11 = totalStrain[0];
  const double e22 = totalStrain[1];
  const double e33 = totalStrain[2];
  const double meanStrainTpdt = (e11 + e22 + e33)/3.0;
  const double meanStrainPPTpdt = meanStrainTpdt - meanPlasticStrainT -
    meanStrainInitial;

  const double strainPPTpdt[] =
    { totalStrain[0] - meanStrainTpdt - devPlasticStrainT[0] -
      devStrainInitial[0],
      totalStrain[1] - meanStrainTpdt - devPlasticStrainT[1] -
      devStrainInitial[1],
      totalStrain[2] - meanStrainTpdt - devPlasticStrainT[2] -
      devStrainInitial[2],
      totalStrain[3] - devPlasticStrainT[3] - devStrainInitial[3],
      totalStrain[4] - devPlasticStrainT[4] - devStrainInitial[4],
      totalStrain[5] - devPlasticStrainT[5] - devStrainInitial[5] };

  // Compute trial elastic stresses and yield function to see if yield should
  // occur.
  const double trialDevStress[] = { strainPPTpdt[0]/ae + devStressInitial[0],
				    strainPPTpdt[1]/ae + devStressInitial[1],
				    strainPPTpdt[2]/ae + devStressInitial[2],
				    strainPPTpdt[3]/ae + devStressInitial[3],
				    strainPPTpdt[4]/ae + devStressInitial[4],
				    strainPPTpdt[5]/ae + devStressInitial[5]};
  const double trialMeanStress = meanStrainPPTpdt/am + meanStressInitial;
  const double yieldFunction = 3.0* alphaYield * trialMeanStress +
    _scalarProduct(trialDevStress, trialDevStress) - beta;
  PetscLogFlops(74);

  // If yield function is greater than zero, compute plastic strains.
  // Otherwise, plastic strains remain the same.
  if (yieldFunction >= 0.0) {
    const double devStressInitialProd = 
      _scalarProduct(devStressInitial, devStressInitial);
    const double strainPPTpdtProd =
      _scalarProduct(strainPPTpdt, strainPPTpdt);
    const double d = sqrt(ae * ae * devStressInitialProd +
			  2.0 * ae *
			  _scalarProduct(devStressInitial, strainPPTpdt) +
			  strainPPTpdtProd);
    plasticMult = 2.0 * ae * am * (3.0 * alphaYield * meanStrainPPTpdt/am +
				   d/(sqrt(2.0) * ae) - beta)/
      (6.0 * alphaYield * alphaFlow * ae + am);
    const double deltaMeanPlasticStrain = plasticMult * alphaFlow;
    double deltaDevPlasticStrain = 0.0;
    for (int iComp=0; iComp < tensorSize; ++iComp) {
      deltaDevPlasticStrain = plasticMult *(strainPPTpdt[iComp] +
					    ae * devStressInitial[iComp])/
	(sqrt(2.0) * d);
      stateVars[s_plasticStrain+iComp] += deltaDevPlasticStrain +
	diag[iComp] * deltaMeanPlasticStrain;
    } // for

    PetscLogFlops(60 + 9 * tensorSize);

  } // if

  _needNewJacobian = true;

} // _updateStateVarsElastoplastic

// ----------------------------------------------------------------------
// Compute scalar product of two tensors.
double
pylith::materials::DruckerPragerEP3D::_scalarProduct(
				    const double* tensor1,
				    const double* tensor2) const
{ // _scalarProduct
  const double scalarProduct = tensor1[0] * tensor2[0] +
    tensor1[1] * tensor2[1] +
    tensor1[2] * tensor2[2] +
    2.0 * (tensor1[3] * tensor2[3] +
	   tensor1[4] * tensor2[4] +
	   tensor1[5] * tensor2[5]);
  return scalarProduct;

} // _scalarProduct

// End of file 