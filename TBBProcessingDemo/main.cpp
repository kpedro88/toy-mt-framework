#include <iostream>
#include <assert.h>
#include <cmath>
#include <boost/property_tree/json_parser.hpp>
#include <sstream>
#include "tbb/task_scheduler_init.h"
#include <memory>
#include "Queues.h"
#include "EventProcessor.h"

#include "SimpleSource.h"
#include "FactoryManager.h"
#include "Producer.h"
#include "Filter.h"

#include "busy_wait_scale_factor.h"
#include <sys/time.h>

#if !defined(__APPLE__)
#include <sys/resource.h>
#endif

#include <errno.h>


int main (int argc, char * const argv[]) {
  
  //demo::s_thread_safe_queue = dispatch_get_global_queue(0, 0);
  //demo::s_non_thread_safe_queue_stack.push_back(dispatch_queue_create("gov.fnal.non_thread_safe_0", NULL));
  //demo::s_non_thread_safe_queue_stack_back = demo::s_non_thread_safe_queue_stack.back();
 // demo::s_non_thread_safe_queue = dispatch_queue_create("gov.fnal.non_thread_safe", NULL);
  //dispatch_retain(demo::s_non_thread_safe_queue);
  
  assert(argc==2);
  boost::property_tree::ptree pConfig; 
  try {
    read_json(argv[1],pConfig);
  } catch(const std::exception& iE) {
    std::cout <<iE.what()<<std::endl;
    exit(1);
  }
  const unsigned int nThreads = pConfig.get<unsigned int>("process.options.nThreads",tbb::task_scheduler_init::default_num_threads());
  std::unique_ptr<tbb::task_scheduler_init> tsi{new tbb::task_scheduler_init(nThreads)};
  
  const size_t iterations = pConfig.get<size_t>("process.source.iterations");
  demo::EventProcessor ep;
  ep.setSource(new demo::SimpleSource( iterations) );
  
  
  busy_wait_scale_factor = pConfig.get<double>("process.options.busyWaitScaleFactor",2.2e+07);
  
  boost::property_tree::ptree& filters=pConfig.get_child("process.filters");
  for(const boost::property_tree::ptree::value_type& f : filters) {
    std::unique_ptr<demo::Filter> pF = demo::FactoryManager<demo::Filter>::create(f.second.get<std::string>("@type"),f.second);
    std::cout << f.second.get<std::string>("@type")<<" "<<f.second.get<std::string>("@label")<<" "<<std::hex<<pF.get()<<std::dec<<std::endl;
    if(pF.get() != 0) {
      ep.addFilter(pF.release());
    } else {
      assert(0=="Failed creating filter");
      exit(1);
    }
  }

  boost::property_tree::ptree& producers=pConfig.get_child("process.producers");
  for(const boost::property_tree::ptree::value_type& p : producers) {
    std::unique_ptr<demo::Producer> pP = demo::FactoryManager<demo::Producer>::create(p.second.get<std::string>("@type"),p.second);
    std::cout << p.second.get<std::string>("@type")<<" "<<p.second.get<std::string>("@label")<<" "<<std::hex<<pP.get()<<std::dec<<std::endl;
    if(pP.get() != 0) {
      ep.addProducer(pP.release());
    } else {
      assert(0=="Failed creating filter");
      exit(1);
    }
  }

  
  boost::property_tree::ptree& paths=pConfig.get_child("process.paths");
  for(const boost::property_tree::ptree::value_type& p : paths) {
    std::string n = p.first;
    std::vector<std::string> modules;
    for(const boost::property_tree::ptree::value_type& m: p.second) {
      //std::cout <<m.first<<" - "<<m.second.data()<<std::endl;
      modules.push_back(m.second.data());
    }
    ep.addPath(n,modules);
  }

  ep.finishSetup();
  {
    struct timeval startCPUTime;
    
    rusage theUsage;
    if(0 != getrusage(RUSAGE_SELF, &theUsage)) {
      std::cerr<<errno;
      return 1;
    }
    startCPUTime.tv_sec =theUsage.ru_stime.tv_sec+theUsage.ru_utime.tv_sec;
    startCPUTime.tv_usec =theUsage.ru_stime.tv_usec+theUsage.ru_utime.tv_usec;

    struct timeval startRealTime;
    gettimeofday(&startRealTime, 0);
    
    //ep.processAll(2);
    unsigned int nEvents = pConfig.get<unsigned int>("process.options.nSimultaneousEvents");
    try {
       ep.processAll(nEvents);
    } catch(std::exception const& iException) {
       std::cerr<<"An exception was thrown: "<<iException.what()<<"\n";
       return 1;
    }
    
    struct timeval tp;
    gettimeofday(&tp, 0);

    if(0 != getrusage(RUSAGE_SELF, &theUsage)) {
      std::cerr<<errno;
      return 1;
    }
    
    
    double const microsecToSec = 1E-6;
    
    double cpuTime = theUsage.ru_stime.tv_sec + theUsage.ru_utime.tv_sec - startCPUTime.tv_sec +
    microsecToSec * (theUsage.ru_stime.tv_usec + theUsage.ru_utime.tv_usec - startCPUTime.tv_usec);
   
    double realTime = tp.tv_sec - startRealTime.tv_sec + microsecToSec * (tp.tv_usec - startRealTime.tv_usec);
    std::cout <<"# threads:"<<nThreads<<" # simultaneous events:"<<nEvents<<" total # events:"<<iterations<<" cpu time:" << cpuTime<<" real time:"<<realTime<<" events/sec:"<<iterations/realTime<<std::endl;
  }
  tsi.release();

  return 0;
}
