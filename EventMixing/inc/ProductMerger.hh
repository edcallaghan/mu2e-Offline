// Merge data products of various provenances, according to some collision rule
//
// Ed Callaghan, 2023

#ifndef EventMixing_ProductMerger_hh
#define EventMixing_ProductMerger_hh

// stl
#include <map>
#include <vector>

// art
#include <art/Persistency/Common/CollectionUtilities.h>

// mu2e
#include <Offline/EventMixing/inc/ProductSelector.hh>
#include <Offline/EventMixing/inc/ProductHash.hh>
#include <Offline/EventMixing/inc/ProductCollisionResolutionPolicy.hh>

namespace mu2e{
  template<typename T, typename TCollection>
  class ProductMerger{
    public:
      ProductMerger();
      ProductMerger(const std::shared_ptr< ProductSelector<T> > selector,
                    const std::shared_ptr< ProductHash<T> > hasher,
                    const std::shared_ptr<
                             ProductCollisionResolutionPolicy<TCollection>
                          > policy);
     ~ProductMerger();

      TCollection Merge(std::vector<TCollection> inputs);
    protected:
      /**/
    private:
      std::shared_ptr< ProductSelector<T> > selector;
      std::shared_ptr< ProductHash<T> > hasher;
      std::shared_ptr< ProductCollisionResolutionPolicy<TCollection> > policy;
  };

  template <typename T, typename TCollection>
  ProductMerger<T,TCollection>::ProductMerger(){
    /**/
  }

  template <typename T, typename TCollection>
  ProductMerger<T,TCollection>::ProductMerger(
                        const std::shared_ptr< ProductSelector<T> > selector,
                        const std::shared_ptr< ProductHash<T> > hasher,
                        const std::shared_ptr<
                                 ProductCollisionResolutionPolicy<TCollection>
                              > policy)
    : selector(selector)
    , hasher(hasher)
    , policy(policy)
  {
    /**/
  }

  template <typename T, typename TCollection>
  ProductMerger<T,TCollection>::~ProductMerger(){
    /**/
  }

  template <typename T, typename TCollection>
  TCollection ProductMerger<T,TCollection>::Merge(std::vector<TCollection> inputs){
    // first, identify any merging collisions
    // this is accomplished by ``hashing'' to underlying products
    // into a map, and looking for overloaded baskets
    std::map< ProductHash_t, TCollection > collisions;
    for (const auto& products: inputs){
      for (const auto& product: products){
        if (!this->selector->Select(product)){
            continue;
        }
        ProductHash_t hash = this->hasher->Hash(product);
        collisions[hash].push_back(product);
      }
    }

    // resolve collisions according to prescribed policy
    std::vector<const TCollection*> staged;
    for (const auto& [key, collision]: collisions){
      TCollection resolved = this->policy->Resolve(collision);
      staged.push_back(&resolved);
    }

    // flatten resolved collections and return
    TCollection rv;
    art::flattenCollections(staged, rv);
    return rv;
  }
}

#endif
