//
//  Schedule.h
//  DispatchProcessingDemo
//
//  Created by Chris Jones on 9/18/11.
//  Copyright 2011 FNAL. All rights reserved.
//

#ifndef DispatchProcessingDemo_Schedule_h
#define DispatchProcessingDemo_Schedule_h
#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>
#include "GroupHolder.h"
#include "PathFilteringCallback.h"
#include "Path.h"
#include "Event.h"

namespace demo {
  class Path;
  class Event;
  class Filter;
  class FilterWrapper;
  
  //NOTE: The implementation of ScheduleFilteringCallback is
  // in the EventProcessor since it is the only class which
  // ever requests a callback
  class ScheduleFilteringCallback {
  public:
    explicit ScheduleFilteringCallback(void* iContext=0):
    m_context(iContext) {}
    
    void operator()(bool) const;
  private:
    void* m_context;
  };
  
  
  class Schedule {
  public:
    friend class PathFilteringCallback;
    
    Schedule();
    Schedule(const Schedule&);
    
    void process(ScheduleFilteringCallback iCallback);

    void reset();
    
    void addPath(const std::vector<std::string>& iPath);
    void addFilter(Filter*);
    
    FilterWrapper* findFilter(const std::string&);
    
    void setFatalJobErrorOccurredPointer(bool* iPtr) {
      m_fatalJobErrorOccuredPtr = iPtr;
    }
    
    Event* event();
    Schedule* clone();
    
    struct PathContext {
      PathContext(Schedule* iSchedule, Path* iPath):
      schedule(iSchedule), path(iPath) {}
      Schedule* schedule;
      boost::shared_ptr<Path> path;
    };
    
  private:
    static void processPresentPath(void*);
    static void do_schedule_callback_f(void*);
    static void reset_f(void*, size_t);

    //used for cloning
    Schedule(Event*);
    void addPath(Path* iPath);
    Event m_event;
    std::vector<PathContext> m_paths;
    std::vector<boost::shared_ptr<FilterWrapper> > m_filters;
    GroupHolder m_allPathsDoneGroup;
    PathFilteringCallback m_callback;
    ScheduleFilteringCallback m_scheduleCallback;
    bool* m_fatalJobErrorOccuredPtr;
  };
}


#endif
