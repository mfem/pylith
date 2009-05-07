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

#include <portinfo>

#include "SolutionFields.hh" // implementation of class methods

#include "pylith/utils/petscerror.h" // USES CHECK_PETSC_ERROR

// ----------------------------------------------------------------------
// Default constructor.
pylith::topology::SolutionFields::SolutionFields(const Mesh& mesh) :
  Fields<Field<Mesh> >(mesh),
  _solutionName(""),
  _solveSolnName("")
{ // constructor
} // constructor

// ----------------------------------------------------------------------
// Destructor.
pylith::topology::SolutionFields::~SolutionFields(void)
{ // destructor
} // destructor

// ----------------------------------------------------------------------
// Set name of solution field.
void
pylith::topology::SolutionFields::solutionName(const char* name)
{ // solutionName
  map_type::const_iterator iter = _fields.find(name);
  if (iter == _fields.end()) {
    std::ostringstream msg;
    msg << "Cannot use unknown field '" << name 
	<< "' when setting name of solution field.";
    throw std::runtime_error(msg.str());
  } // if
  _solutionName = name;
} // solutionName

// ----------------------------------------------------------------------
// Get solution field.
const pylith::topology::Field<pylith::topology::Mesh>&
pylith::topology::SolutionFields::solution(void) const
{ // solution
  if (_solutionName == "")
    throw std::runtime_error("Cannot retrieve solution. Name of solution " \
			     "field has not been specified.");
  return get(_solutionName.c_str());
} // solution

// ----------------------------------------------------------------------
// Get solution field.
pylith::topology::Field<pylith::topology::Mesh>&
pylith::topology::SolutionFields::solution(void)
{ // solution
  if (_solutionName == "")
    throw std::runtime_error("Cannot retrieve solution. Name of solution " \
			     "field has not been specified.");
  return get(_solutionName.c_str());
} // solution

// ----------------------------------------------------------------------
// Set field used in the solve.
void
pylith::topology::SolutionFields::solveSolnName(const char* name)
{ // solveSolnName
  map_type::const_iterator iter = _fields.find(name);
  if (iter == _fields.end()) {
    std::ostringstream msg;
    msg << "Cannot use unknown field '" << name 
	<< "' when setting name of field used in solve.";
    throw std::runtime_error(msg.str());
  } // if
  _solveSolnName = name;
} // solveSolnName

// ----------------------------------------------------------------------
// Get solveSoln field.
const pylith::topology::Field<pylith::topology::Mesh>&
pylith::topology::SolutionFields::solveSoln(void) const
{ // solveSoln
  if (_solveSolnName == "")
    throw std::runtime_error("Cannot retrieve solve field. Name of solve "
			     "field has not been specified.");
  return get(_solveSolnName.c_str());
} // solveSoln

// ----------------------------------------------------------------------
// Get solveSoln field.
pylith::topology::Field<pylith::topology::Mesh>&
pylith::topology::SolutionFields::solveSoln(void)
{ // solveSoln
  if (_solveSolnName == "")
    throw std::runtime_error("Cannot retrieve solve field. Name of solve "
			     "field has not been specified.");
  return get(_solveSolnName.c_str());
} // solveSoln

// ----------------------------------------------------------------------
// Create history manager for a subset of the managed fields.
void
pylith::topology::SolutionFields::createHistory(const char* const* fields,
						const int size)
{ // createHistory
  if (size > 0 && 0 != fields) {
    _history.resize(size);
    for (int i=0; i < size; ++i) {
      map_type::const_iterator iter = _fields.find(fields[i]);
      if (iter == _fields.end()) {
	std::ostringstream msg;
	msg << "Cannot use unknown field '" << fields[i] 
	    << "' when creating history.";
	throw std::runtime_error(msg.str());
      } // if
      _history[i] = fields[i];
    } // for
  } // if
} // createHistory

// ----------------------------------------------------------------------
// Shift fields in history.
void
pylith::topology::SolutionFields::shiftHistory(void)
{ // shiftHistory
  assert(_history.size() > 0);
  const int size = _history.size();
  Field<Mesh>* tmp = _fields[_history[size-1]];
  for (int i=size-1; i > 0; --i)
    _fields[_history[i]] = _fields[_history[i-1]];
  _fields[_history[0]] = tmp;
} // shiftHistory


// End of file 