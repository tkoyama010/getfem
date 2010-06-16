// -*- c++ -*- (enables emacs c++ mode)
//===========================================================================
//
// Copyright (C) 2002-2008 Yves Renard, Julien Pommier.
//
// This file is a part of GETFEM++
//
// Getfem++  is  free software;  you  can  redistribute  it  and/or modify it
// under  the  terms  of the  GNU  Lesser General Public License as published
// by  the  Free Software Foundation;  either version 2.1 of the License,  or
// (at your option) any later version.
// This program  is  distributed  in  the  hope  that it will be useful,  but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or  FITNESS  FOR  A PARTICULAR PURPOSE.  See the GNU Lesser General Public
// License for more details.
// You  should  have received a copy of the GNU Lesser General Public License
// along  with  this program;  if not, write to the Free Software Foundation,
// Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//
//===========================================================================

/**
   @file plasticity.cc
   @brief Small deformation plasticity problem.

   This program is used to check that getfem++ is working. This is also 
   a good example of use of Getfem++.
*/


#include "getfem/getfem_assembling.h" /* import assembly methods (and norms comp.) */
#include "getfem/getfem_regular_meshes.h"
#include "getfem/getfem_model_solvers.h"
#include "getfem/getfem_plasticity.h"
#include "getfem/getfem_export.h"


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

template<typename VEC> static void vecsave(
   std::string fname, const VEC& V);

//function to save a vector
template<typename VEC> static void vecsave(
   std::string fname, const VEC& V) {
       std::ofstream f(fname.c_str()); f.precision(16);
       for (size_type i=0; i < V.size(); ++i) 
	 f << V[i] << "\n"; 
}




/*
  structure for the plastic problem
*/
struct plasticity_problem {

  enum { DIRICHLET_BOUNDARY_NUM = 0, 
	 NEUMANN_BOUNDARY_NUM = 1};

  getfem::mesh mesh;         /* the mesh */
  getfem::mesh_im  mim;      /* integration methods. */
  getfem::mesh_fem mf_u;     /* main mesh_fem, for the elastostatic solution */
  getfem::mesh_fem mf_sigma; /* main mesh_fem, for the elastostatic solution */
  getfem::mesh_fem mf_rhs;   /* mesh_fem for the right hand side (f(x),..)   */
  scalar_type lambda, mu;    /* Lam� coefficients.*/

  scalar_type residual;        /* max residual for the iterative solvers         */

  scalar_type stress_threshold;
  size_type flag_hyp;
  std::vector<std::vector<scalar_type> > sigma_b;

  std::string datafilename;
  bgeot::md_param PARAM;

  bool solve(plain_vector &U);
  void init(void);

  plasticity_problem(void) : mim(mesh), mf_u(mesh), 
			     mf_sigma(mesh), mf_rhs(mesh) {}
};





/* Read parameters from the .param file, build the mesh, set finite element
 * and integration methods and selects the boundaries.
 */
void plasticity_problem::init(void)
{
  std::string MESH_TYPE = 
    PARAM.string_value("MESH_TYPE","Mesh type ");
  std::string FEM_TYPE  = 
    PARAM.string_value("FEM_TYPE","FEM name");
  std::string FEM_TYPE_SIGMA = 
    PARAM.string_value("FEM_TYPE_SIGMA","FEM name");
  std::string INTEGRATION = 
    PARAM.string_value("INTEGRATION", 
		       "Name of integration method");
  cout << "MESH_TYPE=" << MESH_TYPE << "\n";
  cout << "FEM_TYPE="  << FEM_TYPE << "\n";
  cout << "INTEGRATION=" << INTEGRATION << "\n";

  residual = PARAM.real_value("RESIDUAL", "residual");

  //  file to save the mesh
  datafilename = PARAM.string_value("ROOTFILENAME",
				    "Filename for saving");


  /* First step : build the mesh */
  size_type N;
  bgeot::pgeometric_trans pgt = 0; 

  if (MESH_TYPE != "load") {
    std::cout << "created getfem mesh"  << "\n"; 
    pgt = bgeot::geometric_trans_descriptor(MESH_TYPE);
    N = pgt->dim();
    std::vector<size_type> nsubdiv(N);
    nsubdiv[0]=PARAM.int_value("NX", 
      "Nomber of space steps in x direction ");
    nsubdiv[1]=PARAM.int_value("NY", 
      "Nomber of space steps in y direction ");
    if(N==3)
      nsubdiv[2]=PARAM.int_value("NZ",
	 "Nomber of space steps in z direction ");
    getfem::regular_unit_mesh(mesh, nsubdiv, pgt,
         PARAM.int_value("MESH_NOISED")!= 0);
    
    bgeot::base_matrix M(N,N);

    for (size_type i=0; i < N; ++i) {
      static const char *t[] = {"LX","LY","LZ"};
      M(i,i) = (i<3) ? PARAM.real_value(t[i],t[i]) : 1.0;
    }

    if (N>1) { 
      M(0,1) = PARAM.real_value("INCLINE") * 
	       PARAM.real_value("LY"); 
    }


    /* scale the unit mesh to [LX,LY,..] and incline it */
    mesh.transformation(M);

  } else {
    std ::cout << "mesh from pdetool"  << "\n"; 
    std::string MESH_FILE = PARAM.string_value("MESH_FILE",
			    "Mesh file name");
    mesh.read_from_file(MESH_FILE);
    
    N = mesh.dim();
    pgt = mesh.trans_of_convex(
	      mesh.convex_index().first_true());
  }

  mu = PARAM.real_value("MU", "Lam� coefficient mu");
  lambda = PARAM.real_value("LAMBDA", 
			    "Lam� coefficient lambda");
  mf_u.set_qdim(bgeot::dim_type(N));
  mf_sigma.set_qdim(bgeot::dim_type(N*N));


  /* set the finite element on the mf_u */
  getfem::pfem pf_u = 
    getfem::fem_descriptor(FEM_TYPE);
  getfem::pintegration_method ppi = 
    getfem::int_method_descriptor(INTEGRATION);

  mim.set_integration_method(mesh.convex_index(), ppi);
  mf_u.set_finite_element(mesh.convex_index(), pf_u);


  /* set the finite element on the mf_sigma */
  getfem::pfem pf_sigma = 
    getfem::fem_descriptor(FEM_TYPE_SIGMA);
  mf_sigma.set_finite_element(mesh.convex_index(), pf_sigma);


  
  /* set the finite element on mf_rhs (same as mf_u is DATA_FEM_TYPE is not used in the .param file */
  std::string data_fem_name = PARAM.string_value(
				"DATA_FEM_TYPE");
  if (data_fem_name.size() == 0) {
    GMM_ASSERT1(pf_u->is_lagrange(), 
		"You are using a non-lagrange FEM. "
		<< "In that case you need to set "
		<< "DATA_FEM_TYPE in the .param file");
    mf_rhs.set_finite_element(mesh.convex_index(), pf_u);
    GMM_TRACE2("On rentre ici pourtant");
  } else {
    mf_rhs.set_finite_element(mesh.convex_index(), 
		getfem::fem_descriptor(data_fem_name));
    GMM_TRACE2("On rentre ici pourtant : ou pas !!!!!!");
  }


  /* set boundary conditions
   * (Neuman on the upper face, Dirichlet elsewhere) */
  cout << "Selecting Neumann and Dirichlet boundaries\n";
  getfem::mesh_region border_faces;
  getfem::outer_faces_of_mesh(mesh, border_faces);
  for (getfem::mr_visitor it(border_faces); 
       !it.finished(); ++it) {
    assert(it.is_face());
    base_node un = mesh.normal_of_face_of_convex(it.cv(), 
						 it.f());
    un /= gmm::vect_norm2(un);

    if (gmm::abs(un[0] - 1.0) < 1.0E-7)
      mesh.region(NEUMANN_BOUNDARY_NUM).add(it.cv(), it.f());
    else if (gmm::abs(un[0] + 1.0) < 1.0E-7) 
      mesh.region(DIRICHLET_BOUNDARY_NUM).add(it.cv(),it.f());
      
  }
 
  //PARTIE RELATIVE A LA PLASTICITE  
  stress_threshold = PARAM.real_value("STRESS_THRESHOLD",
			      "plasticity stress_threshold");
  flag_hyp=PARAM.int_value("FLAG_HYP");
}



/**************************************************************************/
/*  Model.                                                                */
/**************************************************************************/

bool plasticity_problem::solve(plain_vector &U) {
  size_type nb_dof_rhs = mf_rhs.nb_dof();
  size_type N = mesh.dim();




  getfem::model model;

  // Main unknown of the problem.
  model.add_fem_variable("u", mf_u, 2);


  // if(lambda.size()==0){
    model.add_initialized_scalar_data("lambda", lambda);
    model.add_initialized_scalar_data("mu", mu);
    model.add_initialized_scalar_data("s", stress_threshold);
      /* }else{
    model.add_initialized_fem_data("lambda", mf_rhs, lambda);
    model.add__initialized_fem_data("mu", mf_rhs, mu);
    model.add_initialized_fem_data("s", mf_rhs, stress_threshold);
  }
      */

  model.add_fem_data("sigma", mf_sigma, 
		     bgeot::dim_type(N*N), 2);
  add_plasticity_brick(model, mim, "u", "lambda", "mu", 
		       "s", "sigma");


  plain_vector F(nb_dof_rhs * N);
  model.add_initialized_fem_data("NeumannData", mf_rhs, F);
  getfem::add_source_term_brick
    (model, mim, "u", "NeumannData", NEUMANN_BOUNDARY_NUM);


  model.add_initialized_fem_data("DirichletData", mf_rhs, F);
  getfem::add_Dirichlet_condition_with_multipliers
    (model, mim, "u", mf_u, DIRICHLET_BOUNDARY_NUM, 
     "DirichletData");






  // getfem::standard_model_state MS(final_model);

  const size_type Nb_t = 1;

  for (size_type nb = 0; nb < Nb_t; ++nb) {

    scalar_type t = scalar_type(nb+1) * 0.5;

    // Defining the Neumann condition right hand side.
    base_small_vector v(N);
    v[N-1] = -PARAM.real_value("FORCE");
    gmm::scale(v,t);
    
    for (size_type i = 0; i < nb_dof_rhs; ++i)
      gmm::copy(v, gmm::sub_vector
		(F, gmm::sub_interval(i*N, N)));

    gmm::copy(F, model.set_real_variable("NeumannData"));
    
    // Generic solve.
    cout << "Number of variables : " 
	 << model.nb_dof() << endl;

    gmm::iteration iter(residual, 2, 40000);
    getfem::standard_solve(model, iter);

    // ?? PLAS.compute_constraints(MS);
    
    // Get the solution and save it
    gmm::copy(model.real_variable("u"), U);
    

 //    //Get sigma_bar (remaining constraints) and save it
//     PLAS.get_proj(sigma_b);
    
//     std::ofstream s(sname.c_str(),std::ios_base::app); s.precision(16);
//     size_type nb_elts;
//     size_type nb_cv = gmm::vect_size(sigma_b);
//     s << "\n";
//     for (size_type cv=0;cv<nb_cv;++cv){
//       nb_elts = gmm::vect_size(sigma_b[cv]);
//       for(size_type i=0;i<nb_elts;++i) s <<sigma_b[cv][i] <<" ";
//     }      
    
  }

//   getfem::mesh_fem mf_vm(mesh);
//   mf_vm.set_classical_discontinuous_finite_element(2);
//   getfem::base_vector VM(mf_vm.nb_dof());
//   PLAS.compute_Von_Mises_or_Tresca(mf_vm, VM, false);
//   getfem::vtk_export exp(datafilename + ".vtk");
//   exp.exporting(mf_vm);
//   exp.write_point_data(mf_vm,VM, "Von Mises stress");
//   exp.write_point_data(mf_u, U, "displacement");
//   cout << "export done, you can view the data file with (for example)\n"
//     "mayavi -d " << datafilename << ".vtk -f "
// 	"WarpVector -m BandedSurfaceMap -m Outline\n";

  return true;
}
  
/**************************************************************************/
/*  main program.                                                         */
/**************************************************************************/
int main(int argc, char *argv[]) {

  GMM_SET_EXCEPTION_DEBUG; // Exceptions make a memory fault, to debug.
  FE_ENABLE_EXCEPT;        // Enable floating point exception for Nan.

  // try {    
  plasticity_problem p;
  p.PARAM.read_command_line(argc, argv);
  p.init();
  p.mesh.write_to_file(p.datafilename + ".mesh");
  plain_vector U(p.mf_u.nb_dof());
  if (!p.solve(U)) GMM_ASSERT1(false, "Solve has failed");
  GMM_TRACE2("solve ok");
  cout << "Resultats dans fichier : "
       <<p.datafilename<<".* \n";
  p.mf_u.write_to_file(p.datafilename + ".meshfem",true);
  scalar_type t[2]={p.mu,p.lambda};
  vecsave(p.datafilename+".coef", 
	  std::vector<scalar_type>(t, t+2));    
  // } GMM_STANDARD_CATCH_ERROR;
  
  return 0; 
}
