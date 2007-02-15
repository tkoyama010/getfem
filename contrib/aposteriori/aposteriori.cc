// -*- c++ -*- (enables emacs c++ mode)
//========================================================================
//
// Copyright (C) 2002-2007 Yves Renard, Julien Pommier.
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
  
/**
 * Linear Elastostatic problem with a crack.
 *
 * This program is used to check that getfem++ is working. This is also 
 * a good example of use of Getfem++.
*/

#include "getfem/getfem_assembling.h" /* import assembly methods (and norms comp.) */
#include "getfem/getfem_export.h"   /* export functions (save solution in a file)  */
#include "getfem/getfem_derivatives.h"
#include "getfem/getfem_regular_meshes.h"
#include "getfem/getfem_model_solvers.h"
#include "getfem/getfem_mesh_im_level_set.h"
#include "getfem/getfem_mesh_fem_level_set.h"
#include "getfem/getfem_mesh_fem_product.h"
#include "getfem/getfem_mesh_fem_global_function.h"
#include "getfem/getfem_spider_fem.h"
#include "getfem/getfem_mesh_fem_sum.h"
#include "getfem/getfem_Coulomb_friction.h"
#include "gmm/gmm.h"
#include "getfem/getfem_error_estimate.h"
#include "getfem/getfem_interpolated_fem.h"


/* some Getfem++ types that we will be using */
using bgeot::base_small_vector; /* special class for small (dim<16) vectors */
using bgeot::base_node;  /* geometrical nodes(derived from base_small_vector)*/
using bgeot::scalar_type; /* = double */
using bgeot::size_type;   /* = unsigned long */
using bgeot::base_matrix; /* small dense matrix. */

/* definition of some matrix/vector types. These ones are built
 * using the predefined types in Gmm++
 */
typedef getfem::modeling_standard_sparse_vector sparse_vector;
typedef getfem::modeling_standard_sparse_matrix sparse_matrix;
typedef getfem::modeling_standard_plain_vector  plain_vector;


base_small_vector sol_f(const base_node &x) {
  // int N = x.size();
  // base_small_vector res(N); res[N-1] = x[N-1];
  return base_small_vector(x.size());
}

base_small_vector sol_F(const base_node &x) {
  int N = x.size();
  base_small_vector res(N);
  res[1] = -1.0;
  return res;
}


/**************************************************************************/
/*  Structure for the crack problem.                                      */
/**************************************************************************/

struct crack_problem {

  enum { DIRICHLET_BOUNDARY_NUM = 0, NEUMANN_BOUNDARY_NUM1 = 1,
	 NEUMANN_BOUNDARY_NUM2 = 3};
  getfem::mesh mesh;  /* the mesh */
  getfem::mesh_level_set mls;       /* the integration methods.              */
  getfem::mesh_im_level_set mim;    /* the integration methods.              */
  getfem::mesh_im_level_set mimbound;  /* integration methods on the crack.  */
  getfem::mesh_fem mf_pre_u;
  getfem::mesh_fem mf_mult;
  getfem::mesh_fem_global_function mf_sing_u;
  getfem::mesh_fem_level_set mfls_u; 
  getfem::mesh_fem_sum mf_u_sum;
  
  getfem::mesh_fem& mf_u() { return mf_u_sum; }
  
  scalar_type lambda, mu;    /* Lame coefficients.                */
  getfem::mesh_fem mf_rhs;   /* mesh_fem for the right hand side (f(x),..)   */
  
  
  double lx,ly;             /* size of the mesh */
  double F11,F12,F21,F22,F31,F32,F41,F42;       /* NEUMANN forces */

  getfem::level_set ls;      /* The two level sets defining the crack.       */
  
  scalar_type residual;      /* max residual for the iterative solvers       */
  scalar_type conv_max;
  unsigned dir_with_mult;
  
  std::string datafilename;
  bgeot::md_param PARAM;

  struct cutoff_param {
    scalar_type radius, radius1, radius0;
    size_type fun_num;
  };
  cutoff_param cutoff;

  bool adapted_refine;

  bool solve(plain_vector &U);

  void error_estimate(const plain_vector &U, plain_vector ERR);
  
  void init(void);
  crack_problem(void) : mls(mesh), mim(mls),
			mimbound(mls, getfem::mesh_im_level_set::INTEGRATE_BOUNDARY),
			mf_pre_u(mesh), mf_mult(mesh), mf_sing_u(mesh),
			mfls_u(mls, mf_pre_u),
			mf_u_sum(mesh), mf_rhs(mesh), ls(mesh, 1, true) {}

};

/* Read parameters from the .param file, build the mesh, set finite element
 * and integration methods and selects the boundaries.
 */
void crack_problem::init(void) {
  std::string MESH_TYPE = PARAM.string_value("MESH_TYPE","Mesh type ");
  std::string FEM_TYPE  = PARAM.string_value("FEM_TYPE","FEM name");

  std::string INTEGRATION = PARAM.string_value("INTEGRATION",
					       "Name of integration method");
  std::string SIMPLEX_INTEGRATION = PARAM.string_value("SIMPLEX_INTEGRATION",
					 "Name of simplex integration method");
  std::string SINGULAR_INTEGRATION = PARAM.string_value("SINGULAR_INTEGRATION");

  cout << "MESH_TYPE=" << MESH_TYPE << "\n";
  cout << "FEM_TYPE="  << FEM_TYPE << "\n";
  cout << "INTEGRATION=" << INTEGRATION << "\n";

  
  /* First step : build the mesh */
  bgeot::pgeometric_trans pgt = 
    bgeot::geometric_trans_descriptor(MESH_TYPE);
  size_type N = pgt->dim();
  std::vector<size_type> nsubdiv(N);
  std::fill(nsubdiv.begin(),nsubdiv.end(),
	    PARAM.int_value("NX", "Nomber of space steps ") * 2);
  getfem::regular_unit_mesh(mesh, nsubdiv, pgt,
			    PARAM.int_value("MESH_NOISED") != 0);
  for (dal::bv_visitor i(mesh.convex_index()); !i.finished(); ++i) {
    base_node pt = gmm::mean_value(mesh.points_of_convex(i)) ;
    bool kill = true;
    for (size_type j = 0; j < N; ++j)
      if (pt[j] < 0.5) kill = false;
    if (kill) mesh.sup_convex(i, true);
  }
  
  lx = PARAM.real_value("LX","length x'ox");
  ly = PARAM.real_value("LY","length y'oy");
  
  bgeot::base_matrix M(2,2);
  M(0,0) = lx;   
  M(1,1) = ly;
  mesh.transformation(M);
  
  // base_small_vector tt(N); tt[0] = tt[1] = -(lx/2.);
  // mesh.translation(tt); 

  conv_max = PARAM.int_value("CONV_MAX","Maximal number of convexes in the mesh");
  adapted_refine = PARAM.int_value("ADAPTED_REFINE","Adapted Refinement");
  
  datafilename = PARAM.string_value("ROOTFILENAME","Base name of data files.");
  residual = PARAM.real_value("RESIDUAL");
  if (residual == 0.) residual = 1e-10;
 
  mu = PARAM.real_value("MU", "Lame coefficient mu");
  lambda = PARAM.real_value("LAMBDA", "Lame coefficient lambda");
  
  mf_u().set_qdim(N);

  /* set the finite element on the mf_u */
  getfem::pfem pf_u = getfem::fem_descriptor(FEM_TYPE);
 
  getfem::pintegration_method ppi = 
    getfem::int_method_descriptor(INTEGRATION);
  getfem::pintegration_method simp_ppi = 
    getfem::int_method_descriptor(SIMPLEX_INTEGRATION);
  getfem::pintegration_method sing_ppi = (SINGULAR_INTEGRATION.size() ? getfem::int_method_descriptor(SINGULAR_INTEGRATION) : 0);
  
  mim.set_integration_method(mesh.convex_index(), ppi);
  mimbound.set_integration_method(mesh.convex_index(), ppi);
  mls.add_level_set(ls);

  mim.set_simplex_im(simp_ppi, sing_ppi);
  mimbound.set_simplex_im(simp_ppi, sing_ppi);
  mf_pre_u.set_finite_element(mesh.convex_index(), pf_u);
  mf_mult.set_finite_element(mesh.convex_index(), pf_u);
  mf_mult.set_qdim(N);

  dir_with_mult = PARAM.int_value("DIRICHLET_VERSION");

  cutoff.fun_num = PARAM.int_value("CUTOFF_FUNC", "cutoff function");
  cutoff.radius = PARAM.real_value("CUTOFF", "Cutoff");
  cutoff.radius1 = PARAM.real_value("CUTOFF1", "Cutoff1");
  cutoff.radius0 = PARAM.real_value("CUTOFF0", "Cutoff0");

  /* set the finite element on mf_rhs (same as mf_u is DATA_FEM_TYPE is
     not used in the .param file */
  std::string data_fem_name = PARAM.string_value("DATA_FEM_TYPE");
  if (data_fem_name.size() == 0) {
    GMM_ASSERT1(pf_u->is_lagrange(), "You are using a non-lagrange FEM. "
		<< "In that case you need to set "
		<< "DATA_FEM_TYPE in the .param file");
    mf_rhs.set_finite_element(mesh.convex_index(), pf_u);
  } else {
    mf_rhs.set_finite_element(mesh.convex_index(), 
			      getfem::fem_descriptor(data_fem_name));
  }
  
  /* set boundary conditions
   * (Neuman on the upper face, Dirichlet elsewhere) */
  cout << "Selecting Neumann and Dirichlet boundaries\n";
  getfem::mesh_region border_faces;
  getfem::outer_faces_of_mesh(mesh, border_faces);
  for (getfem::mr_visitor i(border_faces); !i.finished(); ++i) {
    
    base_node un = mesh.normal_of_face_of_convex(i.cv(), i.f());
    un /= gmm::vect_norm2(un);
    if (gmm::abs(un[N-1]-1.0) < 1.0E-7) {
      base_node pt = gmm::mean_value(mesh.points_of_face_of_convex(i.cv(), i.f()));
      if (pt[N-1] > 0.9) mesh.region(DIRICHLET_BOUNDARY_NUM).add(i.cv(), i.f());
    }
    else if (gmm::abs(un[N-1]+1.0) < 1.0E-7)
      mesh.region(NEUMANN_BOUNDARY_NUM1).add(i.cv(), i.f());
    else
      mesh.region(NEUMANN_BOUNDARY_NUM2).add(i.cv(), i.f());
  }
}


base_small_vector ls_function(const base_node P, int num = 0) {
  scalar_type x = P[0], y = P[1];
  base_small_vector res(2);
  switch (num) {
    case 0: {
      res[0] =  2* x - y - 0.5;
      res[1] =  7.0/8.0 - (x + 2*y);
    } break;
    default: assert(0);
  }
  return res;
}


void crack_problem::error_estimate(const plain_vector &U, plain_vector ERR) {


  size_type N = mesh.dim();
  size_type qdim = mf_u().get_qdim();
  gmm::clear(ERR);
  std::vector<scalar_type> coeff1, coeff2;
  base_matrix grad1(qdim, N), grad2(qdim, N), E(N, N), S1(N, N), S2(N, N);
  base_matrix hess1(qdim, N*N);
  base_matrix G1, G2;
  bgeot::geotrans_inv_convex gic;
  base_node xref2(N);
  base_small_vector up(N), jump(N);

  for (dal::bv_visitor cv(mesh.convex_index()); !cv.finished(); ++cv) {

    bgeot::pgeometric_trans pgt1 = mesh.trans_of_convex(cv);
    getfem::papprox_integration pai1 = 
      get_approx_im_or_fail(mim.int_method_of_element(cv));
    getfem::pfem pf1 = mf_u().fem_of_element(cv);
    scalar_type radius = mesh.convex_radius_estimate(cv);

    bgeot::vectors_to_base_matrix(G1, mesh.points_of_convex(cv));

    coeff1.resize(mf_u().nb_dof_of_element(cv));
    gmm::copy(gmm::sub_vector(U, gmm::sub_index(mf_u().ind_dof_of_element(cv))), coeff1);

    getfem::fem_interpolation_context ctx1(pgt1, pf1, base_node(N), G1, cv);
     
    // Residual on the element

    for (unsigned ii=0; ii < pai1->nb_points_on_convex(); ++ii) {
      
      base_small_vector res = sol_f(pai1->point(ii));
      ctx1.set_xref(pai1->point(ii));
      pf1->interpolation_hess(ctx1, coeff1, hess1, qdim);
      for (size_type i = 0; i < N; ++i)
	for (size_type j = 0; j < N; ++j)
	  res[i] += (lambda + mu) * hess1(j, i*N+j) + mu * hess1(i, j*N+j);
      
      ERR[cv] += radius * radius * ctx1.J() * pai1->coeff(ii) * gmm::vect_norm2(res);
    }

    // Stress on the level set.
    getfem::papprox_integration pai_crack = 
      get_approx_im_or_fail(mimbound.int_method_of_element(cv));
    
    getfem::mesher_level_set mmls = ls.mls_of_convex(cv, 0);
    base_small_vector gradls;
    for (unsigned ii=0; ii < pai_crack->nb_points(); ++ii) {
      
      ctx1.set_xref(pai_crack->point(ii));
      mmls.grad(pai_crack->point(ii), gradls);
      gmm::mult(ctx1.B(), gradls, up);
      scalar_type norm = gmm::vect_norm2(up), normls = gmm::vect_norm2(gradls);
      up /= norm;
      scalar_type coefficient = pai_crack->coeff(ii) * ctx1.J() * norm / normls; 

      for (scalar_type e = -1.0; e < 2.0; e += 2.0) {
	
	base_node ptref = pai_crack->point(ii) + e * 1.0e-7 * gradls;
	ctx1.set_xref(ptref);
	pf1->interpolation_grad(ctx1, coeff1, grad1, qdim);
	gmm::copy(grad1, E); gmm::add(gmm::transposed(grad1), E);
	gmm::scale(E, 0.5);
	scalar_type trace = gmm::mat_trace(E);
	gmm::copy(gmm::identity_matrix(), S1);
	gmm::scale(S1, lambda * trace);
	gmm::add(gmm::scaled(E, 2*mu), S1);
	gmm::mult(S1, up, jump);
	
	ERR[cv] += radius * coefficient * gmm::vect_norm2_sqr(jump);

      }
    }
 
    // jump of the stress between the element ant its neighbours.
    for (unsigned f1=0; f1 < mesh.structure_of_convex(cv)->nb_faces(); ++f1) {

      
      
      size_type cvn = mesh.neighbour_of_convex(cv, f1);

      bgeot::pgeometric_trans pgt2 = mesh.trans_of_convex(cvn);
      getfem::pfem pf2 = mf_u().fem_of_element(cvn);
      bgeot::vectors_to_base_matrix(G2, mesh.points_of_convex(cvn));
      coeff2.resize(mf_u().nb_dof_of_element(cvn));
      gmm::copy(gmm::sub_vector(U, gmm::sub_index(mf_u().ind_dof_of_element(cvn))), coeff2);
      getfem::fem_interpolation_context ctx2(pgt2, pf2, base_node(N), G2, cvn);
      gic.init(mesh.points_of_convex(cvn), pgt2);

      for (unsigned ii=0; ii < pai1->nb_points_on_face(f1); ++ii) {

	ctx1.set_xref(pai1->point_on_face(f1, ii));
	gmm::mult(ctx1.B(), pgt1->normals()[f1], up);
	scalar_type norm = gmm::vect_norm2(up);
	up /= norm;
	scalar_type coefficient = pai1->coeff_on_face(f1, ii) * ctx1.J() * norm; 
	
	pf1->interpolation_grad(ctx1, coeff1, grad1, qdim);
	gmm::copy(grad1, E); gmm::add(gmm::transposed(grad1), E);
	gmm::scale(E, 0.5);
	scalar_type trace = gmm::mat_trace(E);
	gmm::copy(gmm::identity_matrix(), S1);
	gmm::scale(S1, lambda * trace);
	gmm::add(gmm::scaled(E, 2*mu), S1);

	bool converged;
	gic.invert(ctx1.xreal(), xref2, converged);
	GMM_ASSERT1(converged, "geometric transformation not well inverted ... !");
	
	ctx2.set_xref(xref2);
	pf2->interpolation_grad(ctx2, coeff2, grad2, qdim);
	gmm::copy(grad2, E); gmm::add(gmm::transposed(grad2), E);
	gmm::scale(E, 0.5);
	trace = gmm::mat_trace(E);
	gmm::copy(gmm::identity_matrix(), S2);
	gmm::scale(S2, lambda * trace);
	gmm::add(gmm::scaled(E, 2*mu), S2);
	
	gmm::mult(S1, up, jump);
	gmm::mult_add(S2, gmm::scaled(up, -1.0), jump);

	ERR[cv] += radius * coefficient * gmm::vect_norm2_sqr(jump);

      }
      
    }
      


    

    
    
  }
  
}




bool crack_problem::solve(plain_vector &U) {
  
  size_type N = mesh.dim();
  dal::bit_vector conv_to_refine;
  bool iteration;
  
  do {
    cout << "Number of elements : "<<  mesh.convex_index().card() << endl;
    size_type nb_dof_rhs = mf_rhs.nb_dof();
    ls.reinit();
    for (size_type d = 0; d < ls.get_mesh_fem().nb_dof(); ++d) {
      base_small_vector v =  ls_function(ls.get_mesh_fem().point_of_dof(d), 0);
      ls.values(0)[d] = v[0];
      ls.values(1)[d] = v[1];
    }
    ls.touch();
    mls.adapt();
    mim.adapt();
    mfls_u.adapt();
    mimbound.adapt();
 
    cout << "Setting up the singular functions for the enrichment\n";
    std::vector<getfem::pglobal_function> vfunc(4);
    for (size_type i = 0; i < vfunc.size(); ++i) {
      /* use the singularity */
      getfem::abstract_xy_function *s = 
	new getfem::crack_singular_xy_function(i);
      getfem::abstract_xy_function *c = 
	new getfem::cutoff_xy_function(cutoff.fun_num,
				       cutoff.radius, 
				       cutoff.radius1, cutoff.radius0);
      s = new getfem::product_of_xy_functions(*s, *c);
      vfunc[i] = getfem::global_function_on_level_set(ls, *s);
    }
    mf_sing_u.set_functions(vfunc);

    mf_u_sum.set_mesh_fems(mf_sing_u, mfls_u);

    getfem::mdbrick_isotropic_linearized_elasticity<>
      ELAS(mim, mf_u(), lambda, mu);
 
    // Defining the volumic source term.
    plain_vector F(nb_dof_rhs * N);
    getfem::interpolation_function(mf_rhs, F, sol_f);
    getfem::mdbrick_source_term<> VOL_F(ELAS, mf_rhs, F);

    // Defining the Neumann condition right hand side.
    getfem::interpolation_function(mf_rhs, F, sol_F, NEUMANN_BOUNDARY_NUM1);
    getfem::mdbrick_source_term<> NEUMANN(VOL_F, mf_rhs, F, NEUMANN_BOUNDARY_NUM1);
  
    // Dirichlet condition brick.
    getfem::mdbrick_Dirichlet<> DIRICHLET(NEUMANN, DIRICHLET_BOUNDARY_NUM, mf_mult);
    DIRICHLET.set_constraints_type(getfem::constraints_type(dir_with_mult));
  
    getfem::mdbrick_abstract<> *final_model = &DIRICHLET;

    // Generic solve.
    cout << "Total number of variables : " << final_model->nb_dof() << endl;
    getfem::standard_model_state MS(*final_model);
    gmm::iteration iter(residual, 1, 40000);
  
    getfem::standard_solve(MS, *final_model, iter);
  
    // Solution extraction
    gmm::resize(U, mf_u().nb_dof());
    gmm::copy(ELAS.get_solution(MS), U);
    iteration = iter.converged();  

    conv_to_refine.clear();
    // Adapted Refinement (suivant une erreur a posteriori)
    if (adapted_refine && mesh.convex_index().card() < conv_max) {
      plain_vector ERR(mesh.convex_index().last_true()+1);
      error_estimate(U, ERR);

      // cout << "ERR = " << ERR << endl; 
    
      cout << "max = " << gmm::vect_norminf(ERR) << endl;
      scalar_type threshold = PARAM.real_value("REFINE_THRESHOLD",
					       "threshold for the refinement");
      scalar_type min_ = 1e18;
      conv_to_refine.clear();
      for (dal::bv_visitor i(mesh.convex_index()); !i.finished(); ++i) {
	if (ERR[i] > threshold) conv_to_refine.add(i);
	min_ = std::min(min_, ERR[i]);
      }
      cout << "min = " << min_ << endl;
      cout << "Refining " <<  conv_to_refine.card() << " elements..."<< endl;  
      mesh.Bank_refine(conv_to_refine);
    }

  } while(adapted_refine && conv_to_refine.card() > 0);

  mesh.write_to_file(datafilename + ".meshh");
  cout << "Refining process complete. The mesh contains now "
       <<  mesh.convex_index().card() << " convexes "<<endl;
  
  dal::bit_vector blocked_dof = mf_u().dof_on_set(5);
  getfem::mesh_fem mf_printed(mesh, N), mf_printed_vm(mesh);
  std::string FEM_DISC = PARAM.string_value("FEM_DISC","fem disc ");
  mf_printed.set_finite_element(mesh.convex_index(),
				getfem::fem_descriptor(FEM_DISC));
  mf_printed_vm.set_finite_element(mesh.convex_index(),
				   getfem::fem_descriptor(FEM_DISC));
  plain_vector W(mf_printed.nb_dof());
  getfem::interpolation(mf_u(), mf_printed, U, W);
  mf_printed.write_to_file(datafilename + ".meshfem", true);
  mf_printed_vm.write_to_file(datafilename + ".meshfem_vm", true);
  gmm::vecsave(datafilename + ".U", W);

  plain_vector VM(mf_printed_vm.nb_dof());
  getfem::interpolation_von_mises(mf_printed, mf_printed_vm, W, VM);
  gmm::vecsave(datafilename + ".VM", VM);

  return (iteration);
}

  
/**************************************************************************/
/*  main program.                                                         */
/**************************************************************************/

int main(int argc, char *argv[]) {

  GMM_SET_EXCEPTION_DEBUG; // Exceptions make a memory fault, to debug.
  FE_ENABLE_EXCEPT;        // Enable floating point exception for Nan.

  //getfem::getfem_mesh_level_set_noisy();

  crack_problem p;
  p.PARAM.read_command_line(argc, argv);
  p.init();
  p.mesh.write_to_file(p.datafilename + ".mesh");
  plain_vector U;
  if (!p.solve(U)) GMM_ASSERT1(false, "Solve has failed");
  
  {
    getfem::mesh mcut;
    p.mls.global_cut_mesh(mcut);
    unsigned Q = p.mf_u().get_qdim();
    getfem::mesh_fem mf(mcut, Q);
    mf.set_classical_discontinuous_finite_element(2, 0.001);
    // mf.set_finite_element
    //	(getfem::fem_descriptor("FEM_PK_DISCONTINUOUS(2, 2, 0.0001)"));
    plain_vector V(mf.nb_dof());
    
    getfem::interpolation(p.mf_u(), mf, U, V);
    
    getfem::stored_mesh_slice sl;
    getfem::mesh mcut_refined;
    
//     unsigned NX = p.PARAM.int_value("NX"), nn;
//     if (NX < 6) nn = 24;
//     else if (NX < 12) nn = 8;
//     else if (NX < 30) nn = 3;
//     else nn = 1;
    unsigned nn = 1;
    
    // choose an adequate slice refinement based on the distance to
    // the crack tip
    std::vector<bgeot::short_type> nrefine(mcut.convex_index().last_true()+1);
    for (dal::bv_visitor cv(mcut.convex_index()); !cv.finished(); ++cv) {
      scalar_type dmin=0, d;
      base_node Pmin,P;
      for (unsigned i=0; i < mcut.nb_points_of_convex(cv); ++i) {
	P = mcut.points_of_convex(cv)[i];
	d = gmm::vect_norm2(ls_function(P));
	if (d < dmin || i == 0) { dmin = d; Pmin = P; }
      }
      
      if (dmin < 1e-5)
	nrefine[cv] = nn*8;
      else if (dmin < .1) 
	nrefine[cv] = nn*2;
      else nrefine[cv] = nn;
      nrefine[cv] = 1;
      // if (dmin < .01)
      //  cout << "cv: "<< cv << ", dmin = " << dmin << "Pmin=" << Pmin 
      //       << " " << nrefine[cv] << "\n";
    }
    
    {
      getfem::mesh_slicer slicer(mcut); 
      getfem::slicer_build_mesh bmesh(mcut_refined);
      slicer.push_back_action(bmesh);
      slicer.exec(nrefine, getfem::mesh_region::all_convexes());
    }
    
    // sl.build(mcut, 
    // getfem::slicer_build_mesh(mcut_refined), nrefine);
    
    getfem::mesh_im mim_refined(mcut_refined); 
    mim_refined.set_integration_method(getfem::int_method_descriptor
				       ("IM_TRIANGLE(6)"));
    
    getfem::mesh_fem mf_refined(mcut_refined, Q);
    mf_refined.set_classical_discontinuous_finite_element(2, 0.0001);
    plain_vector W(mf_refined.nb_dof());
    
    getfem::interpolation(p.mf_u(), mf_refined, U, W);
    
    if (p.PARAM.int_value("VTK_EXPORT")) {
      getfem::mesh_fem mf_refined_vm(mcut_refined, 1);
      mf_refined_vm.set_classical_discontinuous_finite_element(1, 0.0001);
      cerr << "mf_refined_vm.nb_dof=" << mf_refined_vm.nb_dof() << "\n";
      plain_vector VM(mf_refined_vm.nb_dof());
      
      cout << "computing von mises\n";
      getfem::interpolation_von_mises(mf_refined, mf_refined_vm, W, VM);
      
      plain_vector D(mf_refined_vm.nb_dof() * Q), 
	DN(mf_refined_vm.nb_dof());
      
      
      cout << "export to " << p.datafilename + ".vtk" << "..\n";
      getfem::vtk_export exp(p.datafilename + ".vtk",
			     p.PARAM.int_value("VTK_EXPORT")==1);
      exp.exporting(mf_refined); 
      //exp.write_point_data(mf_refined_vm, DN, "error");
      exp.write_point_data(mf_refined_vm, VM, "von mises stress");
      exp.write_point_data(mf_refined, W, "elastostatic_displacement");
      cout << "export done, you can view the data file with (for example)\n"
	"mayavi -d " << p.datafilename << ".vtk -f "
	"WarpVector -m BandedSurfaceMap -m Outline\n";
    }
  }
  
  return 0; 
}



