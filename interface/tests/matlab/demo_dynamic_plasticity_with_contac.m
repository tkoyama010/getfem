% Copyright (C) 2010-2015 Yves Renard, Farshid Dabaghi.
%
% This file is a part of GETFEM++
%
% Getfem++  is  free software;  you  can  redistribute  it  and/or modify it
% under  the  terms  of the  GNU  Lesser General Public License as published
% by  the  Free Software Foundation;  either version 3 of the License,  or
% (at your option) any later version along with the GCC Runtime Library
% Exception either version 3.1 or (at your option) any later version.
% This program  is  distributed  in  the  hope  that it will be useful,  but
% WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
% or  FITNESS  FOR  A PARTICULAR PURPOSE.  See the GNU Lesser General Public
% License and GCC Runtime Library Exception for more details.
% You  should  have received a copy of the GNU Lesser General Public License
% along  with  this program;  if not, write to the Free Software Foundation,
% Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.





% We compute a dynamic plasticity problem with a Von Mises criterion with or
% without kinematic hardening and with unilateral contact with a rigid obstacle.
% For convenience we consider an homogenous Dirichlet condition on the top
% of the domain. 



clear all;
gf_workspace('clear all');
clc;

with_hardening = 1;
bi_material = false;
test_tangent_matrix = 0;
do_plot = true;
plot_mesh = true;
% Initialize used data
LX = 20;
LY = 100;
NX = 20;
NY = 50;


% alpha is parametr of the generalized integration algorithms The
% The choice alpha = 1/2 yields the mid point method and alpha = 1 leads to
% backward Euler integration
alpha_method = true;
alpha = 1;


f = [0 15000]';

dirichlet_val = 7;


% transient part.
T = pi/4;
dt = 0.001;
theta= 1;





% Create the mesh
% m = gfMesh('triangles grid', [0:(LX/NX):LX], [0:(LY/NY):LY]);
m = gfMesh('import','structured',sprintf('GT="GT_PK(2,1)";SIZES=[%d,%d];NOISED=0;NSUBDIV=[%d,%d];', LX, LY, NX, NY));
N = gf_mesh_get(m, 'dim');


r = 100000;                  % Augmentation parameter

% Signed distance representing the obstacle
if (N == 1) obstacle = 'x'; elseif (N == 2) obstacle = 'y'; else obstacle = 'z'; end;
  
% Plotting
% gf_plot_mesh(m, 'vertices', 'on', 'convexes', 'on');
% return;

%lambda_degree =1;

% Define used MeshIm
mim=gfMeshIm(m);  set(mim, 'integ', gfInteg('IM_TRIANGLE(6)')); % Gauss methods on triangles

% Define used MeshFem
mf_u=gfMeshFem(m,2); set(mf_u, 'fem',gfFem('FEM_PK(2,2)'));
mf_data=gfMeshFem(m); set(mf_data, 'fem', gfFem('FEM_PK_DISCONTINUOUS(2,0)'));
% mf_sigma=gfMeshFem(m,4); set(mf_sigma, 'fem',gfFem('FEM_PK_DISCONTINUOUS(2,1)'));
mf_sigma=gfMeshFem(m,4); set(mf_sigma, 'fem',gfFem('FEM_PK_DISCONTINUOUS(2,0)'));
mf_vm = gfMeshFem(m); set(mf_vm, 'fem', gfFem('FEM_PK_DISCONTINUOUS(2,1)'));


%mfflambda = gf_mesh_fem(mesh1, N);  gf_mesh_fem_set(mfflambda, 'classical fem', 1);
mflambda=gfMeshFem(m,1); set(mflambda, 'fem',gfFem('FEM_PK(2,1)'));


GAMMAD = 1; GAMMAC = 2;
% Find the border of the domain
% P=get(m, 'pts');
% pidleft=find(abs(P(1,:))<1e-6); % Retrieve index of points which x near to 0
% pidright=find(abs(P(1,:) - LX)<1e-6); % Retrieve index of points which x near to L
% fleft =get(m,'faces from pid',pidleft); 
% fright=get(m,'faces from pid',pidright);
% %set(m,'boundary',1,fleft); % for Dirichlet condition
% set(m,'region',GAMMAD,fleft);
% %set(m,'boundary',2,fright); % for Contact condition
% set(m,'region',GAMMAC,fright);
P=get(m, 'pts');
border = gf_mesh_get(m,'outer faces');
normals = gf_mesh_get(m, 'normal of faces', border);
contact_boundary=border(:, find(normals(N, :) < -0.01));
gf_mesh_set(m, 'region', GAMMAC, contact_boundary);
contact_boundary=border(:, find(normals(N, :) > 0.01));
gf_mesh_set(m, 'region', GAMMAD, contact_boundary);

if (plot_mesh)
 gf_plot_mesh(m, 'regions', [GAMMAC],'vertices', 'off', 'convexes', 'off');
 title('Mesh and contact boundary (in red)');
 pause(0.1);

end

% Decomposed the mesh into 2 regions with different values of Lamé coeff
if (bi_material) separation = LX/2; else separation = 0; end
pidtop    = find(P(1,:)>=separation-1E-6); % Retrieve index of points of the top part
pidbottom = find(P(1,:)<=separation+1E-6); % Retrieve index of points of the bottom part
cvidtop   = get(m, 'cvid from pid', pidtop);
cvidbottom= get(m, 'cvid from pid', pidbottom);
CVtop     = sort(get(mf_data, 'basic dof from cvid', cvidtop));
CVbottom  = sort(get(mf_data, 'basic dof from cvid', cvidbottom));

% Definition of Lame coeff
lambda(CVbottom,1) = 121150; % Steel
lambda(CVtop,1) = 84605; % Iron
mu(CVbottom,1) = 80769; %Steel
mu(CVtop,1) = 77839; % Iron
% Definition of plastic threshold
von_mises_threshold(CVbottom) = 7000;
von_mises_threshold(CVtop) = 8000;
rho = 0.1;
% Definition of hardening parameter
if (with_hardening)
  H = mu(1)/5;
else
  H = 0;
end

% Create the model
md = gfModel('real');

% Declare that u is the unknown of the system on mf_u
% 2 is the number of version of the data stored, for the time integration scheme 
set(md, 'add fem variable', 'u', mf_u, 2);



% Time integration scheme and inertia term
if(alpha_method) 
 nbdofu = gf_mesh_fem_get(mf_u, 'nbdof');
 M = gf_asm('mass matrix', mim, mf_u);
 gf_model_set(md, 'add fem data', 'Previous_u', mf_u);
 set(md, 'add initialized data', 'rho', rho);
% % gf_model_set(md, 'add mass brick', mim, string varname[, string dataname_rho[, int region]]);
% gf_model_set(md, 'add mass brick', mim,'u' ,'rho');
 
else

gf_model_set(md, 'add theta method for second order', 'u',theta);
gf_model_set(md, 'set time step', dt);


set(md, 'add initialized data', 'rho', rho);
gf_model_set(md, 'add mass brick', mim, 'Dot2_u', 'rho');

end




% Declare that lambda is a data of the system on mf_data
set(md, 'add initialized fem data', 'lambda', mf_data, lambda);

% Declare that mu is a data of the system on mf_data
set(md, 'add initialized fem data', 'mu', mf_data, mu);

% Declare that von_mises_threshold is a data of the system on mf_data
set(md, 'add initialized fem data', 'von_mises_threshold', mf_data, von_mises_threshold);

N = gf_mesh_get(m, 'dim');
% gf_model_set(md, 'add fem data', 'Previous_u', mf_u);
mim_data = gf_mesh_im_data(mim, -1, [N, N]);
gf_model_set(md, 'add im data', 'sigma', mim_data);


 % Declare that alpha is a data of the system 
 
set(md, 'add initialized data', 'alpha', [alpha]);

set(md, 'add initialized data', 'H', [H]);

Is = 'Reshape(Id(meshdim*meshdim),meshdim,meshdim,meshdim,meshdim)';
IxI = 'Id(meshdim)@Id(meshdim)';
coeff_long = '((lambda)*(H))/((2*(mu)+(H))*(meshdim*(lambda)+2*(mu)+(H)))';
B_inv = sprintf('((2*(mu)/(2*(mu)+(H)))*(%s) + (%s)*(%s))', Is, coeff_long, IxI);
B = sprintf('((1+(H)/(2*(mu)))*(%s) - (((lambda)*(H))/(2*(mu)*(meshdim*(lambda)+2*(mu))))*(%s))', Is, IxI);
ApH = sprintf('((2*(mu)+(H))*(%s) + (lambda)*(%s))', Is, IxI);
Enp1 = '((Grad_u+Grad_u'')/2)';
En = '((Grad_Previous_u+Grad_Previous_u'')/2)';
 %expression de sigma for Implicit Euler method
  expr_sigma = strcat('(', B_inv, '*(Von_Mises_projection((-(H)*', Enp1, ')+(', ApH, '*(',Enp1,'-',En,')) + (', B, '*sigma), von_mises_threshold) + H*', Enp1, '))');
  
 
if(alpha_method)
 %expression de sigma for generalized alpha algorithms
  expr_sigma = strcat('(', B_inv, '*(Von_Mises_projection((',B,'*(1-alpha)*sigma)+(-(H)*(((1-alpha)*',En,')+(alpha*', Enp1, ')))+(alpha*', ApH, '*(',Enp1,'-',En,')) + (alpha*', ...
    B, '*sigma), von_mises_threshold) + (H)*(((1-alpha)*',En,')+(alpha*', Enp1, '))))');
end


gf_model_set(md, 'add nonlinear generic assembly brick', mim, strcat(expr_sigma, ':Grad_Test_u'));
% gf_model_set(md, 'add finite strain elasticity brick', mim, 'u', 'SaintVenant Kirchhoff', '[lambda; mu]');



  gf_model_set(md, 'add initialized data', 'dirichletdata', [0; dirichlet_val]);
  gf_model_set(md, 'add Dirichlet condition with multipliers', mim, 'u', mf_u, GAMMAD, 'dirichletdata');

%  gf_model_set(md, 'add initialized data', 'dirichletdata', [0; dirichlet_val]);
% % Add homogeneous Dirichlet condition to u on the left hand side of the domain
% set(md, 'add Dirichlet condition with multipliers', mim, 'u', mf_u, GAMMAD);

% Add a source term to the system
set(md,'add initialized fem data', 'VolumicData', mf_data, get(mf_data, 'eval',{f(1,1)*sin(0);f(2,1)*sin(0)}));
set(md, 'add source term brick', mim, 'u', 'VolumicData', 2);



% Contact brick 


ldof = gf_mesh_fem_get(mflambda, 'dof on region', GAMMAC);
mflambda_partial = gf_mesh_fem('partial', mflambda, ldof);
gf_model_set(md, 'add fem variable', 'lambda_n', mflambda_partial);
gf_model_set(md, 'add initialized data', 'r', [r]);
OBS = gf_mesh_fem_get(mflambda, 'eval', { obstacle });
gf_model_set(md, 'add initialized fem data', 'obstacle',mflambda , OBS);
gf_model_set(md, 'add integral contact with rigid obstacle brick', ...
    mim, 'u', 'lambda_n', 'obstacle', 'r', GAMMAC, 1);

%   gf_model_set(md, 'add nodal contact with rigid obstacle brick', ...
%        mim, 'u', 'lambda_n',  'r', GAMMAC,'y', 1); 
  
%   * ind = gf_model_set(model M, 'add nodal contact with rigid obstacle brick',  mesh_im mim, string varname_u, string multname_n[, string multname_t], string dataname_r[, string dataname_friction_coeff], int region, string obstacle[,  int augmented_version])





% interpolate the initial data
%U0 = get(md, 'variable', 'u');
U0 = (gf_mesh_fem_get(mf_u, 'eval', {0, sprintf('%g +0.0+0.00*y', dirichlet_val)}));
V0 = 0*U0;
%V0 = (gf_mesh_fem_get(mf_u, 'eval', { '-0.0','-600' }))';

if(alpha_method)
gf_model_set(md, 'add explicit matrix', 'u', 'u',rho* M/(dt*dt*alpha));
ind_rhs = gf_model_set(md, 'add explicit rhs', 'u', zeros(nbdofu,1));
MV0=M*V0';
else
    
    
% Initial data.
gf_model_set(md, 'variable', 'Previous_u',  U0);
gf_model_set(md, 'variable', 'Previous_Dot_u',  V0);


% Initialisation of the acceleration 'Previous_Dot2_u'
gf_model_set(md, 'perform init time derivative', dt/20.);
gf_model_get(md, 'solve');

end







VM=zeros(1,get(mf_vm, 'nbdof'));
 step=1;
% Iterations
for t = 0:dt:T
  
  coeff = -sin(1-(4*t));
 % coeff=- 1;
  
   disp(sprintf('step %d, coeff = %g', step , coeff)); 
   set(md, 'variable', 'VolumicData', get(mf_data, 'eval',{f(1,1)*coeff;f(2,1)*coeff}));  
  
if(alpha_method)
    
   MU0=M*U0';
     
   LL = rho*(( 1/(dt*dt*alpha))*MU0+( 1/(dt*alpha))*MV0);
 
   gf_model_set(md, 'set private rhs', ind_rhs, LL);
  get(md, 'solve', 'noisy', 'lsearch', 'simplest',  'alpha min', 0.8, 'max_iter', 100, 'max_res', 1e-6);
  % gf_model_get(md, 'solve', 'max_res', 1e-6, 'max_iter', 100);
   %get(md, 'solve', 'noisy', 'max_iter', 80);
   U = gf_model_get(md, 'variable', 'u');
   MV = ((M*U' - MU0)/dt -(1-alpha)*MV0)/alpha;

  
else
   
   get(md, 'solve', 'noisy', 'lsearch', 'simplest',  'alpha min', 0.8, 'max_iter', 100, 'max_res', 1e-6);
   U = gf_model_get(md, 'variable', 'u');
   V = gf_model_get(md, 'variable', 'Dot_u'); 
  
end

      
  if (test_tangent_matrix)
      gf_model_get(md, 'test tangent matrix', 1E-8, 10, 0.000001);
  end;
    
 if (alpha_method)
      sigma_0 = gf_model_get(md, 'variable', 'sigma');
      sigma = gf_model_get(md, 'interpolation', expr_sigma, mim_data);
      U_0 = gf_model_get(md, 'variable', 'Previous_u');
      U_nalpha = alpha*U + (1-alpha)*U_0;
       
       
       
      M_vm = gf_asm('mass matrix', mim, mf_vm);
      L = gf_asm('generic', mim, 1, 'sqrt(3/2)*Norm(Deviator(sigma))*Test_vm', -1, 'sigma', 0, mim_data, sigma, 'vm', 1, mf_vm, zeros(gf_mesh_fem_get(mf_vm, 'nbdof'),1));
      VM = (M_vm \ L)';
      coeff1='-lambda/(2*mu*(meshdim*lambda+2*mu))';
      coeff2='1/(2*mu)';
      Ainv=sprintf('(%s)*(%s) + (%s)*(%s)', coeff1, IxI, coeff2, Is);
      Ep = sprintf('(Grad_u+Grad_u'')/2 - (%s)*sigma', Ainv);
      L = gf_asm('generic', mim, 1, sprintf('Norm(%s)*Test_vm', Ep), -1, 'sigma', 0, mim_data, sigma, 'u', 0, mf_u, U, 'vm', 1, mf_vm, zeros(gf_mesh_fem_get(mf_vm, 'nbdof'),1), 'mu', 0, mf_data, mu, 'lambda', 0, mf_data, lambda);
      plast = (M_vm \ L)';
      
      gf_model_set(md, 'variable', 'u', U_nalpha);
      Epsilon_u = gf_model_get(md, 'interpolation', '((Grad_u+Grad_u'')/2)', mim_data);
 
      nb_gauss_pt_per_element = size(sigma, 2) / (N*N*gf_mesh_get(m, 'nbcvs'));
      % ind_gauss_pt = 22500;
      ind_gauss_pt = nb_gauss_pt_per_element * 1100 - 1;
      ind_elt = floor(ind_gauss_pt / nb_gauss_pt_per_element);
       P = gf_mesh_get(m, 'pts from cvid', ind_elt);
      disp(sprintf('Point for the strain/stress graph (approximately): (%f,%f)', P(1,1), P(2,1)));
     
    if (size(sigma, 2) <= N*(ind_gauss_pt + 1))
      ind_gauss_pt = floor(3*size(sigma, 2) / (4*N*N));
    end
      sigma_fig(1,step)=sigma(N*N*ind_gauss_pt + 1);
      Epsilon_u_fig(1,step)=Epsilon_u(N*N*ind_gauss_pt + 1);
      sigma = (sigma - (1-alpha)*sigma_0)/alpha;
      gf_model_set(md, 'variable', 'sigma', sigma);
      gf_model_set(md, 'variable', 'Previous_u', U);
  else
   
    
    sigma = gf_model_get(md, 'interpolation', expr_sigma, mim_data);
    gf_model_set(md, 'variable', 'sigma', sigma);
    gf_model_set(md, 'variable', 'Previous_u', U);
    

      
    M_VM = gf_asm('mass matrix', mim, mf_vm);
    L = gf_asm('generic', mim, 1, 'sqrt(3/2)*Norm(Deviator(sigma))*Test_vm', -1, 'sigma', 0, mim_data, sigma, 'vm', 1, mf_vm, zeros(gf_mesh_fem_get(mf_vm, 'nbdof'),1));
    VM = (M_VM \ L)';
    coeff1='-lambda/(2*mu*(meshdim*lambda+2*mu))';
    coeff2='1/(2*mu)';
    Ainv=sprintf('(%s)*(%s) + (%s)*(%s)', coeff1, IxI, coeff2, Is);
    Ep = sprintf('(Grad_u+Grad_u'')/2 - (%s)*sigma', Ainv);
    L = gf_asm('generic', mim, 1, sprintf('Norm(%s)*Test_vm', Ep), -1, 'sigma', 0, mim_data, sigma, 'u', 0, mf_u, U, 'vm', 1, mf_vm, zeros(gf_mesh_fem_get(mf_vm, 'nbdof'),1), 'mu', 0, mf_data, mu, 'lambda', 0, mf_data, lambda);
    plast = (M_VM \ L)';
      
    Epsilon_u = gf_model_get(md, 'interpolation', '((Grad_u+Grad_u'')/2)', mim_data);
    nb_gauss_pt_per_element = size(sigma, 2) / (N*N*gf_mesh_get(m, 'nbcvs'));
    % ind_gauss_pt = 22500;
    ind_gauss_pt = nb_gauss_pt_per_element * 1100 - 1;
    ind_elt = floor(ind_gauss_pt / nb_gauss_pt_per_element);
    P = gf_mesh_get(m, 'pts from cvid', ind_elt);
    disp(sprintf('Point for the strain/stress graph (approximately): (%f,%f)', P(1,1), P(2,1)));
   
    if (size(sigma, 2) <= N*(ind_gauss_pt + 1))
      ind_gauss_pt = floor(3*size(sigma, 2) / (4*N*N));
    end
    sigma_fig(1,step)=sigma(N*N*ind_gauss_pt + 1);
    Epsilon_u_fig(1,step)=Epsilon_u(N*N*ind_gauss_pt + 1);
    
    
 end  
    
    
       
    if (do_plot)
      figure(1)
      subplot(1,2,1);
      gf_plot(mf_vm,VM, 'deformation',U,'deformation_mf',mf_u,'refine', 4, 'deformation_scale',1, 'disp_options', 0); % 'deformed_mesh', 'on')
      hold on
      X=[-5:1:25];
      Y=0*[-5:1:25];
      plot(X,Y ,'k','LineWidth',3 )
      hold off
      
      colorbar;
      axis([-10 30 -5 120]);
      
      
      % caxis([0 10000]);
      n = t;
       title(['Von Mises criterion for t = ', num2str(t)]);
      subplot(1,2,2);
      gf_plot(mf_vm,plast, 'deformation',U,'deformation_mf',mf_u,'refine', 4, 'deformation_scale',1, 'disp_options', 0);  % 'deformed_mesh', 'on')
      hold on
      X=[-5:1:25];
      Y=0*[-5:1:25];
      plot(X,Y ,'k','LineWidth',3 )
      hold off
      colorbar;
      axis([-10 30 -5 120]);
      % caxis([0 10000]);
      n = t;
      title(['Plastification for t = ', num2str(t)]);
    
      figure(2)
      %subplot(2,2,[3 4]);
      plot(Epsilon_u_fig, sigma_fig,'r','LineWidth',2)
      xlabel('Strain');
      ylabel('Stress')
      axis([-0.3 0.3 -16500 16500 ]);
      title(sprintf('step %d / %d, coeff = %g', step,size([0:dt:T],2) , coeff));
      
      pause(0.1);
    end
    
     step= step+ 1;
     
     if(alpha_method)
         
  U0 = U;
  MV0 = MV;
 
     else
         
        
    gf_model_set(md, 'shift variables for time integration');
    
     end
end;










