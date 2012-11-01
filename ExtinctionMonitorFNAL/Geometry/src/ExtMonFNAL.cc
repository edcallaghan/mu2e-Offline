// Andrei Gaponenko, 2011

#include "ExtinctionMonitorFNAL/Geometry/inc/ExtMonFNAL.hh"

#include <iostream>

//#define AGDEBUG(stuff) std::cerr<<"AG: "<<__FILE__<<", line "<<__LINE__<<": "<<stuff<<std::endl;
#define AGDEBUG(stuff)

namespace mu2e {
  namespace ExtMonFNAL {

    //================================================================
    CLHEP::Hep3Vector ExtMon::detectorCenterInMu2e() const {
      return up_.refPointInMu2e();
    }

    //================================================================
    const CLHEP::HepRotation& ExtMon::detectorRotationInMu2e() const {
      return up_.rotationInMu2e();
    }

    //================================================================
    CLHEP::Hep3Vector ExtMon::mu2eToExtMon_position(const CLHEP::Hep3Vector& mu2epos) const {
      return up_.mu2eToStack_position(mu2epos);
    }

    //================================================================
    CLHEP::Hep3Vector ExtMon::mu2eToExtMon_momentum(const CLHEP::Hep3Vector& mu2emom) const {
      return up_.mu2eToStack_momentum(mu2emom);
    }

    //================================================================
    CLHEP::Hep3Vector ExtMon::extMonToMu2e_position(const CLHEP::Hep3Vector& pos) const {
      return up_.stackToMu2e_position(pos);
    }

    //================================================================
    CLHEP::Hep3Vector ExtMon::extMonToMu2e_momentum(const CLHEP::Hep3Vector& mom) const {
      return up_.stackToMu2e_momentum(mom);
    }

    //================================================================
    CLHEP::Hep3Vector ExtMon::sensorCenterInExtMon(unsigned iplane) const {
      return (iplane < dn_.nplanes())?
        dnStackToExtMon_position(dn_.sensorOffsetInStack(iplane)) :
        up_.sensorOffsetInStack(iplane - dn_.nplanes());
    }

    //================================================================
    CLHEP::Hep3Vector ExtMon::pixelPositionInSensorStack(const ExtMonFNALPixelId& id) const {
      using CLHEP::Hep3Vector;

      const unsigned globalPlane = id.chip().sensor().plane();
      bool downStack = (globalPlane < dn_.nplanes());
      const ExtMonFNALSensorStack& stack = downStack ? dn_ : up_;
      const unsigned stackPlane = downStack ? globalPlane : globalPlane - dn_.nplanes();

      // Position of pixel in the sensor
      CLHEP::Hep2Vector sxy = sensor_.sensorCoordinates(id);

      // Position of the pixel in the stack
      Hep3Vector stackPos = stack.sensorOffsetInStack(stackPlane)
        + Hep3Vector(sxy.x(), sxy.y(), 0);

      return stackPos;
    }

    //================================================================

  } // namespace ExtMonFNAL
} // namespace mu2e
