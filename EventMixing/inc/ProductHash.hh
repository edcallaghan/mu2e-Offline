// Merge data products of various provenances, according to some collision rule
//
// Ed Callaghan, 2023

#ifndef EventMixing_ProductHash_hh
#define EventMixing_ProductHash_hh

// stl
#include <ostream>

// cet
#include <cetlib_except/exception.h>

// mu2e
#include <Offline/DataProducts/inc/StrawId.hh>
#include <Offline/RecoDataProducts/inc/StrawDigi.hh>

namespace mu2e{
  typedef size_t ProductHash_t;

  template<typename T>
  class ProductHash{
    public:
      ProductHash();
     ~ProductHash();

      ProductHash_t Hash(const T& product);
    protected:
      /**/
    private:
      /**/
  };

  template<typename T>
  ProductHash<T>::ProductHash(){
    /**/
  }

  template<typename T>
  ProductHash<T>::~ProductHash(){
    /**/
  }

  template<typename T>
  ProductHash_t ProductHash<T>::Hash(const T& product){
    // throw an exception if non-specialized type is hashed
    throw cet::exception("UNSUPPORTED") << "request to hash unsupported type" << std::endl;

    // should never reach here
    ProductHash_t rv = 0;
    return rv;
  }

  template<>
  ProductHash_t ProductHash<StrawDigi>::Hash(const StrawDigi& product);
}

#endif
