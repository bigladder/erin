/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */

#ifndef DISCO_DISCO_H
#define DISCO_DISCO_H
#include <string>
#include <vector>
#include <exception>
#include <functional>
#include <map>
#include <unordered_map>
#include "../../vendor/bdevs/include/adevs.h"

namespace DISCO
{
  ////////////////////////////////////////////////////////////
  // Main Class
  class Main
  {
    public:
      Main(std::string input_toml, std::string output_toml);
      bool run();

    private:
      std::string input_file_path;
      std::string output_file_path;
  };

  ////////////////////////////////////////////////////////////
  // Type Definitions
  typedef double FlowValueType;
  typedef int RealTimeType;
  typedef int LogicalTimeType;

  ////////////////////////////////////////////////////////////
  // Utility Functions
  FlowValueType clamp_toward_0(FlowValueType value, FlowValueType lower, FlowValueType upper);
  template<class T>
  void print_vec(const std::string& tag, const std::vector<T>& vs);
  std::string map_to_string(
      const std::unordered_map<std::string,FlowValueType>& m);

  ////////////////////////////////////////////////////////////
  // MixedStreamsError
  struct MixedStreamsError : public std::exception
  {
    virtual const char* what() const throw()
    {
      return "MixedStreamsError";
    }
  };

  ////////////////////////////////////////////////////////////
  // InconsistentStreamUnitsError
  struct InconsistentStreamTypesError : public std::exception
  {
    virtual const char* what() const throw()
    {
      return "InconsistentStreamTypesError";
    }
  };

  ////////////////////////////////////////////////////////////
  // InconsistentStreamUnitsError
  struct InconsistentStreamUnitsError : public std::exception
  {
    virtual const char* what() const throw()
    {
      return "InconsistentStreamUnitsError";
    }
  };

  ////////////////////////////////////////////////////////////
  // FlowInvariantError
  struct FlowInvariantError : public std::exception
  {
    virtual const char* what() const throw()
    {
      return "FlowInvariantError";
    }
  };

  ////////////////////////////////////////////////////////////
  // BadPortError
  struct BadPortError : public std::exception
  {
    virtual const char* what() const throw()
    {
      return "BadPortError";
    }
  };

  ////////////////////////////////////////////////////////////
  // SimultaneousIORequestError
  struct SimultaneousIORequestError : public std::exception
  {
    virtual const char* what() const throw()
    {
      return "SimultaneousIORequestError";
    }
  };

  ////////////////////////////////////////////////////////////
  // AchievedMoreThanRequestedError
  struct AchievedMoreThanRequestedError : public std::exception
  {
    virtual const char* what() const throw()
    {
      return "AchievedMoreThanRequestedError";
    }
  };

  ////////////////////////////////////////////////////////////
  // BadInputError
  struct BadInputError : public std::exception
  {
    virtual const char* what() const throw()
    {
      return "BadInputError";
    }
  };

  //////////////////////////////////////////////////////////// 
  // LoadItem
  class LoadItem
  {
    public:
      LoadItem(RealTimeType t);
      LoadItem(RealTimeType t, FlowValueType v);
      RealTimeType get_time() const { return time; }
      FlowValueType get_value() const { return value; }
      bool get_is_end() const { return is_end; }
      RealTimeType get_time_advance(const LoadItem& next) const;

    private:
      RealTimeType time;
      FlowValueType value;
      bool is_end;

      bool is_good() const { return (time >= 0); }
  };

  ////////////////////////////////////////////////////////////
  // FlowState
  class FlowState
  {
    public:
      FlowState(FlowValueType in);
      FlowState(FlowValueType in, FlowValueType out);
      FlowState(FlowValueType in, FlowValueType out, FlowValueType store);
      FlowState(
          FlowValueType in,
          FlowValueType out,
          FlowValueType store,
          FlowValueType loss
          );

      FlowValueType getInflow() const { return inflow; };
      FlowValueType getOutflow() const { return outflow; };
      FlowValueType getStoreflow() const { return storeflow; };
      FlowValueType getLossflow() const { return lossflow; };

    private:
      FlowValueType inflow;
      FlowValueType outflow;
      FlowValueType storeflow;
      FlowValueType lossflow;
      void checkInvariants() const;
  };

  ////////////////////////////////////////////////////////////
  // StreamType
  class StreamType
  {
    public:
      StreamType(const std::string& type);
      StreamType(
          const std::string& type,
          const std::string& rate_units,
          const std::string& quantity_units,
          FlowValueType seconds_per_time_unit
          );
      StreamType(
          const std::string& type,
          const std::string& rate_units,
          const std::string& quantity_units,
          FlowValueType seconds_per_time_unit,
          const std::unordered_map<std::string, FlowValueType>& other_rate_units,
          const std::unordered_map<std::string, FlowValueType>& other_quantity_units
          );
      bool operator==(const StreamType& other) const;
      bool operator!=(const StreamType& other) const;
      const std::string& get_type() const { return type; }
      const std::string& get_rate_units() const { return rate_units; }
      const std::string& get_quantity_units() const { return quantity_units; }
      FlowValueType get_seconds_per_time_unit() const { return seconds_per_time_unit; }
      const std::unordered_map<std::string,FlowValueType>& get_other_rate_units() const {
        return other_rate_units;
      }
      const std::unordered_map<std::string,FlowValueType>& get_other_quantity_units() const {
        return other_quantity_units;
      }

    private:
      std::string type;
      std::string rate_units;
      std::string quantity_units;
      FlowValueType seconds_per_time_unit;
      std::unordered_map<std::string,FlowValueType> other_rate_units;
      std::unordered_map<std::string,FlowValueType> other_quantity_units;
  };

  std::ostream& operator<<(std::ostream& os, const StreamType& st);

  ////////////////////////////////////////////////////////////
  // Stream
  class Stream
  {
    public:
      Stream(StreamType stream_type, FlowValueType rate);
      StreamType get_type() const { return type; }
      FlowValueType get_rate() const { return rate; }
      FlowValueType get_quantity(FlowValueType dt_s) const {
        return rate * (dt_s / type.get_seconds_per_time_unit());
      }
      FlowValueType get_rate_in_units(const std::string& units) const {
        const auto& u = type.get_other_rate_units();
        FlowValueType m{u.at(units)};
        return rate * m;
      }
      FlowValueType get_quantity_in_units(
          FlowValueType dt_s, const std::string& units) const {
        const auto& u = type.get_other_quantity_units();
        FlowValueType m{u.at(units)};
        return rate * (dt_s / type.get_seconds_per_time_unit()) * m;
      }

    private:
      StreamType type;
      FlowValueType rate;
  };

  std::ostream& operator<<(std::ostream& os, const Stream& s);

  ////////////////////////////////////////////////////////////
  // Type Definitions
  typedef adevs::port_value<Stream> PortValue;

  std::ostream& operator<<(std::ostream& os, const PortValue& pv);


  ////////////////////////////////////////////////////////////
  // FlowElement - Abstract Class
  class FlowElement : public adevs::Atomic<PortValue>
  {
    public:
      static const int inport_inflow_achieved;
      static const int inport_outflow_request;
      static const int outport_inflow_request;
      static const int outport_outflow_achieved;
      void delta_int() override;
      void delta_ext(adevs::Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      adevs::Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;

      // Delete copy constructors and assignment to prevent slicing issues...
      FlowElement(const FlowElement&) = delete;
      FlowElement& operator=(const FlowElement&) = delete;

      const std::string& get_id() const { return id; }

    protected:
      FlowElement(std::string id, StreamType flow_type);
      FlowElement(
          std::string id, StreamType inflow_type, StreamType outflow_type);
      virtual const FlowState
        update_state_for_outflow_request(FlowValueType outflow_) const;
      virtual const FlowState
        update_state_for_inflow_achieved(FlowValueType inflow_) const;
      virtual adevs::Time calculate_time_advance();
      virtual void update_on_internal_transition();
      virtual void update_on_external_transition();
      virtual void add_additional_outputs(std::vector<PortValue>& ys);
      void print_state() const;
      void print_state(const std::string& prefix) const;
      auto get_real_time() const { return time.real; };
      bool get_report_inflow_request() const { return report_inflow_request; };
      bool get_report_outflow_achieved() const {
        return report_outflow_achieved;
      };
      FlowValueType get_inflow() const { return inflow; };
      FlowValueType get_outflow() const { return outflow; };
      FlowValueType get_storeflow() const { return storeflow; };
      FlowValueType get_lossflow() const { return lossflow; };
      const StreamType& get_inflow_type() const { return inflow_type; };
      const StreamType& get_outflow_type() const { return outflow_type; };

    private:
      std::string id;
      adevs::Time time;
      StreamType inflow_type;
      StreamType outflow_type;
      FlowValueType inflow;
      FlowValueType outflow;
      FlowValueType storeflow;
      FlowValueType lossflow;
      bool report_inflow_request;
      bool report_outflow_achieved;

      void update_state(const FlowState& fs);
      static constexpr FlowValueType tol{1e-6};
      void check_flow_invariants() const;
  };

  ////////////////////////////////////////////////////////////
  // FlowLimits
  class FlowLimits : public FlowElement
  {
    public:
      FlowLimits(
          std::string id,
          StreamType stream_type,
          FlowValueType lower_limit,
          FlowValueType upper_limit
          );

    protected:
      const FlowState update_state_for_outflow_request(FlowValueType outflow_) const override;
      const FlowState update_state_for_inflow_achieved(FlowValueType inflow_) const override;

    private:
      FlowValueType lower_limit;
      FlowValueType upper_limit;
  };

  ////////////////////////////////////////////////////////////
  // FlowMeter
  class FlowMeter : public FlowElement
  {
    public:
      FlowMeter(std::string id, StreamType stream_type);
      std::vector<RealTimeType> get_actual_output_times() const;
      std::vector<FlowValueType> get_actual_output() const;

    protected:
      void update_on_external_transition() override;

    private:
      std::vector<RealTimeType> event_times;
      std::vector<FlowValueType> requested_flows;
      std::vector<FlowValueType> achieved_flows;
  };

  ////////////////////////////////////////////////////////////
  // Transformer
  class Transformer : public FlowElement
  {
    public:
      Transformer(
          std::string id,
          StreamType input_stream_type,
          StreamType output_stream_type,
          std::function<FlowValueType(FlowValueType)> calc_output_from_input,
          std::function<FlowValueType(FlowValueType)> calc_input_from_output
          );

    protected:
      const FlowState update_state_for_outflow_request(FlowValueType outflow_) const override;
      const FlowState update_state_for_inflow_achieved(FlowValueType inflow_) const override;

    private:
      std::function<FlowValueType(FlowValueType)> output_from_input;
      std::function<FlowValueType(FlowValueType)> input_from_output;
  };

  ////////////////////////////////////////////////////////////
  // Sink
  class Sink : public FlowElement
  {
    public:
      Sink(
          std::string id,
          StreamType stream_type,
          std::unordered_map<std::string, std::vector<LoadItem>> loads_by_scenario);
      Sink(
          std::string id,
          StreamType stream_type,
          std::unordered_map<std::string, std::vector<LoadItem>> loads_by_scenario,
          std::string active_scenario);

    protected:
      virtual void update_on_internal_transition() override;
      virtual adevs::Time calculate_time_advance() override;
      virtual const FlowState update_state_for_inflow_achieved(
          FlowValueType inflow_) const override;
      virtual void add_additional_outputs(std::vector<PortValue>& ys) override;

    private:
      std::unordered_map<std::string,std::vector<LoadItem>> loads_by_scenario;
      std::string active_scenario;
      int idx;
      std::vector<LoadItem>::size_type num_loads;
      std::vector<LoadItem> loads;

      void check_loads(
          const std::string& scenario,
          const std::vector<LoadItem>& loads) const;
      void check_loads_by_scenario() const;
      bool switch_scenario(const std::string& active_scenario);
  };
}

#endif // DISCO_DISCO_H
