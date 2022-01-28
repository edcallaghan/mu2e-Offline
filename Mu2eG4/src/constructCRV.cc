//
// Free function to create CRV aka Scintillator Shield in CosmicRayShield
//
//
// Original author KLG
//
// Notes:

// art includes
#include "art/Framework/Services/Registry/ServiceDefinitionMacros.h"

// clhep includes
#include "CLHEP/Vector/ThreeVector.h"

// Mu2e includes.

#include "Offline/Mu2eG4/inc/constructCRV.hh"
#include "Offline/GeometryService/inc/GeomHandle.hh"
#include "Offline/GeometryService/inc/GeometryService.hh"
#include "Offline/GeometryService/inc/G4GeometryOptions.hh"
#include "Offline/Mu2eG4Helper/inc/Mu2eG4Helper.hh"
#include "Offline/CosmicRayShieldGeom/inc/CosmicRayShield.hh"
#include "Offline/CosmicRayShieldGeom/inc/CRSScintillatorShield.hh"
#include "Offline/Mu2eG4/inc/findMaterialOrThrow.hh"
#include "Offline/Mu2eG4/inc/checkForOverlaps.hh"
#include "Offline/Mu2eG4/inc/nestBox.hh"

// G4 includes

#include "Geant4/G4Material.hh"
#include "Geant4/G4Color.hh"
#include "Geant4/G4Box.hh"

#include "Geant4/G4VSolid.hh"
#include "Geant4/G4UnionSolid.hh"
#include "Geant4/G4LogicalVolume.hh"
#include "Geant4/G4PVPlacement.hh"

#include "Geant4/G4VisAttributes.hh"

#include "Geant4/G4SDManager.hh"

#include "TROOT.h"

using namespace std;

namespace mu2e 
{
  void constructCRV( VolumeInfo const & parent, SimpleConfig const & _config)
  {
    GeomHandle<CosmicRayShield> CosmicRayShieldGeomHandle;

    Mu2eG4Helper& _helper       = *(art::ServiceHandle<Mu2eG4Helper>());
    AntiLeakRegistry& reg   = _helper.antiLeakRegistry();
    const auto& geomOptions = art::ServiceHandle<GeometryService>()->geomOptions();
    
    geomOptions->loadEntry( _config, "crsVeto", "crs.veto" );
    const bool scintillatorShieldVisible   = geomOptions->isVisible("crsVeto"); 
    const bool scintillatorShieldDrawSolid = geomOptions->isSolid("crsVeto"); 
    const bool forceAuxEdgeVisible         = geomOptions->forceAuxEdgeVisible("crsVeto"); 
    const bool doSurfaceCheck              = geomOptions->doSurfaceCheck("crsVeto"); 
    //const bool placePV                     = geomOptions->placePV("crsVeto"); 
    
    int  verbosityLevel                    = _config.getInt("crs.verbosityLevel",0);
    bool forMARS                           = _config.getBool("crs.forMARS",false);

    std::string hallAirMaterialName = _config.getString("hall.insideMaterialName");
    G4Material* hallAirMaterial = findMaterialOrThrow(hallAirMaterialName);

    CLHEP::Hep3Vector parentCenterInMu2e = parent.centerInMu2e();

    std::vector<CRSScintillatorShield> const &shields = CosmicRayShieldGeomHandle->getCRSScintillatorShields();
    std::vector<CRSScintillatorShield>::const_iterator ishield;
    for(ishield=shields.begin(); ishield!=shields.end(); ++ishield) 
    {
      CRSScintillatorShield const & shield = *ishield;
      std::string const & CRVsectorName = shield.getName();

      if(verbosityLevel > 0) cout << __func__ << " constructing            : " << CRVsectorName << endl;

      //build mother volume
      const std::vector<CRSScintillatorModule> &modules = shield.getCRSScintillatorModules();
      const CRSScintillatorModule &module0 = modules.front();
      const CRSScintillatorModule &module1 = modules.back();

      int thicknessDirection=shield.getCRSScintillatorBarDetail().getThicknessDirection();

      std::vector<CLHEP::Hep3Vector> motherCenterInMu2e;
      std::vector<std::vector<double> > motherHalfLengths;
      //collect all layers
      for(size_t l=0; l<module0.getLayers().size(); ++l)
      {
        //strong back
        if(l==0)
        {
          const CRSAluminumSheet &aluminumSheet0 = module0.getAluminumSheets().at(0);
          const CRSAluminumSheet &aluminumSheet1 = module1.getAluminumSheets().at(0);
          const CLHEP::Hep3Vector &centerInMu2e0=aluminumSheet0.getPosition();
          const CLHEP::Hep3Vector &centerInMu2e1=aluminumSheet1.getPosition();
          motherCenterInMu2e.emplace_back(0.5*(centerInMu2e0+centerInMu2e1));
          std::vector<double> halfLengths;
          halfLengths.resize(3);
          for(int i=0; i<3; ++i)
          {
            halfLengths[i]=aluminumSheet0.getHalfLengths()[i]+0.5*fabs(centerInMu2e0[i]-centerInMu2e1[i]);
          }
          motherHalfLengths.push_back(halfLengths);
        }

        //scintillator layers
        {
          const CRSScintillatorLayer &layer0 = module0.getLayers().at(l);
          const CRSScintillatorLayer &layer1 = module1.getLayers().at(l);
          const CLHEP::Hep3Vector &centerInMu2e0=layer0.getPosition();
          const CLHEP::Hep3Vector &centerInMu2e1=layer1.getPosition();
          motherCenterInMu2e.emplace_back(0.5*(centerInMu2e0+centerInMu2e1));
          std::vector<double> halfLengths;
          halfLengths.resize(3);
          for(int i=0; i<3; ++i)
          {
            halfLengths[i]=layer0.getHalfLengths()[i]+0.5*fabs(centerInMu2e0[i]-centerInMu2e1[i]);
          }
          motherHalfLengths.push_back(halfLengths);
        }

        //aluminum absorbers
        if(l+1<module0.getLayers().size())
        {
          const CRSAbsorberLayer &aluminumSheet0 = module0.getAbsorberLayers().at(l);
          const CRSAbsorberLayer &aluminumSheet1 = module1.getAbsorberLayers().at(l);
          const CLHEP::Hep3Vector &centerInMu2e0=aluminumSheet0.getPosition();
          const CLHEP::Hep3Vector &centerInMu2e1=aluminumSheet1.getPosition();
          motherCenterInMu2e.emplace_back(0.5*(centerInMu2e0+centerInMu2e1));
          std::vector<double> halfLengths;
          halfLengths.resize(3);
          for(int i=0; i<3; ++i)
          {
            halfLengths[i]=aluminumSheet0.getHalfLengths()[i]+0.5*fabs(centerInMu2e0[i]-centerInMu2e1[i]);
          }
          motherHalfLengths.push_back(halfLengths);
        }

        //thin aluminum cover sheet
        if(l+1==module0.getLayers().size())
        {
          const CRSAluminumSheet &aluminumSheet0 = module0.getAluminumSheets().at(1);
          const CRSAluminumSheet &aluminumSheet1 = module1.getAluminumSheets().at(1);
          const CLHEP::Hep3Vector &centerInMu2e0=aluminumSheet0.getPosition();
          const CLHEP::Hep3Vector &centerInMu2e1=aluminumSheet1.getPosition();
          motherCenterInMu2e.emplace_back(0.5*(centerInMu2e0+centerInMu2e1));
          std::vector<double> halfLengths;
          halfLengths.resize(3);
          for(int i=0; i<3; ++i)
          {
            halfLengths[i]=aluminumSheet0.getHalfLengths()[i]+0.5*fabs(centerInMu2e0[i]-centerInMu2e1[i]);
          }
          motherHalfLengths.push_back(halfLengths);
        }
      }//layers

      //mother solid for all layers of the CRV sector
      G4VSolid *motherSolid=NULL;
      for(size_t l=0; l<motherCenterInMu2e.size(); ++l)
      {
        G4VSolid *motherSolidPart=new G4Box(Form("%s_%s_%lu","CRSmotherSolidPart",CRVsectorName.c_str(),l),
                                            motherHalfLengths[l][0],
                                            motherHalfLengths[l][1],
                                            motherHalfLengths[l][2]);
        if(l+1<motherCenterInMu2e.size())
        {
          CLHEP::Hep3Vector centerInMu2e=0.5*(motherCenterInMu2e[l]+motherCenterInMu2e[l+1]);
          std::vector<double> halfLengths;
          halfLengths.resize(3);
          for(int i=0; i<3; ++i)
          {
            if(i==thicknessDirection)
              halfLengths[i]=0.5*fabs(motherCenterInMu2e[l][i]-motherCenterInMu2e[l+1][i]);
            else
            {
              double xmin=std::max(motherCenterInMu2e[l][i]-motherHalfLengths[l][i],
                                   motherCenterInMu2e[l+1][i]-motherHalfLengths[l+1][i]);
              double xmax=std::min(motherCenterInMu2e[l][i]+motherHalfLengths[l][i],
                                   motherCenterInMu2e[l+1][i]+motherHalfLengths[l+1][i]);
              if(xmax<xmin) throw std::logic_error("can't construct mother volume. non-connected layers found.");
              centerInMu2e[i]=0.5*(xmax+xmin);
              halfLengths[i]=0.5*(xmax-xmin);
            }
          }
          G4VSolid *motherSolidOverlap=new G4Box(Form("%s_%s_%lu","CRSmotherSolidOverlap",CRVsectorName.c_str(),l),
                                                 halfLengths[0],
                                                 halfLengths[1],
                                                 halfLengths[2]);
          G4UnionSolid *motherSolidUnion=new G4UnionSolid(Form("%s_%s_%lu","CRSmotherSolidOverlapUnion",CRVsectorName.c_str(),l),
                                                          motherSolidPart,motherSolidOverlap,NULL,
                                                          centerInMu2e-motherCenterInMu2e[l]);
          motherSolidPart = motherSolidUnion;
        }

        if(l>0)
        {
          G4UnionSolid *motherSolidUnion=new G4UnionSolid(Form("%s_%s_%lu","CRSmotherSolidUnion",CRVsectorName.c_str(),l),
                                                          motherSolid,motherSolidPart,NULL,
                                                          motherCenterInMu2e[l]-motherCenterInMu2e[0]);
          motherSolidPart = motherSolidUnion;
        }
        motherSolid = motherSolidPart;
      } //layers
      motherSolid->SetName("CRSmother_"+CRVsectorName);
      G4LogicalVolume *motherLogical = new G4LogicalVolume(motherSolid, hallAirMaterial,
                                                           motherSolid->GetName());
      motherLogical->SetVisAttributes(G4VisAttributes::GetInvisible());

      CLHEP::Hep3Vector motherAirOffset = motherCenterInMu2e.front() - parentCenterInMu2e;
      G4VPhysicalVolume* motherPhysical = new G4PVPlacement(NULL,
                                                            motherAirOffset,
                                                            motherLogical,
                                                            motherSolid->GetName(),
                                                            parent.logical,
                                                            false,
                                                            0,
                                                            false);
      if(doSurfaceCheck) checkForOverlaps(motherPhysical, _config, verbosityLevel>0);

      //mother solid for individual scintillator/aluminum layers of the CRV sector
      //(placed in big mother volume from above)
      std::vector<G4LogicalVolume*> motherLayersLogical;
      for(size_t l=0; l<motherCenterInMu2e.size(); ++l)
      {
        G4VSolid *motherLayerSolid=new G4Box(Form("%s_%s_%lu","CRSmotherLayer",CRVsectorName.c_str(),l),
                                             motherHalfLengths[l][0],
                                             motherHalfLengths[l][1],
                                             motherHalfLengths[l][2]);
        motherLayersLogical.emplace_back(new G4LogicalVolume(motherLayerSolid, hallAirMaterial,
                                                             motherLayerSolid->GetName()));
        motherLayersLogical.back()->SetVisAttributes(G4VisAttributes::GetInvisible());

        CLHEP::Hep3Vector motherLayerAirOffset = motherCenterInMu2e.at(l) - motherCenterInMu2e.front();
        G4VPhysicalVolume* motherLayerPhysical = new G4PVPlacement(NULL,
                                                                   motherLayerAirOffset,
                                                                   motherLayersLogical.back(),
                                                                   motherLayerSolid->GetName(),
                                                                   motherLogical,
                                                                   false,
                                                                   0,
                                                                   false);
        if(doSurfaceCheck) checkForOverlaps(motherLayerPhysical, _config, verbosityLevel>0);
      }

      // all CRV counter dimensions are the same within a particular CRV sector
      CRSScintillatorBarDetail const &barDetail = shield.getCRSScintillatorBarDetail();

      G4Material* scintillatorBarMaterial = findMaterialOrThrow(barDetail.getMaterialName());
      std::vector<double> const &scintillatorBarHalfLengths = barDetail.getHalfLengths();

      G4VSolid* scintillatorBarSolid = new G4Box("CRSscintBar_"+CRVsectorName,
                                                 scintillatorBarHalfLengths[0],
                                                 scintillatorBarHalfLengths[1],
                                                 scintillatorBarHalfLengths[2]);

      G4LogicalVolume* scintillatorBarLogical = new G4LogicalVolume(scintillatorBarSolid,
                                                                    scintillatorBarMaterial,
                                                                    scintillatorBarSolid->GetName());

      // visibility attributes
      if(!scintillatorShieldVisible) 
      {
        scintillatorBarLogical->SetVisAttributes(G4VisAttributes::GetInvisible());
      }
      else 
      {
        G4Colour  orange(.75, .55, .0);
        G4VisAttributes* visAtt = reg.add(G4VisAttributes(true, orange));
        visAtt->SetForceSolid(scintillatorShieldDrawSolid);
        visAtt->SetForceAuxEdgeVisible(forceAuxEdgeVisible);
        scintillatorBarLogical->SetVisAttributes(visAtt);
      }

      if(verbosityLevel > 1) 
      {
        G4SDManager::GetSDMpointer()->SetVerboseLevel(verbosityLevel-1);
      }

      // build one counter mother board, which will be the same for all counters of this CRV sector
      G4Material* CMBMaterial = findMaterialOrThrow(barDetail.getCMBMaterialName());
      std::vector<double> const & CMBHalfLengths = barDetail.getCMBHalfLengths();

      G4VSolid* CMBSolid = new G4Box("CRSCMB_"+CRVsectorName,
                                     CMBHalfLengths[0],
                                     CMBHalfLengths[1],
                                     CMBHalfLengths[2]);

      G4LogicalVolume* CMBLogical = new G4LogicalVolume(CMBSolid,CMBMaterial,CMBSolid->GetName());

      // visibility attributes
      if(!scintillatorShieldVisible) 
      {
        CMBLogical->SetVisAttributes(G4VisAttributes::GetInvisible());
      }
      else 
      {
        G4Colour  green(.0, .55, .55);
        G4VisAttributes* visAtt = reg.add(G4VisAttributes(true, green));
        visAtt->SetForceSolid(scintillatorShieldDrawSolid);
        visAtt->SetForceAuxEdgeVisible(forceAuxEdgeVisible);
        CMBLogical->SetVisAttributes(visAtt);
      }

      for(size_t m=0; m<modules.size(); ++m) 
      {
        const std::vector<CRSScintillatorLayer> &scintLayers = modules.at(m).getLayers();
        for(size_t l=0; l<scintLayers.size(); ++l) 
        {
          //add mother volumes around each layer of scintillators incl. CMBs (for each module)
          const std::vector<double> &scintLayerHalfLengths=scintLayers.at(l).getHalfLengths();
          G4VSolid *scintLayerSolid=new G4Box(scintLayers.at(l).name("CRSscintLayer_"),
                                              scintLayerHalfLengths[0],
                                              scintLayerHalfLengths[1],
                                              scintLayerHalfLengths[2]);
          G4LogicalVolume *scintLayerLogical=new G4LogicalVolume(scintLayerSolid, hallAirMaterial, scintLayerSolid->GetName());
          scintLayerLogical->SetVisAttributes(G4VisAttributes::GetInvisible());

          const CLHEP::Hep3Vector &scintLayerCenterInMu2e=scintLayers.at(l).getPosition();
          CLHEP::Hep3Vector scintLayerMotherOffset = scintLayerCenterInMu2e - motherCenterInMu2e.at(l*2+1);
          G4VPhysicalVolume* scintLayerPhysical = new G4PVPlacement(NULL,
                                                                    scintLayerMotherOffset,
                                                                    scintLayerLogical,
                                                                    scintLayerSolid->GetName(),
                                                                    motherLayersLogical.at(l*2+1),
                                                                    false,
                                                                    0,
                                                                    false);
          if(doSurfaceCheck) checkForOverlaps(scintLayerPhysical, _config, verbosityLevel>0);

          const std::vector<std::shared_ptr<CRSScintillatorBar> > &bars = scintLayers.at(l).getBars();
          std::vector<std::shared_ptr<CRSScintillatorBar> >::const_iterator ibar;
          for(ibar=bars.begin(); ibar!=bars.end(); ++ibar) 
          {
            const CRSScintillatorBar &bar = **ibar; 

            //bar.getPosition() returns the bar position in Mu2e coordinates
            //need the position relative to the layer center
            CLHEP::Hep3Vector barLayerOffset = bar.getPosition() - scintLayerCenterInMu2e; 
            CLHEP::Hep3Vector CMB0LayerOffset = bar.getCMBPosition(0) - scintLayerCenterInMu2e; 
            CLHEP::Hep3Vector CMB1LayerOffset = bar.getCMBPosition(1) - scintLayerCenterInMu2e; 

            G4VPhysicalVolume* pv = new G4PVPlacement(NULL,
                                                      barLayerOffset,
                                                      scintillatorBarLogical,
                                                      bar.name("CRSScintillatorBar_"),
                                                      scintLayerLogical,
                                                      false,
                                                      bar.index().asInt(),
                                                      false);
            if(doSurfaceCheck) checkForOverlaps(pv, _config, verbosityLevel>0);

            if(verbosityLevel > 4) std::cout << bar.name("CRSScintillatorBar_") << " " << bar.getPosition() <<std::endl;

//FIXME: this is temporary until the GDML issue is fixed
if(!_config.getBool("crs.hideCRVCMBs"))
{
            if(bar.hasCMB(0))
            {
              //MARS requires gdml file with unique logical volumes for the CMBs
              if(forMARS) CMBLogical = new G4LogicalVolume(CMBSolid,CMBMaterial, bar.name("CRSCMB0_")); 

              G4VPhysicalVolume* pvCMB0 = new G4PVPlacement(NULL,
                                                            CMB0LayerOffset,
                                                            CMBLogical,
                                                            bar.name("CRSCMB0_"),
                                                            scintLayerLogical,
                                                            false,
                                                            2*bar.index().asInt(),
                                                            false);
              if(doSurfaceCheck) checkForOverlaps(pvCMB0, _config, verbosityLevel>0);

              if(verbosityLevel > 4) std::cout << bar.name("CRSCMB0_") << " " << bar.getCMBPosition(0) <<std::endl;
            }

            if(bar.hasCMB(1))
            {
              //MARS requires gdml file with unique logical volumes for the CMBs
              if(forMARS) CMBLogical = new G4LogicalVolume(CMBSolid,CMBMaterial, bar.name("CRSCMB1_")); 

              G4VPhysicalVolume* pvCMB1 = new G4PVPlacement(NULL,
                                                            CMB1LayerOffset,
                                                            CMBLogical,
                                                            bar.name("CRSCMB1_"),
                                                            scintLayerLogical,
                                                            false,
                                                            2*bar.index().asInt()+1,
                                                            false);
              if(doSurfaceCheck) checkForOverlaps(pvCMB1, _config, verbosityLevel>0);

              if(verbosityLevel > 4) std::cout << bar.name("CRSCMB1_") << " " << bar.getCMBPosition(1) <<std::endl;
            }
}
          } //ibar
        } //ilayer

        //absorber sheets
        const std::vector<CRSAbsorberLayer> &absorberLayers = modules.at(m).getAbsorberLayers();
        const std::string &absorberNameBase = modules.at(m).name("CRSAbsorber_");
        for(size_t l=0; l<absorberLayers.size(); ++l) 
        {
          const std::string absorberName = absorberNameBase+"_"+std::to_string(l);
          const std::vector<double> &absorberLayerHalflengths=absorberLayers[l].getHalfLengths();
          const CLHEP::Hep3Vector &absorberLayerCenterInMu2e=absorberLayers[l].getPosition();
          CLHEP::Hep3Vector absorberLayerMotherOffset = absorberLayerCenterInMu2e - motherCenterInMu2e.at(2*l+2);

          const std::string &absorberMaterialName = shield.getAbsorberMaterialName();
          G4Material* absorberMaterial = findMaterialOrThrow(absorberMaterialName);

          G4VSolid* absorberSolid = new G4Box(absorberName,
                                              absorberLayerHalflengths[0],
                                              absorberLayerHalflengths[1],
                                              absorberLayerHalflengths[2]);

          G4LogicalVolume* absorberLogical = new G4LogicalVolume(absorberSolid,
                                                                 absorberMaterial,
                                                                 absorberName);

          if(!scintillatorShieldVisible) 
          {
            absorberLogical->SetVisAttributes(G4VisAttributes::GetInvisible());
          }
          else 
          {
            G4Colour  darkorange  (.45, .25, .0);
            G4VisAttributes* visAtt = reg.add(G4VisAttributes(true, darkorange));
            visAtt->SetForceSolid(scintillatorShieldDrawSolid);
            visAtt->SetForceAuxEdgeVisible(forceAuxEdgeVisible);
            absorberLogical->SetVisAttributes(visAtt);
          }

          G4VPhysicalVolume* pv = new G4PVPlacement(NULL,
                                                    absorberLayerMotherOffset,
                                                    absorberLogical,
                                                    absorberName,
                                                    motherLayersLogical.at(2*l+2),
                                                    false,
                                                    0,
                                                    false);
          if(doSurfaceCheck) 
          {
            checkForOverlaps( pv, _config, verbosityLevel>0);
          }
        } //absorberLayerNumber


        //aluminum sheets (strong back and thin cover sheet)
        const std::vector<CRSAluminumSheet> &aluminumSheets = modules.at(m).getAluminumSheets();
        const std::string &aluminumSheetNameBase = modules.at(m).name("CRSAluminumSheet_");
        for(size_t l=0; l<aluminumSheets.size(); ++l) 
        {
          const std::string aluminumSheetName = aluminumSheetNameBase+"_"+std::to_string(l);
          const std::vector<double> &aluminumSheetHalflengths=aluminumSheets[l].getHalfLengths();
          const CLHEP::Hep3Vector &aluminumSheetCenterInMu2e=aluminumSheets[l].getPosition();
          CLHEP::Hep3Vector aluminumSheetMotherOffset = aluminumSheetCenterInMu2e;
          switch(l)
          {
            case 0: aluminumSheetMotherOffset-=motherCenterInMu2e.front(); break;
            case 1: aluminumSheetMotherOffset-=motherCenterInMu2e.back(); break;
            default: std::logic_error("found more than 2 aluminum sheets (strong back, thin cover sheet).");
          } 

          const std::string &aluminumSheetMaterialName = shield.getAluminumSheetMaterialName();
          G4Material* aluminumSheetMaterial = findMaterialOrThrow(aluminumSheetMaterialName);

          G4VSolid* aluminumSheetSolid = new G4Box(aluminumSheetName,
                                                   aluminumSheetHalflengths[0],
                                                   aluminumSheetHalflengths[1],
                                                   aluminumSheetHalflengths[2]);

          G4LogicalVolume* aluminumSheetLogical = new G4LogicalVolume(aluminumSheetSolid,
                                                                      aluminumSheetMaterial,
                                                                      aluminumSheetName);

          if(!scintillatorShieldVisible) 
          {
            aluminumSheetLogical->SetVisAttributes(G4VisAttributes::GetInvisible());
          }
          else 
          {
            G4Colour  darkorange  (.45, .25, .0);
            G4VisAttributes* visAtt = reg.add(G4VisAttributes(true, darkorange));
            visAtt->SetForceSolid(scintillatorShieldDrawSolid);
            visAtt->SetForceAuxEdgeVisible(forceAuxEdgeVisible);
            aluminumSheetLogical->SetVisAttributes(visAtt);
          }

          G4VPhysicalVolume* pv = new G4PVPlacement(NULL,
                                                    aluminumSheetMotherOffset,
                                                    aluminumSheetLogical,
                                                    aluminumSheetName,
                                                    (l==0?motherLayersLogical.front():motherLayersLogical.back()),
                                                    false,
                                                    0,
                                                    false);
          if(doSurfaceCheck) 
          {
            checkForOverlaps( pv, _config, verbosityLevel>0);
          }
        } //aluminumSheetNumber


        //FEBs
        const std::vector<CRSFEB> &FEBs = modules.at(m).getFEBs();
        const std::string &FEBNameBase = modules.at(m).name("CRSFEB_");
        for(unsigned int FEBNumber=0; FEBNumber<FEBs.size(); FEBNumber++) 
        {
          const std::string FEBName = FEBNameBase+"_"+std::to_string(FEBNumber);
          const std::vector<double> &FEBHalflengths=FEBs[FEBNumber].getHalfLengths();
          const CLHEP::Hep3Vector &FEBCenterInMu2e=FEBs[FEBNumber].getPosition();
          CLHEP::Hep3Vector FEBAirOffset = FEBCenterInMu2e - parentCenterInMu2e;

          const std::string &FEBMaterialName = shield.getFEBMaterialName();
          G4Material* FEBMaterial = findMaterialOrThrow(FEBMaterialName);

          G4VSolid* FEBSolid = new G4Box(FEBName,
                                         FEBHalflengths[0],
                                         FEBHalflengths[1],
                                         FEBHalflengths[2]);

          G4LogicalVolume* FEBLogical = new G4LogicalVolume(FEBSolid,
                                                            FEBMaterial,
                                                            FEBName);

          if(!scintillatorShieldVisible) 
          {
            FEBLogical->SetVisAttributes(G4VisAttributes::GetInvisible());
          }
          else 
          {
            G4Colour  darkorange  (.45, .25, .0);
            G4VisAttributes* visAtt = reg.add(G4VisAttributes(true, darkorange));
            visAtt->SetForceSolid(scintillatorShieldDrawSolid);
            visAtt->SetForceAuxEdgeVisible(forceAuxEdgeVisible);
            FEBLogical->SetVisAttributes(visAtt);
          }

          G4VPhysicalVolume* pv = new G4PVPlacement(NULL,
                                                    FEBAirOffset,
                                                    FEBLogical,
                                                    FEBName,
                                                    parent.logical,
                                                    false,
                                                    0,
                                                    false);
          if(doSurfaceCheck) 
          {
            checkForOverlaps( pv, _config, verbosityLevel>0);
          }

          if(verbosityLevel > 4) std::cout << FEBName << " " << FEBCenterInMu2e <<std::endl;
        } //FEBNumber
      } //modules
    } //ishield

    //steel support structures
    std::vector<CRSSupportStructure> const &supportStructures = CosmicRayShieldGeomHandle->getSupportStructures();
    std::vector<CRSSupportStructure>::const_iterator iSupportStructure;
    for(iSupportStructure=supportStructures.begin(); iSupportStructure!=supportStructures.end(); ++iSupportStructure) 
    {
      CRSSupportStructure const & supportStructure = *iSupportStructure;
      std::string const & name = supportStructure.getName();

      const std::vector<double> &halflengths=supportStructure.getHalfLengths();
      const CLHEP::Hep3Vector &positionInMu2e = supportStructure.getPosition();
      CLHEP::Hep3Vector supportStructureAirOffset = positionInMu2e - parentCenterInMu2e;

      const std::string &materialName = supportStructure.getMaterialName();
      G4Material* material = findMaterialOrThrow(materialName);

      G4VSolid* supportStructureSolid = new G4Box(name,
                                         halflengths[0],
                                         halflengths[1],
                                         halflengths[2]);

      G4LogicalVolume* supportStructureLogical = new G4LogicalVolume(supportStructureSolid, material, name);

      if(!scintillatorShieldVisible) 
      {
        supportStructureLogical->SetVisAttributes(G4VisAttributes::GetInvisible());
      }
      else 
      {
        G4Colour  darkorange  (.45, .25, .0);
        G4VisAttributes* visAtt = reg.add(G4VisAttributes(true, darkorange));
        visAtt->SetForceSolid(scintillatorShieldDrawSolid);
        visAtt->SetForceAuxEdgeVisible(forceAuxEdgeVisible);
        supportStructureLogical->SetVisAttributes(visAtt);
      }

      G4VPhysicalVolume* pv = new G4PVPlacement(NULL,
                                                supportStructureAirOffset,
                                                supportStructureLogical,
                                                name,
                                                parent.logical,
                                                false,
                                                0,
                                                false);
      if(doSurfaceCheck) 
      {
        checkForOverlaps( pv, _config, verbosityLevel>0);
      }
    } //iSupportStructure

  } //construct CRV

} //namespace mu2e
