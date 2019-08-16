/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */
#include "disco/disco.h"
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace DISCO
{
  const bool DEBUG{true};

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

  template<class T>
  void print_vec(const std::string& tag, const std::vector<T>& vs)
  {
    char mark = '=';
    std::cout << tag;
    for (const auto &v : vs) {
      std::cout << mark << v;
      if (mark == '=')
        mark = ',';
    }
    std::cout << std::endl;
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

  FlowElement::FlowElement(std::string id, StreamType st) :
    FlowElement(id, st, st)
  {
  }

  FlowElement::FlowElement(std::string id_, StreamType in, StreamType out) :
    adevs::Atomic<PortValue>(),
    id{id_},
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
    if (DEBUG)
      std::cout << "FlowElement::delta_int();id=" << id << "\n";
    update_on_internal_transition();
    report_inflow_request = false;
    report_outflow_achieved = false;
  }

  void
  FlowElement::delta_ext(adevs::Time e, std::vector<PortValue>& xs)
  {
    if (DEBUG)
      std::cout << "FlowElement::delta_ext();id=" << id << "\n";
    time = time + e;
    bool inflow_provided{false};
    bool outflow_provided{false};
    FlowValueType inflow_achieved{0};
    FlowValueType outflow_request{0};
    for (const auto &x : xs) {
      switch (x.port) {
        case inport_inflow_achieved:
          if (DEBUG)
            std::cout << "... <=inport_inflow_achieved\n";
          if (x.value.get_type() != inflow_type)
            throw MixedStreamsError();
          inflow_provided = true;
          inflow_achieved += x.value.get_power();
          break;
        case inport_outflow_request:
          if (DEBUG)
            std::cout << "... <=inport_outflow_request\n";
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
      report_outflow_achieved = true;
      if (inflow >= 0.0 && inflow_achieved > inflow)
        throw AchievedMoreThanRequestedError();
      if (inflow <= 0.0 && inflow_achieved < inflow)
        throw AchievedMoreThanRequestedError();
      update_state_for_inflow_achieved(inflow_achieved);
    }
    else if (outflow_provided && !inflow_provided) {
      report_inflow_request = true;
      update_state_for_outflow_request(outflow_request);
      if (outflow >= 0.0 && outflow > outflow_request)
        throw AchievedMoreThanRequestedError();
      if (outflow <= 0.0 && outflow < outflow_request)
        throw AchievedMoreThanRequestedError();
    }
    else if (inflow_provided && outflow_provided) {
      // assumption: we'll never get here...
      throw SimultaneousIORequestError();
    }
    else
      throw BadPortError();
    if (report_inflow_request || report_outflow_achieved) {
      update_on_external_transition();
      check_flow_invariants();
    }
  }

  void
  FlowElement::delta_conf(std::vector<PortValue>& xs)
  {
    if (DEBUG)
      std::cout << "FlowElement::delta_conf();id=" << id << "\n";
    auto e = adevs::Time{0,0};
    delta_int();
    delta_ext(e, xs);
  }

  adevs::Time
  FlowElement::ta()
  {
    if (DEBUG)
      std::cout << "FlowElement::ta();id=" << id << "\n";
    if (report_inflow_request || report_outflow_achieved) {
      if (DEBUG)
        std::cout << "... dt = (0,1)\n";
      return adevs::Time{0, 1};
    }
    if (DEBUG)
      std::cout << "... dt = infinity\n";
    return adevs_inf<adevs::Time>();
  }

  void
  FlowElement::output_func(std::vector<PortValue>& ys)
  {
    if (DEBUG)
      std::cout << "FlowElement::output_func();id=" << id << "\n";
    if (report_inflow_request) {
      if (DEBUG)
        std::cout << "... send=>outport_inflow_request\n";
      ys.push_back(
          adevs::port_value<Stream>{
            outport_inflow_request, Stream{inflow_type, inflow}});
    }
    if (report_outflow_achieved) {
      if (DEBUG)
        std::cout << "... send=>outport_outflow_achieved\n";
      ys.push_back(
          adevs::port_value<Stream>{
            outport_outflow_achieved, Stream{outflow_type, outflow}});
    }
  }

  void
  FlowElement::update_state_for_outflow_request(FlowValueType outflow_)
  {
    if (DEBUG)
      std::cout << "FlowElement::update_state_for_outflow_request();id=" << id << "\n";
    outflow = outflow_;
    inflow = outflow;
  }

  void
  FlowElement::update_state_for_inflow_achieved(FlowValueType inflow_)
  {
    if (DEBUG)
      std::cout << "FlowElement::update_state_for_inflow_achieved();id=" << id << "\n";
    inflow = inflow_;
    outflow = inflow;
  }

  void
  FlowElement::update_on_internal_transition()
  {
    if (DEBUG)
      std::cout << "FlowElement::update_on_internal_transition();id=" << id << "\n";
  }

  void
  FlowElement::update_on_external_transition()
  {
    if (DEBUG)
      std::cout << "FlowElement::update_on_external_transition();id=" << id << "\n";
  }

  void
  FlowElement::print_state() const
  {
    print_state("");
  }

  void
  FlowElement::print_state(const std::string& prefix) const
  {
    std::cout << prefix << "id=" << id << "\n"
              << prefix << "time=(" << time.real << ", " << time.logical << ")\n"
              << prefix << "inflow=" << inflow << "\n"
              << prefix << "outflow=" << outflow << "\n"
              << prefix << "storeflow=" << storeflow << "\n"
              << prefix << "lossflow=" << lossflow << "\n"
              << prefix << "report_inflow_request=" << report_inflow_request << "\n"
              << prefix << "report_outflow_achieved=" << report_outflow_achieved << "\n";
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
  FlowLimits::FlowLimits(std::string id, StreamType stream_type, FlowValueType low_lim, FlowValueType up_lim) :
    FlowElement(id, stream_type),
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
    if (DEBUG) {
      std::cout << "FlowLimits::update_state_for_outflow_request(" << out << ")\n";
      print_state("... ");
    }
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
    if (DEBUG) {
      print_state("... ");
      std::cout << "end FlowLimits::update_state_for_outflow_request\n";
    }
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
  FlowMeter::FlowMeter(std::string id, StreamType stream_type) :
    FlowElement(id, stream_type),
    event_times{},
    requested_flows{},
    achieved_flows{}
  {
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

  void
  FlowMeter::update_on_external_transition()
  {
    if (DEBUG) {
      std::cout << "FlowMeter::update_on_external_transition()\n";
      print_state("... ");
      print_vec<RealTimeType>("... event_times", event_times);
      print_vec<FlowValueType>("... requested_flows", requested_flows);
      print_vec<FlowValueType>("... achieved_flows", achieved_flows);
    }
    auto num_events{event_times.size()};
    if (num_events == 0 || (num_events > 0 && event_times.back() != time.real)) {
      event_times.push_back(time.real);
      ++num_events;
    }
    auto num_achieved{achieved_flows.size()};
    if (report_inflow_request) {
      requested_flows.push_back(inflow);
      if (num_achieved == num_events && num_achieved > 0) {
        auto &v = achieved_flows.back();
        v = inflow;
      }
      else
        achieved_flows.push_back(inflow);
    }
    else if (report_outflow_achieved) {
      if (num_achieved == num_events && num_achieved > 0) {
        auto &v = achieved_flows.back();
        v = outflow;
      }
      else
        achieved_flows.push_back(outflow);
    }
    if (report_inflow_request && report_outflow_achieved)
      throw SimultaneousIORequestError();
    if (DEBUG) {
      print_state("... ");
      print_vec<RealTimeType>("... event_times", event_times);
      print_vec<FlowValueType>("... requested_flows", requested_flows);
      print_vec<FlowValueType>("... achieved_flows", achieved_flows);
      std::cout << "end FlowMeter::update_on_external_transition()\n";
    }
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
    if (DEBUG)
      std::cout << "Sink::delta_ext()\n";
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
    if (idx < 0) {
      if (DEBUG)
        std::cout << "... dt = infinity\n";
      return adevs::Time{0, 0};
    }
    int next_idx = idx + 1;
    if (next_idx < num_items) {
      RealTimeType time0{times[idx]};
      RealTimeType time1{times[next_idx]};
      RealTimeType dt{time1 - time0};
      if (DEBUG)
        std::cout << "... dt = (" << dt << ", 0)\n";
      return adevs::Time{dt, 0};
    }
    if (DEBUG)
      std::cout << "... dt = infinity\n";
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
