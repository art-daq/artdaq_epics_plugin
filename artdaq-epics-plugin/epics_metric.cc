//epics_metric.cc: Epics Metric Plugin
// Author: Eric Flumerfelt
// Last Modified: 09/25/2015
//
// An implementation of the MetricPlugin interface for Epics/ChannelAccess

#ifndef __EPICS_METRIC__
#define __EPICS_METRIC__ 1

#include "artdaq-utilities/Plugins/MetricMacros.hh"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include <unordered_map>
#undef STATIC_ASSERT
#include "cadef.h"

namespace artdaq {
class EpicsMetric : public MetricPlugin
{
private:
	std::string prefix_;
	bool uniquify_;
	std::unordered_map<std::string, chid> channels_;

	bool checkChannel_(std::string name)
	{
		if (!channels_.count(name))
		{
			chid channel;
			int sts = ca_search(name.c_str(), &channel);
			sts = ca_pend_io(5.0);
			if (sts != ECA_NORMAL)
			{
				SEVCHK(ca_clear_channel(channel), NULL);
				mf::LogWarning("EPICS Plugin") << "Channel " << name << " not found!";
				channels_[name] = nullptr;
				return false;
			}
			channels_[name] = channel;
			return true;
		}
		return channels_[name] != nullptr;
	}

public:
	EpicsMetric(fhicl::ParameterSet pset)
		: MetricPlugin(pset)
		, prefix_(pset.get<std::string>("channel_name_prefix", "artdaq"))
		, uniquify_(pset.get<bool>("unique_channel_names", false))
		, channels_()
	{}

	virtual ~EpicsMetric()
	{
		stopMetrics();
	}

	std::string getLibName() { return "epics"; }
	void stopMetrics_()
	{
		for (auto channel : channels_)
		{
			SEVCHK(ca_clear_channel(channel.second), NULL);
		}
		channels_.clear();
	}
	void startMetrics_() {}

	void sendMetric_(std::string name, std::string value, std::string unit)
	{
		std::string caName = prefix_ + ":" + name + (uniquify_ ? "_" + std::to_string(getpid()) : "");
		std::string tmpValue = value + " " + unit;

		if (checkChannel_(caName))
		{
			//DBR_STRING, 40 characters
			if (tmpValue.size() > 40) { tmpValue = tmpValue.erase(40); }
			SEVCHK(ca_put(DBR_STRING, channels_[caName], tmpValue.c_str()), NULL);
			SEVCHK(ca_flush_io(), NULL);
		}
	}
	void sendMetric_(std::string name, int value, std::string unit)
	{
		//DBR_LONG
		std::string caName = prefix_ + ":" + name + (uniquify_ ? "_" + std::to_string(getpid()) : "");
		if (unit.size() > 0)
		{
			mf::LogDebug("EPICS Plugin") << "Not sure if I can send ChannelAccess Units...configure in db instead.";
		}

		if (checkChannel_(caName))
		{
			dbr_long_t val = static_cast<dbr_long_t>(value);
			SEVCHK(ca_put(DBR_LONG, channels_[caName], &val), NULL);
			SEVCHK(ca_flush_io(), NULL);
		}
	}
	void sendMetric_(std::string name, double value, std::string unit)
	{
		//DBR_DOUBLE
		std::string caName = prefix_ + ":" + name + (uniquify_ ? "_" + std::to_string(getpid()) : "");
		if (unit.size() > 0)
		{
			mf::LogDebug("EPICS Plugin") << "Not sure if I can send ChannelAccess Units...configure in db instead.";
		}

		if (checkChannel_(caName))
		{
			dbr_double_t val = static_cast<dbr_double_t>(value);
			SEVCHK(ca_put(DBR_DOUBLE, channels_[caName], &val), NULL);
			SEVCHK(ca_flush_io(), NULL);
		}
	}
	void sendMetric_(std::string name, float value, std::string unit)
	{
		//DBR_FLOAT
		std::string caName = prefix_ + ":" + name + (uniquify_ ? "_" + std::to_string(getpid()) : "");
		if (unit.size() > 0)
		{
			mf::LogDebug("EPICS Plugin") << "Not sure if I can send ChannelAccess Units...configure in db instead.";
		}

		if (checkChannel_(caName))
		{
			dbr_float_t val = static_cast<dbr_float_t>(value);
			SEVCHK(ca_put(DBR_FLOAT, channels_[caName], &val), NULL);
			SEVCHK(ca_flush_io(), NULL);
		}
	}
	void sendMetric_(std::string name, unsigned long int value, std::string unit)
	{
		//DBR_LONG, only unsigned type is only 16 bits, use widest integral field
		std::string caName = prefix_ + ":" + name + (uniquify_ ? "_" + std::to_string(getpid()) : "");
		if (unit.size() > 0)
		{
			mf::LogDebug("EPICS Plugin") << "Not sure if I can send ChannelAccess Units...configure in db instead.";
		}

		if (checkChannel_(caName))
		{
			dbr_long_t val = static_cast<dbr_long_t>(value);
			SEVCHK(ca_put(DBR_LONG, channels_[caName], &val), NULL);
			SEVCHK(ca_flush_io(), NULL);
		}
	}
};
} //End namespace artdaq

DEFINE_ARTDAQ_METRIC(artdaq::EpicsMetric)

#endif //End ifndef __EPICS_METRIC__
