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

/**
 * @file libsrc/topology/Fields.hh
 *
 * @brief Object for managing fields over a finite-element mesh.
 */

#if !defined(pylith_topology_fields_hh)
#define pylith_topology_fields_hh

// Include directives ---------------------------------------------------
#include "topologyfwd.hh" // forward declarations

#include "pylith/topology/FieldBase.hh" // USES FieldBase::DomainEnum

#include <string> // USES std::string

// Fields ---------------------------------------------------------------
template<typename field_type>
class pylith::topology::Fields
{ // Fields
  friend class TestFieldsMesh; // unit testing
  friend class TestFieldsSubMesh; // unit testing

// PUBLIC MEMBERS ///////////////////////////////////////////////////////
public :

  /** Default constructor.
   *
   * @param mesh Finite-element mesh.
   */
  Fields(const typename field_type::Mesh& mesh);

  /// Destructor.
  ~Fields(void);

  /// Deallocate PETSc and local data structures.
  void deallocate(void);
  
  /** Add field.
   *
   * @param name Name of field.
   * @param label Label for field.
   */
  void add(const char* name,
	   const char* label);

  /** Add field.
   *
   * @param name Name of field.
   * @param label Label for field.
   * @param domain Type of points over which to define field.
   * @param fiberDim Fiber dimension for field.
   */
  void add(const char* name,
	   const char* label,
	   const pylith::topology::FieldBase::DomainEnum domain,
	   const int fiberDim);

  /** Delete field.
   *
   * @param name Name of field.
   */
  void del(const char* name);

  /** Delete field (without conflict with Python del).
   *
   * @param name Name of field.
   */
  void delField(const char* name);

  /** Get field.
   *
   * @param name Name of field.
   */
  const field_type& get(const char* name) const;
	   
  /** Get field.
   *
   * @param name Name of field.
   */
  field_type& get(const char* name);
	   
  /** Copy layout to other fields.
   *
   * @param name Name of field to use as template for layout.
   */
  void copyLayout(const char* name);

  /** Get mesh associated with fields.
   *
   * @returns Finite-element mesh.
   */
  const typename field_type::Mesh& mesh(void) const;

// PROTECTED TYPEDEFS ///////////////////////////////////////////////////
protected :

  typedef std::map< std::string, field_type* > map_type;

// PROTECTED MEMBERS ////////////////////////////////////////////////////
protected :

  map_type _fields;
  const typename field_type::Mesh& _mesh;

// NOT IMPLEMENTED //////////////////////////////////////////////////////
private :

  Fields(const Fields&); ///< Not implemented
  const Fields& operator=(const Fields&); ///< Not implemented

}; // Fields

#include "Fields.icc"

#endif // pylith_topology_fields_hh


// End of file 