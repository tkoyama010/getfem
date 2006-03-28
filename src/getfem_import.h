// -*- c++ -*- (enables emacs c++ mode)
//========================================================================
//
// Copyright (C) 2000-2006 Julien Pommier
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

/**@file getfem_import.h 
   @author  Julien Pommier <Julien.Pommier@insa-toulouse.fr>
   @date Januar 17, 2003.
   @brief Import mesh files from various formats.
*/

#ifndef GETFEM_IMPORT_H__
#define GETFEM_IMPORT_H__

#include <string>
#include <iostream>

namespace getfem {
  class mesh;

  /** imports a mesh file.
      format can be:
      - "gid" for meshes generated by GiD http://gid.cimne.upc.es/ 
         -- mesh nodes are always 3D
      - "gmsh" for meshes generated by GMSH http://www.geuz.org/gmsh/ 
       IMPORTANT NOTE: if you do not assign a physical surface/volume to your
       3D mesh, the file will also contain the mesh of the
       boundary (2D elements) and the boundary of the boundary (line
       elements!).
       getfem makes use of the physical "region" number stored with each element in the gmsh file
       to fill the corresponding region of the mesh object.

       For a mesh of dimension N, getfem builds a mesh with the
       convexes listed in the gmsh file whose dimension are N, the
       convexes of dim N-1 are used to tag "region" of faces,
       according to their gmsh "physical region number", and the
       convexes of lower dimension are ignored.

       Note that the mesh nodes are always 3D.

      - "am_fmt" for 2D meshes from emc2
        [http://pauillac.inria.fr/cdrom/prog/unix/emc2/eng.htm]
  */
  void import_mesh(const std::string& filename, const std::string& format,
		   mesh& m);
  void import_mesh(std::ifstream& f, const std::string& format,
		   mesh& m);
  void import_mesh(const std::string& filename, mesh& m);

  /** for gmsh and gid meshes, the mesh nodes are always 3D, so for a 2D mesh
      the z-component of nodes should be removed */
  void maybe_remove_last_dimension(mesh &msh);
}
#endif /* GETFEM_IMPORT_H__  */
