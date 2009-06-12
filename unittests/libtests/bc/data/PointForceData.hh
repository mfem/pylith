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

#if !defined(pylith_bc_pointforcedata_hh)
#define pylith_bc_pointforcedata_hh

namespace pylith {
  namespace bc {
     class PointForceData;
  } // pylith
} // bc

class pylith::bc::PointForceData
{

// PUBLIC METHODS ///////////////////////////////////////////////////////
public :
  
  /// Constructor
  PointForceData(void);

  /// Destructor
  ~PointForceData(void);

// PUBLIC MEMBERS ///////////////////////////////////////////////////////
public:

  double tRef; ///< Reference time for rate of change of forces.
  double forceRate; ///< Rate of change of force.
  double tResidual; ///< Time for computing residual.

  int numDOF; ///< Number of degrees of freedom at each point.
  int numForceDOF; ///< Number of forces at points.
  int numForcePts; ///< Number of points with forces.

  int id; ///< Boundary condition identifier
  char* label; ///< Label for boundary condition group

  int* forceDOF; ///< Degrees of freedom that are constrained at each point
  int* forcePoints; ///< Array of indices of points with forces.
  double* forceInitial; ///< Forces at points.
  double* residual; ///< Residual field.

  char* meshFilename; ///< Filename for input mesh.
  char* dbFilename; ///< Filename of simple spatial database.
};

#endif // pylith_bc_pointforcedata_hh

// End of file