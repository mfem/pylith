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

#include <portinfo>

#include "RefineEdges2.hh" // implementation of class methods

#include "MeshOrder.hh" // USES MeshOrder

#include <cassert> // USES assert()

// ----------------------------------------------------------------------
// Constructor
ALE::RefineEdges2::RefineEdges2(const mesh_type& mesh) :
  _mesh(mesh)
{ // constructor
} // constructor

// ----------------------------------------------------------------------
// Destructor
ALE::RefineEdges2::~RefineEdges2(void)
{ // destructor
} // destructor

// ----------------------------------------------------------------------
// Set coordinates of new vertices.
void
ALE::RefineEdges2::setCoordsNewVertices(const ALE::Obj<mesh_type::real_section_type>& newCoordsSection,
					   const ALE::Obj<mesh_type::real_section_type>& oldCoordsSection)
{ // setCoordsNewVertices
  assert(!newCoordsSection.isNull());
  assert(!oldCoordsSection.isNull());

  double coordinatesVertex[3];

  assert(_edgeToVertex.size() > 0);
  const int spaceDim = newCoordsSection->getFiberDimension(_edgeToVertex.begin()->second);
  assert(spaceDim > 0 && spaceDim <= 3);

  const edge_map_type::const_iterator edgesEnd = _edgeToVertex.end();
  for (edge_map_type::const_iterator e_iter = _edgeToVertex.begin(); e_iter != edgesEnd; ++e_iter) {
    const point_type newVertex = e_iter->second;
    const point_type edgeVertexA = e_iter->first.first;
    const point_type edgeVertexB = e_iter->first.second;

    assert(spaceDim == oldCoordsSection->getFiberDimension(edgeVertexA));
    assert(spaceDim == oldCoordsSection->getFiberDimension(edgeVertexB));
    assert(spaceDim == newCoordsSection->getFiberDimension(newVertex));

    const mesh_type::real_section_type::value_type* coordsA = oldCoordsSection->restrictPoint(edgeVertexA);
    const mesh_type::real_section_type::value_type* coordsB = oldCoordsSection->restrictPoint(edgeVertexB);
    for (int i=0; i < spaceDim; ++i)
      coordinatesVertex[i] = 0.5*(coordsA[i] + coordsB[i]);

    newCoordsSection->updatePoint(newVertex, coordinatesVertex);
  } // for
} // setCoordsNewVertices

// ----------------------------------------------------------------------
// Add space for new vertices in group.
void
ALE::RefineEdges2::groupAddNewVertices(const ALE::Obj<mesh_type::int_section_type>& newGroup,
					  const ALE::Obj<mesh_type::int_section_type>& oldGroup)
{ // groupAddNewVertices
  assert(!newGroup.isNull());
  assert(!oldGroup.isNull());

  const edge_map_type::const_iterator edgesEnd = _edgeToVertex.end();
  for (edge_map_type::const_iterator e_iter = _edgeToVertex.begin(); e_iter != edgesEnd; ++e_iter) {
    const point_type newVertex = e_iter->second;
    const point_type edgeVertexA = e_iter->first.first;
    const point_type edgeVertexB = e_iter->first.second;

    if (oldGroup->getFiberDimension(edgeVertexA) && oldGroup->getFiberDimension(edgeVertexB)) {
      if (oldGroup->restrictPoint(edgeVertexA)[0] == oldGroup->restrictPoint(edgeVertexB)[0]) {
	  newGroup->setFiberDimension(newVertex, 1);
      } // if
    } // if
  } // for
} // groupAddNewVertices

// ----------------------------------------------------------------------
// Set new vertices in group.
void
ALE::RefineEdges2::groupSetNewVertices(const ALE::Obj<mesh_type::int_section_type>& newGroup,
					  const ALE::Obj<mesh_type::int_section_type>& oldGroup)
{ // groupSetNewVertices
  assert(!newGroup.isNull());
  assert(!oldGroup.isNull());

  const edge_map_type::const_iterator edgesEnd = _edgeToVertex.end();
  for (edge_map_type::const_iterator e_iter = _edgeToVertex.begin(); e_iter != edgesEnd; ++e_iter) {
    const point_type newVertex = e_iter->second;
    const point_type edgeVertexA = e_iter->first.first;
    const point_type edgeVertexB = e_iter->first.second;

    if (oldGroup->getFiberDimension(edgeVertexA) && oldGroup->getFiberDimension(edgeVertexB)) {
      if (oldGroup->restrictPoint(edgeVertexA)[0] == oldGroup->restrictPoint(edgeVertexB)[0]) {
	newGroup->updatePoint(newVertex, oldGroup->restrictPoint(edgeVertexA));
	std::cout << "Adding new vertex: " << newVertex << " based on old vertices " << edgeVertexA << " and " << edgeVertexB << std::endl;
      } // if
    } // if
  } // for
} // groupSetNewVertices

// ----------------------------------------------------------------------
// Add new vertices to label.
void
ALE::RefineEdges2::labelAddNewVertices(const ALE::Obj<mesh_type>& newMesh,
					  const ALE::Obj<mesh_type>& oldMesh,
					  const char* labelName)
{ // labelAddNewVertices
  assert(!newMesh.isNull());
  assert(!oldMesh.isNull());

  const Obj<mesh_type::label_sequence>& oldLabelVertices = oldMesh->getLabelStratum(labelName, 0);
  assert(!oldLabelVertices.isNull());

  const Obj<mesh_type::label_type>& oldLabel = oldMesh->getLabel(labelName);
  assert(!oldLabel.isNull());
  const Obj<mesh_type::label_type>& newLabel = newMesh->getLabel(labelName);
  assert(!newLabel.isNull());

  const int defaultValue = -999;

  const edge_map_type::const_iterator edgesEnd = _edgeToVertex.end();
  for (edge_map_type::const_iterator e_iter = _edgeToVertex.begin(); e_iter != edgesEnd; ++e_iter) {
    const point_type newVertex = e_iter->second;
    const point_type edgeVertexA = e_iter->first.first;
    const point_type edgeVertexB = e_iter->first.second;

    const int valueA = oldMesh->getValue(oldLabel, edgeVertexA, defaultValue);
    const int valueB = oldMesh->getValue(oldLabel, edgeVertexB, defaultValue);

    if (valueA != defaultValue && valueA == valueB) {
      newMesh->setValue(newLabel, newVertex, valueA);
    } // if
  } // for
} // labelAddNewVertices

// ----------------------------------------------------------------------
// Calculate new overlap.
void
ALE::RefineEdges2::overlapAddNewVertices(const Obj<mesh_type>& newMesh,
					    const MeshOrder& orderNewMesh,
					    const Obj<mesh_type>& oldMesh,
					    const MeshOrder& orderOldMesh)
{ // overlapAddNewVertices
  assert(!newMesh.isNull());
  assert(!oldMesh.isNull());

  Obj<mesh_type::send_overlap_type> newSendOverlap = newMesh->getSendOverlap();
  assert(!newSendOverlap.isNull());
  Obj<mesh_type::recv_overlap_type> newRecvOverlap = newMesh->getRecvOverlap();
  assert(!newRecvOverlap.isNull());
  const Obj<mesh_type::send_overlap_type>& oldSendOverlap = oldMesh->getSendOverlap();
  assert(!oldSendOverlap.isNull());
  const Obj<mesh_type::recv_overlap_type>& oldRecvOverlap = oldMesh->getRecvOverlap();
  assert(!oldRecvOverlap.isNull());


  // Check edges in edgeToVertex for both endpoints sent to same process
  //   Put it in section with point being the lowest numbered vertex and value (other endpoint, new vertex)
  Obj<ALE::Section<point_type, EdgeType> > newVerticesSection = new ALE::Section<point_type, EdgeType>(oldMesh->comm());
  assert(!newVerticesSection.isNull());
  std::map<EdgeType, std::vector<int> > bndryEdgeToRank;
  
  const int localOffset = orderNewMesh.verticesNormal().min() - orderOldMesh.verticesNormal().min();

  for(std::map<EdgeType, point_type>::const_iterator e_iter = _edgeToVertex.begin(); e_iter != _edgeToVertex.end(); ++e_iter) {
    const point_type left  = e_iter->first.first;
    const point_type right = e_iter->first.second;
    
    if (oldSendOverlap->capContains(left) && oldSendOverlap->capContains(right)) {
      const Obj<mesh_type::send_overlap_type::traits::supportSequence>& leftRanksSeq = oldSendOverlap->support(left);
      assert(!leftRanksSeq.isNull());
      std::list<int> leftRanks(leftRanksSeq->begin(), leftRanksSeq->end());
      const Obj<mesh_type::send_overlap_type::traits::supportSequence>& rightRanks   = oldSendOverlap->support(right);
      assert(!rightRanks.isNull());
      std::list<int> ranks;
      std::set_intersection(leftRanks.begin(), leftRanks.end(), rightRanks->begin(), rightRanks->end(),
			    std::insert_iterator<std::list<int> >(ranks, ranks.begin()));
      
      if(ranks.size()) {
	newVerticesSection->addFiberDimension(std::min(e_iter->first.first, e_iter->first.second)+localOffset, 1);
	for(std::list<int>::const_iterator r_iter = ranks.begin(); r_iter != ranks.end(); ++r_iter) {
	  bndryEdgeToRank[e_iter->first].push_back(*r_iter);
	} // for
      } // if
    } // if
  } // for
  newVerticesSection->allocatePoint();
  const ALE::Section<point_type, EdgeType>::chart_type& chart = newVerticesSection->getChart();
  
  for(ALE::Section<point_type, EdgeType>::chart_type::const_iterator c_iter = chart.begin(); c_iter != chart.end(); ++c_iter) {
    typedef ALE::Section<point_type, EdgeType>::value_type value_type;
    const point_type p      = *c_iter;
    const int        dim    = newVerticesSection->getFiberDimension(p);
    int              v      = 0;
    value_type* values = (dim > 0) ? new value_type[dim] : 0;
    
    for(std::map<EdgeType, std::vector<int> >::const_iterator e_iter = bndryEdgeToRank.begin(); e_iter != bndryEdgeToRank.end() && v < dim; ++e_iter) {
      if (std::min(e_iter->first.first, e_iter->first.second)+localOffset == p) {
	values[v++] = EdgeType(std::max(e_iter->first.first, e_iter->first.second)+localOffset, _edgeToVertex[e_iter->first]);
      } // if
    } // for
    newVerticesSection->updatePoint(p, values);
    delete [] values;
  } // for
  // Copy across overlap
  typedef ALE::Pair<int, point_type> overlap_point_type;
  Obj<ALE::Section<overlap_point_type, EdgeType> > overlapVertices = new ALE::Section<overlap_point_type, EdgeType>(oldMesh->comm());
  
  ALE::Pullback::SimpleCopy::copy(newSendOverlap, newRecvOverlap, newVerticesSection, overlapVertices);
  // Merge by translating edge to local points, finding edge in _edgeToVertex, and adding (local new vetex, remote new vertex) to overlap
  for(std::map<EdgeType, std::vector<int> >::const_iterator e_iter = bndryEdgeToRank.begin(); e_iter != bndryEdgeToRank.end(); ++e_iter) {
    const point_type localPoint = _edgeToVertex[e_iter->first];
    
    for(std::vector<int>::const_iterator r_iter = e_iter->second.begin(); r_iter != e_iter->second.end(); ++r_iter) {
      point_type remoteLeft = -1, remoteRight = -1;
      const int  rank       = *r_iter;
      
      const Obj<mesh_type::send_overlap_type::traits::supportSequence>& leftRanks = newSendOverlap->support(e_iter->first.first+localOffset);
      for(mesh_type::send_overlap_type::traits::supportSequence::iterator lr_iter = leftRanks->begin(); lr_iter != leftRanks->end(); ++lr_iter) {
	if (rank == *lr_iter) {
	  remoteLeft = lr_iter.color();
	  break;
	} // if
      } // for
      const Obj<mesh_type::send_overlap_type::traits::supportSequence>& rightRanks = newSendOverlap->support(e_iter->first.second+localOffset);
      for(mesh_type::send_overlap_type::traits::supportSequence::iterator rr_iter = rightRanks->begin(); rr_iter != rightRanks->end(); ++rr_iter) {
	if (rank == *rr_iter) {
	  remoteRight = rr_iter.color();
	  break;
	} // if
      } // for
      const point_type remoteMin   = std::min(remoteLeft, remoteRight);
      const point_type remoteMax   = std::max(remoteLeft, remoteRight);
      const int        remoteSize  = overlapVertices->getFiberDimension(overlap_point_type(rank, remoteMin));
      const EdgeType *remoteVals  = overlapVertices->restrictPoint(overlap_point_type(rank, remoteMin));
      point_type       remotePoint = -1;
      
      for(int d = 0; d < remoteSize; ++d) {
	if (remoteVals[d].first == remoteMax) {
	  remotePoint = remoteVals[d].second;
	  break;
	} // if
      } // for
      newSendOverlap->addArrow(localPoint, rank, remotePoint);
      newRecvOverlap->addArrow(rank, localPoint, remotePoint);
    } // for
  } // for

  oldSendOverlap->view("OLD SEND OVERLAP");
  oldRecvOverlap->view("OLD RECV OVERLAP");
  newSendOverlap->view("NEW SEND OVERLAP");
  newRecvOverlap->view("NEW RECV OVERLAP");
} // overlapAddNewVertces


// End of file 