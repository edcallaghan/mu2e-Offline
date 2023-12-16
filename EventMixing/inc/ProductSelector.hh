// Select data products according to some criteria
//
// Ed Callaghan, 2023

#ifndef EventMixing_ProductSelector_hh
#define EventMixing_ProductSelector_hh

// stl
#include <ostream>

// cet
#include <cetlib_except/exception.h>

namespace mu2e{
  template<typename T>
  class ProductSelector{
    public:
      ProductSelector();
     ~ProductSelector();

      virtual bool Select(const T& product);
    protected:
      /**/
    private:
      /**/
  };

  template<typename T>
  ProductSelector<T>::ProductSelector(){
    /**/
  }

  template<typename T>
  ProductSelector<T>::~ProductSelector(){
    /**/
  }

  template<typename T>
  bool ProductSelector<T>::Select(const T& product){
    // throw an exception if non-specialized type is hashed
    throw cet::exception("UNSUPPORTED") << "request to select unsupported type" << std::endl;

    // should never reach here
    bool rv = false;
    return rv;
  }
}

#endif
