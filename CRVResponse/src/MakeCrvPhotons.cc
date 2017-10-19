#include "MakeCrvPhotons.hh"

#include <sstream>

#include "CLHEP/Units/GlobalSystemOfUnits.h"
#include "CLHEP/Vector/TwoVector.h"

namespace mu2eCrv
{
void LookupConstants::Write(const std::string &filename)
{
  std::ofstream lookupfile(filename,std::ios::binary|std::ios::app);
  lookupfile.write(reinterpret_cast<char*>(this),sizeof(LookupConstants));
  lookupfile.close();
}
void LookupConstants::Read(std::ifstream &lookupfile)
{
  lookupfile.read(reinterpret_cast<char*>(this),sizeof(LookupConstants));
}

void LookupBinDefinitions::WriteVector(std::vector<double> &v, std::ofstream &o)
{
  size_t n=v.size();
  o.write(reinterpret_cast<char*>(&n),sizeof(size_t));
  o.write(reinterpret_cast<char*>(v.data()),sizeof(double)*v.size());
}
void LookupBinDefinitions::ReadVector(std::vector<double> &v, std::ifstream &i)
{
  size_t n;
  i.read(reinterpret_cast<char*>(&n),sizeof(size_t));
/*
  double d[n];
  i.read(reinterpret_cast<char*>(d),sizeof(double)*n);
  v.assign(d,d+n);
*/
  double *d=new double[n];
  i.read(reinterpret_cast<char*>(d),sizeof(double)*n);
  v.assign(d,d+n);
  delete [] d;
//  i.read(reinterpret_cast<char*>(v.data()),sizeof(double)*n);
}
void LookupBinDefinitions::Write(const std::string &filename)
{
  std::ofstream lookupfile(filename,std::ios::binary|std::ios::app);
  WriteVector(xBins,lookupfile);
  WriteVector(yBins,lookupfile);
  WriteVector(zBins,lookupfile);
  WriteVector(betaBins,lookupfile);
  WriteVector(thetaBins,lookupfile);
  WriteVector(phiBins,lookupfile);
  WriteVector(rBins,lookupfile);
  lookupfile.close();
}
void LookupBinDefinitions::Read(std::ifstream &lookupfile)
{
  ReadVector(xBins,lookupfile);
  ReadVector(yBins,lookupfile);
  ReadVector(zBins,lookupfile);
  ReadVector(betaBins,lookupfile);
  ReadVector(thetaBins,lookupfile);
  ReadVector(phiBins,lookupfile);
  ReadVector(rBins,lookupfile);
}

unsigned int LookupBinDefinitions::getNScintillatorBins()
{
  unsigned int nXBins = xBins.size()-1;
  unsigned int nYBins = yBins.size()-1;
  unsigned int nZBins = zBins.size()-1;
  return nXBins*nYBins*nZBins;
}
unsigned int LookupBinDefinitions::getNFiberBins()
{
  unsigned int nBetaBins = betaBins.size()-1;
  unsigned int nThetaBins = thetaBins.size()-1;
  unsigned int nPhiBins = phiBins.size()-1;
  unsigned int nRBins = rBins.size()-1;
  unsigned int nZBins = zBins.size()-1;
  return nBetaBins*nThetaBins*nPhiBins*nRBins*nZBins;
}

unsigned int LookupBinDefinitions::findBin(const std::vector<double> &v, const double &x, bool &notFound)
{
  for(unsigned int i=1; i<v.size(); i++)
  {
    if(v[i]>=x)
    {
      if(v[i-1]<=x) return(i-1);
    }
  }
  notFound=true;
  return(-1);
}
int LookupBinDefinitions::findScintillatorBin(double x, double y, double z)
{
  bool notFound=false;
  unsigned int xBin=findBin(xBins,x,notFound);
  unsigned int yBin=findBin(yBins,y,notFound);
  unsigned int zBin=findBin(zBins,z,notFound);
  if(notFound) return(-1);

  unsigned int nYBins = yBins.size()-1;
  unsigned int nZBins = zBins.size()-1;
  return(zBin + yBin*nZBins + xBin*nYBins*nZBins);
}
int LookupBinDefinitions::findFiberBin(double beta, double theta, double phi, double r, double z)
{
  bool notFound=false;
  unsigned int betaBin=findBin(betaBins,beta,notFound);
  unsigned int thetaBin=findBin(thetaBins,theta,notFound);
  unsigned int phiBin=findBin(phiBins,phi,notFound);
  unsigned int rBin=findBin(rBins,r,notFound);
  unsigned int zBin=findBin(zBins,z,notFound);
  if(notFound) return(-1);

  unsigned int nThetaBins = thetaBins.size()-1;
  unsigned int nPhiBins = phiBins.size()-1;
  unsigned int nRBins = rBins.size()-1;
  unsigned int nZBins = zBins.size()-1;
  return(zBin + rBin*nZBins + phiBin*nRBins*nZBins + thetaBin*nPhiBins*nRBins*nZBins + betaBin*nThetaBins*nPhiBins*nRBins*nZBins);
}


void LookupBin::Write(const std::string &filename)
{
  std::ofstream lookupfile(filename,std::ios::binary|std::ios::app);
  lookupfile.write(reinterpret_cast<char*>(arrivalProbability),sizeof(float)*4);
  lookupfile.write(reinterpret_cast<char*>(timeDelays),sizeof(unsigned short)*4*nTimeDelays);
  lookupfile.write(reinterpret_cast<char*>(fiberEmissions),sizeof(unsigned short)*4*nFiberEmissions);
  lookupfile.close();
}
void LookupBin::Read(std::ifstream &lookupfile)
{
  lookupfile.read(reinterpret_cast<char*>(arrivalProbability),sizeof(float)*4);
  lookupfile.read(reinterpret_cast<char*>(timeDelays),sizeof(unsigned short)*4*nTimeDelays);
  lookupfile.read(reinterpret_cast<char*>(fiberEmissions),sizeof(unsigned short)*4*nFiberEmissions);
}

void MakeCrvPhotons::LoadLookupTable(const std::string &filename)
{
  _fileName = filename;
  std::ifstream lookupfile(filename,std::ios::binary);
  if(!lookupfile.good()) throw std::logic_error("Could not open lookup table file "+filename);

  _LC.Read(lookupfile);
  if(_LC.version1<3) throw std::logic_error("This version of Offline expects a lookup table version 3.0 or higher.");

  _LBD.Read(lookupfile);

  unsigned int nScintillatorBins = _LBD.getNScintillatorBins();
  unsigned int nFiberBins = _LBD.getNFiberBins();

  //0...scintillationInScintillator, 1...cerenkovInScintillator 2...cerenkovInFiber0, 3...cerenkovInFiber1
  _bins[0].resize(nScintillatorBins);
  _bins[1].resize(nScintillatorBins);
  _bins[2].resize(nFiberBins);
  _bins[3].resize(nFiberBins);

  std::cout<<"Reading CRV lookup tables "<<filename<<" ... "<<std::flush;
  for(unsigned int i=0; i<nScintillatorBins; i++) _bins[0][i].Read(lookupfile);
  for(unsigned int i=0; i<nScintillatorBins; i++) _bins[1][i].Read(lookupfile);
  for(unsigned int i=0; i<nFiberBins; i++) _bins[2][i].Read(lookupfile);
  for(unsigned int i=0; i<nFiberBins; i++) _bins[3][i].Read(lookupfile);
  std::cout<<"Done."<<std::endl;

  lookupfile.close();
}

MakeCrvPhotons::~MakeCrvPhotons()
{
}

void MakeCrvPhotons::MakePhotons(const CLHEP::Hep3Vector &stepStart,   //they need to be points
                          const CLHEP::Hep3Vector &stepEnd,            //local to the CRV bar
                          double timeStart, double timeEnd,
                          int PDGcode, double beta, double charge,
                          double energyDepositedTotal,
                          double energyDepositedNonIonizing,
                          double trueStepLength,   //may be longer than stepEnd-stepStart due to scattering 
                                                   //is needed for the visible energy adjustment, and for the Cerenkov photons
                          double scintillationYieldAdjustment)
{
  for(int SiPM=0; SiPM<4; SiPM++) _arrivalTimes[SiPM].clear();

  const CLHEP::Hep3Vector distanceVector = stepEnd-stepStart;
  double stepLength = distanceVector.mag();
  double theta = distanceVector.theta();  //0...+pi
  double phi = distanceVector.phi();      //-pi...+pi
  double r=0;  //distance from fiber center for fiber tables

  double energy = VisibleEnergyDeposition(PDGcode, trueStepLength, energyDepositedTotal, energyDepositedNonIonizing);

  double precision=0.1; //mm
  int    steps=static_cast<int>(stepLength/precision);
  double energyPortion=(steps>0?energy/steps:energy);
  double distance=(steps>0?precision/2.0:stepLength/2.0);
  for(; distance<stepLength; distance+=precision)
  {
    CLHEP::Hep3Vector p = stepStart + distanceVector*distance/stepLength;  //start position of photons
    double t = timeStart + (timeEnd-timeStart)*distance/stepLength;

    bool isInScintillator = IsInsideScintillator(p);
    int fiber = IsInsideFiber(p,r);
    int binNumber;
    if(fiber==-1) binNumber=_LBD.findScintillatorBin(p.x(),p.y(),p.z());
    else binNumber=_LBD.findFiberBin(beta,theta,phi,r,p.z());
    if(binNumber==-1) break;  //if the particular bin wasn't found

    int nPhotonsScintillation=0;
    int nPhotonsCerenkov=0;
    if(isInScintillator)
    {
      double averageNumberOfPhotonsScintillation = (_scintillationYield+scintillationYieldAdjustment)*energyPortion;
      double averageNumberOfPhotonsCerenkov = GetAverageNumberOfCerenkovPhotons(beta, charge, _LC.rindexScintillator, _LC.cerenkovEnergyIntervalScintillator)*precision;
      averageNumberOfPhotonsCerenkov*=trueStepLength/stepLength;  //the true path may be longer due to scattering
      nPhotonsScintillation = GetNumberOfPhotonsFromAverage(averageNumberOfPhotonsScintillation);
      nPhotonsCerenkov = GetNumberOfPhotonsFromAverage(averageNumberOfPhotonsCerenkov);
    } 
    if(fiber>0) //this implies that the muon is in the fiber and not in the scintillator
    {
      double averageNumberOfPhotonsCerenkov = GetAverageNumberOfCerenkovPhotons(beta, charge, _LC.rindexFiber, _LC.cerenkovEnergyIntervalFiber)*precision;
      averageNumberOfPhotonsCerenkov*=trueStepLength/stepLength;  //the true path may be longer due to scattering
      nPhotonsCerenkov = GetNumberOfPhotonsFromAverage(averageNumberOfPhotonsCerenkov);
    } 
    int nPhotons = nPhotonsScintillation + nPhotonsCerenkov;

    //loop over all photons created at this point
    for(int i=0; i<nPhotons; i++)
    {
      int table = -1;
      if(isInScintillator)
      {
        if(i<nPhotonsScintillation) table=0;  //scintillation in scintillator
        else table=1;                         //cerenkov in scintillator
      }
      if(fiber!=-1) table = fiber+2;  //cerenkov in fiber
      if(table==-1) break;  //this can't actually happen

      //get the right bin
      const LookupBin &theBin = _bins[table][binNumber]; //TODO: can be optimized that one doesn't have to access the array for every photon

      //loop over all SiPMs
      for(int SiPM=0; SiPM<4; SiPM++)
      {
        //photon arrival probability at SiPM
        double probability = theBin.arrivalProbability[SiPM];
        if(_randFlat.fire()<=probability)  //a photon arrives at the SiPM --> calculate arrival time
        {
          //start time of photons
          double arrivalTime = t;

          //add straigt line travel time
          double zSiPM=(SiPM%2==0?-_LC.halfLength:_LC.halfLength);
          double straightLineTravelTime=fabs(p.z()-zSiPM)/_LC.speedOfLightFiber;
          arrivalTime+=straightLineTravelTime;

          //add fiber decay times depending on the number of emissions
          int nEmissions = GetRandomFiberEmissions(theBin,SiPM);
          for(int iEmission=0; iEmission<nEmissions; iEmission++) arrivalTime+=-_fiberDecayTime*log(_randFlat.fire());

          //add scintillation decay time
          if(table==0)  //for scintillation in scintillator
          {
//            if(_randFlat.fire()<=_LC.ratioFastSlow)   //use user variable, instead of lookup value
            if(_randFlat.fire()<=_scintillatorRatioFastSlow)
              arrivalTime+=-_scintillatorDecayTimeFast*log(_randFlat.fire());
            else
              arrivalTime+=-_scintillatorDecayTimeSlow*log(_randFlat.fire());
          }

          //add additional time delay due to the photons bouncing around
          bool overflow=false;
          arrivalTime+=GetRandomTime(theBin,SiPM,overflow);

          if(!overflow) _arrivalTimes[SiPM].push_back(arrivalTime); //don't include photons which arrive very late. they are spread out, and can be ignored

        }// if a photon was created
      }//loop over all SiPMs
    }//loop over all photons at this point
  }//loop over all points along the track
}

bool MakeCrvPhotons::IsInsideScintillator(const CLHEP::Hep3Vector &p)
{
  if(fabs(p.x())>=_LC.halfThickness) return false;
  if(fabs(p.y())>=_LC.halfWidth) return false;
  if(fabs(p.z())>=_LC.halfLength) return false;

  CLHEP::Hep2Vector pos2D(fabs(p.x()), fabs(p.y()));
  CLHEP::Hep2Vector fiberPos2D(0.0, _LC.fiberSeparation/2.0);
  CLHEP::Hep2Vector fiberRadius2D(_LC.holeRadiusX, _LC.holeRadiusY);

  double ellipseSum=0;
  for(int i=0; i<2; i++)
  {
    double tmp=(pos2D[i]-fiberPos2D[i])/fiberRadius2D[i];
    ellipseSum+=tmp*tmp;
  } 
  if(ellipseSum<1) return false; // <1 means inside ellipse, i.e. inside fiber hole. ellipse formula: (x-P_x)^2 / r_x^2 + (y-P_y)^2 / r_y^2 = 1

  return true;
}

int MakeCrvPhotons::IsInsideFiber(const CLHEP::Hep3Vector &p, double &r)
{
  CLHEP::Hep2Vector p2D(p.x(), p.y());
  CLHEP::Hep2Vector fiber0(0.0, -_LC.fiberSeparation/2.0);
  CLHEP::Hep2Vector fiber1(0.0, _LC.fiberSeparation/2.0);
  double distanceFiber0=(p2D-fiber0).mag();
  double distanceFiber1=(p2D-fiber1).mag();
  if(distanceFiber0<=_LC.fiberRadius) {r=distanceFiber0; return 0;}
  if(distanceFiber1<=_LC.fiberRadius) {r=distanceFiber1; return 1;}
  return -1;
}

double MakeCrvPhotons::GetRandomTime(const LookupBin &theBin, int SiPM, bool &overflow)
{
  double rand=_randFlat.fire()*LookupBin::probabilityScale;
  double sumProb=0;
  int timeDelay=0;
  for(; timeDelay<LookupBin::nTimeDelays; timeDelay++)
  {
    sumProb+=theBin.timeDelays[SiPM][timeDelay];
    if(rand<=sumProb) break;
  }
  if(timeDelay>=LookupBin::nTimeDelays-1) overflow=true; else overflow=false;

  return timeDelay;
}

int MakeCrvPhotons::GetRandomFiberEmissions(const LookupBin &theBin, int SiPM)
{
  double rand=_randFlat.fire()*LookupBin::probabilityScale;
  double sumProb=0;
  int emissions=0;
  for(; emissions<LookupBin::nFiberEmissions; emissions++)
  {
    sumProb+=theBin.fiberEmissions[SiPM][emissions];
    if(rand<=sumProb) break;
  }

  return emissions;
}

int MakeCrvPhotons::GetNumberOfPhotonsFromAverage(double average)  //from G4Scintillation
{
  int nPhotons;
  if(average>10.0)
  {
    double sigma = std::sqrt(average);
    nPhotons = int(_randGaussQ.fire(average,sigma)+0.5);
  }
  else
  {
    nPhotons = int(_randPoissonQ.fire(average));
  }
  return nPhotons;
}

int MakeCrvPhotons::GetNumberOfPhotons(int SiPM)
{
  return _arrivalTimes[SiPM].size();
}

const std::vector<double> &MakeCrvPhotons::GetArrivalTimes(int SiPM)
{
  return _arrivalTimes[SiPM];
}

//average number of cerenkov photons per millimeter
double MakeCrvPhotons::GetAverageNumberOfCerenkovPhotons(double beta, double charge, double rindex, double cerenkovEnergyInterval) 
{ 
  const double Rfact = 369.81/(CLHEP::eV * CLHEP::cm); //from G4Cerenkov::GetAverageNumberOfPhotons() 

  if(beta<=1.0/rindex) return(0);  //particle too slow -> no Cerenkov radiation

  double n = 1.0 - 1.0/(rindex*rindex*beta*beta);
  n *= Rfact * charge/eplus * charge/eplus * cerenkovEnergyInterval;

  return n;		
}

//this mimics G4EmSaturation::VisibleEnergyDeposition
//but assumes that nloss/(protonRange/chargesq) is small enough so that it can be approximated as 0
//and uses a lookup table for the energyDepositedTotal/electronRange values obtained specifically for Polystyrene
double MakeCrvPhotons::VisibleEnergyDeposition(int PDGcode, double stepLength,
                                            double energyDepositedTotal,
                                            double energyDepositedNonIonizing)
{
  if(energyDepositedTotal <= 0.0) { return 0.0; }

  double evis = energyDepositedTotal;

  if(PDGcode==22)
  {
    if(evis>0)
    {
      double correctionFactor=FindVisibleEnergyAdjustmentFactor(energyDepositedTotal);
      evis /= (1.0 + _scintillatorBirksConstant*correctionFactor);
    }
  }
  else 
  {
    // protections
    double nloss = energyDepositedNonIonizing;
    if(nloss < 0.0) nloss = 0.0;
    double eloss = energyDepositedTotal - nloss;

    // neutrons
    if(PDGcode==2112 || eloss < 0.0 || stepLength <= 0.0) 
    {
      nloss = energyDepositedTotal;
      eloss = 0.0;
    }

    // continues energy loss
    if(eloss > 0.0) 
    { 
      eloss /= (1.0 + _scintillatorBirksConstant*eloss/stepLength); 
    }

    evis = eloss + nloss;
  }

/*
  std::cout<<"PDGcode: "<<PDGcode<<std::endl;
  std::cout<<"Original Energy Deposition (manual): "<<energyDepositedTotal<<std::endl;
  std::cout<<"Original Nonionizing Energy Deposition (manual): "<<energyDepositedNonIonizing<<std::endl;
  std::cout<<"Visible Energy Deposition (manual): "<<evis<<std::endl;
*/

  return evis;
}

void MakeCrvPhotons::LoadVisibleEnergyAdjustmentTable(const std::string &filename)
{
  std::ifstream visibleEnergyAdjustmentFile(filename);
  if(!visibleEnergyAdjustmentFile.good()) throw std::logic_error("Could not open visible energy correction table file "+filename);
  double lnEnergy, factor;
  while(visibleEnergyAdjustmentFile >> lnEnergy >> factor)
  {
    _visibleEnergyAdjustmentTable[lnEnergy]=factor;
  } 
  visibleEnergyAdjustmentFile.close();
}

double MakeCrvPhotons::FindVisibleEnergyAdjustmentFactor(double energy)
{
  if(_visibleEnergyAdjustmentTable.size()==0) throw std::logic_error("Found no visible energy correction table.");

  double lnEnergy=log(energy);
  double correctionFactor=(--_visibleEnergyAdjustmentTable.end())->second;

  std::map<double,double>::const_iterator iter=_visibleEnergyAdjustmentTable.begin();
  if(lnEnergy<iter->first) correctionFactor=iter->second;
  else
  {
    double prevLnEnergy=iter->first;
    double prevFactor=iter->second;
    iter++;
    for(; iter!=_visibleEnergyAdjustmentTable.end(); iter++)
    {
      if(lnEnergy<iter->first)
      {
        double r=(lnEnergy-prevLnEnergy)/(iter->first-prevLnEnergy);
        correctionFactor=(iter->second-prevFactor)*r + prevFactor;
        break;
      } 
      prevLnEnergy=iter->first;
      prevFactor=iter->second;
    }
  }

  return correctionFactor;
}

} //namespace mu2e