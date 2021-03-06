//
//  ModuleWrapper.h
//  DispatchProcessingDemo
//
//  Created by Chris Jones on 10/8/11.
//  Copyright 2011 FNAL. All rights reserved.
//

#ifndef DispatchProcessingDemo_ModuleWrapper_h
#define DispatchProcessingDemo_ModuleWrapper_h
#include <dispatch/dispatch.h>
#include <atomic>
#include "GroupHolder.h"

namespace demo {
  class Module;
  class Event;

  class ModuleWrapper {
  public:
    ModuleWrapper(Module* iModule,
                  Event* iEvent);
    ~ModuleWrapper();
    
    Module* module() const {
      return m_module;
    }

    Event* event() const {
      return m_event;
    }
    
    void reset() {
      m_requestedPrefetch.clear();
    }

    void prefetchAsync();

    dispatch_queue_t runQueue() const {
      return m_runQueue;
    }

    dispatch_group_t prefetchGroup() const {
      return m_prefetchGroup.get();
    }
    
  protected:
    ModuleWrapper(const ModuleWrapper&, Event*);
    ModuleWrapper(const ModuleWrapper&);
    ModuleWrapper& operator=(const ModuleWrapper&);
    
  private:
    static void do_prefetch_task(void* iContext);
    void doPrefetch();
    
    
    Module* m_module;
    Event* m_event;
    GroupHolder m_prefetchGroup;
    dispatch_queue_t m_runQueue;
    std::atomic_flag m_requestedPrefetch;

  };
  
};


#endif
