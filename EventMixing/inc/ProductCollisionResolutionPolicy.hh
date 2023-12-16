// Interface for rules to choose which elements of a collection to propagate to a merge
//
// Ed Callaghan, 2023

#ifndef EventMixing_ProductCollisionResolutionPolicy_hh
#define EventMixing_ProductCollisionResolutionPolicy_hh

// stl
#include <ostream>

// cet
#include <cetlib_except/exception.h>

namespace mu2e{
  template<typename T>
  class ProductCollisionResolutionPolicy{
    public:
      ProductCollisionResolutionPolicy();
     ~ProductCollisionResolutionPolicy();

      virtual T Resolve(const T& collision);
    protected:
      /**/
    private:
      /**/
  };

  template<typename T>
  ProductCollisionResolutionPolicy<T>::ProductCollisionResolutionPolicy(){
    /**/
  }

  template<typename T>
  ProductCollisionResolutionPolicy<T>::~ProductCollisionResolutionPolicy(){
    /**/
  }

  template<typename T>
  T ProductCollisionResolutionPolicy<T>::Resolve(const T& collision){
    // throw an exception if non-specialized type is resolved
    throw cet::exception("UNSUPPORTED") << "request to resolve collision of unsupported type" << std::endl;

    // should never reach here
    T rv;
    return rv;
  }
}

#endif
