// -*- c++ -*- (enables emacs c++ mode)
//========================================================================
//
// Library : Dynamic Array Library (dal)
// File    : dal_static_stored_objects.h : object which should be stored.
//           
// Date    : February 19, 2005
// Authors : Yves Renard <Yves.Renard@insa-toulouse.fr>
//
//========================================================================
//
// Copyright (C) 2002-2005 Yves Renard
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


#ifndef DAL_STATIC_STORED_OBJECTS_H__
#define DAL_STATIC_STORED_OBJECTS_H__

#include <dal_except.h>
#include <getfem_boost/intrusive_ptr.hpp>

namespace dal {


  // Stored object :  
  // A type of object to be stored should derive from static_stored_object
  // and a key should derive from  static_stored_object_key with an overloaded
  // method compare.
  // 
  // To store a new object, you have to test if the object is not
  // already stored and then to call add_stored_object:
  // 
  // if (!search_stored_object(your_object_key(parameters))) {
  //   add_stored_object(new your_object_key(parameters),
  //                     new your_object(parameters));
  // }
  // You can add a dependency of your new object with
  // add_dependency(pointer_on_your_object,
  //                pointer_on_the_object_object_from_which_it_depends,
  //                permanence)
  // your object will be automatically deleted if the second object is
  // deleted.
  // The dependency can be added when the addition is done by
  // add_stored_object(new your_object_key(parameters), 
  //                   new your_object(parameters),
  //                   dependence)
  // Boost intrusive pointer are used

  enum permanence { PERMANENT_STATIC_OBJECT = 0, // not deletable object
		    STRONG_STATIC_OBJECT = 1,    // preferable not to delete it
		    STANDARD_STATIC_OBJECT = 2,  // standard
		    WEAK_STATIC_OBJECT = 3,      // delete it if necessary
		    AUTODELETE_STATIC_OBJECT = 4 // automatically deleted 
		             // when the last dependent object is deleted
  };


  class static_stored_object_key {
  protected :
    virtual bool compare(const static_stored_object_key &) const {
      DAL_THROW(failure_error, "This method should not be called");
    }

  public :
    bool operator < (const static_stored_object_key &o) const {
      // comparaison des noms d'objet
      if (typeid(*this).before(typeid(o))) return true;
      if (typeid(o).before(typeid(*this))) return false;
      return compare(o);
    }
    
    
    virtual ~static_stored_object_key() {}
    
  };

  typedef const static_stored_object_key *pstatic_stored_object_key;
  
  class static_stored_object {
    mutable long pointer_ref_count_;
    
    
  public :
    static_stored_object(void) : pointer_ref_count_(0) {}
    virtual ~static_stored_object() {}
    friend void intrusive_ptr_add_ref(const static_stored_object *o);
    friend void intrusive_ptr_release(const static_stored_object *o);
  };

  typedef boost::intrusive_ptr<const static_stored_object>
  pstatic_stored_object;

  template<class T> boost::intrusive_ptr<const T>
  stored_cast(pstatic_stored_object o) {
    return boost::intrusive_ptr<const T>(dynamic_cast<const T *>(o.get()));
  }

  inline void intrusive_ptr_add_ref(const static_stored_object *o)
  { o->pointer_ref_count_++; }

  inline void intrusive_ptr_release(const static_stored_object *o)
  { if (--(o->pointer_ref_count_) == 0) delete o; }


  /** Gives a pointer to an object from a key pointer. */
  pstatic_stored_object search_stored_object(pstatic_stored_object_key k);

  /** Gives a pointer to an object from a key reference. */
  inline pstatic_stored_object
  search_stored_object(const static_stored_object_key &k)
  { return search_stored_object(&k); }

  /** Test if an object is stored. */
  bool exists_stored_object(pstatic_stored_object o);

  /** Add a dependency, object o1 will depend on object o2. */
  void add_dependency(pstatic_stored_object o1, pstatic_stored_object o2);

  /** remove a dependency. Return true if o2 has no more dependent object. */
  bool del_dependency(pstatic_stored_object o1, pstatic_stored_object o2);

  /** Add an object with two optional dependencies. */
  void add_stored_object(pstatic_stored_object_key k, pstatic_stored_object o,
			 permanence perm = STANDARD_STATIC_OBJECT);

  inline void
  add_stored_object(pstatic_stored_object_key k, pstatic_stored_object o,
		    pstatic_stored_object dep1,
		    permanence perm = STANDARD_STATIC_OBJECT) {
    add_stored_object(k, o, perm);
    add_dependency(o, dep1);
  }
			 
  inline void
  add_stored_object(pstatic_stored_object_key k, pstatic_stored_object o,
		    pstatic_stored_object dep1, pstatic_stored_object dep2, 
		    permanence perm = STANDARD_STATIC_OBJECT) {
    add_stored_object(k, o, perm);
    add_dependency(o, dep1);
    add_dependency(o, dep2);
  }
  
  inline void
  add_stored_object(pstatic_stored_object_key k, pstatic_stored_object o,
		    pstatic_stored_object dep1, pstatic_stored_object dep2,
		    pstatic_stored_object dep3,
		    permanence perm = STANDARD_STATIC_OBJECT) {
    add_stored_object(k, o, perm);
    add_dependency(o, dep1);
    add_dependency(o, dep2);
    add_dependency(o, dep3);
  }
  
  inline void
  add_stored_object(pstatic_stored_object_key k, pstatic_stored_object o,
		    pstatic_stored_object dep1, pstatic_stored_object dep2,
		    pstatic_stored_object dep3, pstatic_stored_object dep4,
		    permanence perm = STANDARD_STATIC_OBJECT) {
    add_stored_object(k, o, perm);
    add_dependency(o, dep1);
    add_dependency(o, dep2);
    add_dependency(o, dep3);
    add_dependency(o, dep4);
  }

  /** Delete an object and the object which depend on it. */
  void del_stored_object(pstatic_stored_object o);
  
  /** Delete all the object whose permanence is greater or equal to perm. */
  void del_stored_objects(int perm);

  /** Gives a pointer to a key of an object from its pointer. */
  pstatic_stored_object_key key_of_stored_object(pstatic_stored_object o);

}

#endif /* DAL_STATIC_STORED_OBJECTS_H__ */
