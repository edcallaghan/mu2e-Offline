// Select all data products
//
// Ed Callaghan, 2023

#ifndef EventMixing_AllProductSelector_hh
#define EventMixing_AllProductSelector_hh

#include <Offline/EventMixing/inc/ProductSelector.hh>

namespace mu2e{
  template<typename T>
  class AllProductSelector:public ProductSelector<T>{
    public:
      virtual bool Select(const T& product) override;
    protected:
      /**/
    private:
      /**/
  };

  template<typename T>
  bool AllProductSelector<T>::Select(const T& product){
    bool rv = true;
    return rv;
  }
}

#endif
