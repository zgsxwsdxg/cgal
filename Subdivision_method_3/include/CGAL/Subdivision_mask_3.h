// ======================================================================
//
// Copyright (c) 2005-20176 GeometryFactory (France).  All Rights Reserved.
//
// This file is part of CGAL (www.cgal.org); you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 3 of the License,
// or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
// 
//
// Author(s): Le-Jeng Shiue <Andy.Shiue@gmail.com>
//
// ======================================================================

#ifndef CGAL_POLYHEDRON_SUBDIVISION_STENCILS_H_01292002
#define CGAL_POLYHEDRON_SUBDIVISION_STENCILS_H_01292002

#include <CGAL/basic.h>
#include <CGAL/Origin.h>

#include <CGAL/circulator.h>
#include <CGAL/boost/graph/iterator.h>

namespace CGAL {

// ======================================================================
/// The stencil of the Primal-Quadrilateral-Quadrisection 
template <class Poly, class VertexPointMap = typename boost::property_map<Poly, vertex_point_t>::type >
class PQQ_stencil_3 {

public:
  typedef Poly                                         Polyhedron;
  
  typedef typename boost::property_map<Polyhedron, vertex_point_t>::type Vertex_pmap;

  typedef typename boost::graph_traits<Polyhedron>::vertex_descriptor   vertex_descriptor;
  typedef typename boost::graph_traits<Polyhedron>::halfedge_descriptor halfedge_descriptor;
  typedef typename boost::graph_traits<Polyhedron>::face_descriptor     face_descriptor;

  typedef typename boost::property_traits<Vertex_pmap>::value_type Point;
  
  typedef typename Kernel_traits<Point>::Kernel       Kernel;
  typedef typename Kernel::FT                          FT;
  typedef typename Kernel::Vector_3                    Vector;

  Polyhedron& polyhedron;
  VertexPointMap vpmap;

public:
  PQQ_stencil_3(Polyhedron& polyhedron, VertexPointMap vpmap = get(vertex_point,polyhedron))
    : polyhedron(polyhedron), vpmap(vpmap)
  {}

  void face_node(face_descriptor, Point&) {};
  void edge_node(halfedge_descriptor, Point&) {};
  void vertex_node(vertex_descriptor, Point&) {};

  void border_node(halfedge_descriptor, Point&, Point&) {};
};


// ======================================================================
/// Bi-linear geometry mask for PQQ, PTQ, and Sqrt(3) scheme 
template <class Poly, class VertexPointMap = typename boost::property_map<Poly, vertex_point_t>::type >
class Linear_mask_3 : public PQQ_stencil_3<Poly,VertexPointMap> {
  typedef PQQ_stencil_3<Poly,VertexPointMap>         Base;
public:
  typedef Poly                                       Polyhedron;

  typedef typename Base::vertex_descriptor           vertex_descriptor;
  typedef typename Base::halfedge_descriptor         halfedge_descriptor;
  typedef typename Base::face_descriptor             face_descriptor;

  typedef typename Base::Kernel                      Kernel;

  typedef typename Base::FT                          FT;
  typedef typename Base::Point                       Point;
  typedef typename Base::Vector                      Vector;

public:
  Linear_mask_3(Polyhedron& poly, VertexPointMap vpmap = get(vertex_point,poly))
    : Base(poly,vpmap)
  {}

  //
  void face_node(face_descriptor facet, Point& pt) {
    int n = 0;
    Point p(0,0,0);
    BOOST_FOREACH(vertex_descriptor vd, vertices_around_face(halfedge(facet,this->polyhedron),this->polyhedron))
    {
      p = p + ( get(this->vpmap,vd) - ORIGIN);
      ++n;
    } 
    pt = ORIGIN + (p - ORIGIN)/FT(n);
  }
  //
  void edge_node(halfedge_descriptor edge, Point& pt) {
    Point p1 = get(this->vpmap, target(edge,this->polyhedron ));
    Point p2 = get(this->vpmap, source(edge,this->polyhedron ));
    pt = Point((p1[0]+p2[0])/2, (p1[1]+p2[1])/2, (p1[2]+p2[2])/2);
  }
  //
  void vertex_node(vertex_descriptor vertex, Point& pt) {
    pt = get(this->vpmap, vertex);
  }
  //
  void border_node(halfedge_descriptor edge, Point& ept, Point& /*vpt*/){
    edge_node(edge, ept);
  }
};

// ======================================================================
/// The geometry mask of Catmull-Clark subdivision 
  template <class Poly, class VertexPointMap = typename boost::property_map<Poly, vertex_point_t>::type >
  class CatmullClark_mask_3 : public Linear_mask_3<Poly,VertexPointMap> {
  typedef Linear_mask_3<Poly,VertexPointMap>                          Base;
public:
  typedef typename Base::Polyhedron   Polyhedron;

  typedef typename Base::vertex_descriptor           vertex_descriptor;
  typedef typename Base::halfedge_descriptor         halfedge_descriptor;
  typedef typename Base::face_descriptor             face_descriptor;

  typedef typename Base::Kernel                    Kernel;

  typedef typename Base::FT                          FT;
  typedef typename Base::Point                       Point;
  typedef typename Base::Vector                      Vector;

public:
  CatmullClark_mask_3(Polyhedron& poly,
                      VertexPointMap vpmap = get(vertex_point,poly))
    : Base(poly,vpmap)
  {}

  //
  void edge_node(halfedge_descriptor edge, Point& pt) {
    Point p1 = get(this->vpmap,target(edge, this->polyhedron));
    Point p2 = get(this->vpmap,source(edge, this->polyhedron));
    Point f1, f2;
    this->face_node(face(edge,this->polyhedron), f1);
    this->face_node(face(opposite(edge,this->polyhedron),this->polyhedron), f2);
    pt = Point((p1[0]+p2[0]+f1[0]+f2[0])/4,
	       (p1[1]+p2[1]+f1[1]+f2[1])/4,
	       (p1[2]+p2[2]+f1[2]+f2[2])/4 );
  }
  //
  void vertex_node(vertex_descriptor vertex, Point& pt) {
    Halfedge_around_target_circulator<Poly> vcir(vertex,this->polyhedron);
    typename boost::graph_traits<Poly>::degree_size_type n = degree(vertex,this->polyhedron);    

    FT Q[] = {0.0, 0.0, 0.0}, R[] = {0.0, 0.0, 0.0};
    Point& S = get(this->vpmap,vertex);
    
    Point q;
    for (unsigned int i = 0; i < n; i++, ++vcir) {
      Point& p2 = get(this->vpmap, target(opposite(*vcir,this->polyhedron),this->polyhedron));
      R[0] += (S[0]+p2[0])/2;
      R[1] += (S[1]+p2[1])/2;
      R[2] += (S[2]+p2[2])/2;
      this->face_node(face(*vcir,this->polyhedron), q);
      Q[0] += q[0];      
      Q[1] += q[1];      
      Q[2] += q[2];
    }
    R[0] /= n;    R[1] /= n;    R[2] /= n;
    Q[0] /= n;    Q[1] /= n;    Q[2] /= n;
      
    pt = Point((Q[0] + 2*R[0] + S[0]*(n-3))/n,
	       (Q[1] + 2*R[1] + S[1]*(n-3))/n,
	       (Q[2] + 2*R[2] + S[2]*(n-3))/n );
  }
  //
  void border_node(halfedge_descriptor edge, Point& ept, Point& vpt) {
    Point& ep1 = get(this->vpmap,target(edge, this->polyhedron));
    Point& ep2 = get(this->vpmap,target(opposite(edge, this->polyhedron), this->polyhedron));
    ept = Point((ep1[0]+ep2[0])/2, (ep1[1]+ep2[1])/2, (ep1[2]+ep2[2])/2);

    Halfedge_around_target_circulator<Poly> vcir(edge, this->polyhedron);
    Point& vp1  = get(this->vpmap,target(opposite(*vcir, this->polyhedron ), this->polyhedron));
    Point& vp0  = get(this->vpmap, target(*vcir, this->polyhedron));
    --vcir;
      Point& vp_1 = get(this->vpmap, target(opposite(*vcir, this->polyhedron), this->polyhedron));
    vpt = Point((vp_1[0] + 6*vp0[0] + vp1[0])/8,
		(vp_1[1] + 6*vp0[1] + vp1[1])/8,
		(vp_1[2] + 6*vp0[2] + vp1[2])/8 );
  }
};


// ======================================================================
/// The geometry mask of Loop subdivision
template <class Poly, class VertexPointMap = typename boost::property_map<Poly, vertex_point_t>::type >
class Loop_mask_3 : public PQQ_stencil_3<Poly,VertexPointMap> {
  typedef PQQ_stencil_3<Poly,VertexPointMap>         Base;
public:
  typedef Poly                                       Polyhedron;

  typedef typename Base::vertex_descriptor           vertex_descriptor;
  typedef typename Base::halfedge_descriptor         halfedge_descriptor;
  typedef typename Base::face_descriptor             face_descriptor;

  typedef typename Base::Kernel                      Kernel;

  typedef typename Base::FT                          FT;
  typedef typename Base::Point                       Point;
  typedef typename Base::Vector                      Vector;

  typedef Halfedge_around_face_circulator<Polyhedron>  
                                            Halfedge_around_facet_circulator;
  typedef typename Halfedge_around_target_circulator<Polyhedron>  
                                            Halfedge_around_vertex_circulator;

  

public:

  Loop_mask_3(Polyhedron& poly,
              VertexPointMap vpmap= get(vertex_point,poly))
    : Base(poly,vpmap)
  {}

  //
  void edge_node(halfedge_descriptor edge, Point& pt) {
    Point& p1 = get(this->vpmap,target(edge, this->polyhedron));
    Point& p2 = get(this->vpmap,target(opposite(edge, this->polyhedron), this->polyhedron));
    Point& f1 = get(this->vpmap,target(next(edge, this->polyhedron), this->polyhedron));
    Point& f2 = get(this->vpmap,target(next(opposite(edge, this->polyhedron), this->polyhedron), this->polyhedron));
      
    pt = Point((3*(p1[0]+p2[0])+f1[0]+f2[0])/8,
	       (3*(p1[1]+p2[1])+f1[1]+f2[1])/8,
	       (3*(p1[2]+p2[2])+f1[2]+f2[2])/8 );
  }
  //
  void vertex_node(vertex_descriptor vertex, Point& pt) {
    Halfedge_around_vertex_circulator vcir(vertex, this->polyhedron);
    size_t n = circulator_size(vcir);    

    FT R[] = {0.0, 0.0, 0.0};
    Point& S = get(this->vpmap,vertex);
    
    for (size_t i = 0; i < n; i++, ++vcir) {
      Point& p = get(this->vpmap,target(opposite(*vcir, this->polyhedron), this->polyhedron));
      R[0] += p[0]; 	R[1] += p[1]; 	R[2] += p[2];
    }
    if (n == 6) {
      pt = Point((10*S[0]+R[0])/16, (10*S[1]+R[1])/16, (10*S[2]+R[2])/16);
    } else {
      FT Cn = (FT) (5.0/8.0 - CGAL::square(3+2*std::cos(2 * CGAL_PI/(double) n))/64.0);
      FT Sw = (double)n*(1-Cn)/Cn;
      FT W = (double)n/Cn;
      pt = Point((Sw*S[0]+R[0])/W, (Sw*S[1]+R[1])/W, (Sw*S[2]+R[2])/W);
    }
  }
  //
  //void face_node(face_descriptor facet, Point& pt) {};
  //
  void border_node(halfedge_descriptor edge, Point& ept, Point& vpt) {
    Point& ep1 = get(this->vpmap,target(edge, this->polyhedron));
    Point& ep2 = get(this->vpmap,target(opposite(edge, this->polyhedron), this->polyhedron));
    ept = Point((ep1[0]+ep2[0])/2, (ep1[1]+ep2[1])/2, (ep1[2]+ep2[2])/2);

    Halfedge_around_vertex_circulator vcir(edge,this->polyhedron);
    Point& vp1  = get(this->vpmap,target(opposite(*vcir,this->polyhedron ),this->polyhedron));
    Point& vp0  = get(this->vpmap,target(*vcir,this->polyhedron));
    --vcir;
    Point& vp_1 = get(this->vpmap,target(opposite(*vcir,this->polyhedron),this->polyhedron));
    vpt = Point((vp_1[0] + 6*vp0[0] + vp1[0])/8,
		(vp_1[1] + 6*vp0[1] + vp1[1])/8,
		(vp_1[2] + 6*vp0[2] + vp1[2])/8 );
  }
};


//==========================================================================
/// The setncil of the Dual-Quadrilateral-Quadrisection 
template <class Poly, class VertexPointMap = typename boost::property_map<Poly, vertex_point_t>::type >
class DQQ_stencil_3 {
public:
  typedef Poly                                        Polyhedron;
  
  typedef typename boost::property_map<Polyhedron, vertex_point_t>::type Vertex_pmap;

  typedef typename boost::graph_traits<Polyhedron>::vertex_descriptor   vertex_descriptor;
  typedef typename boost::graph_traits<Polyhedron>::halfedge_descriptor halfedge_descriptor;
  typedef typename boost::graph_traits<Polyhedron>::face_descriptor     face_descriptor;

  typedef typename boost::property_traits<Vertex_pmap>::value_type Point;
  
  typedef typename Kernel_traits<Point>::Kernel       Kernel;
  typedef typename Kernel::FT                          FT;
  typedef typename Kernel::Vector_3                    Vector;

  Polyhedron& polyhedron;
  Vertex_pmap vpm;
public:
  //
  void corner_node(halfedge_descriptor /*edge*/, Point& /*pt*/) {};

  DQQ_stencil_3(Polyhedron& polyhedron, VertexPointMap vpmap = get(vertex_point,polyhedron))
    : polyhedron(polyhedron), vpm(vpmap)
  {}
};


// ======================================================================
/// The geometry mask of Doo-Sabin subdivision
template <class Poly, class VertexPointMap = typename boost::property_map<Poly, vertex_point_t>::type >
class DooSabin_mask_3 : public DQQ_stencil_3<Poly,VertexPointMap> {
public:
  typedef DQQ_stencil_3<Poly,VertexPointMap> Base;
  typedef Poly                                        Polyhedron;

  typedef typename Base::vertex_descriptor           vertex_descriptor;
  typedef typename Base::halfedge_descriptor         halfedge_descriptor;
  typedef typename Base::face_descriptor             face_descriptor;

  typedef typename Base::Kernel                      Kernel;

  typedef typename Base::FT                          FT;
  typedef typename Base::Point                       Point;
  typedef typename Base::Vector                      Vector;
 

public:

  DooSabin_mask_3(Polyhedron& polyhedron,
                      VertexPointMap vpmap = get(vertex_point,polyhedron))
    : Base(polyhedron,vpmap)
  {}

  //
  void corner_node(halfedge_descriptor he, Point& pt) {
    size_t n = 0;
    halfedge_descriptor hd = he;
    do{
      hd = next(hd,this->polyhedron);
      ++n;
    }while(hd != he);
      
    Vector cv(0,0,0);
    if (n == 4) {
      cv = cv + (get(this->vpm, target(he,this->polyhedron))-CGAL::ORIGIN)*9;
      cv = cv + (get(this->vpm, target(next(he,this->polyhedron),this->polyhedron))-CGAL::ORIGIN)*3;
      cv = cv + (get(this->vpm, target(next(next(he,this->polyhedron),this->polyhedron),this->polyhedron))-CGAL::ORIGIN);
      cv = cv + (get(this->vpm, target(prev(he,this->polyhedron),this->polyhedron))-CGAL::ORIGIN)*3;
      cv = cv/16;
    }  else {
      FT a;
      for (size_t k = 0; k < n; ++k, he = next(he,this->polyhedron)) {
        if (k == 0) a = (FT) ((5.0/n) + 1);
        else a = (FT) (3+2*std::cos(2*k*CGAL_PI/n))/n;
        cv = cv + (get(this->vpm, target(he,this->polyhedron))-CGAL::ORIGIN)*a;
      }
      cv = cv/4;
    }
    pt = CGAL::ORIGIN + cv;
  }
};

// ======================================================================
// The geometry mask of Sqrt(3) subdivision
  template <class Poly, class VertexPointMap = typename boost::property_map<Poly, vertex_point_t>::type >
  class Sqrt3_mask_3 : public Linear_mask_3<Poly,VertexPointMap> {
    typedef Linear_mask_3<Poly,VertexPointMap>       Base;
public:
  typedef Poly                                       Polyhedron;
  
  typedef typename Base::vertex_descriptor           vertex_descriptor;
  typedef typename Base::halfedge_descriptor         halfedge_descriptor;
  typedef typename Base::face_descriptor             face_descriptor;

  typedef typename Base::Kernel                      Kernel;

  typedef typename Base::FT                          FT;
  typedef typename Base::Point                       Point;
  typedef typename Base::Vector                      Vector;

  typedef Halfedge_around_target_circulator<Poly> Halfedge_around_target_circulator;
public:
  //
  //void edge_node(halfedge_descriptor edge, Point& pt) {}
  //

  Sqrt3_mask_3(Polyhedron& poly,
               VertexPointMap vpmap = get(vertex_point,poly))
    : Base(poly,vpmap)
  {}

  void vertex_node(vertex_descriptor vertex, Point& pt) {
    Halfedge_around_target_circulator vcir(vertex,this->polyhedron);
    size_t n = degree(vertex,this->polyhedron);

    FT a = (FT) ((4.0-2.0*std::cos(2.0*CGAL_PI/(double)n))/9.0);

    Vector cv = ((FT)(1.0-a)) * (get(this->vpmap, vertex) - CGAL::ORIGIN);
    for (size_t i = 1; i <= n; ++i, --vcir) {
      cv = cv + (a/FT(n))*(get(this->vpmap, target(opposite(*vcir, this->polyhedron), this->polyhedron))-CGAL::ORIGIN);
    }

    pt = CGAL::ORIGIN + cv;
  }
  //
  // TODO
  //void border_node(halfedge_descriptor edge, Point& ept, Point& vpt) {}
};

} //namespace CGAL

#endif //CGAL_POLYHEDRON_SUBDIVISION_STENCILS_H_01292002
