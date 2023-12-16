// Merge data products of various provenances, according to some collision rule
//
// Ed Callaghan, 2023

#ifndef EventMixing_NoCollisionResolutionPolicy_hh
#define EventMixing_NoCollisionResolutionPolicy_hh

#include <Offline/EventMixing/inc/ProductCollisionResolutionPolicy.hh>

namespace mu2e{
  template<typename T>
  class NoCollisionResolutionPolicy:public ProductCollisionResolutionPolicy<T>{
    public:
      virtual T Resolve(const T& collision) override;
    protected:
      /**/
    private:
      /**/
  };

  template<typename T>
  T NoCollisionResolutionPolicy<T>::Resolve(const T& collision){
    T rv = collision;
    return rv;
  }
}

#endif
