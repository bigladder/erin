/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */
#include "disco/disco.h"
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace DISCO
{
  const bool DEBUG{false};

  FlowValueType
  clamp_toward_0(FlowValueType value, FlowValueType lower, FlowValueType upper)
  {
    if (lower > upper) {
      std::ostringstream oss;
      oss << "DISCO::clamp_toward_0 error: lower (" << lower
          << ") greater than upper (" << upper << ")"; 
      throw std::invalid_argument(oss.str());
    }
    if (value > upper) {
      if (upper > 0)
        return upper;
      else
        return 0;
    }
    else if (value < lower) {
      if (lower > 0)
        return 0;
      else
        return lower;
    }
    return value;
  }

  std::ostream&
  operator<<(std::ostream& os, const PortValue& pv)
  {
    return os << "PortValue(port=" << pv.port << ", flow=" << pv.value << ")";
  }

  ////////////////////////////////////////////////////////////
  // StreamType
  StreamType::StreamType(
      std::string t, std::string eu, std::string fu, std::string pu):
    type{t},
    effort_units{eu},
    flow_units{fu},
    power_units{pu}
  {
  }

  bool
  StreamType::operator==(const StreamType& other) const
  {
    return (type != other.type)                 ? false :
           (effort_units != other.effort_units) ? false :
           (flow_units != other.flow_units)     ? false :
           (power_units != other.power_units)   ? false :
           true;
  }

  bool
  StreamType::operator!=(const StreamType& other) const
  {
    return !operator==(other);
  }

  std::ostream&
  operator<<(std::ostream& os, const StreamType& st)
  {
    return os << "StreamType(type=\"" << st.get_type()
              << "\", effort_units=\"" << st.get_effort_units()
              << "\", flow_units=\"" << st.get_flow_units()
              << "\", power_units=\"" << st.get_power_units()
              << "\")";
  }

  ///////////////////////////////////////////////////////////////////
  // Stream
  Stream::Stream(StreamType stream_type, FlowValueType power_value):
    Stream(stream_type, 1.0, power_value)
  {
  }

  Stream::Stream(StreamType stream_type, FlowValueType effort_value, FlowValueType flow_value):
    type{stream_type},
    effort{effort_value},
    flow{flow_value},
    power{effort_value * flow_value}
  {
  }

  std::ostream&
  operator<<(std::ostream& os, const Stream& s)
  {
    auto t = s.get_type();
    return os << "Stream(stream_type=" << t
              << ", effort=" << s.get_effort() << " " << t.get_effort_units()
              << ", flow=" << s.get_flow() << " " << t.get_flow_units()
              << ", power=" << s.get_power() << " " << t.get_power_units()
              << ")";
  }

  ////////////////////////////////////////////////////////////
  // FlowElement
  const int FlowElement::inport_inflow_achieved = 0;
  const int FlowElement::inport_outflow_request = 1;
  const int FlowElement::outport_inflow_request = 2;
  const int FlowElement::outport_outflow_achieved = 3;

  FlowElement::FlowElement(StreamType st) :
    FlowElement(st, st)
  {
  }

  FlowElement::FlowElement(StreamType in, StreamType out) :
    adevs::Atomic<PortValue>(),
    time{0,0},
    inflow_type{in},
    outflow_type{out},
    inflow{0},
    outflow{0},
    storeflow{0},
    lossflow{0},
    report_inflow_request{false},
    report_outflow_achieved{false}
  {
  }

  void
  FlowElement::delta_int()
  {
    report_inflow_request = false;
    report_outflow_achieved = false;
  }

  void
  FlowElement::delta_ext(adevs::Time e, std::vector<PortValue>& xs)
  {
    time = time + e;
    bool inflow_provided{false};
    bool outflow_provided{false};
    FlowValueType inflow_achieved{0};
    FlowValueType outflow_request{0};
    for (const auto &x : xs) {
      switch (x.port) {
        case inport_inflow_achieved:
          if (x.value.get_type() != inflow_type)
            throw MixedStreamsError();
          inflow_provided = true;
          inflow_achieved += x.value.get_power();
          break;
        case inport_outflow_request:
          if (x.value.get_type() != outflow_type)
            throw MixedStreamsError();
          outflow_provided = true;
          outflow_request += x.value.get_power();
          break;
        default:
          throw BadPortError();
      }
    }
    if (inflow_provided && !outflow_provided) {
      if (std::abs(inflow - inflow_achieved) > tol) {
        report_outflow_achieved = true;
        if (inflow >= 0.0 && inflow_achieved > inflow)
          throw AchievedMoreThanRequestedError();
        if (inflow <= 0.0 && inflow_achieved < inflow)
          throw AchievedMoreThanRequestedError();
        update_state_for_inflow_achieved(inflow_achieved);
      }
    }
    else if (outflow_provided && !inflow_provided) {
      if (std::abs(outflow - outflow_request) > tol) {
        report_inflow_request = true;
        update_state_for_outflow_request(outflow_request);
        if (outflow >= 0.0 && outflow > outflow_request)
          throw AchievedMoreThanRequestedError();
        if (outflow <= 0.0 && outflow < outflow_request)
          throw AchievedMoreThanRequestedError();
      }
    }
    else if (inflow_provided && outflow_provided) {
      // assumption: we'll never get here...
      throw SimultaneousIORequestError();
    }
    if (report_inflow_request || report_outflow_achieved)
      check_flow_invariants();
  }

  void
  FlowElement::delta_conf(std::vector<PortValue>& xs)
  {
    auto e = adevs::Time{0,0};
    delta_int();
    delta_ext(e, xs);
  }

  adevs::Time
  FlowElement::ta()
  {
    if (report_inflow_request)
      return adevs::Time{0, 1};
    if (report_outflow_achieved)
      return adevs::Time{0, 1};
    return adevs_inf<adevs::Time>();
  }

  void
  FlowElement::output_func(std::vector<PortValue>& ys)
  {
    if (report_inflow_request)
      ys.push_back(
          adevs::port_value<Stream>{
            outport_inflow_request, Stream{inflow_type, inflow}});
    if (report_outflow_achieved)
      ys.push_back(
          adevs::port_value<Stream>{
            outport_outflow_achieved, Stream{outflow_type, outflow}});
  }

  void
  FlowElement::check_flow_invariants() const
  {
    auto diff = inflow - (outflow + storeflow + lossflow);
    if (diff > tol)
      throw FlowInvariantError();
  }


  ///////////////////////////////////////////////////////////////////
  // FlowLimits
  FlowLimits::FlowLimits(StreamType stream_type, FlowValueType low_lim, FlowValueType up_lim) :
    FlowElement(stream_type),
    lower_limit{low_lim},
    upper_limit{up_lim}
  {
    if (lower_limit > upper_limit) {
      std::ostringstream oss;
      oss << "FlowLimits error: lower_limit (" << lower_limit
          << ") > upper_limit (" << upper_limit << ")";
      throw std::invalid_argument(oss.str());
    }
  }

  void
  FlowLimits::update_state_for_outflow_request(FlowValueType out)
  {
    if (out > upper_limit) {
      report_outflow_achieved = true;
      outflow = upper_limit;
    }
    else if (out < lower_limit) {
      report_outflow_achieved = true;
      outflow = lower_limit;
    }
    else
      outflow = out;
    inflow = outflow;
  }

  void
  FlowLimits::update_state_for_inflow_achieved(FlowValueType in)
  {
    if (in > upper_limit)
      throw AchievedMoreThanRequestedError();
    else if (in < lower_limit)
      throw AchievedMoreThanRequestedError();
    else
      inflow = in;
    outflow = inflow;
  }

  ////////////////////////////////////////////////////////////
  // FlowMeter
  const int FlowMeter::inport_input_achieved = 1;
  const int FlowMeter::inport_output_request = 2;
  const int FlowMeter::outport_input_request = 3;
  const int FlowMeter::outport_output_achieved = 4;

  FlowMeter::FlowMeter(StreamType stream_type) :
    adevs::Atomic<PortValue>(),
    stream{stream_type},
    event_times{},
    requested_flows{},
    achieved_flows{},
    send_requested{false},
    send_achieved{false},
    event_time{0},
    requested_flow{0.0},
    achieved_flow{0.0}
  {
  }

  void
  FlowMeter::delta_int()
  {
    if (DEBUG) {
      std::cout << "FlowMeter::delta_int()\n";
      std::cout << "... S.event_time = " << event_time << "\n";
      std::cout << "... S.requested_flow = " << requested_flow << "\n";
      std::cout << "... S.achieved_flow = " << requested_flow << "\n";
      for (int i{0}; i < event_times.size(); ++i) {
        std::cout << "... S.event_times[" << i << "] = " << event_times[i] << "\n";
      }
      for (int i{0}; i < requested_flows.size(); ++i) {
        std::cout << "... S.requested_flows[" << i << "] = " << requested_flows[i] << "\n";
      }
      for (int i{0}; i < achieved_flows.size(); ++i) {
        std::cout << "... S.achieved_flows[" << i << "] = " << achieved_flows[i] << "\n";
      }
      std::cout << "---\n";
    }
    int num_events{static_cast<int>(event_times.size())};
    if (num_events == 0 || (num_events > 0 && event_times.back() != event_time)) {
      event_times.push_back(event_time);
      ++num_events;
    }
    if (send_requested) {
      requested_flows.push_back(requested_flow);
      if (achieved_flows.size() == num_events && achieved_flows.size() > 0) {
        auto &v = achieved_flows.back();
        v = requested_flow;
      }
      else
        achieved_flows.push_back(requested_flow);
    }
    if (send_achieved) {
      if (achieved_flows.size() == num_events && achieved_flows.size() > 0) {
        auto &v = achieved_flows.back();
        v = achieved_flow;
      }
      else
        achieved_flows.push_back(achieved_flow);
    }
    send_achieved = false;
    send_requested = false;
    requested_flow = 0.0;
    achieved_flow = 0.0;
    if (DEBUG) {
      std::cout << "... S.event_time = " << event_time << "\n";
      std::cout << "... S.requested_flow = " << requested_flow << "\n";
      std::cout << "... S.achieved_flow = " << requested_flow << "\n";
      for (int i{0}; i < event_times.size(); ++i) {
        std::cout << "... S.event_times[" << i << "] = " << event_times[i] << "\n";
      }
      for (int i{0}; i < requested_flows.size(); ++i) {
        std::cout << "... S.requested_flows[" << i << "] = " << requested_flows[i] << "\n";
      }
      for (int i{0}; i < achieved_flows.size(); ++i) {
        std::cout << "... S.achieved_flows[" << i << "] = " << achieved_flows[i] << "\n";
      }
      std::cout << "... exit FlowMeter::delta_int(.)\n";
    }
  }

  void
  FlowMeter::delta_ext(adevs::Time e, std::vector<PortValue>& xs)
  {
    if (DEBUG) {
      std::cout << "FlowMeter::delta_ext(dt(" << e.real << ", " << e.logical << "), xs)\n";
      for (int i{0}; i < xs.size(); ++i)
        std::cout << "... xs[" << i << "] = " << xs[i] << "\n";
      std::cout << "... S.event_time = " << event_time << "\n";
      std::cout << "... S.requested_flow = " << requested_flow << "\n";
      std::cout << "... S.achieved_flow = " << requested_flow << "\n";
      for (int i{0}; i < event_times.size(); ++i) {
        std::cout << "... S.event_times[" << i << "] = " << event_times[i] << "\n";
      }
      for (int i{0}; i < requested_flows.size(); ++i) {
        std::cout << "... S.requested_flows[" << i << "] = " << requested_flows[i] << "\n";
      }
      for (int i{0}; i < achieved_flows.size(); ++i) {
        std::cout << "... S.achieved_flows[" << i << "] = " << achieved_flows[i] << "\n";
      }
      std::cout << "---\n";
    }
    event_time += e.real;
    for (const auto &x : xs) {
      switch (x.port) {
        case inport_output_request:
          send_requested = true;
          requested_flow += x.value.get_power();
          break;
        case inport_input_achieved:
          send_achieved = true;
          achieved_flow += x.value.get_power();
          break;
        default:
          break;
      }
    }
    if (DEBUG) {
      std::cout << "... S.event_time = " << event_time << "\n";
      std::cout << "... S.requested_flow = " << requested_flow << "\n";
      std::cout << "... S.achieved_flow = " << requested_flow << "\n";
      for (int i{0}; i < event_times.size(); ++i) {
        std::cout << "... S.event_times[" << i << "] = " << event_times[i] << "\n";
      }
      for (int i{0}; i < requested_flows.size(); ++i) {
        std::cout << "... S.requested_flows[" << i << "] = " << requested_flows[i] << "\n";
      }
      for (int i{0}; i < achieved_flows.size(); ++i) {
        std::cout << "... S.achieved_flows[" << i << "] = " << achieved_flows[i] << "\n";
      }
      std::cout << "... exit FlowMeter::delta_ext(.)\n";
    }
  }

  void
  FlowMeter::delta_conf(std::vector<PortValue>& xs)
  {
    if (DEBUG)
      std::cout << "FlowMeter::delta_conf()\n";
    delta_int();
    delta_ext(adevs::Time{0, 0}, xs);
  }

  adevs::Time
  FlowMeter::ta()
  {
    if (DEBUG)
      std::cout << "FlowMeter::ta()\n";
    if (send_requested) {
      if (DEBUG)
        std::cout << "... send_requested branch; returning dt(0,0)\n";
      return adevs::Time{0, 0};
    }
    if (send_achieved) {
      if (DEBUG)
        std::cout << "... send_requested branch; returning dt(0,0)\n";
      return adevs::Time{0, 0};
    }
    if (DEBUG)
      std::cout << "... returning infinity\n";
    return adevs_inf<adevs::Time>();
  }

  void
  FlowMeter::output_func(std::vector<PortValue>& ys)
  {
    if (DEBUG)
      std::cout << "FlowMeter::output_func()\n";
    if (send_requested)
      ys.push_back(
          PortValue{outport_input_request, Stream{stream, requested_flow}});
    if (send_achieved)
      ys.push_back(
          PortValue{outport_output_achieved, Stream{stream, achieved_flow}});
  }

  std::vector<RealTimeType>
  FlowMeter::get_actual_output_times() const
  {
    return std::vector<RealTimeType>(event_times);
  }

  std::vector<FlowValueType>
  FlowMeter::get_actual_output() const
  {
    return std::vector<FlowValueType>(achieved_flows);
  }

  ////////////////////////////////////////////////////////////
  // Transformer
  //  public:
  const int Transformer::inport_input_achieved = 1;
  const int Transformer::inport_output_request = 2;
  const int Transformer::outport_input_request = 3;
  const int Transformer::outport_output_achieved = 4;

  Transformer::Transformer(
      StreamType input_stream_type,
      StreamType output_stream_type,
      std::function<FlowValueType(FlowValueType)> calc_output_from_input,
      std::function<FlowValueType(FlowValueType)> calc_input_from_output
      ) :
    adevs::Atomic<PortValue>(),
    input_stream{input_stream_type},
    output_stream{output_stream_type},
    output_from_input{calc_output_from_input},
    input_from_output{calc_input_from_output},
    output{0.0},
    input{0.0},
    send_input_request{false},
    send_output_achieved{false}
  {
  }

  void
  Transformer::delta_int()
  {
    if (DEBUG)
      std::cout << "Transformer::delta_int()\n";
    input = 0.0;
    output = 0.0;
    send_input_request = false;
    send_output_achieved = false;
  }

  void
  Transformer::delta_ext(adevs::Time e, std::vector<PortValue>& xs)
  {
    if (DEBUG) {
      std::cout << "Transformer::delta_ext(" << e.real << ")\n";
      for (int i{0}; i < xs.size(); ++i)
        std::cout << "... xs[" << i <<  "] = " << xs[i] << "\n";
    }
    for (const auto &x : xs) {
      switch (x.port) {
        case inport_input_achieved:
          send_output_achieved = true;
          if (x.value.get_type() != input_stream)
            throw MixedStreamsError();
          input += x.value.get_power();
          break;
        case inport_output_request:
        {
          send_input_request = true;
          if (x.value.get_type() != output_stream)
            throw MixedStreamsError();
          output += x.value.get_power();
          break;
        }
        default:
          break;
      }
    }
    // update inputs / outputs
    if (send_output_achieved) {
      output = 0.0;
      output = output_from_input(input);
    }
    if (send_input_request) {
      input = 0.0;
      input = input_from_output(output);
    }
  }

  void
  Transformer::delta_conf(std::vector<PortValue>& xs)
  {
    if (DEBUG)
      std::cout << "Transformer::delta_conf()\n";
    delta_int();
    delta_ext(adevs::Time{0, 0}, xs);
  }

  adevs::Time
  Transformer::ta()
  {
    if (DEBUG)
      std::cout << "Transformer::ta()\n";
    if (send_input_request)
      return adevs::Time{0, 0};
    if (send_output_achieved)
      return adevs::Time{0, 0};
    return adevs_inf<adevs::Time>();
  }

  void
  Transformer::output_func(std::vector<PortValue>& ys)
  {
    if (DEBUG)
      std::cout << "Transformer::output_func()\n";
    if (send_input_request)
      ys.push_back(
          adevs::port_value<Stream>{
            outport_input_request, Stream{input_stream, input}});
    if (send_output_achieved)
      ys.push_back(
          adevs::port_value<Stream>{
            outport_output_achieved, Stream{output_stream, output}});
  }

  ///////////////////////////////////////////////////////////////////
  // Sink
  const int Sink::outport_inflow_request = 1;

  Sink::Sink(StreamType st, std::vector<RealTimeType> ts, std::vector<FlowValueType> vs):
    adevs::Atomic<PortValue>(),
    stream{st},
    idx{-1},
    num_items{ts.size()},
    times{ts},
    loads{vs}
  {
    auto num_vs{vs.size()};
    if (num_items != num_vs) {
      std::ostringstream oss;
      oss << "DISCO::Sink::Sink: times.size() (" << num_items
          << ") != loads.size() (" << num_vs << ")";
      throw std::invalid_argument(oss.str());
    }
  }

  void
  Sink::delta_int()
  {
    if (DEBUG)
      std::cout << "Sink::delta_int()\n";
    ++idx;
  }

  void
  Sink::delta_ext(adevs::Time e, std::vector<PortValue>& xs)
  {
    if (DEBUG) {
      std::cout << "Sink::delta_ext(" << e.real << ")\n";
      for (int i{0}; i < xs.size(); ++i)
        std::cout << "... xs[" << i << "] = " << xs[i] << "\n";
    }
    // Nothing to do. This model is generate only...
  }

  void
  Sink::delta_conf(std::vector<PortValue>& xs)
  {
    if (DEBUG)
      std::cout << "Sink::delta_conf()\n";
    delta_int();
    delta_ext(adevs::Time{0, 0}, xs);
  }

  adevs::Time
  Sink::ta()
  {
    if (DEBUG)
      std::cout << "Sink::ta()\n";
    if (idx < 0)
      return adevs::Time{0, 1};
    int next_idx = idx + 1;
    if (next_idx < num_items) {
      RealTimeType time0{times[idx]};
      RealTimeType time1{times[next_idx]};
      RealTimeType dt{time1 - time0};
      return adevs::Time{dt, 0};
    }
    return adevs_inf<adevs::Time>();
  }

  void
  Sink::output_func(std::vector<PortValue>& ys)
  {
    if (DEBUG)
      std::cout << "Sink::output_func()\n";
    int next_idx = idx + 1;
    if (next_idx < num_items)
      ys.push_back(
          adevs::port_value<Stream>{
            outport_inflow_request,
            Stream{stream, loads[next_idx]
          }});
  }
}
