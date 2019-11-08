/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_ERIN_H
#define ERIN_ERIN_H
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../../vendor/bdevs/include/adevs.h"
#include "../../vendor/toml11/toml.hpp"

namespace ERIN
{
  ////////////////////////////////////////////////////////////
  // Type Definitions
  typedef double FlowValueType;
  typedef int RealTimeType;
  typedef int LogicalTimeType;

  ////////////////////////////////////////////////////////////
  // Datum
  struct Datum
  {
    RealTimeType time;
    FlowValueType requested_value;
    FlowValueType achieved_value;
  };

  ////////////////////////////////////////////////////////////
  // ScenarioStats
  struct ScenarioStats
  {
    RealTimeType uptime;
    RealTimeType downtime;
    FlowValueType load_not_served;
  };

  ////////////////////////////////////////////////////////////
  // StreamInfo
  class StreamInfo
  {
    public:
      StreamInfo();
      StreamInfo(
          std::string rate_unit,
          std::string quantity_unit);
      StreamInfo(
          std::string rate_unit,
          std::string quantity_unit,
          double default_seconds_per_time_unit);
      [[nodiscard]] const std::string& get_rate_unit() const {return rate_unit;}
      [[nodiscard]] const std::string& get_quantity_unit() const {return quantity_unit;}
      [[nodiscard]] double get_seconds_per_time_unit() const {return seconds_per_time_unit;}
      bool operator==(const StreamInfo& other) const;
      bool operator!=(const StreamInfo& other) const {
        return !(operator==(other));
      }

    private:
      std::string rate_unit;
      std::string quantity_unit;
      double seconds_per_time_unit;
  };

  ////////////////////////////////////////////////////////////
  // InputReader
  class StreamType; // Forward declaration
  class Component;  // Forward declaration
  class Scenario;   // Forward declaration
  class LoadItem;   // Forward declaration
  class InputReader
  {
    public:
      virtual StreamInfo read_stream_info() = 0;
      virtual std::unordered_map<std::string, StreamType>
        read_streams(const StreamInfo& si) = 0;
      virtual std::unordered_map<std::string, std::vector<LoadItem>>
        read_loads() = 0;
      virtual std::unordered_map<std::string, std::shared_ptr<Component>>
        read_components(
            const std::unordered_map<std::string, StreamType>& stm,
            const std::unordered_map<std::string, std::vector<LoadItem>>& loads_by_id) = 0;
      virtual std::unordered_map<std::string,
        std::unordered_map<std::string, std::vector<std::string>>>
        read_networks() = 0;
      virtual std::unordered_map<std::string, std::shared_ptr<Scenario>>
        read_scenarios() = 0;
      virtual ~InputReader() = default;
  };

  ////////////////////////////////////////////////////////////
  // TomlInputReader
  class TomlInputReader : public InputReader
  {
    public:
      explicit TomlInputReader(toml::value  v);
      explicit TomlInputReader(const std::string& path);
      explicit TomlInputReader(std::istream& in);

      StreamInfo read_stream_info() override;
      std::unordered_map<std::string, StreamType>
        read_streams(const StreamInfo& si) override;
      std::unordered_map<std::string, std::vector<LoadItem>>
        read_loads() override;
      std::unordered_map<std::string, std::shared_ptr<Component>>
        read_components(
            const std::unordered_map<std::string, StreamType>& stm,
            const std::unordered_map<std::string, std::vector<LoadItem>>& loads_by_id)
        override;
      std::unordered_map<std::string,
        std::unordered_map<std::string, std::vector<std::string>>>
        read_networks() override;
      std::unordered_map<std::string, std::shared_ptr<Scenario>>
        read_scenarios() override;

    private:
      toml::value data;
  };

  ////////////////////////////////////////////////////////////
  // ScenarioResults
  class ScenarioResults
  {
    public:
      ScenarioResults();
      ScenarioResults(
          bool is_good_,
          std::unordered_map<std::string,std::vector<Datum>> results_);
      [[nodiscard]] bool get_is_good() const { return is_good; }
      [[nodiscard]] std::unordered_map<std::string, std::vector<Datum>>
        get_results() const { return results; }

      [[nodiscard]] std::string to_csv(const RealTimeType& max_time) const;
      [[nodiscard]] std::unordered_map<std::string,double> calc_energy_availability();
      [[nodiscard]] std::unordered_map<std::string,RealTimeType> calc_max_downtime();

    private:
      bool is_good;
      std::unordered_map<std::string, std::vector<Datum>> results;
      std::unordered_map<std::string, ScenarioStats> statistics;
      std::vector<std::string> keys;

      // TODO: how to we abstract calc_energy_availability() and calc_max_downtime()?
      //template<class T>
      //std::unordered_map<std::string,T> do_calcs()
      //{

      //}
  };

  ScenarioStats calc_scenario_stats(const std::vector<Datum>& ds);
  double do_calc_energy_availability(const ScenarioStats& ss);
  RealTimeType do_calc_max_downtime(const ScenarioStats& ss);

  ////////////////////////////////////////////////////////////
  // Main Class
  class Main
  {
    public:
      explicit Main(const std::string& input_toml);
      Main(
          StreamInfo si,
          std::unordered_map<std::string, StreamType> streams,
          std::unordered_map<std::string, std::shared_ptr<Component>> comps,
          std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::string>>> networks,
          std::unordered_map<std::string, std::shared_ptr<Scenario>> scenarios);
      ScenarioResults run(const std::string& scenario_id);
      RealTimeType max_time_for_scenario(const std::string& scenario_id);

    private:
      StreamInfo stream_info;
      std::unordered_map<std::string, StreamType> stream_types_map;
      std::unordered_map<std::string, std::shared_ptr<Component>> components;
      std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::string>>> networks;
      std::unordered_map<std::string, std::shared_ptr<Scenario>> scenarios;

      void check_data() const;
  };

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
    [[nodiscard]] const char* what() const noexcept override
    {
      return "MixedStreamsError";
    }
  };

  ////////////////////////////////////////////////////////////
  // MixedStreamsError
  struct InvariantError : public std::exception
  {
    [[nodiscard]] const char* what() const noexcept override
    {
      return "InvariantError";
    }
  };

  ////////////////////////////////////////////////////////////
  // InconsistentStreamUnitsError
  struct InconsistentStreamTypesError : public std::exception
  {
    [[nodiscard]] const char* what() const noexcept override
    {
      return "InconsistentStreamTypesError";
    }
  };

  ////////////////////////////////////////////////////////////
  // InconsistentStreamUnitsError
  struct InconsistentStreamUnitsError : public std::exception
  {
    [[nodiscard]] const char* what() const noexcept override
    {
      return "InconsistentStreamUnitsError";
    }
  };

  ////////////////////////////////////////////////////////////
  // FlowInvariantError
  struct FlowInvariantError : public std::exception
  {
    [[nodiscard]] const char* what() const noexcept override
    {
      return "FlowInvariantError";
    }
  };

  ////////////////////////////////////////////////////////////
  // BadPortError
  struct BadPortError : public std::exception
  {
    [[nodiscard]] const char* what() const noexcept override
    {
      return "BadPortError";
    }
  };

  ////////////////////////////////////////////////////////////
  // SimultaneousIORequestError
  struct SimultaneousIORequestError : public std::exception
  {
    [[nodiscard]] const char* what() const noexcept override
    {
      return "SimultaneousIORequestError";
    }
  };

  ////////////////////////////////////////////////////////////
  // AchievedMoreThanRequestedError
  struct AchievedMoreThanRequestedError : public std::exception
  {
    [[nodiscard]] const char* what() const noexcept override
    {
      return "AchievedMoreThanRequestedError";
    }
  };

  ////////////////////////////////////////////////////////////
  // BadInputError
  struct BadInputError : public std::exception
  {
    [[nodiscard]] const char* what() const noexcept override
    {
      return "BadInputError";
    }
  };

  //////////////////////////////////////////////////////////// 
  // LoadItem
  class LoadItem
  {
    public:
      explicit LoadItem(RealTimeType t);
      LoadItem(RealTimeType t, FlowValueType v);
      [[nodiscard]] RealTimeType get_time() const { return time; }
      [[nodiscard]] FlowValueType get_value() const { return value; }
      [[nodiscard]] bool get_is_end() const { return is_end; }
      [[nodiscard]] RealTimeType get_time_advance(const LoadItem& next) const;

    private:
      RealTimeType time;
      FlowValueType value;
      bool is_end;

      [[nodiscard]] bool is_good() const { return (time >= 0); }
  };

  ////////////////////////////////////////////////////////////
  // FlowState
  class FlowState
  {
    public:
      explicit FlowState(FlowValueType in);
      FlowState(FlowValueType in, FlowValueType out);
      FlowState(FlowValueType in, FlowValueType out, FlowValueType store);
      FlowState(
          FlowValueType in,
          FlowValueType out,
          FlowValueType store,
          FlowValueType loss
          );

      [[nodiscard]] FlowValueType getInflow() const { return inflow; };
      [[nodiscard]] FlowValueType getOutflow() const { return outflow; };
      [[nodiscard]] FlowValueType getStoreflow() const { return storeflow; };
      [[nodiscard]] FlowValueType getLossflow() const { return lossflow; };

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
      StreamType();
      explicit StreamType(const std::string& type);
      StreamType(
          const std::string& type,
          const std::string& rate_units,
          const std::string& quantity_units,
          FlowValueType seconds_per_time_unit
          );
      StreamType(
          std::string  type,
          std::string  rate_units,
          std::string  quantity_units,
          FlowValueType seconds_per_time_unit,
          std::unordered_map<std::string, FlowValueType>  other_rate_units,
          std::unordered_map<std::string, FlowValueType>  other_quantity_units
          );
      bool operator==(const StreamType& other) const;
      bool operator!=(const StreamType& other) const;
      [[nodiscard]] const std::string& get_type() const { return type; }
      [[nodiscard]] const std::string& get_rate_units() const { return rate_units; }
      [[nodiscard]] const std::string& get_quantity_units() const { return quantity_units; }
      [[nodiscard]] FlowValueType get_seconds_per_time_unit() const { return seconds_per_time_unit; }
      [[nodiscard]] const std::unordered_map<std::string,FlowValueType>& get_other_rate_units() const {
        return other_rate_units;
      }
      [[nodiscard]] const std::unordered_map<std::string,FlowValueType>& get_other_quantity_units() const {
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

  ////////////////////////////////////////////////////////////
  // Type Definitions
  typedef adevs::port_value<FlowValueType> PortValue;

  std::ostream& operator<<(std::ostream& os, const PortValue& pv);
  ////////////////////////////////////////////////////////////
  // Scenario
  // TODO: finish this class...
  //       This needs to be a separate DEVS sim over ~1000 years where we
  //       predict and enter scenarios and kick off detailed runs. Optionally,
  //       reliability can be taken into account to present the possibility of
  //       downed equipment at scenario start...
  class Scenario // : public adevs::Atomic<PortValue>
  {
    public:
      Scenario(std::string  name, std::string  network_id, RealTimeType max_times);

      [[nodiscard]] const std::string& get_name() const { return name; };
      [[nodiscard]] RealTimeType get_max_time() const { return max_time; };
      [[nodiscard]] const std::string& get_network_id() const { return network_id; };
      bool operator==(const Scenario& other) const;
      bool operator!=(const Scenario& other) const {
        return !(operator==(other));
      }
      //static const int outport_scenario_start;
      //static const int outport_scenario_end;
      //void delta_int() override;
      //void delta_ext(adevs::Time e, std::vector<PortValue>& xs) override;
      //void delta_conf(std::vector<PortValue>& xs) override;
      //adevs::Time ta() override;
      //void output_func(std::vector<PortValue>& ys) override;

    private:
      std::string name;
      std::string network_id;
      RealTimeType max_time;
  };

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

      [[nodiscard]] const std::string& get_id() const { return id; }
      [[nodiscard]] virtual std::vector<Datum> get_results() const {
        return std::vector<Datum>(0);
      }
      [[nodiscard]] const StreamType& get_inflow_type() const { return inflow_type; };
      [[nodiscard]] const StreamType& get_outflow_type() const { return outflow_type; };

    protected:
      FlowElement(std::string id, StreamType flow_type);
      FlowElement(
          std::string id, StreamType inflow_type, StreamType outflow_type);
      [[nodiscard]] virtual FlowState
        update_state_for_outflow_request(FlowValueType outflow_) const;
      [[nodiscard]] virtual FlowState
        update_state_for_inflow_achieved(FlowValueType inflow_) const;
      virtual adevs::Time calculate_time_advance();
      virtual void update_on_internal_transition();
      virtual void update_on_external_transition();
      virtual void add_additional_outputs(std::vector<PortValue>& ys);
      void print_state() const;
      void print_state(const std::string& prefix) const;
      [[nodiscard]] auto get_real_time() const { return time.real; };
      [[nodiscard]] bool get_report_inflow_request() const { return report_inflow_request; };
      [[nodiscard]] bool get_report_outflow_achieved() const {
        return report_outflow_achieved;
      };
      [[nodiscard]] FlowValueType get_inflow() const { return inflow; };
      [[nodiscard]] FlowValueType get_outflow() const { return outflow; };
      [[nodiscard]] FlowValueType get_storeflow() const { return storeflow; };
      [[nodiscard]] FlowValueType get_lossflow() const { return lossflow; };

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
      [[nodiscard]] FlowState update_state_for_outflow_request(FlowValueType outflow_) const override;
      [[nodiscard]] FlowState update_state_for_inflow_achieved(FlowValueType inflow_) const override;

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
      [[nodiscard]] std::vector<RealTimeType> get_event_times() const;
      [[nodiscard]] std::vector<FlowValueType> get_achieved_flows() const;
      [[nodiscard]] std::vector<FlowValueType> get_requested_flows() const;
      [[nodiscard]] std::vector<Datum> get_results() const override;

    protected:
      void update_on_external_transition() override;

    private:
      std::vector<RealTimeType> event_times;
      std::vector<FlowValueType> requested_flows;
      std::vector<FlowValueType> achieved_flows;
  };

  ////////////////////////////////////////////////////////////
  // Converter
  class Converter : public FlowElement
  {
    public:
      Converter(
          std::string id,
          StreamType input_stream_type,
          StreamType output_stream_type,
          std::function<FlowValueType(FlowValueType)> calc_output_from_input,
          std::function<FlowValueType(FlowValueType)> calc_input_from_output
          );

    protected:
      [[nodiscard]] FlowState update_state_for_outflow_request(FlowValueType outflow_) const override;
      [[nodiscard]] FlowState update_state_for_inflow_achieved(FlowValueType inflow_) const override;

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
          const StreamType& stream_type,
          const std::vector<LoadItem>& loads);

    protected:
      void update_on_internal_transition() override;
      adevs::Time calculate_time_advance() override;
      [[nodiscard]] FlowState update_state_for_inflow_achieved(
          FlowValueType inflow_) const override;
      void add_additional_outputs(std::vector<PortValue>& ys) override;

    private:
      std::vector<LoadItem> loads;
      int idx;
      std::vector<LoadItem>::size_type num_loads;

      void check_loads() const;
  };

  ////////////////////////////////////////////////////////////
  // Component
  // base class for all types of components
  class Component
  {
    public:
      Component(const Component&) = delete;
      Component& operator=(const Component&) = delete;
      Component(
          std::string  id,
          std::string  type,
          StreamType  input_stream,
          StreamType  output_stream);
      virtual ~Component() = default;;

      void add_input(std::shared_ptr<Component>& c);
      [[nodiscard]] const std::string& get_id() const { return id; }
      [[nodiscard]] const std::string& get_component_type() const { return component_type; }
      [[nodiscard]] const StreamType& get_input_stream() const { return input_stream; }
      [[nodiscard]] const StreamType& get_output_stream() const { return output_stream; }

      virtual std::unordered_set<FlowElement*>
        add_to_network(
            adevs::Digraph<FlowValueType>& nw,
            const std::string& active_scenario) = 0;
      FlowElement* get_connecting_element();

    protected:
      virtual FlowElement* create_connecting_element() = 0;
      [[nodiscard]] const std::vector<std::shared_ptr<Component>>& get_inputs() const {
        return inputs;
      }
      void connect_source_to_sink(
          adevs::Digraph<FlowValueType>& nw,
          FlowElement* source,
          FlowElement* sink,
          bool both_way) const;

    private:
      std::string id;
      std::string component_type;
      StreamType input_stream;
      StreamType output_stream;
      std::vector<std::shared_ptr<Component>> inputs;
      FlowElement* connecting_element;
  };

  ////////////////////////////////////////////////////////////
  // LoadComponent
  class LoadComponent : public Component
  {
    public:
      LoadComponent(
            const std::string& id,
            const StreamType& input_stream,
            std::unordered_map<std::string, std::vector<LoadItem>>
              loads_by_scenario);
      std::unordered_set<FlowElement*>
        add_to_network(
            adevs::Digraph<FlowValueType>& nw,
            const std::string& active_scenario) override;

    protected:
      FlowElement* create_connecting_element() override;

    private:
      std::unordered_map<std::string,std::vector<LoadItem>> loads_by_scenario;
  };

  ////////////////////////////////////////////////////////////
  // SourceComponent
  class SourceComponent : public Component
  {
    public:
      SourceComponent(
          const std::string& id,
          const StreamType& output_stream);
      std::unordered_set<FlowElement*>
        add_to_network(
            adevs::Digraph<FlowValueType>& nw,
            const std::string& active_scenario) override;

    protected:
      FlowElement* create_connecting_element() override;
  };
}

#endif // ERIN_ERIN_H
