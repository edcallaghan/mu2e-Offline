#ifndef __TrkPatRec_StraightTrackFinder_types_hh__
#define __TrkPatRec_StraightTrackFinder_types_hh__

#include "TObject.h"
#include <vector>

namespace art {
  class Event;
};

namespace fhicl {
  class ParameterSet;
};

namespace mu2e {

  class StraightTrackFit;

  namespace StraightTrackFinderTypes {
  
    struct Data_t {
      const art::Event*               event;
      fhicl::ParameterSet*            timeOffsets;
      enum {kMaxSeeds = 100, kMaxNHits = 500 };
      int     nseeds   [2]; // 0:all, 1:nhits > nhitsMin; assume nseeds <= 100
      int     nTimePeaks;               // number of time peaks (input)
      
      int     nChPPanel[kMaxSeeds];    // maximum number of combohits per panel found in the TimeCluster
      int     nChHits[kMaxSeeds];    // maximum number of combohits per panel found in the TimeCluster
      int     ntclhits [kMaxSeeds]; //number of time cluster hits for each seed
      int     nhits    [kMaxSeeds]; //number of hits for each seed
      double  chi2XY   [kMaxSeeds];
      double  chi2d_track[kMaxSeeds]; //chi-squared of fit for each tracl seed
      int     xyniter  [kMaxSeeds];
      int     niter    [kMaxSeeds];
      int     nShFitXY [kMaxSeeds];
      int     nChFitXY [kMaxSeeds];
      int     good     [kMaxSeeds];
      int     nXYSh    [kMaxSeeds];
      int     nXYCh    [kMaxSeeds];
      //for chi2 fitting:
      int     nshsxy_0    [kMaxSeeds];
      double  rsxy_0      [kMaxSeeds];
      double  chi2dsxy_0  [kMaxSeeds];
                  
      int     nshsxy_1    [kMaxSeeds];
      double  rsxy_1      [kMaxSeeds];
      double  chi2dsxy_1  [kMaxSeeds];
      //TODO: may need to edit this:

      int maxSeeds() { return kMaxSeeds; }
    };
   
  }
}
#endif
