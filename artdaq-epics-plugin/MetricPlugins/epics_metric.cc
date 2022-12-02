// epics_metric.cc: Epics Metric Plugin
// Author: Eric Flumerfelt
// Last Modified: 09/25/2015
//
// An implementation of the MetricPlugin interface for Epics/ChannelAccess

#ifndef __EPICS_METRIC__
#define __EPICS_METRIC__ 1

#include "TRACE/tracemf.h"  // order matters -- trace.h (no "mf") is nested from MetricMacros.hh
#define TRACE_NAME (app_name_ + "_epics_metric").c_str()

#include <unordered_map>
#include <utility>
#include "artdaq-utilities/Plugins/MetricMacros.hh"
#include "messagefacility/MessageLogger/MessageLogger.h"
#undef STATIC_ASSERT

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif __GNUC__ > 9
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif
#include <cadef.h>
#ifdef __clang__
#pragma clang diagnostic pop
#elif __GNUC__ > 9
#pragma GCC diagnostic pop
#endif

/**
 * \brief The artdaq namespace
 */
namespace artdaq {
/**
 * \brief An instance of the MetricPlugin class that sends metric data using the Channel Access protocol from EPICS.
 */
class EpicsMetric : public MetricPlugin
{
private:
	std::string prefix_;
	std::unordered_map<std::string, chid> channels_;

	bool checkChannel_(const std::string& name)
	{
		if (channels_.count(name) == 0u)
		{
			chid channel;
			ca_search(name.c_str(), &channel);
			auto sts = ca_pend_io(5.0);
			if (sts != ECA_NORMAL)
			{
				SEVCHK(ca_clear_channel(channel), NULL);
				METLOG(TLVL_WARNING) << "Channel \"" << name << "\" not found!";
				channels_[name] = nullptr;
				return false;
			}
			channels_[name] = channel;
			return true;
		}
		return channels_[name] != nullptr;
	}

	std::string parseChannelName_(std::string prefix_, std::string name)
	{
		std::string caName = std::move(name);
		const std::string& caPrefix_ = std::move(prefix_);

		while (caName.find(' ') != std::string::npos)
		{
			caName = caName.replace(caName.find(' '), 1, "_");
		}
		while (caName.find('.') != std::string::npos)
		{
			caName = caName.replace(caName.find('.'), 1, ":");
		}
		while (caName.find('/') != std::string::npos)
		{
			caName = caName.replace(caName.find('/'), 1, "_");
		}
		while (caName.find("_%") != std::string::npos)
		{
			caName = caName.replace(caName.find("_%"), 2, "");
		}
		caName = caPrefix_ + ":" + caName;

		TLOG(TLVL_DEBUG + 32) << "Channel name is: \"" << caName << "\"";
		return caName;
	}

	EpicsMetric(EpicsMetric const&) = delete;
	EpicsMetric(EpicsMetric&&) = delete;
	EpicsMetric& operator=(EpicsMetric const&) = delete;
	EpicsMetric& operator=(EpicsMetric&&) = delete;

public:
	/**
   * \brief Construct an instance of the EpicsMetric plugin.
   * \param pset Parameter set to configure with. MetricPlugin parameters plus "channel_name_prefix", default "artdaq".
   * \param app_name Name of the application sending metrics
   * \param metric_name Name of this metric instance
   */
	explicit EpicsMetric(fhicl::ParameterSet const& pset, std::string const& app_name, std::string const& metric_name)
	    : MetricPlugin(pset, app_name, metric_name), prefix_(pset.get<std::string>("channel_name_prefix", "artdaq")), channels_() {}

	~EpicsMetric() override { MetricPlugin::stopMetrics(); }

	/**
   * \brief Gets the unique library name of this plugin
   * \return The library name of this plugin, "epics".
   */
	std::string getLibName() const override { return "epics"; }

	/**
   * \brief Clears the registered ChannelAccess channels.
   */
	void stopMetrics_() override
	{
		for (const auto& channel : channels_)
		{
			if (channel.second != 0)
			{
				SEVCHK(ca_clear_channel(channel.second), NULL);
			}
		}
		channels_.clear();
	}

	/**
   * \brief No initialization is needed to start sending metrics.
   */
	void startMetrics_() override {}

	/**
   * \brief Send a string metric data point to ChannelAccess.
   * \param name Name of the metric
   * \param value Value of the metric
   * \param unit Units used (not really relevant for string metrics)
   *
   * Send a string metric data point to ChannelAccess. The name will be channel_name_prefix:name.
   * If the named channel is not yet open, it will be opened. If the channel is not registered with an
   * IOC, then the metric data will not be sent and a warning message will be printed the first time.
   */
	void sendMetric_(const std::string& name, const std::string& value, const std::string& unit, const std::chrono::system_clock::time_point&) override
	{
		//std::string caName = prefix_ + ":" + name;
		std::string caName = parseChannelName_(prefix_, name);

		std::string tmpValue = value + " " + unit;

		if (checkChannel_(caName))
		{
			// DBR_STRING, 40 characters
			if (tmpValue.size() > 40)
			{
				tmpValue = tmpValue.erase(40);
			}
			TLOG(TLVL_DEBUG) << "Putting value " << tmpValue << " into " << caName;
			SEVCHK(ca_put(DBR_STRING, channels_[caName], tmpValue.c_str()), NULL);
			SEVCHK(ca_flush_io(), NULL);
		}
	}

	/**
   * \brief Send an integer metric data point to ChannelAccess.
   * \param name Name of the metric
   * \param value Value of the metric
   * \param unit Units used
   *
   * Send a string metric data point to ChannelAccess. The name will be channel_name_prefix:name.
   * If the named channel is not yet open, it will be opened. If the channel is not registered with an
   * IOC, then the metric data will not be sent and a warning message will be printed the first time.
   */
	void sendMetric_(const std::string& name, const int& value, const std::string& unit, const std::chrono::system_clock::time_point&) override
	{
		// DBR_LONG
		//std::string caName = prefix_ + ":" + name;
		std::string caName = parseChannelName_(prefix_, name);

		if (!unit.empty())
		{
			METLOG(TLVL_DEBUG + 32) << "Not sure if I can send ChannelAccess Units...configure in db instead.";
		}

		if (checkChannel_(caName))
		{
			auto val = static_cast<dbr_long_t>(value);
			TLOG(TLVL_DEBUG) << "Putting value " << value << " into " << caName;
			SEVCHK(ca_put(DBR_LONG, channels_[caName], &val), NULL);
			SEVCHK(ca_flush_io(), NULL);
		}
	}

	/**
   * \brief Send a double metric data point to ChannelAccess.
   * \param name Name of the metric
   * \param value Value of the metric
   * \param unit Units used
   *
   * Send a string metric data point to ChannelAccess. The name will be channel_name_prefix:name.
   * If the named channel is not yet open, it will be opened. If the channel is not registered with an
   * IOC, then the metric data will not be sent and a warning message will be printed the first time.
   */
	void sendMetric_(const std::string& name, const double& value, const std::string& unit, const std::chrono::system_clock::time_point&) override
	{
		// DBR_DOUBLE
		//std::string caName = prefix_ + ":" + name;
		std::string caName = parseChannelName_(prefix_, name);

		if (!unit.empty())
		{
			METLOG(TLVL_DEBUG + 32) << "Not sure if I can send ChannelAccess Units...configure in db instead.";
		}

		if (checkChannel_(caName))
		{
			auto val = static_cast<dbr_double_t>(value);
			TLOG(TLVL_DEBUG) << "Putting value " << value << " into " << caName;
			SEVCHK(ca_put(DBR_DOUBLE, channels_[caName], &val), NULL);
			SEVCHK(ca_flush_io(), NULL);
		}
	}

	/**
   * \brief Send a float metric data point to ChannelAccess.
   * \param name Name of the metric
   * \param value Value of the metric
   * \param unit Units used
   *
   * Send a string metric data point to ChannelAccess. The name will be channel_name_prefix:name.
   * If the named channel is not yet open, it will be opened. If the channel is not registered with an
   * IOC, then the metric data will not be sent and a warning message will be printed the first time.
   */
	void sendMetric_(const std::string& name, const float& value, const std::string& unit, const std::chrono::system_clock::time_point&) override
	{
		// DBR_FLOAT
		//std::string caName = prefix_ + ":" + name;
		std::string caName = parseChannelName_(prefix_, name);

		if (!unit.empty())
		{
			METLOG(TLVL_DEBUG + 32) << "Not sure if I can send ChannelAccess Units...configure in db instead.";
		}

		if (checkChannel_(caName))
		{
			auto val = static_cast<dbr_float_t>(value);
			TLOG(TLVL_DEBUG) << "Putting value " << value << " into " << caName;
			SEVCHK(ca_put(DBR_FLOAT, channels_[caName], &val), NULL);
			SEVCHK(ca_flush_io(), NULL);
		}
	}

	/**
   * \brief Send an unsigned integer metric data point to ChannelAccess.
   * \param name Name of the metric
   * \param value Value of the metric. Will be truncated t fit in the size of a dbr_ulong_t, a 32-bit unsigned integer.
   * \param unit Units used
   *
   * Send a string metric data point to ChannelAccess. The name will be channel_name_prefix:name.
   * If the named channel is not yet open, it will be opened. If the channel is not registered with an
   * IOC, then the metric data will not be sent and a warning message will be printed the first time.
   */
	void sendMetric_(const std::string& name, const uint64_t& value, const std::string& unit, const std::chrono::system_clock::time_point&) override
	{
		// DBR_LONG, only unsigned type is only 16 bits, use widest integral field
		//std::string caName = prefix_ + ":" + name;
		std::string caName = parseChannelName_(prefix_, name);

		if (!unit.empty())
		{
			METLOG(TLVL_DEBUG + 32) << "Not sure if I can send ChannelAccess Units...configure in db instead.";
		}

		if (checkChannel_(caName))
		{
			auto val = static_cast<dbr_ulong_t>(value);
			TLOG(TLVL_DEBUG) << "Putting value " << value << " into " << caName;
			SEVCHK(ca_put(DBR_LONG, channels_[caName], &val), NULL);
			SEVCHK(ca_flush_io(), NULL);
		}
	}
};
}  // End namespace artdaq

DEFINE_ARTDAQ_METRIC(artdaq::EpicsMetric)

#endif  // End ifndef __EPICS_METRIC__
