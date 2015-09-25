//epics_metric.cc: Epics Metric Plugin
// Author: Eric Flumerfelt
// Last Modified: 09/25/2015
//
// An implementation of the MetricPlugin interface for Epics/ChannelAccess

#ifndef __EPICS_METRIC__
#define __EPICS_METRIC__ 1

#include "artdaq-utilities/Plugins/MetricMacros.hh"
#include "messagefacility/MessageLogger/MessageLogger.h"

namespace artdaq {

  class EpicsMetric : public MetricPlugin {
  private:
    
  public:
    EpicsMetric(fhicl::ParameterSet pset) : MetricPlugin(pset)
    {
    }

    virtual ~EpicsMetric()
    {
      stopMetrics();
    }

    std::string getLibName() { return "epics"; }
    void stopMetrics_() { }
    void startMetrics_() { }

    void sendMetric_(std::string name, std::string value, std::string unit ) 
    {
    }
    void sendMetric_(std::string name, int value, std::string unit ) 
    {
    }
    void sendMetric_(std::string name, double value, std::string unit ) 
    { 
    }
    void sendMetric_(std::string name, float value, std::string unit ) 
    {
    }
    void sendMetric_(std::string name, unsigned long int value, std::string unit ) 
    { 
    }
  };
} //End namespace artdaq

DEFINE_ARTDAQ_METRIC(artdaq::EpicsMetric)

#endif //End ifndef __EPICS_METRIC__
