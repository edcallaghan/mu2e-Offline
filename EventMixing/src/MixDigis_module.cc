// Mix digis
//
// Ed Callaghan, 2023

// stl

// art
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Framework/IO/ProductMix/MixHelper.h"
#include "art/Framework/Modules/MixFilter.h"
#include "art_root_io/RootIOPolicy.h"
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/Comment.h"
#include "fhiclcpp/types/Name.h"
#include "fhiclcpp/types/Sequence.h"
#include "fhiclcpp/types/Table.h"
#include "fhiclcpp/types/TupleAs.h"

// mu2e
#include "Offline/EventMixing/inc/Mu2eProductMixer.hh"

namespace mu2e{
  // detail class for art::MixFilter<>
  class MixDigisDetail{
    public:
      struct Mu2eConfig{
        fhicl::Table<Mu2eProductMixer::Config> products{
          fhicl::Name("products"),
          fhicl::Comment("Products to be mixed, as form of a mixingMap, i.e. tuples of InputTags to output instance names.")

        };
        // TODO others?
      };

      struct Config{
        fhicl::Table<Mu2eConfig> mu2e{ fhicl::Name("mu2e") };
      };

      using Parameters = art::MixFilterTable<Config>;
      explicit MixDigisDetail(const Parameters& parameters,
                                  art::MixHelper& helper);
      size_t nSecondaries();

      void beginSubRun(const art::SubRun& subrun);
      void endSubRun(art::SubRun& subrun);
      void startEvent(const art::Event& event);
      void finalizeEvent(art::Event& event);

    protected:
      /**/

    private:
      Mu2eProductMixer mixer;
  };

  // implementation
  MixDigisDetail::MixDigisDetail(const Parameters& parameters,
                                         art::MixHelper& helper)
      : mixer{parameters().mu2e().products(), helper}{
    /**/
  }

  // always overlay one event window onto the primary
  size_t MixDigisDetail::nSecondaries(){
    size_t rv = 1;
    return rv;
  }

  void MixDigisDetail::beginSubRun(const art::SubRun& subrun){
    mixer.beginSubRun(subrun);
  }

  void MixDigisDetail::endSubRun(art::SubRun& subrun){
    mixer.endSubRun(subrun);
  }

  void MixDigisDetail::startEvent(const art::Event& event){
    mixer.startEvent(event);
  }

  void MixDigisDetail::finalizeEvent(art::Event& event){
    /*
    mixer.finalizeEvent(event);
    */
  }

  // define the module class
  typedef art::MixFilter<MixDigisDetail,art::RootIOPolicy> MixDigis;
}

DEFINE_ART_MODULE(mu2e::MixDigis);
