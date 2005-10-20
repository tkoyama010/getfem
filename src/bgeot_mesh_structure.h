// -*- c++ -*- (enables emacs c++ mode)
//========================================================================
//
// Library : Basic GEOmetric Tool  (bgeot)
// File    : bgeot_mesh_structure.h : mesh structures.
//           
// Date    : November 5, 1999.
// Author  : Yves Renard <Yves.Renard@insa-toulouse.fr>
//
//========================================================================
//
// Copyright (C) 1999-2005 Yves Renard
//
// This file is a part of GETFEM++
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//========================================================================

/**@file bgeot_mesh_structure.h
   @brief Mesh structure definition
*/

#ifndef BGEOT_MESH_STRUCTURE_H__
#define BGEOT_MESH_STRUCTURE_H__

#include <bgeot_convex_structure.h>

namespace bgeot
{

  /* ******************************************************************** */
  /* transfert containers and iterators.                                  */
  /* ******************************************************************** */

  class mesh_structure;
  
  struct mesh_point {        /* structure for a point.                    */
    size_type first;         /* first convex attached to point or ST_NIL. */
    short_type ind_in_first; /* index of the point in first convex.       */
    mesh_point(void) { first = ST_NIL; }
    bool is_valid(void) const { return (first != ST_NIL); }
  };
  
  struct mesh_convex_structure {
    pconvex_structure cstruct;       /* type of convexe.                  */
    size_type pts;                   /* index in  points list.            */
    pconvex_structure structure(void) const { return cstruct; }
    pconvex_structure &structure(void) { return cstruct; }
  };
  
  struct mesh_point_link {    /* link between convex and points.          */
    size_type next;           /* index of next convex linked to the point.*/
    short_type ind_in_next;   /* index of this point in next convex.      */
    mesh_point_link(void) { next = ST_NIL; }
  };
  
  typedef dal::dynamic_array<mesh_point, 8>          mesh_point_st_ct;
  typedef dal::dynamic_tas<mesh_convex_structure, 8> mesh_convex_ct;
  typedef dal::dynamic_alloc<mesh_point_link, 8>     mesh_link_ct;
  typedef dal::dynamic_array<size_type, 8>           mesh_point_ind_ct;
  typedef dal::tab_ref<mesh_point_ind_ct::const_iterator>
                                                     ref_mesh_point_ind_ct;
  typedef std::vector<size_type>                     mesh_point_search_ind_ct;
  typedef dal::tab_ref_index_ref<mesh_point_ind_ct::const_iterator,
                                 convex_ind_ct::const_iterator>
                                                     ind_ref_mesh_point_ind_ct;
  // iterator for mesh_convex_ind_ct
  struct mc_const_iterator {
    typedef size_t              value_type;
    typedef const value_type*   pointer;
    typedef const value_type&   reference;
    typedef size_t              size_type;
    typedef ptrdiff_t           difference_type;
    typedef std::forward_iterator_tag iterator_category;
    
    const mesh_structure *p;
    const mesh_convex_structure *pc;
    size_type ind_cv;
    short_type ind_in_cv;
    
    mc_const_iterator(void) { ind_cv = ST_NIL; }
    mc_const_iterator(const mesh_structure &ms, size_type ip);
    
    mc_const_iterator &operator ++();
    
    mc_const_iterator operator ++(int)
      { mc_const_iterator tmp = *this; ++(*this); return tmp; }
    
    template<class ITER> bool is_convex_has_points(short_type nb,
						   const ITER &ipt) const;
    
    bool is_end(void) const { return (ind_cv == ST_NIL); }
    value_type operator *() const { return ind_cv; }
    pconvex_structure structure(void) const
      { return pc->structure(); }
    
    bool operator ==(const mc_const_iterator &i) const
      { return (i.ind_cv == ind_cv);}
    bool operator !=(const mc_const_iterator &i) const
      { return (i.ind_cv != ind_cv);}
  };
  
  /// pseudo-container for a list of convexes attached to a point.
  class mesh_convex_ind_ct
  {
  public :
      
    typedef size_t               value_type;
    typedef value_type*          pointer;
    typedef const value_type*    const_pointer;
    typedef value_type&          reference;
    typedef const value_type&    const_reference;
    typedef size_t               size_type;
    typedef mc_const_iterator    const_iterator;
    typedef mc_const_iterator    iterator;
    
  protected :
    
    iterator begin_, end_;
    
  public :
    
    bool empty(void) const { return begin_ == end_; }
    
    mesh_convex_ind_ct(const mesh_structure &ms, size_type ip)
      { begin_ = iterator(ms, ip); }
    mesh_convex_ind_ct(void) { }
    
    const iterator &begin() const { return begin_; }
    const iterator &end() const { return end_; }
    iterator &begin() { return begin_; }
    iterator &end() { return end_; }
    
    value_type front(void) const { return *begin_; }
    void pop_front(void) { ++begin_; }
    size_type size() { size_type n=0; for (iterator i = begin(); i != end(); ++i) ++n; return n; }
  };
  
  /**@addtogroup mesh */
  ///@{
  /** Mesh structure definition. 
      At this point, the mesh is just a graph: the points have no associated coordinates
  */
  class mesh_structure
  {
  protected :
    
    mesh_point_st_ct    points_tab;
    mesh_convex_ct      convex_tab;
    mesh_point_ind_ct   point_lists;
    mesh_link_ct        point_links;
    
  public :
    
    mesh_link_ct &links(void) { return point_links; }
    mesh_convex_ct &convex(void) { return convex_tab; }
    mesh_point_st_ct &points(void) { return points_tab; }
    const mesh_point_st_ct &point_structures(void) const
      { return points_tab; }
    const mesh_link_ct  &links(void) const { return point_links; }
    const mesh_convex_ct &convex(void) const { return convex_tab; }

    /// Return the list of valid convex IDs
    const dal::bit_vector &convex_index(void) const
      { return convex_tab.index(); }
    /// Return the list of valid convex IDs of a given dimension
    dal::bit_vector convex_index(dim_type) const;
    /// The total number of convexes in the mesh
    size_type nb_convex(void) const { return convex_tab.card(); }
    /// Return true if i is in convex_index()
    bool convex_is_valid(size_type i) { return (convex_tab.index())[i]; }
    /// Return true if the point i is used by at least one convex
    bool point_is_valid(size_type i)
      { return (points_tab[i].first != size_type(-1)); }
    /** Return a pseudo-container to the list of points attached to convex ic.
	They are ordered according to structure_of_convex(ic) */
    ref_mesh_point_ind_ct ind_points_of_convex(size_type ic) const;
    /// Return the "local" index for point ip of the mesh
    size_type local_ind_of_convex_point(size_type ic, size_type ip) const;
    /// Return the pconvex_structure of the convex ic.
    pconvex_structure structure_of_convex(size_type ic) const
      { return convex_tab[ic].cstruct; }
    /// Return the number of points of convex ic.
    short_type nb_points_of_convex(size_type ic) const
      { return convex_tab[ic].cstruct->nb_points(); }
    /// Return the number of faces of convex ic.
    dim_type nb_faces_of_convex(size_type ic) const 
      { return convex_tab[ic].cstruct->nb_faces(); }
    /// Exchange two point IDs
    void swap_points(size_type i, size_type j);
    /// Exchange two convex IDs
    void swap_convex(size_type cv1, size_type cv2);
    
    template<class ITER>
    size_type add_convex_noverif(pconvex_structure cs, ITER ipts,
				 size_type to_index = size_type(-1));
    /** Insert a new convex in the mesh_structure.
	@param cs the structure of the new convex.
	@param ipts an iterator over a sequence of integers (point IDs of the convex nodes).
	@param present an optional argument, contains true on return if the convex already exists in the mesh_structure.
	@return the convex ID
    */
    template<class ITER>
    size_type add_convex(pconvex_structure cs,
			 ITER ipts, bool *present = 0);
    template<class ITER> size_type add_simplex(dim_type dim, ITER ipts)
      { return add_convex(simplex_structure(dim), ipts); }
    size_type add_segment(size_type a, size_type b);
    /** Remove the convex ic */
    void sup_convex(size_type ic);
    /** Remove a convex given its points 
	@param nb the number of points for the convex
	@param ipts an iterator over the list of point IDs of the convex
    */
    template<class ITER> 
    void sup_convex_with_points(ITER ipts, short_type nb);
    void sup_segment(size_type a, size_type b)
      { size_type t[2]; t[0] = a; t[1] = b; sup_convex_with_points(&t[0], 2); }
    /** Insert a new convex corresponding to face f of the convex ic */
    size_type add_face_of_convex(size_type ic, short_type f);
    /** Insert a new convexes corresponding to the faces of the convex ic */
    void add_faces_of_convex(size_type ic);
    /** build a new mesh, such that its convexes are the faces of the
	convexes of the previous one */
    void to_faces(dim_type n);
    /** build a new mesh, such that its convexes are the edges of the
	convexes of the previous one */
    void to_edges(void);
    
    /** Return a pseudo-container of the convexes attached to point ip */
    mesh_convex_ind_ct convex_to_point(size_type ip) const
      { return mesh_convex_ind_ct(*this, ip); }
    /** Return a pseudo-container of the points attached (via an edge) to point ip */
    mesh_point_search_ind_ct ind_points_to_point(size_type ip) const;
    
    /** Return true if the convex contains the listed points.
	@param ic the convex ID.
	@param nb the number of points which are searched in ic.
	@param pit an iterator to the list of points searched.
    */
    template<class ITER>
      bool is_convex_has_points(size_type ic,short_type nb, ITER pit) const;
    
    /** Return true if the face of the convex contains the given list of points */
    template<class ITER> 
      bool is_convex_face_has_points(size_type ic, size_type face_num,
				     short_type nb, ITER pit) const;

    /** Return a container of the (global) point number for face f or convex ic */
    ind_ref_mesh_point_ind_ct ind_points_of_face_of_convex(size_type ic,
							   short_type f) const;
    
    size_type memsize(void) const;
    /** Reorder the convex IDs and point IDs, such that there is no
	hole in their numbering. */
    void optimize_structure(void);
    /// erase the mesh
    void clear(void);
    void stat(void);
    
    /** Convex ID of the first convex attached to the point ip. */
    size_type first_convex_of_point(size_type ip) const {
      return points_tab[ip].first;
    }
    /** Local index in the first convex attached to the point
	ip. @return local index (a number smaller than
	nb_points_of_convex(first_convex_of_point(ip)))
    */
    size_type ind_in_first_convex_of_point(size_type ip) const {
      return points_tab[ip].ind_in_first;
    }
    
  };
  ///@}

  /* ******************************************************************** */
  /* template member function of search iterator.                         */
  /* ******************************************************************** */
    
  template<class ITER> bool mc_const_iterator::is_convex_has_points
                                       (short_type nb, const ITER &ipt) const
  { return p->is_convex_has_points(this->ind_cv, nb, ipt); }


  /* ******************************************************************** */
  /* search functions.                                                    */
  /* ******************************************************************** */

  template<class ITER> struct mesh_convex_has_points
  {
    ITER ipt;
    short_type nb;
    dim_type dim;
    bool tnb;
    size_type ict;

    bool operator() (const mesh_convex_ind_ct::iterator &it) const
    { 
      if (it.is_end()) return true;
      return (nb < it.structure()->nb_points()
	      && *it != ict
	      && (!tnb || nb == it.structure()->nb_points())
	      && (dim == dim_type(-1) || dim == it.structure()->dim())
	      && it.is_convex_has_points(nb, ipt));
    }
    mesh_convex_has_points(short_type n, ITER ip,
	     dim_type d = dim_type(-1), bool t = false, size_type ic = ST_NIL)
    { nb = n; ipt = ip; dim = d; tnb = t; ict = ic; }
  };

  template<class ITER> class mesh_convex_with_points_ind_ct
    : public dal::tab_ref_with_selection<mesh_convex_ind_ct::iterator,
                                         mesh_convex_has_points<ITER> >
  {
    public :

      mesh_convex_with_points_ind_ct(const mesh_convex_ind_ct::iterator &b,
				     const mesh_convex_ind_ct::iterator &e,
				     const mesh_convex_has_points<ITER> &c)
      : dal::tab_ref_with_selection<mesh_convex_ind_ct::iterator,
                                    mesh_convex_has_points<ITER> >(b,e,c)
      {}
  }; 

  /**
     Give a container with the indexes of all elements in 'mesh'
     having a certain set of points for vertices. The set of points is
     describe by an iterator 'ipts' on an array and the number of
     points 'nb'.
  */
  template<class ITER> mesh_convex_with_points_ind_ct<ITER>
    convex_with_points(const mesh_structure &ms, short_type nb, ITER ipts)
  {
    mesh_convex_ind_ct ct = ms.convex_to_point(*ipts);
    return mesh_convex_with_points_ind_ct<ITER>(ct.begin(), ct.end(),
				  mesh_convex_has_points<ITER>(--nb, ++ipts));
  }

  typedef mesh_convex_with_points_ind_ct<ref_mesh_point_ind_ct::const_iterator>
    mesh_over_convex_ind_ct;

  mesh_over_convex_ind_ct over_convex(const mesh_structure &ms, size_type ic);

  typedef mesh_convex_with_points_ind_ct<
    ind_ref_mesh_point_ind_ct::const_iterator> mesh_face_convex_ind_ct;

  mesh_face_convex_ind_ct face_of_convex(const mesh_structure &ms, 
					 size_type ic, short_type iff);

  /** Return a list of neighbours of a given convex face.
      @param ms the mesh_structure.
      @param ic the convex id.
      @param f the face number of the convex.
      @return a pseudo-container of convex IDs (ic is not in this container).
  */
  mesh_face_convex_ind_ct neighbour_of_convex(const mesh_structure &ms, 
					      size_type ic, short_type f);

  /** Return the cuthill_mc_kee ordering on the convexes */
  void cuthill_mckee_on_convexes(const bgeot::mesh_structure &ms, 
				 std::vector<size_type> &cmk);

  /* ******************************************************************** */
  /* template member functions of mesh_structure.                         */
  /* ******************************************************************** */

  template<class ITER>
    bool mesh_structure::is_convex_has_points(size_type ic,
					      short_type nb, ITER pit) const
  {
    for (short_type i = 0; i < nb; ++i, ++pit)
    {
      ref_mesh_point_ind_ct pt = ind_points_of_convex(ic);
      if (std::find(pt.begin(), pt.end(), *pit) == pt.end()) return false;
    }
    return true;
  }
  

  template<class ITER>
  bool mesh_structure::is_convex_face_has_points(size_type ic, size_type face_num,
						 short_type nb, ITER pit) const
  {
    for (short_type i = 0; i < nb; ++i, ++pit)
    {
      ind_ref_mesh_point_ind_ct pt = ind_points_of_face_of_convex(ic, face_num);
      if (std::find(pt.begin(), pt.end(), *pit) == pt.end()) return false;
    }
    return true;
  }

  template<class ITER>
    size_type mesh_structure::add_convex_noverif(pconvex_structure cs,
						 ITER ipts, size_type is)
  {
    short_type nb = cs->nb_points();
    mesh_convex_structure s; s.cstruct = cs; s.pts = point_links.alloc(nb);
    if (nb > 0)
      { point_lists[s.pts+nb-1] = 0; point_links[s.pts+nb-1].next = 0; }
    dal::copy_n(ipts, nb, point_lists.begin()+s.pts);

    if (is != size_type(-1)) {
      if (convex_index()[is]) sup_convex(is);
      convex_tab.add_to_index(is, s);
    }
    else
      is = convex_tab.add(s);

    mesh_link_ct::iterator ipl = point_links.begin(); ipl += s.pts;
    for (short_type i = 0; i < nb; ++i, ++ipts, ++ipl)
    {
      mesh_point *os = &(points_tab[*ipts]);
      (*ipl).next = os->first; os->first = is;
      (*ipl).ind_in_next = os->ind_in_first; os->ind_in_first = i;
    }
    
    return is;
  }

  template<class ITER>
    size_type mesh_structure::add_convex(pconvex_structure cs,
					 ITER ipts, bool *present) {
    if (present != 0) *present = false;
    mesh_convex_with_points_ind_ct<ITER>
      ct = convex_with_points(*this, cs->nb_points(), ipts);
    typename mesh_convex_with_points_ind_ct<ITER>::const_iterator
      it = ct.begin(), ite = ct.end();
    for (; it != ite; ++it)
      if (structure_of_convex(*it) == cs)
	{ if (present != 0) *present = true; return *it; }
    return add_convex_noverif(cs, ipts);
  }

  template<class ITER>
    void mesh_structure::sup_convex_with_points(ITER ipts, short_type nb)
  {
    mesh_convex_with_points_ind_ct<ITER>
      ct = convex_with_points(*this, nb, ipts);
    typename mesh_convex_with_points_ind_ct<ITER>::iterator b = ct.begin();
    typename mesh_convex_with_points_ind_ct<ITER>::iterator e = ct.end();
    for ( ; b != e; ++b) sup_convex(*b);
  }


  /* ********************************************************************* */
  /*                                                                       */
  /*  Gives the list of edges of a mesh.                                   */
  /*                                                                       */
  /* ********************************************************************* */

  /* maybe this should be remove from the matlab interface and obsoleted oneday */
  struct edge_list_elt  {
    size_type i, j;
    size_type cv;
    inline bool operator < (const edge_list_elt &e) const
    {
      if (i < e.i) return true; if (i > e.i) return false; 
      if (j < e.j) return true; else if (j > e.j) return false;
      if (cv < e.cv) return true; return false;
    }
    edge_list_elt(size_type ii, size_type jj, size_type ic = 0) : cv(ic)
    { i = std::min(ii, jj); j = std::max(ii, jj); }
    edge_list_elt(void) {}
  };

  typedef dal::dynamic_tree_sorted<edge_list_elt> edge_list;
  
  /* do not use that */
  void mesh_edge_list_convex(pconvex_structure cvs, 
                             std::vector<size_type> points_of_convex, 
                             size_type cv_id, edge_list &el, 
                             bool merge_convex);
  void mesh_edge_list(const mesh_structure &m, edge_list &el, 
                      bool merge_convex = true);



}  /* end of namespace bgeot.                                              */


#endif /* BGEOT_MESH_STRUCTURE_H__                                         */
