// -*- c++ -*- (enables emacs c++ mode)
//========================================================================
//
// Library : Dynamic Array Library (dal)
// File    : dal_static_stored_objects.cc : object which should be stored.
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


#include <dal_static_stored_objects.h>
#include <dal_singleton.h>
#include <map>
#include <list>
#include <set>

namespace dal {

  // Pointer to an object with the dependencies
  struct enr_static_stored_object {
    pstatic_stored_object p;
    bool valid;
    permanence perm;
    std::set<pstatic_stored_object> dependent_object;
    std::set<pstatic_stored_object> dependencies;
    enr_static_stored_object(pstatic_stored_object o, permanence perma)
      : p(o), valid(true), perm(perma) {}
    enr_static_stored_object(void)
      : p(0), valid(true), perm(STANDARD_STATIC_OBJECT) {}
  };
  
  // Pointer to a key with a coherent order
  struct enr_static_stored_object_key {
    pstatic_stored_object_key p;
    bool operator < (const enr_static_stored_object_key &o) const
    { return (*p) < (*(o.p)); }
    enr_static_stored_object_key(pstatic_stored_object_key o) : p(o) {}
  };

  // Storing array types
  typedef std::map<enr_static_stored_object_key, enr_static_stored_object>
  stored_object_tab;
  struct stored_key_tab : public std::map<pstatic_stored_object,
					  pstatic_stored_object_key> {
    ~stored_key_tab() {
      for (iterator it = begin(); it != end(); ++it) {
	/*cerr << "~stored_key_tab: it->first = " << it->first << " of type "
	  << typeid(*(it->first)).name() << " . Delete key@" << it->second << endl;*/
	delete it->second;
      }
    }
  };
  
  // Gives a pointer to a key of an object from its pointer
  pstatic_stored_object_key key_of_stored_object(pstatic_stored_object o) {
    stored_key_tab& stored_keys = dal::singleton<stored_key_tab>::instance();
    stored_key_tab::iterator it = stored_keys.find(o);
    if (it != stored_keys.end()) return it->second;
    return 0;
  }

  // Test if an object is stored.
  bool exists_stored_object(pstatic_stored_object o) {
    stored_key_tab& stored_keys = dal::singleton<stored_key_tab>::instance();
    return (stored_keys.find(o) != stored_keys.end());
  }

  // Gives a pointer to an object from a key pointer
  pstatic_stored_object search_stored_object(pstatic_stored_object_key k) {
    stored_object_tab& stored_objects
      = dal::singleton<stored_object_tab>::instance();
    stored_object_tab::iterator it
      = stored_objects.find(enr_static_stored_object_key(k));
    if (it != stored_objects.end()) return it->second.p;
    return 0;
  }

  // Gives an iterator on stored object from a pointer object
  static inline stored_object_tab::iterator 
  iterator_of_object(pstatic_stored_object o) {
    stored_object_tab& stored_objects
      = dal::singleton<stored_object_tab>::instance();
    pstatic_stored_object_key k = key_of_stored_object(o);
    if (k) {
      stored_object_tab::iterator it
	= stored_objects.find(enr_static_stored_object_key(k));
      if (it == stored_objects.end())
	DAL_THROW(internal_error, "Object has key but cannot be found");
      return it;
    }
    return stored_objects.end();
  }

  // Test the validity of arrays
  void test_stored_objects(void) {
    stored_key_tab& stored_keys = dal::singleton<stored_key_tab>::instance();
    for (stored_key_tab::iterator it = stored_keys.begin();
	 it != stored_keys.end(); ++it)
      iterator_of_object(it->first);
    stored_object_tab& stored_objects
      = dal::singleton<stored_object_tab>::instance();
    for (stored_object_tab::iterator it = stored_objects.begin();
	 it != stored_objects.end(); ++it)
      if (iterator_of_object(it->second.p) == stored_objects.end())
	DAL_THROW(internal_error, "Object has key but cannot be found");
  }

  // Add a dependency, object o1 will depend on object o2
  void add_dependency(pstatic_stored_object o1, pstatic_stored_object o2) {
    stored_object_tab& stored_objects
      = dal::singleton<stored_object_tab>::instance();
    stored_object_tab::iterator it1 = iterator_of_object(o1);
    stored_object_tab::iterator it2 = iterator_of_object(o2);
    if (it1 != stored_objects.end() && it2 != stored_objects.end()) {
      it2->second.dependent_object.insert(o1);
      it1->second.dependencies.insert(o2);
    }
    else {
      cerr << "Problem adding dependency between " << o1 << " of type "
	   << typeid(*o1).name() << " and " << o2 << " of type "
	   << typeid(*o2).name() << ". ";
      if (it1 == stored_objects.end()) cerr << "First object does non exist.";
      if (it2 == stored_objects.end()) cerr << "Second object does non exist.";
      cerr << endl;
      DAL_THROW(failure_error, "Add_dependency : Inexistent object");
    }
  }

  // remove a dependency. Return true if o2 has no more dependent object.
  bool del_dependency(pstatic_stored_object o1, pstatic_stored_object o2) {
    stored_object_tab& stored_objects
      = dal::singleton<stored_object_tab>::instance();
    stored_object_tab::iterator it1 = iterator_of_object(o1);
    stored_object_tab::iterator it2 = iterator_of_object(o2);
    if (it1 != stored_objects.end() && it2 != stored_objects.end()) {
      it2->second.dependent_object.erase(o1);
      it1->second.dependencies.erase(o2);
      return it2->second.dependent_object.empty();
    }
    return true;
  }

  // Add an object with two optional dependencies
  void add_stored_object(pstatic_stored_object_key k, pstatic_stored_object o,
			 permanence perm) {
    stored_object_tab& stored_objects
      = dal::singleton<stored_object_tab>::instance();
    stored_key_tab& stored_keys = dal::singleton<stored_key_tab>::instance();
    if (stored_keys.find(o) != stored_keys.end())
      DAL_THROW(failure_error, "This object has already been stored, "
		"possibly with another key");
    stored_keys[o] = k;
    stored_objects[enr_static_stored_object_key(k)]
      = enr_static_stored_object(o, perm);
    /*cerr << "add_stored_object " << o.get() << " of type "
      << typeid(*o).name() << endl;*/
  }

  // Only delete the object but not the dependencies
  static void basic_delete(std::list<pstatic_stored_object> &to_delete){
    stored_object_tab& stored_objects
      = dal::singleton<stored_object_tab>::instance();
    stored_key_tab& stored_keys = dal::singleton<stored_key_tab>::instance();
    std::list<pstatic_stored_object>::iterator it;
    for (it = to_delete.begin(); it != to_delete.end(); ++it) {
      // cout << "delete object " << (*it).get() << " of type "
      //      << typeid(*(*it)).name() << endl;
      pstatic_stored_object_key k = key_of_stored_object(*it);
      stored_object_tab::iterator ito = stored_objects.find(k);
      if (k) stored_keys.erase(*it);
      if (ito != stored_objects.end()) {
	delete ito->first.p;
	stored_objects.erase(ito);
      }
    }
  }
  
  // Delete a list of objects and their dependencies
  void del_stored_objects(std::list<pstatic_stored_object> &to_delete) {
    stored_object_tab& stored_objects
      = dal::singleton<stored_object_tab>::instance();
    std::list<pstatic_stored_object>::iterator it;
    for (it = to_delete.begin(); it != to_delete.end(); ++it) {
      stored_object_tab::iterator ito = iterator_of_object(*it);
      if (ito == stored_objects.end())
	DAL_THROW(failure_error, "This object is not stored : " << it->get());
      iterator_of_object(*it)->second.valid = false;
    }
    std::set<pstatic_stored_object>::iterator itd;
    for (it = to_delete.begin(); it != to_delete.end(); ++it) {
      if (*it) {
	stored_object_tab::iterator ito = iterator_of_object(*it);
	if (ito == stored_objects.end())
	  DAL_THROW(internal_error, "An object disapeared !");
	ito->second.valid = false;
	std::set<pstatic_stored_object> dep = ito->second.dependencies;
	for (itd = dep.begin(); itd != dep.end(); ++itd) {
	  if (del_dependency(*it, *itd)) {
	    stored_object_tab::iterator itod=iterator_of_object(*itd);
	    if (itod->second.perm == AUTODELETE_STATIC_OBJECT
		&& itod->second.valid) {
	      itod->second.valid = false;
	      to_delete.push_back(*itd);
	    }
	  }
	}
	for (itd = ito->second.dependent_object.begin();
	     itd != ito->second.dependent_object.end(); ++itd) {
	  stored_object_tab::iterator itod=iterator_of_object(*itd);
	  if (itod != stored_objects.end()) {
	    if (itod->second.perm == PERMANENT_STATIC_OBJECT)
	      DAL_THROW(failure_error,"Trying to delete a permanent object");
	    if (itod->second.valid) {
	      itod->second.valid = false;
	      to_delete.push_back(itod->second.p);
	    }
	  }
	}
      }
    }
    basic_delete(to_delete);
  }

  // Delete an object and its dependencies
  void del_stored_object(pstatic_stored_object o) {
    std::list<pstatic_stored_object> to_delete;
    to_delete.push_back(o);
    del_stored_objects(to_delete);
  }
  
  // Delete all the object whose perm is greater or equal to perm
  void del_stored_objects(permanence perm) {
    stored_object_tab& stored_objects
      = dal::singleton<stored_object_tab>::instance();
    if (perm == PERMANENT_STATIC_OBJECT) perm = STRONG_STATIC_OBJECT;
    std::list<pstatic_stored_object> to_delete;
    stored_object_tab::iterator it;
    for (it = stored_objects.begin(); it != stored_objects.end(); ++it)
      if (it->second.perm >= perm)
	to_delete.push_back(it->second.p);
    del_stored_objects(to_delete);
  }


}
