//epics_metric.cc: Epics Metric Plugin
// Author: Eric Flumerfelt
// Last Modified: 09/25/2015
//
// An implementation of the MetricPlugin interface for Epics/ChannelAccess

#ifndef __EPICS_METRIC__
#define __EPICS_METRIC__ 1

#include "artdaq-utilities/Plugins/MetricMacros.hh"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cadef.h"

namespace artdaq {

  class EpicsMetric : public MetricPlugin {
  private:
    std::string prefix_;

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
      std::string caName = prefix_ + ":" + name;
	  std::string tmpValue = value + " " + unit;

      chid channel;
      int sts = ca_search(caName.c_str(), &channel);
      sts = ca_pend_io(5.0);
      if(sts != ECA_NORMAL){
        SEVCHK(ca_clear_channel(channel),NULL);
		mf::LogWarning("EPICS Plugin") << "Channel " << caName << " not found!";
		return;
      }
      //DBR_STRING, 40 characters
      if(tmpValue.size() > 40) { tmpValue = tmpValue.erase(40); } 
      sts = ca_put(DBR_STRING,channel,tmpValue.c_str());
      SEVCHK(sts, NULL);
      SEVCHK(ca_pend_io(5.0), NULL);
	  SEVCHK(ca_clear_channel(channel),NULL);
    }
    void sendMetric_(std::string name, int value, std::string unit ) 
    {
      //DBR_LONG
      std::string caName = prefix_ + ":" + name;
      if(unit.size() > 0) { 
		mf::LogDebug("EPICS Plugin") << "Not sure if I can send ChannelAccess Units...configure in db instead.";	
	  }

	  chid channel;
	  int sts = ca_search(caName.c_str(), &channel);
	  sts = ca_pend_io(5.0);
	  if(sts != ECA_NORMAL){
		SEVCHK(ca_clear_channel(channel),NULL);
		mf::LogWarning("EPICS Plugin") << "Channel " << caName << " not found!";
		return;
	  }
	  dbr_long_t val = static_cast<dbr_long_t>(value);
	  sts = ca_put(DBR_LONG,channel,&val);
	  SEVCHK(sts, NULL);
	  SEVCHK(ca_pend_io(5.0), NULL);
	  SEVCHK(ca_clear_channel(channel),NULL);
	}
    void sendMetric_(std::string name, double value, std::string unit ) 
    { 
      //DBR_DOUBLE
      std::string caName = prefix_ + ":" + name;
      if(unit.size() > 0) { 
		mf::LogDebug("EPICS Plugin") << "Not sure if I can send ChannelAccess Units...configure in db instead.";	
	  }

	  chid channel;
	  int sts = ca_search(caName.c_str(), &channel);
	  sts = ca_pend_io(5.0);
	  if(sts != ECA_NORMAL){
		SEVCHK(ca_clear_channel(channel),NULL);
		mf::LogWarning("EPICS Plugin") << "Channel " << caName << " not found!";
		return;
	  }
	  dbr_double_t val = static_cast<dbr_double_t>(value);
	  sts = ca_put(DBR_DOUBLE,channel,&val);
	  SEVCHK(sts, NULL);
	  SEVCHK(ca_pend_io(5.0), NULL);
	  SEVCHK(ca_clear_channel(channel),NULL);
    }
	void sendMetric_(std::string name, float value, std::string unit ) 
	{
	  //DBR_FLOAT
      std::string caName = prefix_ + ":" + name;
      if(unit.size() > 0) { 
		mf::LogDebug("EPICS Plugin") << "Not sure if I can send ChannelAccess Units...configure in db instead.";	
	  }

	  chid channel;
	  int sts = ca_search(caName.c_str(), &channel);
	  sts = ca_pend_io(5.0);
	  if(sts != ECA_NORMAL){
		SEVCHK(ca_clear_channel(channel),NULL);
		mf::LogWarning("EPICS Plugin") << "Channel " << caName << " not found!";
		return;
	  }
	  dbr_float_t val = static_cast<dbr_float_t>(value);
	  sts = ca_put(DBR_FLOAT,channel,&val);
	  SEVCHK(sts, NULL);
	  SEVCHK(ca_pend_io(5.0), NULL);
	  SEVCHK(ca_clear_channel(channel),NULL);
	}
	void sendMetric_(std::string name, unsigned long int value, std::string unit ) 
	{ 
	  //DBR_LONG, only unsigned type is only 16 bits, use widest integral field
      std::string caName = prefix_ + ":" + name;
      if(unit.size() > 0) { 
		mf::LogDebug("EPICS Plugin") << "Not sure if I can send ChannelAccess Units...configure in db instead.";	
	  }

	  chid channel;
	  int sts = ca_search(caName.c_str(), &channel);
	  sts = ca_pend_io(5.0);
	  if(sts != ECA_NORMAL){
		SEVCHK(ca_clear_channel(channel),NULL);
		mf::LogWarning("EPICS Plugin") << "Channel " << caName << " not found!";
		return;
	  }
	  dbr_long_t val = static_cast<dbr_long_t>(value);
	  sts = ca_put(DBR_LONG,channel,&val);
	  SEVCHK(sts, NULL);
	  SEVCHK(ca_pend_io(5.0), NULL);
	  SEVCHK(ca_clear_channel(channel),NULL);
	}
  };
} //End namespace artdaq

DEFINE_ARTDAQ_METRIC(artdaq::EpicsMetric)

#endif //End ifndef __EPICS_METRIC__
