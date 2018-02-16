//
// TTracker Pattern Recognition based on Robust Helix Fit
//
// $Id: RobustHelixFinder_module.cc,v 1.2 2014/08/30 12:19:38 tassiell Exp $
// $Author: tassiell $
// $Date: 2014/08/30 12:19:38 $
//
// Original author D. Brown and G. Tassielli
//

#include "art/Framework/Principal/Event.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "GeneralUtilities/inc/Angles.hh"
#include "Mu2eUtilities/inc/MVATools.hh"

#include "RecoDataProducts/inc/StrawHitCollection.hh"
#include "RecoDataProducts/inc/StrawHitPositionCollection.hh"
#include "RecoDataProducts/inc/StrawHitFlagCollection.hh"
#include "RecoDataProducts/inc/StereoHit.hh"
#include "RecoDataProducts/inc/TimeCluster.hh"
#include "RecoDataProducts/inc/HelixSeed.hh"
#include "RecoDataProducts/inc/TrkFitFlag.hh"

#include "TrkReco/inc/TrkTimeCalculator.hh"
#include "GeometryService/inc/getTrackerOrThrow.hh"
#include "TTrackerGeom/inc/TTracker.hh"
#include "BTrk/BaBar/BaBar.hh"
#include "TrkReco/inc/TrkDef.hh"
#include "BTrkData/inc/TrkStrawHit.hh"
#include "TrkReco/inc/RobustHelixFit.hh"

#include "CLHEP/Units/PhysicalConstants.h"
#include "CLHEP/Matrix/Vector.h"
#include "CLHEP/Matrix/SymMatrix.h"

#include <boost/accumulators/accumulators.hpp>
#include "boost_fix/accumulators/statistics/stats.hpp"
#include "boost_fix/accumulators/statistics.hpp"
#include <boost/accumulators/statistics/median.hpp>

#include "TH1F.h"
#include "Math/VectorUtil.h"

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <utility>
#include <functional>
#include <float.h>
#include <vector>
#include <map>

using namespace std;
using namespace boost::accumulators;
using namespace ROOT::Math::VectorUtil;

namespace {

  struct HelixHitMVA
  {
    std::vector <Double_t> _pars,_pars2;
    Double_t& _dtrans; // distance from hit to helix perp to the wrire
    Double_t& _dwire;  // distance from hit to helix along the wrire
    Double_t& _chisq;  // chisq of spatial information, using average errors
    Double_t& _dt;     // time difference of hit WRT average
    Double_t& _drho;   // hit transverse radius minus helix radius
    Double_t& _dphi;   // hit azimuth minus helix azimuth (at the hit z)
    Double_t& _rwdot;  // dot product between circle radial direction and wire direction
    Double_t& _hrho;   // helix transverse radius (at the hit z)
    Double_t& _hhrho;  // hit transverse radius
    //HelixHitMVA() : _pars(9,0.0),_dtrans(_pars[0]),_dwire(_pars[1]),_chisq(_pars[2]),_dt(_pars[3]),
    // _drho(_pars[4]),_dphi(_pars[5]),_rwdot(_pars[6]),_hrho(_pars[7]),_hhrho(_pars[8]){}
    HelixHitMVA() : _pars(7,0.0),_pars2(2,0.0),_dtrans(_pars[0]),_dwire(_pars[1]),_chisq(_pars[2]),_dt(_pars[3]),
     _drho(_pars[4]),_dphi(_pars[5]),_rwdot(_pars[6]),_hrho(_pars[0]),_hhrho(_pars2[1]) {}
  };

}

namespace mu2e {

  class RobustHelixFinder : public art::EDProducer {
  public:
    explicit RobustHelixFinder(fhicl::ParameterSet const&);
    virtual ~RobustHelixFinder();
    virtual void beginJob();
    virtual void produce(art::Event& event );

  private:
    int                                 _diag,_debug;
    int                                 _printfreq;
    bool				_prefilter; // prefilter hits based on sector
    bool				_updatestereo; // update the stereo hit positions each iteration
    unsigned				_minnhit; // minimum # of hits to work with
    float				_maxdr; // maximum hit-helix radius difference
    float				_maxrpull; // maximum hit-helix radius difference pull
    float				_maxphisep; // maximum separation in global azimuth of hits
    TrkFitFlag				_saveflag; // write out all helices that satisfy these flags
    unsigned				_maxniter;  // maximum # of iterations over outlier filtering + fitting
    float				_cradres; // average center resolution along center position (mm)
    float				_cperpres; // average center resolution perp to center position (mm)
    float				_maxdwire; // outlier cut on distance between hit and helix along wire
    float				_maxdtrans; // outlier cut on distance between hit and helix perp to wire
    float				_maxchisq; // outlier cut on chisquared
    float				_maxrwdot[2]; // outlier cut on angle between radial direction and wire: smaller is better
    float				_minrerr; // minimum radius error

    bool				_usemva; // use MVA to cut outliers
    float				_minmva; // outlier cut on MVA

    art::InputTag			_ccTag;
    art::InputTag			_chTag;
    art::InputTag			_tcTag;

    StrawHitFlag  _hsel, _hbkg;

    MVATools _stmva, _nsmva;
    HelixHitMVA _vmva; // input variables to TMVA for filtering hits

    TH1F* _niter, *_nitermva;

    RobustHelixFit   _hfit;
    std::vector<Helicity> _hels; // helicity values to fit 
    TrkTimeCalculator _ttcalc;
    float            _t0shift;   
    StrawHitFlag      _outlier;
    bool              _updateStereo;
    
    
    void     findHelices(ComboHitCollection& chcol, const TimeClusterCollection& tccol);    
    void     prefilterHits(HelixSeed& hseed); 
    unsigned filterCircleHits(HelixSeed& hseed); 
    bool     filterHits(HelixSeed& hseed);
    void     fillMVA(HelixSeed& hseed); 
    bool     filterHitsMVA(HelixSeed& hseed);
    void     updateT0(HelixSeed& hseed);
    bool     updateStereo(HelixSeed& hseed);
    unsigned hitCount(HelixSeed const& hseed);
    void     fitHelix(HelixSeed& hseed);
    void     refitHelix(HelixSeed& hseed);

  };

  RobustHelixFinder::RobustHelixFinder(fhicl::ParameterSet const& pset) :
    _diag        (pset.get<int>("diagLevel",0)),
    _debug       (pset.get<int>("debugLevel",0)),
    _printfreq   (pset.get<int>("printFrequency",101)),
    _prefilter   (pset.get<bool>("PrefilterHits",true)),
    _updatestereo(pset.get<bool>("UpdateStereoHits",false)),
    _minnhit	 (pset.get<unsigned>("minNHit",5)),
    _maxdr	 (pset.get<float>("MaxRadiusDiff",100.0)), // mm
    _maxrpull	 (pset.get<float>("MaxRPull",5.0)), // unitless
    _maxphisep	 (pset.get<float>("MaxPhiHitSeparation",1.0)),
    _saveflag    (pset.get<vector<string> >("SaveHelixFlag",vector<string>{"HelixOK"})),
    _maxniter    (pset.get<unsigned>("MaxIterations",10)), // iterations over outlier removal
    _cradres	 (pset.get<float>("CenterRadialResolution",20.0)),
    _cperpres	 (pset.get<float>("CenterPerpResolution",12.0)),
    _maxdwire    (pset.get<float>("MaxWireDistance",200.0)), // max distance along wire
    _maxdtrans   (pset.get<float>("MaxTransDistance",80.0)), // max distance perp to wire (and z)
    _maxchisq    (pset.get<float>("MaxChisquared",100.0)), // max chisquared
    _minrerr     (pset.get<float>("MinRadiusErr",20.0)), // mm
    _usemva      (pset.get<bool>("UseHitMVA",false)),
    _minmva      (pset.get<float> ("MinMVA",0.1)), // min MVA output to define an outlier
    _ccTag	 (pset.get<art::InputTag>("CaloClusterCollection","CaloClusterFast")),
    _chTag	 (pset.get<art::InputTag>("ComboHitCollection")),
    _tcTag	 (pset.get<art::InputTag>("TimeClusterCollection")),
    _hsel        (pset.get<std::vector<std::string> >("HitSelectionBits",std::vector<string>{"TimeDivision"})),
    _hbkg        (pset.get<std::vector<std::string> >("HitBackgroundBits",std::vector<std::string>{"Background"})),
    _stmva       (pset.get<fhicl::ParameterSet>("HelixStereoHitMVA",fhicl::ParameterSet())),
    _nsmva       (pset.get<fhicl::ParameterSet>("HelixNonStereoHitMVA",fhicl::ParameterSet())),
    _hfit        (pset.get<fhicl::ParameterSet>("RobustHelixFit",fhicl::ParameterSet())),
    _ttcalc      (pset.get<fhicl::ParameterSet>("T0Calculator",fhicl::ParameterSet())),
    _t0shift     (pset.get<float>("T0Shift",4.0)),
    _outlier     (StrawHitFlag::outlier),
    _updateStereo    (pset.get<bool>("UpdateStereo",true))
  {
    _maxrwdot[0] = pset.get<float>("MaxStereoRWDot",1.0);
    _maxrwdot[1] = pset.get<float>("MaxNonStereoRWDot",1.0);
    std::vector<int> helvals = pset.get<std::vector<int> >("Helicities",vector<int>{Helicity::neghel,Helicity::poshel});
    for(auto hv : helvals) {
      Helicity hel(hv);
      _hels.push_back(hel);
      produces<HelixSeedCollection>(Helicity::name(hel));
    }
  }

  RobustHelixFinder::~RobustHelixFinder(){}

  void RobustHelixFinder::beginJob() {
    _stmva.initMVA();
    _nsmva.initMVA();
    if (_debug > 0)
    {
      std::cout << "RobustHeilxFinder Stereo Hit MVA parameters: " << std::endl;
      _stmva.showMVA();
      std::cout << "RobustHeilxFinder Non-Stereo Hit MVA parameters: " << std::endl;
      _nsmva.showMVA();
    }

    if (_diag > 0){
      art::ServiceHandle<art::TFileService> tfs;
      _niter = tfs->make<TH1F>( "niter" , "Number of Fit Iteraions",201,-0.5,200.5);
      _nitermva = tfs->make<TH1F>( "nitermva" , "Number of MVA Fit Iteraions",201,-0.5,200.5);
    }
  }

  void RobustHelixFinder::produce(art::Event& event ) {
    // find input
    auto tcH = event.getValidHandle<TimeClusterCollection>(_tcTag);
    const TimeClusterCollection& tccol(*tcH);

    art::Handle<ComboHitCollection> chH;
    if(!event.getByLabel(_chTag, chH))
      throw cet::exception("RECO")<<"RobustHelixFinder: No ComboHit collection found for tag" <<  _chTag << endl;
    const ComboHitCollection& chcol(*chH);

    // create output: seperate by helicity
    std::map<Helicity,unique_ptr<HelixSeedCollection>> helcols;
    for( auto const& hel : _hels) 
      helcols[hel] = unique_ptr<HelixSeedCollection>(new HelixSeedCollection());

    // create initial helicies from time clusters: to begin, don't specificy helicity
    for (size_t index=0;index< tccol.size();++index) {
      const auto& tclust = tccol[index];
      HelixSeed hseed;
      hseed._hhits.setParent(chcol.parent());;
      hseed._t0 = tclust._t0;
      hseed._timeCluster = art::Ptr<TimeCluster>(tcH,index);
// copy combo hits
      for (const auto& ind : tclust._strawHitIdxs) {
	ComboHit const& ch = chcol[ind];
	if(ch.flag().hasAnyProperty(_hsel) && !ch.flag().hasAnyProperty(_hbkg)) {
	  ComboHit hhit(ch);
	  hhit._flag.clear(StrawHitFlag::resolvedphi);
	  hseed._hhits.push_back(hhit);        
	}
      }
      // filter hits and test
      if (_prefilter) prefilterHits(hseed);
      if (hitCount(hseed) >= _minnhit){
	hseed._status.merge(TrkFitFlag::hitsOK);
	// initial circle fit
	_hfit.fitCircle(hseed);
	if (hseed._status.hasAnyProperty(TrkFitFlag::circleOK)) {
	  // loop over helicities. 
	  for(auto const& hel : _hels ) {
	  // tentatively put a copy with the specified helicity in the appropriate output vector
	    hseed._helix._helicity = hel;
	    HelixSeedCollection* hcol = helcols[hel].get();
	    hcol->push_back(hseed);
	  // attempt complete fit 
	    fitHelix(hcol->back());
	    // test fit status; if not successful, pop it off
	    if (!hcol->back().status().hasAllProperties(_saveflag))
	      hcol->pop_back();
	  }
	}	
      }	
    }
    // put final collections into event 
    for(auto const& hel : _hels ) {
      event.put(std::move(helcols[hel]),Helicity::name(hel));
    }
  }

  void RobustHelixFinder::fillMVA(HelixSeed& hseed)
  {
    RobustHelix& helix = hseed._helix;

    static XYZVec zaxis(0.0,0.0,1.0); // unit in z direction
    for(auto& hhit : hseed._hhits)
    {

      const XYZVec& wdir = hhit.wdir();
      XYZVec wtdir = zaxis.Cross(wdir); // transverse direction to the wire
      XYZVec cvec = PerpVector(hhit.pos() - helix.center(),Geom::ZDir());// direction from the circle center to the hit
      XYZVec cdir = cvec.Unit();        // direction from the circle center to the hit
      XYZVec cperp = zaxis.Cross(cdir); // direction perp to the radius

      XYZVec hpos = hhit.pos();      // this sets the z position to the hit z
      helix.position(hpos);                     // this computes the helix expectation at that z
      XYZVec dh = hhit.pos() - hpos; // this is the vector between them

      _vmva._dtrans = fabs(dh.Dot(wtdir));              // transverse projection
      _vmva._dwire = fabs(dh.Dot(wdir));               // projection along wire direction
      _vmva._drho = fabs(sqrtf(cvec.mag2()) - helix.radius()); // radius difference
      _vmva._dphi = fabs(hhit.helixPhi() - helix.circleAzimuth(hhit.pos().z())); // azimuth difference WRT circle center
      _vmva._hhrho = sqrtf(cvec.mag2());            // hit transverse radius WRT circle center
      _vmva._hrho = sqrtf(hpos.Perp2());            // hit detector transverse radius
      _vmva._rwdot = fabs(wdir.Dot(cdir));  // compare directions of radius and wire

      // compute the total resolution including hit and helix parameters first along the wire
      float wres2 = std::pow(hhit.posRes(StrawHitPosition::wire),(int)2) +
	std::pow(_cradres*cdir.Dot(wdir),(int)2) +
	std::pow(_cperpres*cperp.Dot(wdir),(int)2);

      // transverse to the wires
      float wtres2 = std::pow(hhit.posRes(StrawHitPosition::trans),(int)2) +
	std::pow(_cradres*cdir.Dot(wtdir),(int)2) +
	std::pow(_cperpres*cperp.Dot(wtdir),(int)2);

      _vmva._chisq = sqrtf( _vmva._dwire*_vmva._dwire/wres2 + _vmva._dtrans*_vmva._dtrans/wtres2 );          
      _vmva._dt = hhit.time() - hseed._t0.t0();

      if (hhit._flag.hasAnyProperty(StrawHitFlag::stereo))
      {
	hhit._qual = _stmva.evalMVA(_vmva._pars);
      } else {
	hhit._qual = _nsmva.evalMVA(_vmva._pars);
      }
    }
  }

  bool RobustHelixFinder::filterHitsMVA(HelixSeed& hseed)
  {  
    bool changed(false);
    for (auto& hhit : hseed._hhits)
    {
      bool oldout = hhit._flag.hasAnyProperty(_outlier);

      if (hhit._qual < _minmva ) hhit._flag.merge(_outlier);
      else                        hhit._flag.clear(_outlier);

      changed |= oldout != hhit._flag.hasAnyProperty(_outlier);
    }
    return changed;
  }

  unsigned  RobustHelixFinder::filterCircleHits(HelixSeed& hseed)
  {
    unsigned changed(0);
    static XYZVec zaxis(0.0,0.0,1.0); // unit in z direction
    RobustHelix& helix = hseed._helix;

    for(auto& hhit : hseed._hhits)
    {
      bool oldout = hhit._flag.hasAnyProperty(_outlier);

      const XYZVec& wdir = hhit.wdir();
      XYZVec cvec = PerpVector(hhit.pos() - helix.center(),Geom::ZDir()); // direction from the circle center to the hit
      XYZVec cdir = cvec.Unit(); // direction from the circle center to the hit
      float rwdot = wdir.Dot(cdir); // compare directions of radius and wire
      float rwdot2 = rwdot*rwdot;
      // compute radial difference and pull
      float dr = sqrtf(cvec.mag2())-helix.radius();
      float werr = hhit.posRes(StrawHitPosition::wire);
      float terr = hhit.posRes(StrawHitPosition::trans);
      // the resolution is dominated the resolution along the wire
      float rres = std::max(sqrtf(werr*werr*rwdot2 + terr*terr*(1.0-rwdot2)),_minrerr);
      float rpull = dr/rres;
      unsigned ist = hhit._flag.hasAnyProperty(StrawHitFlag::stereo) ? 0 : 1;

      if ( std::abs(dr) > _maxdr || fabs(rpull) > _maxrpull || rwdot > _maxrwdot[ist])
      {
	hhit._flag.merge(_outlier);
      } else {
	hhit._flag.clear(_outlier);
      }
      if (oldout != hhit._flag.hasAnyProperty(_outlier)) ++changed;
    }
    return changed;
  }


  // 3d selection on top of radial selection
  bool RobustHelixFinder::filterHits(HelixSeed& hseed)
  {
    RobustHelix& helix = hseed._helix;
    ComboHitCollection& hhits = hseed._hhits;
    bool changed(false);
    static XYZVec zaxis(0.0,0.0,1.0); // unit in z direction

    // loop over hits
    for(auto& hhit : hhits)
    {     
      if (hhit._flag.hasAnyProperty(_outlier)) continue;

      float hphi = hhit.pos().phi();
      float dphi = fabs(Angles::deltaPhi(hphi,helix.fcent()));

      const XYZVec& wdir = hhit.wdir();
      XYZVec wtdir = zaxis.Cross(wdir);   // transverse direction to the wire
      XYZVec cvec = PerpVector(hhit.pos() - helix.center(),Geom::ZDir()); // direction from the circle center to the hit
      XYZVec cdir = cvec.Unit();          // direction from the circle center to the hit
      XYZVec cperp = zaxis.Cross(cdir);   // direction perp to the radius

      XYZVec hpos = hhit.pos(); // this sets the z position to the hit z
      helix.position(hpos);                // this computes the helix expectation at that z
      XYZVec dh = hhit.pos() - hpos;   // this is the vector between them
      float dtrans = fabs(dh.Dot(wtdir)); // transverse projection
      float dwire = fabs(dh.Dot(wdir));   // projection along wire direction

      // compute the total resolution including hit and helix parameters first along the wire
      float wres2 = std::pow(hhit.posRes(StrawHitPosition::wire),(int)2) +
	std::pow(_cradres*cdir.Dot(wdir),(int)2) +
	std::pow(_cperpres*cperp.Dot(wdir),(int)2);
      // transverse to the wires
      float wtres2 = std::pow(hhit.posRes(StrawHitPosition::trans),(int)2) +
	std::pow(_cradres*cdir.Dot(wtdir),(int)2) +
	std::pow(_cperpres*cperp.Dot(wtdir),(int)2);

      float chisq = dwire*dwire/wres2 + dtrans*dtrans/wtres2;

      if( dphi > _maxphisep || fabs(dwire) > _maxdwire || fabs(dtrans) > _maxdtrans || chisq > _maxchisq) 
      {
	hhit._flag.merge(_outlier);
	changed = true;
      }
    }
    return changed;
  }

  void RobustHelixFinder::prefilterHits(HelixSeed& hseed)
  {
    ComboHitCollection& hhits = hseed._hhits;

    bool changed(true);
    size_t nhit = hhits.size();
    while (changed && nhit > 0)
    {
      nhit = 0;
      changed = false;
      accumulator_set<float, stats<tag::median(with_p_square_quantile) > > accx;
      accumulator_set<float, stats<tag::median(with_p_square_quantile) > > accy;
      for (const auto& hhit : hhits ) 
      {
	if (hhit._flag.hasAnyProperty(_outlier)) continue;
	accx(hhit._pos.x());
	accy(hhit._pos.y());
	++nhit;
      }

      float mx = extract_result<tag::median>(accx);
      float my = extract_result<tag::median>(accy);
      float mphi = atan2f(my,mx);

      float maxdphi{0.0};
      auto worsthit = hhits.end();
      for(auto ihit = hhits.begin(); ihit != hhits.end(); ++ihit)
      {
	if (ihit->_flag.hasAnyProperty(_outlier)) continue;
	float phi  = ihit->pos().phi();
	float dphi = fabs(Angles::deltaPhi(phi,mphi));
	if(dphi > maxdphi)
	{
	  maxdphi = dphi;
	  worsthit = ihit;
	}
      }

      if (maxdphi > _maxphisep)
      {
	worsthit->_flag.merge(_outlier);
	changed = true;
      }
    }
  }

  void RobustHelixFinder::updateT0(HelixSeed& hseed)
  {
    const auto& hhits = hseed.hits();
    accumulator_set<float, stats<tag::weighted_variance(lazy)>, float > terr;
    for (const auto& hhit : hhits) 
    {
      if (hhit._flag.hasAnyProperty(_outlier)) continue;
      float wt = std::pow(1.0/_ttcalc.strawHitTimeErr(),2);
      terr(_ttcalc.comboHitTime(hhit),weight=wt);
    }

    if (hseed.caloCluster().isNonnull())
    {
      float time = _ttcalc.caloClusterTime(*hseed.caloCluster());
      float wt = std::pow(1.0/_ttcalc.caloClusterTimeErr(hseed.caloCluster()->diskId()),2);
      terr(time,weight=wt);
    }

    if (sum_of_weights(terr) > 0.0)
    {
      hseed._t0._t0 = extract_result<tag::weighted_mean>(terr) + _t0shift; // ad-hoc correction FIXME!!
      hseed._t0._t0err = sqrtf(std::max(float(0.0),extract_result<tag::weighted_variance(lazy)>(terr))/extract_result<tag::count>(terr));
    }
  }

  void RobustHelixFinder::fitHelix(HelixSeed& hseed){
    // iteratively fit the helix including filtering
    unsigned niter(0);
    unsigned nitermva(0);
    bool changed(true);

    do {
      unsigned xyniter(0);
      unsigned xychanged = filterCircleHits(hseed);
      while (hseed._status.hasAllProperties(TrkFitFlag::circleOK) && xyniter < _maxniter && xychanged) {
	_hfit.fitCircle(hseed);
	xychanged = filterCircleHits(hseed);
	++xyniter;
      } 
      // then fit phi-Z
      if (hseed._status.hasAnyProperty(TrkFitFlag::circleOK)) {
	if (xyniter < _maxniter)
	  hseed._status.merge(TrkFitFlag::circleConverged);
	else
	  hseed._status.clear(TrkFitFlag::circleConverged);

	// solve for the longitudinal parameters
	unsigned fzniter(0);
	bool fzchanged(false);
	do {
	  _hfit.fitFZ(hseed);
	  fzchanged = filterHits(hseed);
	  ++fzniter;
	} while (hseed._status.hasAllProperties(TrkFitFlag::phizOK)  && fzniter < _maxniter && fzchanged);

	if (hseed._status.hasAnyProperty(TrkFitFlag::phizOK)) {
	  if (fzniter < _maxniter)
	    hseed._status.merge(TrkFitFlag::phizConverged);
	  else
	    hseed._status.clear(TrkFitFlag::phizConverged);
	}
      }
      ++niter;

      // update the stereo hit positions; this checks how much the positions changed
      // do this only in non trigger mode

      if (_updateStereo && _hfit.goodHelix(hseed.helix())) changed = updateStereo(hseed);      
    } while (_hfit.goodHelix(hseed.helix()) && niter < _maxniter && changed);


    if (_hfit.goodHelix(hseed.helix())) {
      hseed._status.merge(TrkFitFlag::helixOK);
      updateT0(hseed);
      if (niter < _maxniter) hseed._status.merge(TrkFitFlag::helixConverged);

      if (_usemva) {
	bool changed = true;
	while (hseed._status.hasAllProperties(TrkFitFlag::helixOK)  && nitermva < _maxniter && changed) {
	  fillMVA(hseed);
	  changed = filterHitsMVA(hseed);
	  if (!changed) break;
	  refitHelix(hseed);
	  // update t0 each iteration as that's used in the MVA
	  updateT0(hseed);
	  ++nitermva;
	}
	if (nitermva < _maxniter)
	  hseed._status.merge(TrkFitFlag::helixConverged);
	else
	  hseed._status.clear(TrkFitFlag::helixConverged);
      }
    }
    if (_diag > 0){
      _niter->Fill(niter);
      _nitermva->Fill(nitermva);
      if (!_usemva) fillMVA(hseed);
    }
  }

  void RobustHelixFinder::refitHelix(HelixSeed& hseed) {
    // reset the fit status flags, in case this is called iteratively
    hseed._status.clear(TrkFitFlag::helixOK);      
    _hfit.fitCircle(hseed);
    if (hseed._status.hasAnyProperty(TrkFitFlag::circleOK)) {
      _hfit.fitFZ(hseed);
      if (_hfit.goodHelix(hseed._helix)) hseed._status.merge(TrkFitFlag::helixOK);
    }
  }

  unsigned RobustHelixFinder::hitCount(const HelixSeed& hseed) {
    return std::count_if(hseed._hhits.begin(),hseed._hhits.end(),
	[&](const ComboHit& hhit){return !hhit.flag().hasAnyProperty(_outlier);});
  }


  bool RobustHelixFinder::updateStereo(HelixSeed& hseed) {
    static StrawHitFlag stereo(StrawHitFlag::stereo);
    bool retval(false);
      // loop over the stereo hits in the helix and update their positions given the local helix direction
      for(auto& ch : hseed._hhits){
	if(ch.flag().hasAllProperties(stereo) && ch.nCombo() >=2) {
// local helix direction at the average z of this hit
	  XYZVec hdir;
	  hseed.helix().direction(ch.pos().z(),hdir);
	  
//	XYZVec pos1, pos2;
//	sthit.position(shcol,tracker,pos1,pos2,hdir);
      }
    }
    return retval;
  }

}
using mu2e::RobustHelixFinder;
DEFINE_ART_MODULE(RobustHelixFinder);
