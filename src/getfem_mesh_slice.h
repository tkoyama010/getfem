/* -*- c++ -*- (enables emacs c++ mode)                                    */
#ifndef GETFEM_MESH_SLICES_H
#define GETFEM_MESH_SLICES_H

#include <bitset>
#include <getfem_mesh.h>
#include <getfem_fem.h>
#include <getfem_poly_composite.h>

namespace getfem {
  struct convex_face  {
    size_type cv;
    size_type f;
    inline bool operator < (const convex_face &e) const
    {
      if (cv < e.cv) return true; if (cv > e.cv) return false; 
      if (f < e.f) return true; else if (f > e.f) return false;
      return false;
    }
    bool is_face() const { return f != size_type(-1); }
    convex_face(size_type _cv, size_type _f = size_type(-1)) : cv(_cv), f(_f) {}
    convex_face() : cv(size_type(-1)), f(size_type(-1)) {}
  };
  typedef std::vector<convex_face> convex_face_ct;

  /**
     returns a list of "exterior" faces of a mesh (i.e. faces which are not shared by two convexes)
  */
  void  outer_faces_of_mesh(const getfem::getfem_mesh &m, const dal::bit_vector& cvlst, convex_face_ct& flist);

  /**
     node data in a slice: contains both real position, and position in the reference convex
   */
  
  struct slice_node {
    typedef std::bitset<32> faces_ct; /// broken for convexes with more than 32 faces
    base_node pt, pt_ref;
    faces_ct faces; 
    slice_node() {}
    slice_node(const base_node& _pt, const base_node& _pt_ref) : pt(_pt), pt_ref(_pt_ref) {}
  };

  /**
     simplex data in a slice: just a list of slice_node ids.
   */
  struct slice_simplex {
    std::vector<size_type> inodes;
    size_type dim() const { return inodes.size()-1; }
    slice_simplex(size_type n) : inodes(n) {} 
    slice_simplex() : inodes(4) {}
    bool operator==(const slice_simplex& o) const { return inodes == o.inodes; }
    bool operator!=(const slice_simplex& o) const { return inodes != o.inodes; }
  };
  
  /**
     generic slicer class: given a list of slice_simplex/slice_node, it 
     build a news list of slice_simplex/slice_node
  */
  class slicer {
  public:
    enum { IN=1, BOUND=2, OUT=4 };
    static const float EPS;
    virtual void test_point(const base_node&, bool& in, bool& bound) const { in=true; bound=true; }
    bool is_in(const base_node& P, int where=IN|BOUND) const { 
      bool in, bound;
      test_point(P, in, bound);
      switch (where) {
      case 0: return false;
      case IN: return in && !bound;
      case OUT: return !in && !bound;
      case BOUND: return bound;
      case OUT|BOUND: return bound || (!in);
      case IN|BOUND: return bound || in;
      case IN|OUT: return !bound;
      }
      return true;
    }
    virtual scalar_type edge_intersect(const base_node& , const base_node& ) const { return -1.; };
    virtual void slice(size_type cv, dim_type& fcnt,
                       std::deque<slice_node>& nodes, std::deque<slice_simplex>& splxs, 
                       dal::bit_vector& splx_in) const = 0;    
    size_type is_in(const std::deque<slice_node>& nodes, const slice_simplex& s, int where=IN|BOUND) const {
      size_type in_cnt = 0;
      for (size_type i=0; i < s.dim()+1; ++i)
        if (is_in(nodes[s.inodes[i]].pt,where)) ++in_cnt;
      return in_cnt;
    }
    void split_simplex(std::deque<slice_node>& nodes, 
                       dal::bit_vector& pt_in, dal::bit_vector& pt_bin,
                       std::deque<slice_simplex>& splxs, 
                       const slice_simplex& s, 
                       size_type sstart, bool reduce_dimension) const;
    virtual ~slicer() {}
  };

  class slicer_boundary : public slicer {
    slicer *A;
    std::vector<slice_node::faces_ct> convex_faces;
    bool test_bound(const slice_simplex& s, slice_node::faces_ct& fmask, 
                    const std::deque<slice_node>& nodes) const;
  public:
    slicer_boundary(const getfem_mesh& m, slicer *_A, const convex_face_ct& fbound);
    void slice(size_type cv, dim_type& fcnt,
	       std::deque<slice_node>& nodes, std::deque<slice_simplex>& splxs, 
               dal::bit_vector& splx_in) const;
  };

  class slicer_volume : public slicer {
  protected:
    bool is_boundary_slice;
    slicer_volume(bool _is_boundary_slice) : is_boundary_slice(_is_boundary_slice) {}
    static scalar_type trinom(scalar_type a, scalar_type b, scalar_type c) {
      scalar_type delta = b*b - 4*a*c;
      if (delta < 0.) return 1./EPS;
      delta = sqrt(delta);
      scalar_type s1 = (-b - delta) / (2*a);
      scalar_type s2 = (-b + delta) / (2*a);
      if (dal::abs(s1-.5) < dal::abs(s2-.5)) return s1; else return s2;
    }
  public:
    void slice(size_type cv, dim_type& fcnt,
	       std::deque<slice_node>& nodes, std::deque<slice_simplex>& splxs, 
	       dal::bit_vector& splx_in) const;    
  };

  class slicer_half_space : public slicer_volume {
    base_node x0, n; /* normal directed from inside toward outside */
  public:
    slicer_half_space(base_node _x0, base_node _n, bool on_boundary) : 
      slicer_volume(on_boundary), x0(_x0), n(_n) {
      n *= (1./bgeot::vect_norm2(n));
    }
    void test_point(const base_node& P, bool& in, bool& bound) const {
      scalar_type s = 0.;
      for (unsigned i=0; i < P.size(); ++i) s += (P[i] - x0[i])*n[i];
      in = (s <= 0); bound = (s*s <= dal::sqr(EPS*bgeot::vect_norm2_sqr(P)));
    }
    scalar_type edge_intersect(const base_node& A, const base_node& B) const {
      scalar_type s1 = 0., s2 = 0.;
      for (unsigned i=0; i < A.size(); ++i) { s1 += (A[i] - B[i])*n[i]; s2 += (A[i]-x0[i])*n[i]; }
      if (dal::abs(s1) < EPS) return 1./EPS;
      else return s2/s1;
    }
  };

  class slicer_sphere : public slicer_volume {
    base_node x0;
    scalar_type R;
  public:
    slicer_sphere(base_node _x0, scalar_type _R, bool boundary_slice) : 
      slicer_volume(boundary_slice), x0(_x0), R(_R) {} //cerr << "slicer_volume, x0=" << x0 << ", R=" << R << endl; }
    void test_point(const base_node& P, bool& in, bool& bound) const {
      scalar_type R2 = bgeot::vect_dist2_sqr(P,x0);
      bound = (R2 >= (1-EPS)*R*R && R2 <= (1+EPS)*R*R);
      in = R2 <= R*R;
    }
    scalar_type edge_intersect(const base_node& A, const base_node& B) const {
      scalar_type a,b,c; // a*x^2 + b*x + c = 0
      a = bgeot::vect_norm2_sqr(B-A); if (a < EPS) return is_in(A,BOUND) ? 0. : 1./EPS;
      b = 2*bgeot::vect_sp(A-x0,B-A);
      c = bgeot::vect_norm2_sqr(A-x0)-R*R;
      return slicer_volume::trinom(a,b,c);
    }
  };
  
  class slicer_cylinder : public slicer_volume {
    base_node x0, d;
    scalar_type R;
  public:
    slicer_cylinder(base_node _x0, base_node _x1, scalar_type _R, bool boundary_slice) : 
      slicer_volume(boundary_slice), x0(_x0), d(_x1-_x0), R(_R) {
      d /= bgeot::vect_norm2(d);
    }
    void test_point(const base_node& P, bool& in, bool& bound) const {
      base_node N = P-x0;
      scalar_type axpos = bgeot::vect_sp(d, N);
      scalar_type dist2 = bgeot::vect_norm2_sqr(N) - dal::sqr(axpos);
      bound = dal::abs(dist2-R*R) < EPS;
      in = dist2 < R*R;
    }
    scalar_type edge_intersect(const base_node& A, const base_node& B) const {
      base_node F=A-x0; scalar_type Fd = bgeot::vect_sp(F,d);
      base_node D=B-A;  scalar_type Dd = bgeot::vect_sp(D,d);
      scalar_type a = bgeot::vect_norm2_sqr(D) - dal::sqr(Dd); if (a < EPS) return is_in(A,BOUND) ? 0. : 1./EPS; assert(a> -EPS);
      scalar_type b = 2*(bgeot::vect_sp(F,D) - Fd*Dd);
      scalar_type c = bgeot::vect_norm2_sqr(F) - dal::sqr(Fd) - dal::sqr(R);
      return slicer_volume::trinom(a,b,c);
    }
  };

  class slicer_union : public slicer {
    slicer *A, *B;
  public:
    slicer_union(slicer *_A, slicer *_B) : A(_A), B(_B) {}
    void test_point(const base_node& P, bool& in, bool& bound) const {
      bool inA, boundA, inB, boundB;
      A->test_point(P,inA,boundA); B->test_point(P,inB,boundB);
      bound = (boundA && !inB) || (boundB && !inA) || (boundA && boundB);
      in = inA || inB;
    }
    void slice(size_type cv, dim_type& fcnt,
	       std::deque<slice_node>& nodes, std::deque<slice_simplex>& splxs, 
	       dal::bit_vector& splx_in) const;
  };

  class slicer_intersect : public slicer {
    slicer *A, *B;
  public:
    slicer_intersect(slicer *_A, slicer *_B) : A(_A), B(_B) {}
    void test_point(const base_node& P, bool& in, bool& bound) const {
      bool inA, boundA, inB, boundB;
      A->test_point(P,inA,boundA); B->test_point(P,inB,boundB);
      bound = (boundA && inB) || (boundB && inA) || (boundA && boundB);
      in = inA && inB;
    }
    void slice(size_type cv, dim_type& fcnt,
	       std::deque<slice_node>& nodes, std::deque<slice_simplex>& splxs, 
	       dal::bit_vector& splx_in) const;
  };

  class slicer_complementary : public slicer {
    slicer *A;
  public:
    slicer_complementary(slicer *_A) : A(_A) {}
    void test_point(const base_node& P, bool& in, bool& bound) const {
      A->test_point(P,in,bound); in = !in;
    }
    void slice(size_type cv, dim_type& fcnt,
	       std::deque<slice_node>& nodes, std::deque<slice_simplex>& splxs, 
	       dal::bit_vector& splx_in) const;    
  };
  

  /** this slicer does nothing! */
  class slicer_none : public slicer {
  public:
    slicer_none() {}
    void slice(size_type /*cv*/, dim_type& /*fcnt*/, std::deque<slice_node>& /*nodes*/, 
	       std::deque<slice_simplex>& /*splxs*/, dal::bit_vector& /*splx_in*/) const {}
  };

  class mesh_slice_cv_dof_data_base {
  public:
    const mesh_fem *pmf;    
    virtual void copy(size_type cv, base_vector& coeff) = 0;
    virtual ~mesh_slice_cv_dof_data_base() {}
  };

  /**
     use this structure to specifiy that the mesh must be deformed 
     (with a mesh_fem and an associated field)
     before the slicing 
  */
  template<typename VEC> class mesh_slice_cv_dof_data : public mesh_slice_cv_dof_data_base {
    const VEC *u;
  public:
    mesh_slice_cv_dof_data(const mesh_fem &_mf, VEC &_u) : u(_u) { pmf = &mf; }
    virtual void copy(size_type cv, base_vector& coeff) {
      coeff.resize(pmf->nb_dof_of_element(cv));
      ref_mesh_dof_ind_ct dof = pmf->ind_dof_of_element(cv);
      base_vector::iterator out = coeff.begin();
      for (ref_mesh_dof_ind_ct::iterator it=dof.begin(); it != dof.end(); ++it, ++out)
        *out = u[*it];
    }
    virtual ~mesh_slice_cv_dof_data() {}
  };
  
  /**
     mesh slice: a list of nodes/simplexes, which can be seen as a P1 discontinuous
     mesh_fem on which the interpolation is very fast
  */
  class mesh_slice {
  public:
    typedef std::deque<slice_node> cs_nodes_ct;
    typedef std::deque<slice_simplex> cs_simplexes_ct;
  private:
    struct convex_slice {
      size_type cv_num; 
      dim_type cv_dim;
      dim_type fcnt, cv_nbfaces; // number of faces of the convex (fcnt also counts the faces created by the slicing of the convex)
      cs_nodes_ct nodes;
      cs_simplexes_ct simplexes;
    };
    typedef std::deque<convex_slice> cvlst_ct;
    const getfem_mesh& m;
    std::vector<size_type> simplex_cnt; // count simplexes of dimension 0,1,...,dim
    size_type points_cnt;
    cvlst_ct cvlst;
    size_type _dim;
    void do_slicing(size_type cv, bgeot::pconvex_ref cvr, const slicer *ms, cs_nodes_ct cv_nodes, 
		    cs_simplexes_ct cv_simplexes, dal::bit_vector& splx_in);
  public:
    mesh_slice(const getfem_mesh& m, const slicer* ms, size_type nrefine, 
               convex_face_ct& cvlst, mesh_slice_cv_dof_data_base *def_mf_data=0);
    size_type nb_convex() const { return cvlst.size(); }
    size_type convex_num(size_type ic) const { return cvlst[ic].cv_num; }
    size_type dim() const { return _dim; }
    const getfem_mesh& linked_mesh() const { return m; }
    void nb_simplexes(std::vector<size_type>& c) const { c = simplex_cnt; }
    size_type nb_simplexes(size_type sdim) const { return simplex_cnt[sdim]; }
    size_type nb_points() const { return points_cnt; }
    const std::deque<slice_node>& nodes(size_type ic) const { return cvlst[ic].nodes; }
    void edges_mesh(getfem_mesh &edges_m) const;
    const std::deque<slice_simplex>& simplexes(size_type ic) const { return cvlst[ic].simplexes; }
    size_type memsize() const;

    /** merges with another mesh slice */
    void merge(const mesh_slice& sl);

    /** interpolation of a mesh_fem on a slice (the mesh_fem
	and the slice must share the same mesh, of course)
    */
    template<typename V1, typename V2> void 
    interpolate(const getfem::mesh_fem &mf, const V1& U, V2& V) {
      _fem_precomp fprecomp;
      bgeot::stored_point_tab refpts;
      base_vector coeff;
      base_matrix G;
      base_node val(mf.get_qdim());
      typename V2::iterator out = V.begin();
      for (size_type i=0; i < nb_convex(); ++i) {
        size_type cv = convex_num(i);
        refpts.resize(nodes(i).size());
        for (size_type j=0; j < refpts.size(); ++j) refpts[j] = nodes(i)[j].pt_ref;
        pfem pf = mf.fem_of_element(cv);
        fem_precomp_not_stored(pf, &refpts, fprecomp);
        
        ref_mesh_dof_ind_ct dof = mf.ind_dof_of_element(cv);
        coeff.resize(mf.nb_dof_of_element(cv));
        base_vector::iterator cit = coeff.begin();
        for (ref_mesh_dof_ind_ct::iterator it=dof.begin(); it != dof.end(); ++it, ++cit)
          *cit = U[*it];

        for (size_type j=0; j < refpts.size(); ++j) {
          pf->interpolation(&fprecomp, j,
                            G, mf.linked_mesh().trans_of_convex(cv),
                            coeff, val, mf.get_qdim());
          out = std::copy(val.begin(), val.end(), out);
        }
      }
    }

  };

}

#endif /*GETFEM_MESH_SLICES_H*/
