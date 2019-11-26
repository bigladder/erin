/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_ELEMENT_H
#define ERIN_ELEMENT_H
#include "erin/type.h"
#include "erin/stream.h"
#include <stdexcept>

namespace ERIN
{
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

      // Delete copy and move operators to prevent slicing issues...
      // see:
      // http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c21-if-you-define-or-delete-any-default-operation-define-or-delete-them-all
      // and http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c67-a-polymorphic-class-should-suppress-copying
      virtual ~FlowElement() = default;
      FlowElement(const FlowElement&) = delete;
      FlowElement& operator=(const FlowElement&) = delete;
      FlowElement(FlowElement&&) = delete;
      FlowElement& operator=(const FlowElement&&) = delete;

      [[nodiscard]] const std::string& get_id() const { return id; }
      [[nodiscard]] virtual std::vector<Datum>
        get_results(RealTimeType /* max_time */) const {
          return std::vector<Datum>(0);
        }
      [[nodiscard]] const StreamType& get_inflow_type() const { return inflow_type; };
      [[nodiscard]] const StreamType& get_outflow_type() const { return outflow_type; };
      [[nodiscard]] ComponentType get_component_type() const {
        return component_type;
      };

    protected:
      FlowElement(
          std::string id, ComponentType component_type, StreamType flow_type);
      FlowElement(
          std::string id, ComponentType component_type,
          StreamType inflow_type, StreamType outflow_type);
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
      ComponentType component_type;

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
          ComponentType component_type,
          StreamType stream_type,
          FlowValueType lower_limit,
          FlowValueType upper_limit);

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
      FlowMeter(
          std::string id, ComponentType component_type, StreamType stream_type);
      [[nodiscard]] std::vector<RealTimeType> get_event_times() const;
      [[nodiscard]] std::vector<FlowValueType> get_achieved_flows() const;
      [[nodiscard]] std::vector<FlowValueType> get_requested_flows() const;
      [[nodiscard]] std::vector<Datum>
        get_results(RealTimeType max_time) const override;

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
          ComponentType component_type,
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
          ComponentType component_type,
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
  // MixedStreamsError
  struct MixedStreamsError : public std::exception
  {
    [[nodiscard]] const char* what() const noexcept override
    {
      return "MixedStreamsError";
    }
  };

  ////////////////////////////////////////////////////////////
  // InvariantError
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
  // BadInputError
  struct BadInputError : public std::exception
  {
    [[nodiscard]] const char* what() const noexcept override
    {
      return "BadInputError";
    }
  };
}

#endif // ERIN_ELEMENT_H
