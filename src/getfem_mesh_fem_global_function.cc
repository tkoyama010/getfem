// -*- c++ -*- (enables emacs c++ mode)
//========================================================================
//
// Copyright (C) 2004-2006 Yves Renard
//
// This file is a part of GETFEM++
//
// Getfem++ is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; version 2.1 of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301,
// USA.
//
//========================================================================

#include <getfem_mesh_fem_global_function.h>
#include <getfem_level_set.h>


namespace getfem {
  
  void global_function_fem::init(void) {
    is_pol = is_lag = false; es_degree = 5;
    is_equiv = real_element_defined = true;
    ntarget_dim = 1; // An extension for vectorial elements should be easy
    dim_ = cvr->structure()->dim();
    init_cvs_node();
    base_node P(dim()); P.fill(1./30.);
    for (size_type i = 0; i < functions.size(); ++i) {
      add_node(global_dof(dim()), P);
    }
  }

  size_type global_function_fem::nb_dof(size_type) const
  { return functions.size(); }
  
  size_type global_function_fem::index_of_global_dof
  (size_type /*cv*/, size_type i) const
  { return i; }
   
  void global_function_fem::base_value(const base_node &, base_tensor &) const
  { DAL_THROW(internal_error, "No base values, real only element."); }
  void global_function_fem::grad_base_value(const base_node &,
					 base_tensor &) const
  { DAL_THROW(internal_error, "No grad values, real only element."); }
  void global_function_fem::hess_base_value(const base_node &,
					 base_tensor &) const
  { DAL_THROW(internal_error, "No hess values, real only element."); }
  
  void global_function_fem::real_base_value(const fem_interpolation_context& c,
					    base_tensor &t, bool) const {

    mib.resize(2); mib[0] = 1; mib[1] = functions.size();
    t.adjust_sizes(mib);
    for (size_type i=0; i < functions.size(); ++i) 
      t[i] = (*functions[i]).val(c);
    
  } 
   
  void global_function_fem::real_grad_base_value
  (const fem_interpolation_context& c, base_tensor &t, bool) const {
    mig.resize(3); 
    mig[2] = dim(); mig[1] = target_dim(); mig[0] = functions.size();
    t.adjust_sizes(mig);
    base_small_vector G(dim());
    for (size_type i=0; i < functions.size(); ++i) {
      (*functions[i]).grad(c,G);
      for (unsigned j=0; j < dim(); ++j)
	t[j*functions.size() + i] = G[j];
      //std::copy(G.const_begin(), G.const_end(), t.begin() + i*dim());
    }
  }
  
  void global_function_fem::real_hess_base_value
  (const fem_interpolation_context&, base_tensor &, bool) const { 
    DAL_THROW(dal::to_be_done_error, "");
    /*
    mih.resize(4); 
    mih[3] = mih[2] = dim(); mih[1] = target_dim(); mih[0] = functions.size();
    t.adjust_sizes(mih);
    base_matrix H(dim(),dim());
    for (size_type i=0; i < functions.size(); ++i) {
      (*functions[i]).hess(c,H);
      
      NO !! NOT THE RIGHT ORDER!
      std::copy(H.begin(), H.end(), t.begin() + i*H.size());
    }
    */
  }
  
  DAL_SIMPLE_KEY(special_int_globf_fem_key, pfem);

  pfem new_global_function_fem(bgeot::pconvex_ref cvr, 
			       const std::vector<pglobal_function> &f) {
    pfem pf = new global_function_fem(cvr,f);
    dal::add_stored_object(new special_int_globf_fem_key(pf), pf);
    return pf;
  }

  void mesh_fem_global_function::receipt(const MESH_CLEAR &) { clear(); }
  void mesh_fem_global_function::receipt(const MESH_DELETE &) { clear(); }
  void mesh_fem_global_function::clear_build_methods() {
    for (std::map<bgeot::pconvex_ref,pfem>::const_iterator 
	   it = build_methods.begin(); 
	 it != build_methods.end(); ++it) 
      del_global_function_fem((*it).second);
    build_methods.clear();
  }
  void mesh_fem_global_function::clear(void) {
    mesh_fem::clear();
    clear_build_methods();
  }
  
  void mesh_fem_global_function::adapt(void) {
    clear();
    for (dal::bv_visitor cv(linked_mesh().convex_index()); 
	 !cv.finished(); ++cv) {
      bgeot::pconvex_ref cvr = 
	linked_mesh().trans_of_convex(cv)->convex_ref()->basic_convex_ref();
      
      std::map<bgeot::pconvex_ref,pfem>::iterator it = build_methods.find(cvr);
      pfem pf;
      if (it == build_methods.end()) {
	build_methods[cvr] = pf = new_global_function_fem(cvr, fun);
      } else pf = (*it).second;
      set_finite_element(cv, pf);
    }
    touch();
  }


  static scalar_type sing_function(scalar_type x, scalar_type y, size_type l) {

    scalar_type sgny = (y < 0 ? -1.0 : 1.0);
    scalar_type r = sqrt(x*x + y*y);
    /* ci-dessous: la valeur absolue est malheureusement necessaire,
     * sinon il peut arriver qu'on cherche sqrt(-1e-16) ...
     */
    scalar_type sin2 = sqrt(gmm::abs(.5-x/(2*r))) * sgny;
    scalar_type cos2 = sqrt(gmm::abs(.5+x/(2*r)));
    switch(l){
      case 0 :
	return sqrt(r)*sin2;
      case 1 :
	return sqrt(r)*cos2;
      case 2 :
	return sin2*y/sqrt(r);
      case 3 :
	return cos2*y/sqrt(r);
      default:
	DAL_INTERNAL_ERROR("arg");
    }
  }


  static base_small_vector sing_function_grad(scalar_type x, scalar_type y,
					      size_type l) {
    base_small_vector res(2);
    scalar_type sgny = (y < 0 ? -1.0 : 1.0);
    scalar_type r = sqrt(x*x + y*y);
    /* ci-dessous: la valeur absolue est malheureusement necessaire,
     * sinon il peut arriver qu'on cherche sqrt(-1e-16) ...
     */
    scalar_type sin2 = sqrt(gmm::abs(.5-x/(2*r))) * sgny;
    scalar_type cos2 = sqrt(gmm::abs(.5+x/(2*r)));

    switch(l){
    case 0 :
      res[0] = -sin2/(2*sqrt(r));
      res[1] = cos2/(2*sqrt(r));
      break;
    case 1 :
      res[0] = cos2/(2*sqrt(r));  
      res[1] = sin2/(2*sqrt(r));
      break;
    case 2 :
      res[0] = cos2*(-5*cos2*cos2 + 1. + 4*(cos2*cos2*cos2*cos2))/sqrt(r);
      res[1] = sin2*(-3*cos2*cos2 + 1. + 4*(cos2*cos2*cos2*cos2))/sqrt(r);
      break;
    case 3 :
      res[0] = -cos2*cos2*sin2*(4*cos2*cos2 - 3)/sqrt(r);
      res[1] = cos2*(4*cos2*cos2*cos2*cos2 + 2 - 5*cos2*cos2)/sqrt(r);
      break;
    default: 
      DAL_INTERNAL_ERROR("oups");
    }
    return res;
  }
  /*added lines
declaration of cutoff_radius1 and cutoffradius0

  */
  struct crack_singular : public global_function, public context_dependencies {
    size_type l;
    const level_set &ls;
    scalar_type a4;
    scalar_type cutoff_radius1;
    scalar_type cutoff_radius0;
    size_type cutoff_func;
    mutable mesher_level_set mls0, mls1;
    mutable size_type cv;

    void update_mls(size_type cv_) const { 
      if (cv_ != cv) 
	{ cv=cv_; mls0=ls.mls_of_convex(cv, 0); mls1=ls.mls_of_convex(cv, 1); }
    }

    virtual scalar_type val(const fem_interpolation_context& c) const {
      update_mls(c.convex_num());
      scalar_type x = mls1(c.xref()), y = mls0(c.xref());
      scalar_type v=sing_function(x, y, l);
      return v*cutoff(x,y);
    }

    
 
//     base_small_vector cutoff_grad(scalar_type x, scalar_type y) const {
//       base_small_vector g(2);
//       if (a4>0) {
// 	scalar_type r2 = x*x+y*y;
// 	g[0] = cutoff(x,y) * (-4*a4*r2*x);
// 	g[1] = cutoff(x,y) * (-4*a4*r2*y);
//       }
//       return g;
//     }

 
    
    scalar_type cutoff(scalar_type x, scalar_type y) const {
       
      switch (cutoff_func) {
	
      case 0:
	return (a4>0) ? exp(-a4 * gmm::sqr(x*x+y*y)) : 1;
	
      case 1:
	{
	  assert(cutoff_radius0 > cutoff_radius1);
	  scalar_type r = gmm::sqrt(x*x+y*y);
	  
	  if (r <= cutoff_radius1)
	    return scalar_type(1);
	  else if (r >= cutoff_radius0)
	    return scalar_type(0);
	  else {
	    scalar_type c = 6./(pow(cutoff_radius0,3) - pow(cutoff_radius1,3)
				+ 3*cutoff_radius1*cutoff_radius0
				* (cutoff_radius1-cutoff_radius0));
	    scalar_type k = -(c/6.)*(- pow(cutoff_radius0,3)
				     + 3*cutoff_radius1*pow(cutoff_radius0,2));

	    return (c/3.)*pow(r,3)
	      - (c*(cutoff_radius0 + cutoff_radius1)/2.)*pow(r,2)
	      + c*cutoff_radius0*cutoff_radius1*r + k;
	  }
	}
      default : return scalar_type(1);
      }
    }
    
    
    base_small_vector cutoff_grad(scalar_type x, scalar_type y) const {
      
      switch (cutoff_func) {
      case 0:
	{
	  scalar_type r2 = x*x+y*y, ratio = -4.*exp(-a4*r2*r2)*a4*r2;
	  return base_small_vector(ratio*x, ratio*y);
	}
      case 1:
	{
	  scalar_type r = gmm::sqrt(x*x+y*y);
	  scalar_type ratio = scalar_type(0);

	  if ( r > cutoff_radius1 && r < cutoff_radius0 ) {
	    scalar_type c = 6./(pow(cutoff_radius0,3) - pow(cutoff_radius1,3)
				+ 3*cutoff_radius1*cutoff_radius0
				* (cutoff_radius1-cutoff_radius0));
	    ratio = c*(r - cutoff_radius0)*(r - cutoff_radius1);
	  }
	  
	  return base_small_vector(ratio*x/r,ratio*y/r);
	}
      default : return base_small_vector(2);
      }
    }
    virtual void grad(const fem_interpolation_context& c,
		      base_small_vector &v) const {
      update_mls(c.convex_num());
      size_type P = c.xref().size();
      base_small_vector dx(P), dy(P), dfr(2);
      scalar_type x = mls1.grad(c.xref(), dx), y = mls0.grad(c.xref(), dy);
      if (x*x + y*y < 1e-20) {
	cerr << "Warning, point very close to the singularity. xreal = "
	     << c.xreal() << ", x_crack = " << x << ", y_crack=" << y << "\n";
      }
      
      switch (cutoff_func){
      case 0: {
	if (a4 > 0)
	  dfr = sing_function(x,y,l)*cutoff_grad(x,y)
	    + cutoff(x,y)*sing_function_grad(x, y, l);
	else dfr = sing_function_grad(x, y, l);
	
	gmm::mult(c.B(), dfr[0]*dx + dfr[1]*dy, v);
	
      } break;
      case 1: {
	  if (cutoff_radius1 > 0)
	    dfr = sing_function(x,y,l)*cutoff_grad(x,y)
	      + cutoff(x,y)*sing_function_grad(x, y, l);
	  else dfr = sing_function_grad(x, y, l);
	  
	  gmm::mult(c.B(), dfr[0]*dx + dfr[1]*dy, v);
	} break;
      }
    }
    virtual void hess(const fem_interpolation_context&, base_matrix &) const
    { DAL_THROW(dal::to_be_done_error, "hessian to be done ..."); }
    
    void update_from_context(void) const { cv =  size_type(-1); }

    crack_singular(size_type l_, const level_set &ls_, 
		   scalar_type cutoff_R, scalar_type cutoff_R1, scalar_type cutoff_R0, size_type func) : l(l_), ls(ls_) {
      if (cutoff_R) a4 = (cutoff_R > 0.0) ? pow(2.7/cutoff_R,4) : 0.0;
      cutoff_radius1 = cutoff_R1;
      cutoff_radius0 = cutoff_R0;
      cutoff_func = func;
      cerr << "cutoff radius: " << cutoff_radius0 << ", " << cutoff_radius1 << "\n";
      cv = size_type(-1);
      this->add_dependency(ls);
    }

  };

  pglobal_function isotropic_crack_singular_2D(size_type i,
					       const level_set &ls, 
					       scalar_type cutoff_radius, 
					       scalar_type cutoff_radius1, 
					       scalar_type cutoff_radius0,
					       size_type cutoff_func) {
    return new crack_singular(i, ls, cutoff_radius, cutoff_radius1, cutoff_radius0, cutoff_func);
  }

}  /* end of namespace getfem.                                            */





