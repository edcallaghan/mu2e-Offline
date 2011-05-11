//
// Generate some number of DIO electrons.
//
// $Id: DecayInOrbitGun.cc,v 1.20 2011/05/11 19:21:31 onoratog Exp $ 
// $Author: onoratog $
// $Date: 2011/05/11 19:21:31 $
//
// Original author Rob Kutschke
// 

// C++ includes.
#include <iostream>

// Framework includes
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Services/interface/TFileService.h"
#include "FWCore/Framework/interface/TFileDirectory.h"

// Mu2e includes
#include "EventGenerator/inc/DecayInOrbitGun.hh"
#include "Mu2eUtilities/inc/SimpleConfig.hh"
#include "Mu2eUtilities/inc/safeSqrt.hh"
#include "ConditionsService/inc/ConditionsHandle.hh"
#include "ConditionsService/inc/AcceleratorParams.hh"
#include "ConditionsService/inc/ParticleDataTable.hh"
#include "ConditionsService/inc/DAQParams.hh"
#include "Mu2eUtilities/inc/PDGCode.hh"

// General Utilities
#include "GeneralUtilities/inc/pow.hh"

//ROOT includes
#include "TH1D.h"

// CLHEP includes.
#include "CLHEP/Random/RandFlat.h"
#include "CLHEP/Units/PhysicalConstants.h"

using namespace std;

//using CLHEP::Hep3Vector;
//using CLHEP::HepLorentzVector;
//using CLHEP::RandFlat;
//using CLHEP::twopi;

namespace mu2e {

  // Need a Conditions entity to hold info about conversions:
  // endpoints and lifetimes for different materials etc
  // Grab them from Andrew's minimc package?
  static const double conversionEnergyAluminum = 104.96;
  
  DecayInOrbitGun::DecayInOrbitGun( edm::Run& run, const SimpleConfig& config ):

    // Base class
    GeneratorBase(),

    // Information from config file.
    _mean(config.getDouble("decayinorbitGun.mean",1.)),
    _elow(config.getDouble("decayinorbitGun.elow",100.)),
    _ehi(config.getDouble("decayinorbitGun.ehi",conversionEnergyAluminum)),
    _nbins(config.getInt("decayinorbitGun.nbins",1000)),
    _czmin(config.getDouble("decayinorbitGun.czmin",  0.3)),
    _czmax(config.getDouble("decayinorbitGun.czmax",  0.6)),
    _phimin(config.getDouble("decayinorbitGun.phimin", 0. )),
    _phimax(config.getDouble("decayinorbitGun.phimax", CLHEP::twopi )),
    _doHistograms(config.getBool("decayinorbitGun.doHistograms", true)),
    _spectrumResolution(config.getDouble("decayinorbitGun.spectrumResolution", 0.1)),
    _useSimpleSpectrum(config.getBool("decayinorbitGun.useSimpleSpectrum", false)),

    // Random number distributions; getEngine comes from the base class.
    _randSimpleEnergy(getEngine(), &(binnedEnergySpectrum()[0]), _nbins ),

    _randPoissonQ( getEngine(), std::abs(_mean) ),
    _randomUnitSphere ( getEngine(), _czmin, _czmax, _phimin, _phimax ),  

    // Histograms.
    _hMultiplicity(0),
    _hEElec(0),
    _hEElecZ(0),
    _hzPosition(0),
    _hcz(0),
    _hphi(0),
    _ht()  {

    // Sanity check.
    if ( std::abs(_mean) > 99999. ) {
      throw cms::Exception("RANGE") 
        << "DecayInOrbit Gun has been asked to produce a crazily large number of electrons."
        << _mean
        << "\n";
    }

    // About the ConditionsService:
    // The argument to the constructor is ignored for now.  It will be a
    // data base key.  There is a second argument that I have let take its
    // default value of "current"; it will be used to specify a version number.
    ConditionsHandle<AcceleratorParams> accPar("ignored");
    ConditionsHandle<DAQParams>         daqPar("ignored");
    ConditionsHandle<ParticleDataTable> pdt("ignored");
    
    //pick up particle mass    
    const HepPDT::ParticleData& e_data = pdt->particle(PDGCode::e_minus).ref();
    _mass = e_data.mass().value();

    _tmin   = config.getDouble("decayinorbitGun.tmin",  daqPar->t0 );
    _tmax   = config.getDouble("decayinorbitGun.tmax",  accPar->deBuncherPeriod );

    // Make ROOT subdirectory to hold diagnostic histograms; book those histograms.
  
    if ( _doHistograms ){
      edm::Service<edm::TFileService> tfs;
      edm::TFileDirectory tfdir = tfs->mkdir( "DecayInOrbit" );
      _hMultiplicity = tfdir.make<TH1D>( "hMultiplicity", "DIO Multiplicity",                20,     0.,   20.    );
      _hEElec        = tfdir.make<TH1D>( "hEElec",        "DIO Electron Energy",             10,  _elow,    0.105 );
      _hEElecZ       = tfdir.make<TH1D>( "hEElecZ",       "DIO Electron Energy (zoom)",     200,  _elow, _ehi     );
      _hzPosition    = tfdir.make<TH1D>( "hzPosition",    "DIO z Position (Tracker Coord)", 200, -6600., -5600.   );
      _hcz           = tfdir.make<TH1D>( "hcz",           "DIO cos(theta)",                 100,    -1.,     1.   );
      _hphi          = tfdir.make<TH1D>( "hphi",          "DIO azimuth",                    100,  -M_PI,   M_PI   );
      _ht            = tfdir.make<TH1D>( "ht",            "DIO time ", 210, -200., 2000. );
    }

    _fGenerator = auto_ptr<FoilParticleGenerator>(new FoilParticleGenerator( getEngine(), _tmin, _tmax, 
                                                                             FoilParticleGenerator::volWeightFoil, 
                                                                             FoilParticleGenerator::flatPos, 
                                                                             FoilParticleGenerator::limitedExpoTime));

    _randEnergy = auto_ptr<DIOShankerWanatabe>(new DIOShankerWanatabe(13,_elow, _ehi, _spectrumResolution, getEngine()));



  }

  DecayInOrbitGun::~DecayInOrbitGun(){
  }
  
  void DecayInOrbitGun::generate( ToyGenParticleCollection& genParts ){
    // Choose the number of electrons to generate this event.
    long n = (_mean < 0 ? static_cast<long>(-_mean): _randPoissonQ.fire());
      
    if ( _doHistograms ) { 
      _hMultiplicity->Fill(n); 
    }
    
    //Loop over particles to generate
    
    for (int i=0; i<n; ++i) {

      //Pick up position and momentum
      CLHEP::Hep3Vector pos(0,0,0);
      double time;
      _fGenerator->generatePositionAndTime(pos, time);
      

      //Pick up energy and momentum vector
      double e;
      if (_useSimpleSpectrum) {
        e = _elow + _randSimpleEnergy.fire() * (_ehi - _elow); 
      } else if (!_useSimpleSpectrum) {
        e = _randEnergy->fire();
      }

      _p = safeSqrt(e*e - _mass*_mass);
      CLHEP::Hep3Vector p3 = _randomUnitSphere.fire(_p);
      
      //Set Four-momentum
      CLHEP::HepLorentzVector mom(p3,e);
      
      // Add the particle to  the list.
      genParts.push_back( ToyGenParticle( PDGCode::e_minus, GenId::dio1, pos, mom, time));

      if( _doHistograms ){
        _hEElec    ->Fill( e );
        _hEElecZ   ->Fill( e );
        _hzPosition->Fill( pos.z() );
        _hcz       ->Fill( p3.cosTheta() );
        _hphi      ->Fill( p3.phi() );
        _ht        ->Fill( time );
      }
      
    } // End of loop over particles
    
  } // DecayInOrbitGun::generate
  
  
  // Energy spectrum of the electron from DIO.
  // Input energy in MeV
  double DecayInOrbitGun::energySpectrum( double e )
  {
    return pow<5>(conversionEnergyAluminum - e) ;
  }
    

  // Compute a binned representation of the energy spectrum of the electron from DIO.
  // Not used. Still in the code until a new class can reproduce it. 

  std::vector<double> DecayInOrbitGun::binnedEnergySpectrum(){
    
    // Sanity check.
    if (_nbins <= 0) {
      throw cms::Exception("RANGE") 
        << "Nonsense nbins requested in "
        << "DecayInOrbit = "
        << _nbins
        << "\n";
    }
    
    // Bin width.
    double dE = (_ehi - _elow) / _nbins;

    // Vector to hold the binned representation of the energy spectrum.
    std::vector<double> spectrum;
    spectrum.reserve(_nbins);
    
    for (int ib=0; ib<_nbins; ib++) {
      double x = _elow+(ib+0.5) * dE;
      spectrum.push_back(energySpectrum(x));
    }

    return spectrum;
  } // DecayInOrbitGun::binnedEnergySpectrum


}
