/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_ELEMENT_H
#define ERIN_ELEMENT_H
#include "erin/type.h"
#include "erin/stream.h"
#include "erin/devs.h"
#include "erin/devs/converter.h"
#include "erin/devs/flow_limits.h"
#include "erin/devs/load.h"
#include "erin/devs/on_off_switch.h"
#include "erin/devs/storage.h"
#include "erin/devs/mux.h"
#include "erin/reliability.h"
#include "adevs.h"
#include <functional>
#include <memory>
#include <stdexcept>
#include <unordered_map>

namespace ERIN
{
  ////////////////////////////////////////////////////////////
  // FlowWriter
  class FlowWriter
  {
    public:
      FlowWriter() = default;
      FlowWriter(const FlowWriter&) = delete;
      FlowWriter& operator=(const FlowWriter&) = delete;
      FlowWriter(FlowWriter&&) = delete;
      FlowWriter& operator=(FlowWriter&&) = delete;
      virtual ~FlowWriter() = default;

      [[nodiscard]] virtual int register_id(
          const std::string& element_tag,
          const std::string& stream_tag,
          ComponentType comp_type,
          PortRole port_role,
          bool record_history) = 0;
      virtual void write_data(
          int element_id,
          RealTimeType time,
          FlowValueType requested_flow,
          FlowValueType achieved_flow) = 0;
      virtual void finalize_at_time(RealTimeType time) = 0;
      [[nodiscard]] virtual std::unordered_map<std::string, std::vector<Datum>>
        get_results() const = 0;
      [[nodiscard]] virtual std::unordered_map<std::string,ComponentType>
        get_component_types() const = 0;
      [[nodiscard]] virtual std::unordered_map<std::string,PortRole>
        get_port_roles() const = 0;
      [[nodiscard]] virtual std::unordered_map<std::string,std::string>
        get_stream_ids() const = 0;
      virtual void clear() = 0;
  };

  ////////////////////////////////////////////////////////////
  // DefaultFlowWriter
  class DefaultFlowWriter : public FlowWriter
  {
    public:
      using size_type_D = std::vector<Datum>::size_type;
      using size_type_H = std::vector<std::vector<Datum>>::size_type;
      using size_type_B = std::vector<bool>::size_type;

      DefaultFlowWriter();

      [[nodiscard]] int register_id(
          const std::string& element_tag,
          const std::string& stream_tag,
          ComponentType comp_type,
          PortRole port_role,
          bool record_history) override;
      void write_data(
          int element_id,
          RealTimeType time,
          FlowValueType requested_flow,
          FlowValueType achieved_flow) override;
      void finalize_at_time(RealTimeType time) override;
      [[nodiscard]] std::unordered_map<std::string, std::vector<Datum>>
        get_results() const override;
      [[nodiscard]] std::unordered_map<std::string,std::string>
        get_stream_ids() const override;
      [[nodiscard]] std::unordered_map<std::string,ComponentType>
        get_component_types() const override;
      [[nodiscard]] std::unordered_map<std::string,PortRole>
        get_port_roles() const override;
      void clear() override;

    private:
      bool recording_started;
      bool is_final;
      RealTimeType current_time;
      int next_id;
      std::vector<FlowValueType> current_requests;
      std::vector<FlowValueType> current_achieved;
      std::unordered_map<std::string, int> element_tag_to_id;
      std::unordered_map<int, std::string> element_id_to_tag;
      std::unordered_map<int, std::string> element_id_to_stream_tag;
      std::unordered_map<int, ComponentType> element_id_to_comp_type;
      std::unordered_map<int, PortRole> element_id_to_port_role;
      std::vector<bool> recording_flags;
      std::vector<RealTimeType> time_history;
      std::vector<FlowValueType> request_history;
      std::vector<FlowValueType> achieved_history;

      int num_elements() const { return next_id; }
      int get_next_id() {
        auto element_id{next_id};
        ++next_id;
        return element_id;
      }
      void ensure_element_tag_is_unique(const std::string& element_tag) const;
      void ensure_element_id_is_valid(int element_id) const;
      void ensure_time_is_valid(RealTimeType time) const;
      void record_history_and_update_current_time(RealTimeType time);
      void ensure_not_final() const;
      void ensure_not_recording() const;
  };

  /**
   * ElementType - the types of Elements available
   */
  enum class ElementType
  {
    FlowLimits = 0,
    FlowMeter,
    Converter,
    Sink,
    Mux,
    Store,
    OnOffSwitch
  };

  ElementType tag_to_element_type(const std::string& tag);

  std::string element_type_to_tag(const ElementType& et);

  ////////////////////////////////////////////////////////////
  // FlowElement - Abstract Class
  class FlowElement : public adevs::Atomic<PortValue, Time>
  {
    public:
      static constexpr int max_port_numbers{1000};
      static constexpr int inport_inflow_achieved{0*max_port_numbers};
      static constexpr int inport_outflow_request{1*max_port_numbers};
      static constexpr int outport_inflow_request{2*max_port_numbers};
      static constexpr int outport_outflow_achieved{3*max_port_numbers};

      void delta_int() override;
      void delta_ext(Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;

      virtual void set_flow_writer(const std::shared_ptr<FlowWriter>& /* writer */) {}
      virtual void set_recording_on() {}

      [[nodiscard]] virtual std::string get_inflow_type_by_port(int inflow_port) const = 0;
      [[nodiscard]] virtual std::string get_outflow_type_by_port(int outflow_port) const = 0;

      // Delete copy and move operators to prevent slicing issues...
      // see:
      // http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c21-if-you-define-or-delete-any-default-operation-define-or-delete-them-all
      // and http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c67-a-polymorphic-class-should-suppress-copying
      virtual ~FlowElement() = default;
      FlowElement(const FlowElement&) = delete;
      FlowElement& operator=(const FlowElement&) = delete;
      FlowElement(FlowElement&&) = delete;
      FlowElement& operator=(const FlowElement&&) = delete;

      [[nodiscard]] std::string get_id() const { return id; }
      [[nodiscard]] std::string get_inflow_type() const { return inflow_type; };
      [[nodiscard]] std::string get_outflow_type() const { return outflow_type; };
      [[nodiscard]] ComponentType get_component_type() const {
        return component_type;
      };
      [[nodiscard]] ElementType get_element_type() const {
        return element_type;
      };

    protected:
      FlowElement(
          std::string id,
          ComponentType component_type,
          ElementType element_type,
          const std::string& flow_type);
      FlowElement(
          std::string id,
          ComponentType component_type,
          ElementType element_type,
          std::string inflow_type,
          std::string outflow_type);
      [[nodiscard]] virtual FlowState
        update_state_for_outflow_request(FlowValueType outflow_) const;
      [[nodiscard]] virtual FlowState
        update_state_for_inflow_achieved(FlowValueType inflow_) const;
      void run_checks_after_receiving_inputs(
          bool inflow_provided,
          FlowValueType inflow_achieved,
          bool outflow_provided,
          FlowValueType outflow_request,
          bool lossflow_provided,
          FlowValueType lossflow_request);
      virtual Time calculate_time_advance();
      virtual void update_on_internal_transition();
      virtual void update_on_external_transition();
      virtual void add_additional_outputs(std::vector<PortValue>& ys);
      void print_state() const;
      void print_state(const std::string& prefix) const;
      [[nodiscard]] auto get_real_time() const { return time.real; }
      [[nodiscard]] auto get_logical_time() const { return time.logical; }
      [[nodiscard]] bool get_report_inflow_request() const {
        return report_inflow_request;
      }
      [[nodiscard]] bool get_report_outflow_achieved() const {
        return report_outflow_achieved;
      }
      [[nodiscard]] bool get_report_lossflow_achieved() const {
        return report_lossflow_achieved;
      }
      void set_report_inflow_request(bool b) { report_inflow_request = b; }
      void set_report_outflow_achieved(bool b) { report_outflow_achieved = b; }
      void set_report_lossflow_achieved(bool b) {
        report_lossflow_achieved = b;
      }
      [[nodiscard]] FlowValueType get_inflow() const { return inflow; }
      [[nodiscard]] FlowValueType get_inflow_request() const {
        return inflow_request;
      }
      [[nodiscard]] FlowValueType get_outflow() const { return outflow; }
      [[nodiscard]] FlowValueType get_outflow_request() const {
        return outflow_request;
      }
      [[nodiscard]] FlowValueType get_storeflow() const { return storeflow; }
      [[nodiscard]] FlowValueType get_lossflow() const { return lossflow; }
      // for objects with multiple input and/or output ports
      // only port values above 0 stored in inflows and outflows
      [[nodiscard]] std::vector<FlowValueType> get_inflows() const {
        return std::vector<FlowValueType>{inflow};
      }
      [[nodiscard]] std::vector<FlowValueType> get_outflows() const {
        return std::vector<FlowValueType>{outflow};
      }
      [[nodiscard]] int get_num_inflows() const { return 1; }
      [[nodiscard]] int get_num_outflows() const { return 1; }

      void update_state(const FlowState& fs);
      void check_flow_invariants() const;
      void update_time(Time dt) { time = time + dt; }

    private:
      std::string id;
      Time time;
      std::string inflow_type;
      std::string outflow_type;
      FlowValueType inflow; // achieved
      FlowValueType inflow_request;
      FlowValueType outflow; // achieved
      FlowValueType outflow_request;
      FlowValueType storeflow;
      FlowValueType lossflow; // achieved
      FlowValueType lossflow_request;
      FlowValueType spillage; // achieved
      bool lossflow_connected;
      bool report_inflow_request;
      bool report_outflow_achieved;
      bool report_lossflow_achieved;
      ComponentType component_type;
      ElementType element_type;
  };

  ////////////////////////////////////////////////////////////
  // FlowLimits
  class FlowLimits : public FlowElement
  {
    public:
      FlowLimits(
          std::string id,
          ComponentType component_type,
          const std::string& stream_type,
          FlowValueType lower_limit,
          FlowValueType upper_limit,
          PortRole role = PortRole::Outflow);

      void delta_int() override;
      void delta_ext(Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;

      void set_flow_writer(const std::shared_ptr<FlowWriter>& writer) override;
      void set_recording_on() override;

      [[nodiscard]] std::string get_inflow_type_by_port(int /* inflow_port */) const override {
        return get_inflow_type();
      };
      [[nodiscard]] std::string get_outflow_type_by_port(int /* outflow_port */) const override {
        return get_outflow_type();
      };

    private:
      erin::devs::FlowLimitsState state;
      std::shared_ptr<FlowWriter> flow_writer;
      int element_id;
      bool record_history;
      PortRole port_role;
  };

  ////////////////////////////////////////////////////////////
  // FlowMeter
  class FlowMeter : public FlowElement
  {
    public:
      FlowMeter(
          std::string id,
          ComponentType component_type,
          const std::string& stream_type,
          PortRole port_role = PortRole::Outflow);
      void set_flow_writer(const std::shared_ptr<FlowWriter>& writer) override;
      void set_recording_on() override;

      [[nodiscard]] std::string get_inflow_type_by_port(int /* inflow_port */) const override {
        return get_inflow_type();
      };
      [[nodiscard]] std::string get_outflow_type_by_port(int /* outflow_port */) const override {
        return get_outflow_type();
      };

    protected:
      void update_on_external_transition() override;

    private:
      std::shared_ptr<FlowWriter> flow_writer;
      int element_id;
      bool record_history;
      PortRole port_role;
  };

  ////////////////////////////////////////////////////////////
  // Converter
  class Converter : public FlowElement
  {
    public:
      Converter(
          std::string id,
          ComponentType component_type,
          std::string input_stream_type,
          std::string output_stream_type,
          std::function<FlowValueType(FlowValueType)> calc_output_from_input,
          std::function<FlowValueType(FlowValueType)> calc_input_from_output,
          std::string lossflow_stream = std::string{"waste_heat"});

      void delta_int() override;
      void delta_ext(Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;

      void set_flow_writer(const std::shared_ptr<FlowWriter>& writer) override;
      void set_recording_on() override;
      void set_wasteflow_recording_on();

      [[nodiscard]] std::string get_inflow_type_by_port(int /* inflow_port */) const override {
        return get_inflow_type();
      };
      [[nodiscard]] std::string get_outflow_type_by_port(int outflow_port) const override {
        switch (outflow_port) {
          case 1:
          case 2:
            return lossflow_stream;
          default:
            return get_outflow_type();
        }
      };

    private:
      erin::devs::ConverterState state;
      std::function<FlowValueType(FlowValueType)> output_from_input;
      std::function<FlowValueType(FlowValueType)> input_from_output;
      std::shared_ptr<FlowWriter> flow_writer;
      int inflow_element_id;
      int outflow_element_id;
      int lossflow_element_id;
      int wasteflow_element_id;
      bool record_history;
      bool record_wasteflow_history;
      std::string lossflow_stream;

      void log_ports();
  };

  ////////////////////////////////////////////////////////////
  // Sink
  class Sink : public FlowElement
  {
    public:
      Sink(
          std::string id,
          ComponentType component_type,
          const std::string& stream_type,
          const std::vector<LoadItem>& loads);

      void delta_int() override;
      void delta_ext(Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;

      void set_flow_writer(const std::shared_ptr<FlowWriter>& writer) override;
      void set_recording_on() override;

      [[nodiscard]] std::string get_inflow_type_by_port(int /* inflow_port */) const override {
        return get_inflow_type();
      };
      [[nodiscard]] std::string get_outflow_type_by_port(int /* outflow_port */) const override {
        return get_outflow_type();
      };

    private:
      erin::devs::LoadData data;
      erin::devs::LoadState state;
      std::shared_ptr<FlowWriter> flow_writer;
      int element_id;
      bool record_history;
  };

  ////////////////////////////////////////////////////////////
  // MuxerDispatchStrategy
  //enum class MuxerDispatchStrategy
  //{
  //  InOrder = 0,
  //  Distribute
  //};

  //MuxerDispatchStrategy tag_to_muxer_dispatch_strategy(const std::string& tag);
  //std::string muxer_dispatch_strategy_to_string(MuxerDispatchStrategy mds);
  using erin::devs::MuxerDispatchStrategy;
  using erin::devs::tag_to_muxer_dispatch_strategy;
  using erin::devs::muxer_dispatch_strategy_to_string;

  ////////////////////////////////////////////////////////////
  // Mux
  class Mux : public FlowElement
  {
    public:
      Mux(
          std::string id,
          ComponentType component_type,
          const std::string& stream_type,
          int num_inflows,
          int num_outflows,
          MuxerDispatchStrategy outflow_strategy = MuxerDispatchStrategy::Distribute);

      void delta_int() override;
      void delta_ext(Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;

      void set_flow_writer(const std::shared_ptr<FlowWriter>& writer) override;
      void set_recording_on() override;

      [[nodiscard]] std::string get_inflow_type_by_port(int /* inflow_port */) const override {
        return get_inflow_type();
      };
      [[nodiscard]] std::string get_outflow_type_by_port(int /* outflow_port */) const override {
        return get_outflow_type();
      };

    private:
      erin::devs::MuxState state;
      std::shared_ptr<FlowWriter> flow_writer;
      std::vector<int> outflow_element_ids;
      std::vector<int> inflow_element_ids;
      bool record_history;

      void log_ports();
  };

  ////////////////////////////////////////////////////////////
  // Storage
  class Storage : public FlowElement
  {
    public:
      Storage(
          std::string id,
          ComponentType component_type,
          const std::string& stream_type,
          FlowValueType capacity,
          FlowValueType max_charge_rate);

      void delta_int() override;
      void delta_ext(Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;

      void set_flow_writer(const std::shared_ptr<FlowWriter>& writer) override;
      void set_recording_on() override;
      void set_storeflow_discharge_recording_on();

      [[nodiscard]] std::string get_inflow_type_by_port(int /* inflow_port */) const override {
        return get_inflow_type();
      }
      [[nodiscard]] std::string get_outflow_type_by_port(int /* outflow_port */) const override {
        return get_outflow_type();
      }

    private:
      erin::devs::StorageData data;
      erin::devs::StorageState state;
      std::shared_ptr<FlowWriter> flow_writer;
      bool record_history;
      bool record_storeflow_and_discharge;
      int inflow_element_id;
      int outflow_element_id;
      int storeflow_element_id;
      int discharge_element_id;

      void log_ports();
  };

  ////////////////////////////////////////////////////////////
  // OnOffSwitch
  class OnOffSwitch : public FlowElement
  {
    public:
      OnOffSwitch(
          std::string id,
          ComponentType component_type,
          const std::string& stream_type,
          const std::vector<TimeState>& schedule);

      void delta_int() override;
      void delta_ext(Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;

      void set_flow_writer(const std::shared_ptr<FlowWriter>& writer) override;
      void set_recording_on() override;

      [[nodiscard]] std::string get_inflow_type_by_port(int /* inflow_port */) const override {
        return get_inflow_type();
      }
      [[nodiscard]] std::string get_outflow_type_by_port(int /* outflow_port */) const override {
        return get_outflow_type();
      }

    private:
      erin::devs::OnOffSwitchData data;
      erin::devs::OnOffSwitchState state;
      std::shared_ptr<FlowWriter> flow_writer;
      bool record_history;
      int element_id;

      void log_ports();
  };
}

#endif // ERIN_ELEMENT_H
