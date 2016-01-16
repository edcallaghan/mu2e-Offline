#ifndef CalorimeterGeom_HexMapper_hh
#define CalorimeterGeom_HexMapper_hh


#include "CalorimeterGeom/inc/CrystalMapper.hh"

#include <vector>
#include "CLHEP/Vector/TwoVector.h"



namespace mu2e {

    class HexLK {        

         public:

            HexLK()             : _l(0),_k(0) {}
            HexLK(int l, int k) : _l(l),_k(k) {}
            
	    void add(const HexLK &x) {_l+=x._l;_k+=x._k;}

            int _l; 
	    int _k;                    

    };




    class HexMapper : public CrystalMapper {


	public:

	    HexMapper();

            int               nCrystalMax(int maxRing)                    {return 1 + 3*maxRing*(maxRing+1);}
            int               nApex()                               const {return _apexX.size();}
            double            apexX(int i)                          const {return _apexX.at(i);}
            double            apexY(int i)                          const {return _apexY.at(i);}


	    CLHEP::Hep2Vector xyFromIndex(int thisIndex)            const;
            int               indexFromXY(double x, double y)       const;

	    std::vector<int>  neighbors(int thisIndex, unsigned int level=1) const;


	private:

	    HexLK        lk(int index) const;
	    int index(const HexLK &lk) const;
	    int ring(const HexLK &lk)  const;

	    std::vector<HexLK>   _step;
	    std::vector<double>  _apexX;
	    std::vector<double>  _apexY;
    };

}
#endif

