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
#include <cadef.h>

/**
 * \brief The artdaq namespace
 */
namespace artdaq
{
	/**
	 * \brief An instance of the MetricPlugin class that sends metric data using the Channel Access protocol from EPICS.
	 */
	class EpicsMetric : public MetricPlugin
	{
	private:
		std::string prefix_;
		std::unordered_map<std::string, chid> channels_;

		bool checkChannel_(std::string name)
		{
			if (!channels_.count(name))
			{
				chid channel;
				auto sts = ca_search(name.c_str(), &channel);
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
		/**
		 * \brief Construct an instance of the EpicsMetric plugin.
		 * \param pset Parameter set to configure with. MetricPlugin parameters plus "channel_name_prefix", default "artdaq".
		 */
		explicit EpicsMetric(fhicl::ParameterSet pset)
			: MetricPlugin(pset)
			, prefix_(pset.get<std::string>("channel_name_prefix", "artdaq"))
			, channels_() {}

		virtual ~EpicsMetric()
		{
			MetricPlugin::stopMetrics();
		}

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
			for (auto channel : channels_)
			{
				SEVCHK(ca_clear_channel(channel.second), NULL);
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
		void sendMetric_(const std::string& name, const std::string& value, const std::string& unit) override
		{
			std::string caName = prefix_ + ":" + name;
			std::string tmpValue = value + " " + unit;

			if (checkChannel_(caName))
			{
				//DBR_STRING, 40 characters
				if (tmpValue.size() > 40) { tmpValue = tmpValue.erase(40); }
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
		void sendMetric_(const std::string& name, const int& value, const std::string& unit) override
		{
			//DBR_LONG
			std::string caName = prefix_ + ":" + name;
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
		void sendMetric_(const std::string& name, const double& value, const std::string& unit) override
		{
			//DBR_DOUBLE
			std::string caName = prefix_ + ":" + name;
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
		void sendMetric_(const std::string& name, const float& value, const std::string& unit) override
		{
			//DBR_FLOAT
			std::string caName = prefix_ + ":" + name;
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
		void sendMetric_(const std::string& name, const unsigned long int& value, const std::string& unit) override
		{
			//DBR_LONG, only unsigned type is only 16 bits, use widest integral field
			std::string caName = prefix_ + ":" + name;
			if (unit.size() > 0)
			{
				mf::LogDebug("EPICS Plugin") << "Not sure if I can send ChannelAccess Units...configure in db instead.";
			}

			if (checkChannel_(caName))
			{
				dbr_ulong_t val = static_cast<dbr_ulong_t>(value);
				SEVCHK(ca_put(DBR_LONG, channels_[caName], &val), NULL);
				SEVCHK(ca_flush_io(), NULL);
			}
		}
	};
} //End namespace artdaq

DEFINE_ARTDAQ_METRIC(artdaq::EpicsMetric)

#endif //End ifndef __EPICS_METRIC__