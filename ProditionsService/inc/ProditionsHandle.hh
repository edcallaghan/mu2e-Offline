#ifndef ProditionService_ProditionsHandle_hh
#define ProditionService_ProditionsHandle_hh

//
// A safe pointer to a ProditionsEntity
//

#include <string>
#include <boost/type_index.hpp>
#include "canvas/Persistency/Provenance/EventID.h"
#include "ProditionsService/inc/ProditionsService.hh"
#include "DbTables/inc/DbIoV.hh"

namespace mu2e {
  template <typename ENTITY>
  class ProditionsHandle
  {
  public:

    typedef std::shared_ptr<ENTITY> ptr_t;
    typedef std::shared_ptr<const ENTITY> cptr_t;

    ProditionsHandle():ptr(nullptr) {
      // find the name of the ENTITY and strip the namespace
      _name =  boost::typeindex::type_id<ENTITY>().pretty_name();
      if(_name.find("::")!=std::string::npos) _name=_name.erase(0,_name.find("::")+2);
      // connect to the service cache of this type
      art::ServiceHandle<ProditionsService> sg;
      _cptr = sg->getCache(_name);

      if(!_cptr) {
	throw cet::exception("PRODITIONSHANDLE_NO_CACHE")
	  << "ProditionsHandle could not get cache " << _name
	  << " from ProditionsService ";
      }

    }
    ~ProditionsHandle() { }

    ENTITY const& get(art::RunID const& rid) { 
      return get(art::EventID(rid.run(),0,0));
    }
    ENTITY const& get(art::SubRunID const& sid) { 
      return get(art::EventID(sid,0));
    }
    cptr_t getPtr(art::EventID const& eid) { 
      get(eid);
      return ptr;
    }
    ENTITY const& get(art::EventID const& eid) { 

      uint32_t r = eid.run();
      uint32_t s = eid.subRun();
      if(!_iov.inInterval(r,s)) {
	ProditionsEntity::ptr bptr;
	std::tie(bptr,_iov) = _cptr->update(eid);
	ptr = std::dynamic_pointer_cast
	  <const ENTITY,const ProditionsEntity>(bptr);
      }

      if(!ptr) {
	throw cet::exception("PRODITIONSHANDLE_NO_ENTITY") 
	  << "ProditionsHandle could not load entity " << _name
	  << " for Run "<<eid.run() << " SubRun " << eid.subRun();
      }

      return *ptr; 

    }

    DbIoV const& iov() const { return _iov;}

  private:
    ProditionsCache::ptr _cptr;
    cptr_t ptr;
    std::string _name;
    DbIoV _iov;
  };
}

#endif
