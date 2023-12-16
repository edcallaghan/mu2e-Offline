// Merge data products of various provenances, according to some collision rule
//
// Ed Callaghan, 2023

#include <Offline/EventMixing/inc/ProductHash.hh>

namespace mu2e{
  template<>
  ProductHash_t ProductHash<StrawDigi>::Hash(const StrawDigi& product){
    StrawId sid = product.strawId();
    uint16_t raw = sid.asUint16();
    size_t casted = static_cast<size_t>(raw);
    ProductHash_t rv = casted;
    return rv;
  }
}
