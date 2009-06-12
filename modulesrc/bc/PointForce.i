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

/** @file modulesrc/bc/PointForce.i
 *
 * @brief Python interface to C++ PointForce object.
 */

namespace pylith {
  namespace bc {

    class pylith::bc::PointForce : public TimeDependentPoints,
				   public pylith::feassemble::Integrator<pylith::feassemble::Quadrature<pylith::topology::Mesh> >
    { // class PointForce

      // PUBLIC METHODS /////////////////////////////////////////////////
    public :

      /// Default constructor.
      PointForce(void);
      
      /// Destructor.
      ~PointForce(void);
      
      /// Deallocate PETSc and local data structures.
      void deallocate(void);
  
      /** Initialize boundary condition.
       *
       * @param mesh PETSc mesh
       * @param upDir Vertical direction (somtimes used in 3-D problems).
       */
      void initialize(const pylith::topology::Mesh& mesh,
		      const double upDir[3]);
      
      /** Integrate contributions to residual term (r) for operator.
       *
       * @param residual Field containing values for residual
       * @param t Current time
       * @param fields Solution fields
       */
      void integrateResidualAssembled(pylith::topology::Field<pylith::topology::Mesh>* residual,
				      const double t,
				      pylith::topology::SolutionFields* const fields);
      
      /** Verify configuration is acceptable.
       *
       * @param mesh Finite-element mesh
       */
      void verifyConfiguration(const pylith::topology::Mesh& mesh) const;

      // PROTECTED METHODS //////////////////////////////////////////////////
    protected :
      
      /** Get manager of scales used to nondimensionalize problem.
       *
       * @returns Nondimensionalizer.
       */
      const spatialdata::units::Nondimensional& _getNormalizer(void) const;

    }; // class PointForce

  } // bc
} // pylith


// End of file 