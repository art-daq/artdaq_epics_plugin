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
#include <filesystem>
#include <fstream>
#include <wordexp.h>
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


namespace fs = std::filesystem;
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
	bool running_;
    std::string dbg_out_name_; // file to write append all used channels to, if dbg_out_name_ != ""

	bool checkChannel_(const std::string& name)
	{
		if (channels_.count(name) == 0u)
		{
            addChannelToDbg(name);
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

		METLOG(TLVL_DEBUG + 32) << "Channel name is: \"" << caName << "\"";
		return caName;
	}

    void addChannelToDbg(const std::string& name) {
        if(dbg_out_name_ == "") return;
        METLOG(TLVL_WARNING) << "Adding \"" + name + "\" to \"" << dbg_out_name_ << "\"." << std::endl;
        // check if we can parse the name, needs at least 3 parts
        // if 4 parts, assume the first part is a prefix that can be dropped
        // if more than 4 parts, assume 1 sybsystem, 1 varname, and the rest is location
        std::vector<size_t> parts;
        size_t pos = 0;
        while ((pos = name.find(":", pos)) != std::string::npos) {
            parts.push_back(pos);
            pos++;  
        }
        if(parts.size() < 2) {
            METLOG(TLVL_ERROR) << "Can't parse \"" << name << "\" for dbg file, at least 3 parts spearated with \":\" are expected.";
            return;
        }
        std::string subsystem = name.substr(((parts.size() == 2) ? 0 : (parts[0]+1)),            
                                            ((parts.size() == 2) ? (parts[0]) : (parts[1]-parts[0]-1)));
        std::string loc       = name.substr(((parts.size() == 2) ? (parts[0]+1) : (parts[1]+1)), 
                                            (parts.size() == 2) ? (parts[parts.size()-1]-parts[0]-1) : (parts[parts.size()-1]-parts[1]-1)); 
        std::string pvar      = name.substr((parts[parts.size()-1]+1));

        std::fstream file(parseDbgOutName_(dbg_out_name_), std::ios::in | std::ios::out | std::ios::ate);
        if(file) {
            std::streamoff currentPos = file.tellp();
            char ch;
            do {
                currentPos--;
                file.seekg(currentPos);
                ch = file.get();
            } while (ch != '\n' && currentPos > 0);
            if (ch == '\n') {
                file.seekp(currentPos + 1, std::ios::beg);
                //file.truncate();
                file << "           {\"" << subsystem << "\", \"" << loc << "\", \"" << pvar << "\", ";
                file << "\"0\", "; // PREC: precision
                file << "\"\", "; // EGU: engineering units
                file << "\"\", \"\", \"\", \"\", "; // LOLO, LOW, HIGH, HIHI
                file << "\"\", \"\", \"\", \"\", \"\", "; // MDEL (monitor dead band), ADEL (archive deadband), INP (input link), SCAN (), DTYP (device type)
                file << "\"" << app_name_ << "\"}" << std::endl; //DESC descriptions
                file << "}";
            }
            file.close(); 
        }
    }

    std::string parseDbgOutName_(const std::string& name) {
    wordexp_t p;
    if (wordexp(name.c_str(), &p, WRDE_NOCMD) == 0) {
        std::string expanded = p.we_wordv[0];
        wordfree(&p);
        return expanded;
    } else {
        return name;
    }
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
	    : MetricPlugin(pset, app_name, metric_name)
        , prefix_(pset.get<std::string>("channel_name_prefix", "artdaq"))
        , channels_()
        , running_(0)
        , dbg_out_name_(pset.get<std::string>("dbg_out_name", "")) {
		METLOG(TLVL_DEBUG + 30) << "EpicsMetric CONSTRUCTOR";

        if(dbg_out_name_ != "") {
            // if the file doesn't exist, write header
            //if (!fs::exists(dbg_out_name_)) {:
                METLOG(TLVL_DEBUG) << "Creating " << dbg_out_name_ << " :: " << parseDbgOutName_(dbg_out_name_) << std::endl;
                std::ofstream file(parseDbgOutName_(dbg_out_name_));
                if (file.is_open()) { 
                    file << "file \"dbt/subst_ai.dbt\" {" << std::endl;
                    file << "    pattern { Subsystem, loc, pvar, "; // used
                    file << "PREC, EGU, LOLO, LOW, HIGH, HIHI, MDEL, ADEL, INP, SCAN, DTYP, DESC }" << std::endl;
                    file << "}";
                    file.close();
                } else {
                    METLOG(TLVL_WARNING) << "Failed to create '"+dbg_out_name_+"'";
                }
            //}
        }
	}

	~EpicsMetric() override
	{
		TLOG(TLVL_INFO) << "EPICS Metric Destructor";
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
		TLOG(TLVL_INFO) << "EpicsMetric::stopMetrics_";
		for (const auto& channel : channels_)
		{
			if (channel.second != 0)
			{
				TLOG(TLVL_INFO) << "Clearing channel " << channel.first;
				SEVCHK(ca_clear_channel(channel.second), NULL);
			}
		}
		channels_.clear();
		if (running_) ca_context_destroy();  // destroy blocks if the context is already gone/not running
		running_ = false;
	}

	/**
	 * \brief No initialization is needed to start sending metrics.
	 */
	void startMetrics_() override
	{
		SEVCHK(ca_context_create(ca_enable_preemptive_callback), NULL);
		running_ = true;
	}

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
		// std::string caName = prefix_ + ":" + name;
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
		// std::string caName = prefix_ + ":" + name;
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
		// std::string caName = prefix_ + ":" + name;
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
		// std::string caName = prefix_ + ":" + name;
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
		// std::string caName = prefix_ + ":" + name;
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
