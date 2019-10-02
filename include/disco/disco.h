/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */

#ifndef DISCO_DISCO_H
#define DISCO_DISCO_H
#include <string>
#include <vector>
#include <exception>
#include <functional>
#include "../../vendor/bdevs/include/adevs.h"

namespace DISCO
{
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
  // FlowState
  class FlowState
  {
    public:
      FlowState(FlowValueType in, FlowValueType out);
      FlowState(FlowValueType in, FlowValueType out, FlowValueType store);
      FlowState(FlowValueType in, FlowValueType out, FlowValueType store, FlowValueType loss);

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
      StreamType(std::string type, std::string units);
      bool operator==(const StreamType& other) const;
      bool operator!=(const StreamType& other) const;
      std::string get_type() const { return type; }
      std::string get_units() const { return units; }

    private:
      std::string type;
      std::string units;
  };

  std::ostream& operator<<(std::ostream& os, const StreamType& st);

  ////////////////////////////////////////////////////////////
  // Stream
  class Stream
  {
    public:
      Stream(StreamType stream_type, FlowValueType value);
      StreamType get_type() const { return type; }
      FlowValueType get_value() const { return value; }

    private:
      StreamType type;
      FlowValueType value;
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

    protected:
      FlowElement(std::string id, StreamType flow_type);
      FlowElement(
          std::string id, StreamType flow_type, bool do_check);
      FlowElement(
          std::string id, StreamType inflow_type, StreamType outflow_type);
      FlowElement(
          std::string id, StreamType inflow_type, StreamType outflow_type,
          bool do_check);
      virtual const FlowState update_state_for_outflow_request(FlowValueType outflow_) const;
      virtual const FlowState update_state_for_inflow_achieved(FlowValueType inflow_) const;
      virtual void update_on_internal_transition();
      virtual void update_on_external_transition();
      void print_state() const;
      void print_state(const std::string& prefix) const;
      std::string get_id() const { return id; }
      auto get_real_time() const { return time.real; };
      bool get_report_inflow_request() const { return report_inflow_request; };
      bool get_report_outflow_achieved() const { return report_outflow_achieved; };
      FlowValueType get_inflow() const { return inflow; };
      FlowValueType get_outflow() const { return outflow; };
      FlowValueType get_storeflow() const { return storeflow; };
      FlowValueType get_lossflow() const { return lossflow; };

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
      bool do_invariant_check;

      void update_state(const FlowState& fs);
      static constexpr FlowValueType tol{1e-6};
      void check_flow_invariants() const;
  };

  ////////////////////////////////////////////////////////////
  // FlowLimits
  class FlowLimits : public FlowElement
  {
    public:
      FlowLimits(std::string id, StreamType stream_type, FlowValueType lower_limit, FlowValueType upper_limit);

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
  class Sink : public adevs::Atomic<PortValue>
  {
    public:
      static const int outport_inflow_request;
      Sink(StreamType stream_type, std::vector<RealTimeType> times, std::vector<FlowValueType> loads);
      void delta_int() override;
      void delta_ext(adevs::Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      adevs::Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;

    private:
      StreamType stream;
      int idx;
      std::vector<RealTimeType>::size_type num_items;
      std::vector<RealTimeType> times;
      std::vector<FlowValueType> loads;
  };
}

#endif // DISCO_DISCO_H
