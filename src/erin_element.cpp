/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/element.h"
#include "debug_utils.h"
#include <algorithm>
#include <cmath>
#include <set>

namespace ERIN
{
  ElementType
  tag_to_element_type(const std::string& tag)
  {
    if (tag == "flow_limits") {
      return ElementType::FlowLimits;
    }
    else if (tag == "flow_meter") {
      return ElementType::FlowMeter;
    }
    else if (tag == "converter") {
      return ElementType::Converter;
    }
    else if (tag == "sink") {
      return ElementType::Sink;
    }
    else if (tag == "mux") {
      return ElementType::Mux;
    }
    else {
      std::ostringstream oss;
      oss << "unhandled tag '" << tag << "' for element_type\n";
      throw std::invalid_argument(oss.str());
    }
  }

  std::string
  element_type_to_tag(const ElementType& et)
  {
    switch (et) {
      case ElementType::FlowLimits:
        return std::string{"flow_limits"};
      case ElementType::FlowMeter:
        return std::string{"flow_meter"};
      case ElementType::Converter:
        return std::string{"converter"};
      case ElementType::Sink:
        return std::string{"sink"};
      case ElementType::Mux:
        return std::string{"mux"};
      default:
        {
          std::ostringstream oss;
          oss << "unhandled ElementType '" << static_cast<int>(et) << "'\n";
          throw std::invalid_argument(oss.str());
        }
    }
  }

  ////////////////////////////////////////////////////////////
  // DefaultFlowWriter
  DefaultFlowWriter::DefaultFlowWriter():
    FlowWriter(),
    current_time{0},
    next_id{0},
    current_status{},
    element_tag_to_id{},
    element_id_to_tag{},
    element_id_to_stream_tag{},
    recording_flags{},
    history{}
  {
  }

  std::unique_ptr<FlowWriter>
  DefaultFlowWriter::clone() const
  {
    std::unique_ptr<FlowWriter> p = std::make_unique<DefaultFlowWriter>();
    return p;
  }

  int
  DefaultFlowWriter::register_id(
      const std::string& element_tag,
      const std::string& stream_tag,
      bool record_history)
  {
    int element_id = next_id;
    ++next_id;
    current_status.emplace_back(Datum{current_time,0.0,0.0});
    auto it = element_tag_to_id.find(element_tag);
    if (it != element_tag_to_id.end()) {
      std::ostringstream oss;
      oss << "element_tag '" << element_tag << "' already registered!";
      throw std::invalid_argument(oss.str());
    }
    element_tag_to_id.emplace(std::make_pair(element_tag, element_id));
    element_id_to_tag.emplace(std::make_pair(element_id, element_tag));
    element_id_to_stream_tag.emplace(std::make_pair(element_id, stream_tag));
    recording_flags.emplace_back(record_history);
    history.emplace_back(std::vector<Datum>{});
    return element_id;
  }

  void
  DefaultFlowWriter::write_data(
      int element_id,
      RealTimeType time,
      FlowValueType requested_flow,
      FlowValueType achieved_flow)
  {
    auto num = num_elements();
    auto num_st = static_cast<size_type_D>(num);
    if ((element_id >= num) || (element_id < 0)) {
      std::ostringstream oss;
      oss << "element_id is invalid. "
          << "element_id must be >= 0 and < " << num_elements() << "\n"
          << "element_id: " << element_id << "\n";
      throw std::invalid_argument(oss.str());
    }
    if (time < current_time) {
      std::ostringstream oss;
      oss << "time is invalid. "
          << "This value indicates time is flowing backward!\n"
          << "time: " << time << "\n"
          << "current_time: " << current_time << "\n";
      throw std::invalid_argument(oss.str());
    }
    if (time > current_time) {
      for (size_type_D i{0}; i < num_st; ++i) {
        auto& d = current_status[i];
        if (recording_flags[i]) {
          d.time = current_time;
          history[i].emplace_back(d);
        }
        d.time = time;
      }
      current_time = time;
    }
    current_status[element_id] = Datum{time,requested_flow,achieved_flow};
  }

  void
  DefaultFlowWriter::finalize_at_time(RealTimeType time)
  {
    auto num = num_elements();
    auto num_st = static_cast<size_type_D>(num);
    auto d_final = Datum{time,0.0,0.0};
    for (size_type_D i{0}; i < num_st; ++i) {
      if (recording_flags[i]) {
        if (current_time == time) {
          history[i].emplace_back(d_final);
        } else {
          history[i].emplace_back(current_status[i]);
        }
      }
    }
    if (time > current_time) {
      for (size_type_D i{0}; i < num_st; ++i) {
        if (recording_flags[i]) {
          history[i].emplace_back(d_final);
        }
      }
    }
    current_time = time;
  }

  std::unordered_map<std::string, std::vector<Datum>>
  DefaultFlowWriter::get_results() const
  {
    std::unordered_map<std::string, std::vector<Datum>> out{};
    for (const auto& tag_id: element_tag_to_id) {
      const auto& element_tag = tag_id.first;
      auto element_id = tag_id.second;
      if (recording_flags[element_id]) {
        out[element_tag] = std::move(history[element_id]);
      }
    }
    return out;
  }

  void
  DefaultFlowWriter::clear()
  {
    current_time = 0;
    next_id = 0;
    current_status.clear();
    element_tag_to_id.clear();
    element_id_to_tag.clear();
    element_id_to_stream_tag.clear();
    recording_flags.clear();
    history.clear();
  }

  ////////////////////////////////////////////////////////////
  // FlowElement
  FlowElement::FlowElement(
      std::string id_,
      ComponentType component_type_,
      ElementType element_type_,
      const StreamType& st) :
    FlowElement(std::move(id_), component_type_, element_type_, st, st)
  {
  }

  FlowElement::FlowElement(
      std::string id_,
      ComponentType component_type_,
      ElementType element_type_,
      StreamType in,
      StreamType out):
    adevs::Atomic<PortValue, Time>(),
    id{std::move(id_)},
    time{0,0},
    inflow_type{std::move(in)},
    outflow_type{std::move(out)},
    inflow{0},
    inflow_request{0},
    outflow{0},
    outflow_request{0},
    storeflow{0},
    lossflow{0},
    lossflow_request{0},
    spillage{0},
    lossflow_connected{false},
    report_inflow_request{false},
    report_outflow_achieved{false},
    report_lossflow_achieved{false},
    component_type{component_type_},
    element_type{element_type_}
  {
    if (inflow_type.get_rate_units() != outflow_type.get_rate_units()) {
      std::ostringstream oss;
      oss << "InconsistentStreamUnitsError: inflow != outflow stream type";
      throw std::invalid_argument(oss.str());
    }
  }

  void
  FlowElement::delta_int()
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "FlowElement::delta_int();id=" << id << "\n";
    }
    update_on_internal_transition();
    report_inflow_request = false;
    report_outflow_achieved = false;
    report_lossflow_achieved = false;
  }

  void
  FlowElement::delta_ext(Time e, std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "FlowElement::delta_ext();id=" << id << "\n";
    }
    time = time + e;
    bool inflow_provided{false};
    bool outflow_provided{false};
    bool lossflow_provided{false};
    FlowValueType the_inflow_achieved{0};
    FlowValueType the_outflow_request{0};
    FlowValueType the_lossflow_request{0};
    for (const auto &x : xs) {
      switch (x.port) {
        case inport_inflow_achieved:
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "... receive<=inport_inflow_achieved\n";
          }
          inflow_provided = true;
          the_inflow_achieved += x.value;
          break;
        case inport_outflow_request:
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "... receive<=inport_outflow_request\n";
          }
          outflow_provided = true;
          the_outflow_request += x.value;
          break;
        case inport_lossflow_request:
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "... receive<=inport_lossflow_request;id="
                      << get_id() << "\n";
          }
          lossflow_provided = true;
          the_lossflow_request += x.value;
          lossflow_connected = true;
          break;
        default:
          {
            std::ostringstream oss;
            oss << "BadPortError: unhandled port: \"" << x.port << "\"";
            throw std::runtime_error(oss.str());
          }
      }
    }
    run_checks_after_receiving_inputs(
        inflow_provided, the_inflow_achieved,
        outflow_provided, the_outflow_request,
        lossflow_provided, the_lossflow_request);
  }

  void
  FlowElement::run_checks_after_receiving_inputs(
      bool inflow_provided,
      FlowValueType the_inflow_achieved,
      bool outflow_provided,
      FlowValueType the_outflow_request,
      bool lossflow_provided,
      FlowValueType the_lossflow_request)
  {
    if (inflow_provided && !outflow_provided) {
      if (lossflow_provided) {
        if constexpr (debug_level >= debug_level_high) {
          std::cout << "inflow_provided "
                    << "&& !outflow_provided "
                    << "&& lossflow_provided;id=" << get_id() << "\n";
        }
        lossflow_request = the_lossflow_request;
        spillage = 0.0; // reset spillage when request updated
      }
      report_outflow_achieved = true;
      if ((inflow > 0.0) && (the_inflow_achieved > inflow_request)) {
        std::ostringstream oss;
        oss << "AchievedMoreThanRequestedError\n";
        oss << "inflow > 0.0 and the_inflow_achieved > inflow\n";
        oss << "inflow: " << inflow << "\n";
        oss << "the_inflow_achieved: " << the_inflow_achieved << "\n";
        oss << "id = \"" << id << "\"\n";
        throw std::runtime_error(oss.str());
      }
      if (the_inflow_achieved < neg_flow_value_tol) {
        std::ostringstream oss;
        oss << "FlowReversalError!!!\n";
        oss << "inflow should never be below 0.0!!!\n";
        if (the_inflow_achieved < inflow_request) {
          oss << "AchievedMoreThanRequestedError\n";
          oss << "inflow < 0.0 and the_inflow_achieved < inflow_request\n";
          oss << "inflow: " << inflow << "\n";
          oss << "the_inflow_achieved: " << the_inflow_achieved << "\n";
          oss << "inflow_request : " << inflow_request << "\n";
        }
        throw std::runtime_error(oss.str());
      }
      const FlowState& fs = update_state_for_inflow_achieved(the_inflow_achieved);
      // if 
      // - another element cares to hear about lossflow (lossflow_connected)
      // - AND actual lossflow (fs.get_lossflow()) is different than requested
      //   (lossflow_request)
      // - AND the actual lossflow (fs.get_lossflow()) is different than what
      //   was previously reported (lossflow, the PREVIOUS value)
      // THEN report_lossflow_achieved
      report_lossflow_achieved = (
          lossflow_connected
          && (std::fabs(lossflow_request - fs.get_lossflow()) > flow_value_tolerance)
          && (std::fabs(lossflow - fs.get_lossflow()) > flow_value_tolerance));
      update_state(fs);
      if (lossflow_request < lossflow) {
        spillage = lossflow - lossflow_request;
      }
    }
    else if (outflow_provided && !inflow_provided) {
      if (lossflow_provided) {
        if constexpr (debug_level >= debug_level_high) {
          std::cout << "!inflow_provided "
                    << "&& outflow_provided "
                    << "&& lossflow_provided;id=" << get_id() << "\n";
        }
        lossflow_request = the_lossflow_request;
        spillage = 0.0;
      }
      report_inflow_request = true;
      outflow_request = the_outflow_request;
      const FlowState fs = update_state_for_outflow_request(outflow_request);
      // update what the inflow_request is based on all outflow_requests
      inflow_request = fs.get_inflow();
      auto diff = std::fabs(fs.get_outflow() - outflow_request);
      if (diff > flow_value_tolerance) {
        report_outflow_achieved = true;
      }
      // if 
      // - another element cares to hear about lossflow (lossflow_connected)
      // - AND actual lossflow (fs.get_lossflow()) is different than requested
      //   (lossflow_request)
      // - AND the actual lossflow (fs.get_lossflow()) is different than what
      //   was previously reported (lossflow, the PREVIOUS value)
      // THEN report_lossflow_achieved
      report_lossflow_achieved = (
          lossflow_connected
          && (std::fabs(lossflow_request - fs.get_lossflow()) > flow_value_tolerance)
          && (std::fabs(lossflow - fs.get_lossflow()) > flow_value_tolerance));
      update_state(fs);
      if (lossflow_request < lossflow) {
        spillage = lossflow - lossflow_request;
      }
      if (outflow > 0.0 && outflow > outflow_request) {
        std::ostringstream oss;
        oss << "AchievedMoreThanRequestedError\n";
        oss << "outflow > 0.0 && outflow > outflow_request\n";
        oss << "outflow: " << outflow << "\n";
        oss << "outflow_request: " << outflow_request << "\n";
        throw std::runtime_error(oss.str());
      }
      if (outflow < neg_flow_value_tol) {
        std::ostringstream oss;
        oss << "FlowReversalError\n";
        oss << "outflow should not be negative\n";
        if (outflow < outflow_request) {
          oss << "AchievedMoreThanRequestedError\n";
          oss << "outflow < 0.0 && outflow < outflow_request\n";
          oss << "outflow: " << outflow << "\n";
          oss << "outflow_request: " << outflow_request << "\n";
        }
        throw std::runtime_error(oss.str());
      }
    }
    else if (inflow_provided && outflow_provided) {
      // assumption: we'll never get here...
      std::ostringstream oss;
      oss << "SimultaneousIORequestError: assumption was we'd never get here...\n";
      oss << "... id=" << get_id() << "\n";
      oss << "... inflow_provided=" << inflow_provided << "\n";
      oss << "... outflow_provided=" << outflow_provided << "\n";
      oss << "... lossflow_provided=" << lossflow_provided << "\n";
      oss << "... the_inflow_achieved=" << the_inflow_achieved << "\n";
      oss << "... the_outflow_request=" << the_outflow_request << "\n";
      oss << "... the_lossflow_request=" << the_lossflow_request << "\n";
      throw std::runtime_error(oss.str());
    }
    else if (lossflow_provided) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "!inflow_provided "
                  << "&& !outflow_provided "
                  << "&& lossflow_provided;id=" << get_id() << "\n";
      }
      // if 
      // - another element cares to hear about lossflow (lossflow_connected)
      // - AND current lossflow request (the_lossflow_request) is different
      //   than previous (lossflow_request)
      // - AND current lossflow request (the_lossflow_request) is different
      //   from what was previously reported (lossflow)
      // THEN report_lossflow_achieved
      report_lossflow_achieved = (
          lossflow_connected
          && (the_lossflow_request != lossflow_request)
          && (the_lossflow_request != lossflow));
      lossflow_request = the_lossflow_request;
      spillage = 0.0;
      if (lossflow_request < lossflow) {
        spillage = lossflow - lossflow_request;
      }
    }
    else {
      std::ostringstream oss;
      oss << "BadPortError: no relevant ports detected...";
      throw std::runtime_error(oss.str());
    }
    if (report_inflow_request || report_outflow_achieved) {
      update_on_external_transition();
      check_flow_invariants();
    }
  }

  void
  FlowElement::update_state(const FlowState& fs)
  {
    inflow = fs.get_inflow();
    outflow = fs.get_outflow();
    storeflow = fs.get_storeflow();
    lossflow = fs.get_lossflow();
  }

  void
  FlowElement::delta_conf(std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "FlowElement::delta_conf();id=" << id << "\n";
    }
    auto e = Time{0,0};
    delta_int();
    delta_ext(e, xs);
  }

  Time
  FlowElement::calculate_time_advance()
  {
    return inf;
  }

  Time
  FlowElement::ta()
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "FlowElement::ta();id=" << id << "\n";
    }
    bool flag{report_inflow_request
              || report_outflow_achieved
              || report_lossflow_achieved};
    if (flag) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... dt = (0,1);id=" << get_id() << "\n";
      }
      return Time{0, 1};
    }
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "... dt = infinity\n";
    }
    return calculate_time_advance();
  }

  void
  FlowElement::output_func(std::vector<PortValue>& ys)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "FlowElement::output_func();id=" << id << "\n";
    }
    if (report_inflow_request) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... send=>outport_inflow_request;id="
                  << get_id() << "\n";
      }
      ys.emplace_back(
          adevs::port_value<FlowValueType>{outport_inflow_request, inflow});
    }
    if (report_outflow_achieved) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... send=>outport_outflow_achieved;id="
                  << get_id() << "\n";
      }
      ys.emplace_back(
          adevs::port_value<FlowValueType>{outport_outflow_achieved, outflow});
    }
    if (report_lossflow_achieved) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... send=>outport_lossflow_achieved;id="
                  << get_id() << "\n";
      }
      ys.emplace_back(
          adevs::port_value<FlowValueType>{
            outport_lossflow_achieved, lossflow - spillage});
    }
    add_additional_outputs(ys);
  }

  void
  FlowElement::add_additional_outputs(std::vector<PortValue>&)
  {
  }

  FlowState
  FlowElement::update_state_for_outflow_request(FlowValueType outflow_) const
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "FlowElement::update_state_for_outflow_request();id=" << id << "\n";
    }
    return FlowState{outflow_, outflow_};
  }

  FlowState
  FlowElement::update_state_for_inflow_achieved(FlowValueType inflow_) const
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "FlowElement::update_state_for_inflow_achieved();id="
                << id << "\n";
    }
    return FlowState{inflow_, inflow_};
  }

  void
  FlowElement::update_on_internal_transition()
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "FlowElement::update_on_internal_transition();id="
                << id << "\n";
    }
  }

  void
  FlowElement::update_on_external_transition()
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "FlowElement::update_on_external_transition();id="
                << id << "\n";
    }
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
    auto diff{inflow - (outflow + storeflow + lossflow)};
    if (std::fabs(diff) > flow_value_tolerance) {
      std::cout << "FlowElement ERROR! " << inflow << " != " << outflow << " + "
        << storeflow << " + " << lossflow << "!\n";
      throw FlowInvariantError();
    }
  }

  ///////////////////////////////////////////////////////////////////
  // FlowLimits
  FlowLimits::FlowLimits(
      std::string id_,
      ComponentType component_type_,
      const StreamType& stream_type_,
      FlowValueType lower_limit,
      FlowValueType upper_limit):
    FlowElement(
        std::move(id_),
        component_type_,
        ElementType::FlowLimits,
        stream_type_),
    state{lower_limit, upper_limit}
  {
  }

  FlowState
  FlowLimits::update_state_for_outflow_request(FlowValueType out) const
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "FlowLimits::update_state_for_outflow_request(" << out << ")\n";
      print_state("... ");
    }
    FlowValueType out_{0.0};
    if (out > state.get_upper_limit()) {
      out_ = state.get_upper_limit();
    }
    else if (out < state.get_lower_limit()) {
      out_ = state.get_lower_limit();
    }
    else {
      out_ = out;
    }
    if constexpr (debug_level >= debug_level_high) {
      print_state("... ");
      std::cout << "end FlowLimits::update_state_for_outflow_request\n";
    }
    return FlowState{out_, out_};
  }

  FlowState
  FlowLimits::update_state_for_inflow_achieved(FlowValueType in) const
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "FlowLimits::update_state_for_inflow_achieved(" << in << ")\n";
      print_state("... ");
    }
    FlowValueType in_{0.0};
    if (in > state.get_upper_limit()) {
      std::ostringstream oss;
      oss << "AchievedMoreThanRequestedError\n";
      oss << "in > upper_limit\n";
      oss << "in: " << in << "\n";
      oss << "upper_limit: " << state.get_upper_limit() << "\n";
      throw std::runtime_error(oss.str());
    }
    else if (in < state.get_lower_limit()) {
      std::ostringstream oss;
      oss << "AchievedMoreThanRequestedError\n";
      oss << "in < lower_limit\n";
      oss << "in: " << in << "\n";
      oss << "lower_limit: " << state.get_lower_limit() << "\n";
      throw std::runtime_error(oss.str());
    }
    else {
      in_ = in;
    }
    if constexpr (debug_level >= debug_level_high) {
      print_state("... ");
      std::cout << "end FlowLimits::update_state_for_inflow_achieved\n";
    }
    return FlowState{in_, in_};
  }

  ////////////////////////////////////////////////////////////
  // FlowMeter
  FlowMeter::FlowMeter(
      std::string id,
      ComponentType component_type,
      const StreamType& stream_type) :
    FlowElement(
        std::move(id),
        component_type,
        ElementType::FlowMeter,
        stream_type),
    event_times{},
    requested_flows{},
    achieved_flows{},
    flow_writer{}
  {
  }

  std::vector<RealTimeType>
  FlowMeter::get_event_times() const
  {
    return std::vector<RealTimeType>(event_times);
  }

  std::vector<FlowValueType>
  FlowMeter::get_achieved_flows() const
  {
    return std::vector<FlowValueType>(achieved_flows);
  }

  std::vector<FlowValueType>
  FlowMeter::get_requested_flows() const
  {
    return std::vector<FlowValueType>(requested_flows);
  }

  std::vector<Datum>
  FlowMeter::get_results(RealTimeType max_time) const
  {
    using size_type_D = std::vector<Datum>::size_type;
    using size_type_RTT = std::vector<RealTimeType>::size_type;
    const auto num_events{event_times.size()};
    const auto num_rfs = requested_flows.size();
    const auto num_afs = achieved_flows.size();
    if (num_events == 0) {
      std::vector<Datum> results{};
      results.emplace_back(Datum{0, 0.0, 0.0});
      results.emplace_back(Datum{max_time, 0.0, 0.0});
      return results;
    }
    if ((num_rfs != num_events) || (num_afs != num_events)) {
      std::ostringstream oss;
      oss << "invariant_error: requested_flows.size() "
          << "!= achieved_flows.size() != num_events\n"
          << "requested_flows.size(): " << num_rfs << "\n"
          << "achieved_flows.size() : " << num_afs << "\n"
          << "num_events            : " << num_events << "\n"
          << "id                    : \"" << get_id() << "\"\n";
      for (size_type_RTT i{0}; i < num_events; ++i) {
        oss << "event_times[" << i << "]      = " << event_times[i] << "\n";
        if (i < num_rfs) {
          oss << "requested_flows[" << i << "]  = "
              << requested_flows[i] << "\n";
        }
        if (i < num_afs) {
          oss << "achieved_flows[" << i << "]   = "
              << achieved_flows[i] << "\n";
        }
      }
      throw std::runtime_error(oss.str());
    }
    size_type_D max_idx = 0;
    for (size_type_D i{0}; i < num_events; ++i) {
      const auto& t = event_times[i];
      if (t <= max_time) {
        max_idx = i;
      }
      else {
        break;
      }
    }
    bool time_0_missing{event_times[0] != 0};
    bool max_time_missing{event_times[max_idx] < max_time};
    size_type_D num_datums = num_events;
    if (time_0_missing) {
      ++num_datums;
    }
    if (max_time_missing) {
      ++num_datums;
    }
    std::vector<Datum> results(num_datums);
    for (size_type_D i{0}; i < num_datums; ++i) {
      auto j = i;
      if (time_0_missing) {
        if (i == 0) {
          results[0] = Datum{0, 0.0, 0.0};
          continue;
        }
        --j;
      }
      if (max_time_missing && (j > max_idx)) {
        results[i] = Datum{max_time, 0.0, 0.0};
      }
      else {
        results[i] = Datum{
          event_times[j],
          requested_flows[j],
          achieved_flows[j]};
      }
    }
    return results;
  }

  void
  FlowMeter::clear_results()
  {
    event_times.clear();
    requested_flows.clear();
    achieved_flows.clear();
  }

  void
  FlowMeter::update_on_external_transition()
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "FlowMeter::update_on_external_transition()\n";
      print_state("... ");
      print_vec<RealTimeType>("... event_times", event_times);
      print_vec<FlowValueType>("... requested_flows", requested_flows);
      print_vec<FlowValueType>("... achieved_flows", achieved_flows);
    }
    auto num_events{event_times.size()};
    auto real_time{get_real_time()};
    RealTimeType t_last{-1};
    if (num_events > 0) {
      t_last = event_times.back();
    }
    if (real_time > t_last) {
      event_times.emplace_back(real_time);
      ++num_events;
    }
    auto num_requested{requested_flows.size()};
    auto num_achieved{achieved_flows.size()};
    if (get_report_inflow_request()) {
      if (num_requested == (num_events - 1)) {
        requested_flows.emplace_back(get_inflow());
        ++num_requested;
      }
      else if (num_requested == num_events) {
        *requested_flows.rbegin() = get_inflow();
      }
      else {
        std::ostringstream oss;
        oss << "unexpected condition 1\n";
        throw std::runtime_error(oss.str());
      }
      if (num_achieved == num_events) {
        auto &v = achieved_flows.back();
        v = get_inflow();
      }
      else if (num_achieved == (num_events - 1)) {
        achieved_flows.emplace_back(get_inflow());
        ++num_achieved;
      }
      else {
        std::ostringstream oss;
        oss << "unexpected condition 2\n";
        throw std::runtime_error(oss.str());
      }
    }
    if (get_report_outflow_achieved()) {
      auto of = get_outflow();
      if (num_achieved == num_events) {
        auto &v = achieved_flows.back();
        v = of;
      }
      else {
        achieved_flows.emplace_back(of);
        ++num_achieved;
      }
      if (num_requested < num_achieved) {
        if (num_requested == 0) {
          std::ostringstream oss;
          oss << "no previous requested flows and an achieved flow shows up\n";
          oss << "num_requested: " << num_requested << "\n";
          oss << "num_achieved: " << num_achieved << "\n";
          oss << "id: \"" << get_id() << "\"\n";
          throw std::runtime_error(oss.str());
        }
        else {
          // repeat the previous request -- requests don't change if upstream
          // conditions change.
          requested_flows.emplace_back(requested_flows.back());
        }
        ++num_requested;
      }
    }
    if ((num_requested != num_achieved) && (num_events != num_achieved)) {
      std::ostringstream oss;
      oss << "FlowMeter: ";
      oss << "invariant error: num_requested != num_achieved != num_events\n";
      oss << "num_requested: " << num_requested << "\n";
      oss << "num_achieved : " << num_achieved << "\n";
      oss << "num_events   : " << num_events << "\n";
      oss << "id           : \"" << get_id() << "\"\n";
      throw std::runtime_error(oss.str());
    }
    if constexpr (debug_level >= debug_level_high) {
      print_state("... ");
      print_vec<RealTimeType>("... event_times", event_times);
      print_vec<FlowValueType>("... requested_flows", requested_flows);
      print_vec<FlowValueType>("... achieved_flows", achieved_flows);
      std::cout << "end FlowMeter::update_on_external_transition()\n";
    }
  }

  ////////////////////////////////////////////////////////////
  // Converter
  Converter::Converter(
      std::string id,
      ComponentType component_type,
      StreamType input_stream_type,
      StreamType output_stream_type,
      std::function<FlowValueType(FlowValueType)> calc_output_from_input,
      std::function<FlowValueType(FlowValueType)> calc_input_from_output):
    FlowElement(
        std::move(id),
        component_type,
        ElementType::Converter,
        std::move(input_stream_type),
        std::move(output_stream_type)),
    output_from_input{std::move(calc_output_from_input)},
    input_from_output{std::move(calc_input_from_output)}
  {
  }

  FlowState
  Converter::update_state_for_outflow_request(FlowValueType outflow_) const
  {
    return FlowState{input_from_output(outflow_), outflow_};
  }

  FlowState
  Converter::update_state_for_inflow_achieved(FlowValueType inflow_) const
  {
    return FlowState{inflow_, output_from_input(inflow_)};
  }

  ///////////////////////////////////////////////////////////////////
  // Sink
  Sink::Sink(
      std::string id,
      ComponentType component_type,
      const StreamType& st,
      const std::vector<LoadItem>& loads_):
    FlowElement(
        std::move(id),
        component_type,
        ElementType::Sink,
        st),
    loads{loads_},
    idx{-1},
    num_loads{loads_.size()}
  {
    check_loads();
  }

  void
  Sink::update_on_internal_transition()
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "Sink::update_on_internal_transition()\n";
    }
    ++idx;
  }

  Time
  Sink::calculate_time_advance()
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "Sink::calculate_time_advance()\n";
      std::cout << "id  = " << get_id() << "\n";
      std::cout << "idx = " << idx << "\n";
    }
    if (idx < 0) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... dt = (0, 0)\n";
      }
      return Time{0, 0};
    }
    std::vector<LoadItem>::size_type next_idx =
      static_cast<std::vector<LoadItem>::size_type>(idx) + 1;
    if (next_idx < num_loads) {
      RealTimeType dt{loads[idx].get_time_advance(loads[next_idx])};
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... dt = (" << dt << ", 0)\n";
        std::cout << "loads[" << idx << "].is_end = "
                  << loads[idx].get_is_end() << "\n"
                  << "loads[" << idx << "].time = "
                  << loads[idx].get_time() << "\n";
        if (!loads[idx].get_is_end()) {
          std::cout << "loads[" << idx << "].value = "
                    << loads[idx].get_value() << "\n";
        }
        std::cout << "loads[" << next_idx << "].is_end = "
                  << loads[next_idx].get_is_end() << "\n"
                  << "loads[" << next_idx << "].time = "
                  << loads[next_idx].get_time() << "\n";
        if (!loads[next_idx].get_is_end()) {
          std::cout << "loads[" << next_idx << "].value = "
                    << loads[next_idx].get_value() << "\n";
        }
      }
      return Time{dt, 0};
    }
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "... dt = infinity\n";
    }
    return inf;
  }

  FlowState
  Sink::update_state_for_inflow_achieved(FlowValueType inflow_) const
  {
    return FlowState{inflow_};
  }

  void
  Sink::add_additional_outputs(std::vector<PortValue>& ys)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "Sink::output_func()\n";
    }
    std::vector<LoadItem>::size_type next_idx =
      static_cast<std::vector<LoadItem>::size_type>(idx) + 1;
    auto max_idx{num_loads - 1};
    if (next_idx < max_idx) {
      ys.emplace_back(
          adevs::port_value<FlowValueType>{
            outport_inflow_request, loads[next_idx].get_value()});
    } else if (next_idx == max_idx) {
      ys.emplace_back(
          adevs::port_value<FlowValueType>{
            outport_inflow_request, 0.0});
    }
  }

  void
  Sink::check_loads() const
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "Sink::check_loads\n";
    }
    auto N{loads.size()};
    auto last_idx{N - 1};
    if (N < 2) {
      std::ostringstream oss;
      oss << "Sink: must have at least two LoadItems but "
             "only has " << N << std::endl;
      throw std::invalid_argument(oss.str());
    }
    RealTimeType t{-1};
    for (std::vector<LoadItem>::size_type idx_=0; idx_ < loads.size(); ++idx_) {
      const auto& x{loads.at(idx_)};
      auto t_{x.get_time()};
      if (idx_ == last_idx) {
        if (!x.get_is_end()) {
          std::ostringstream oss;
          oss << "Sink: LoadItem[" << idx_ << "] (last index) "
                 "must not specify a value but it does..."
              << std::endl;
          throw std::invalid_argument(oss.str());
        }
      } else {
        if (x.get_is_end()) {
          std::ostringstream oss;
          oss << "Sink: non-last LoadItem[" << idx_ << "] "
                 "doesn't specify a value but it must..."
              << std::endl;
          throw std::invalid_argument(oss.str());
        }
      }
      if ((t_ < 0) || (t_ <= t)) {
        std::ostringstream oss;
        oss << "Sink: LoadItems must have time points that are everywhere "
               "increasing and positive but it doesn't..."
            << std::endl;
        throw std::invalid_argument(oss.str());
      }
    }
  }

  ////////////////////////////////////////////////////////////
  // MuxerDispatchStrategy
  MuxerDispatchStrategy
  tag_to_muxer_dispatch_strategy(const std::string& tag)
  {
    if (tag == "in_order") {
      return MuxerDispatchStrategy::InOrder;
    }
    else if (tag == "distribute") {
      return MuxerDispatchStrategy::Distribute;
    }
    std::ostringstream oss;
    oss << "unhandled tag '" << tag << "' for Muxer_dispatch_strategy\n";
    throw std::runtime_error(oss.str());
  }

  std::string
  muxer_dispatch_strategy_to_string(MuxerDispatchStrategy mds)
  {
    switch (mds) {
      case MuxerDispatchStrategy::InOrder:
        return std::string{"in_order"};
      case MuxerDispatchStrategy::Distribute:
        return std::string{"distribute"};
    }
    std::ostringstream oss;
    oss << "unhandled Muxer_dispatch_strategy '"
      << static_cast<int>(mds) << "'\n";
    throw std::runtime_error(oss.str());
  }

  ////////////////////////////////////////////////////////////
  // Mux
  Mux::Mux(
      std::string id,
      ComponentType ct,
      const StreamType& st,
      int num_inflows_,
      int num_outflows_,
      MuxerDispatchStrategy strategy_,
      MuxerDispatchStrategy outflow_strategy_):
    FlowElement(
        std::move(id),
        ct,
        ElementType::Mux,
        st),
    num_inflows{num_inflows_},
    num_outflows{num_outflows_},
    strategy{strategy_},
    outflow_strategy{outflow_strategy_},
    inflows{},
    prev_inflows{},
    inflows_achieved{},
    outflows{},
    prev_outflows{},
    outflow_requests{}
  {
    const int min_ports = 1;
    if ((num_inflows < min_ports) || (num_inflows > max_port_numbers)) {
      std::ostringstream oss;
      oss << "Number of inflows on Mux must be " << min_ports
          << " <= num_inflows <= "
          << max_port_numbers << "; "
          << "num_inflows = " << num_inflows << "\n";
      throw std::invalid_argument(oss.str());
    }
    if ((num_outflows < min_ports) || (num_outflows > max_port_numbers)) {
      std::ostringstream oss;
      oss << "Number of outflows on Mux must be " << min_ports
          << " <= num_outflows <= "
          << max_port_numbers << "; "
          << "num_outflows = " << num_outflows << "\n";
      throw std::invalid_argument(oss.str());
    }
    inflows = std::vector<FlowValueType>(num_inflows, 0.0);
    prev_inflows = std::vector<FlowValueType>(num_inflows, 0.0);
    inflows_achieved = std::vector<FlowValueType>(num_inflows, 0.0);
    outflows = std::vector<FlowValueType>(num_outflows, 0.0);
    prev_outflows = std::vector<FlowValueType>(num_outflows, 0.0);
    outflow_requests = std::vector<FlowValueType>(num_outflows, 0.0);
  }

  void
  Mux::delta_ext(Time e, std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "Mux::delta_ext();id=" << get_id() << "\n";
    }
    update_time(e);
    bool inflow_provided{false};
    bool outflow_provided{false};
    int highest_inflow_port_received{-1};
    int highest_outflow_port_received{-1};
    for (const auto& x : xs) {
      int port = x.port;
      int port_n_ia = port - inport_inflow_achieved;
      int port_n_or = port - inport_outflow_request;
      int port_n;
      if ((port_n_ia >= 0) && (port_n_ia < num_inflows)) {
        port_n = port_n_ia;
        if (port_n > highest_inflow_port_received) {
          highest_inflow_port_received = port_n;
        }
        if constexpr (debug_level >= debug_level_high) {
          std::cout << "... receive<=inport_inflow_achieved(" << port_n << ");id="
                    << get_id() << "\n";
        }
        prev_inflows[port_n] = x.value;
        inflows_achieved[port_n] = x.value;
        inflow_provided = true;
      }
      else if ((port_n_or >= 0) && (port_n_or < num_outflows)) {
        port_n = port_n_or;
        if (port_n > highest_outflow_port_received) {
          highest_outflow_port_received = port_n;
        }
        if constexpr (debug_level >= debug_level_high) {
          std::cout << "... receive<=inport_outflow_request(" << port_n << ");id="
                    << get_id() << "\n";
        }
        // by setting both prev_outflows and outflow_requests we prevent
        // reporting back downstream to the component. If, however, our
        // outflows end up differing from requests, we will report.
        prev_outflows[port_n] = x.value;
        outflow_requests[port_n] = x.value;
        outflow_provided = true;
      }
      else {
        std::ostringstream oss;
        oss << "BadPortError: unhandled port: \"" << x.port << "\"";
        throw std::runtime_error(oss.str());
      }
    }
    FlowValueType total_inflow_achieved{0.0};
    for (const auto x: inflows_achieved) {
      total_inflow_achieved += x;
    }
    FlowValueType total_outflow_request{0.0};
    for (const auto x: outflow_requests) {
      total_outflow_request += x;
    }
    if (inflow_provided && !outflow_provided) {
      set_report_outflow_achieved(true);
      if (total_inflow_achieved > total_outflow_request) {
        std::ostringstream oss;
        oss << "AchievedMoreThanRequestedError\n";
        oss << "total_outflow_request > inflow\n";
        oss << "inflow: " << get_inflow() << "\n";
        oss << "total_inflow_achieved: " << total_inflow_achieved << "\n";
        oss << "total_outflow_request: " << total_outflow_request << "\n";
        oss << "id: " << get_id() << "\n";
        oss << "time: (" << get_real_time() << ", "
                         << get_logical_time() << ")\n";
        throw std::runtime_error(oss.str());
      }
      if (strategy == MuxerDispatchStrategy::InOrder) {
        auto diff = total_outflow_request - total_inflow_achieved;
        if (std::abs(diff) <= flow_value_tolerance) {
          // We met the loads.
          update_state(FlowState{total_inflow_achieved});
          inflows = inflows_achieved;
        }
        else if (diff < neg_flow_value_tol) {
          // we're oversupplying... reset
          set_report_outflow_achieved(false);
          set_report_inflow_request(true);
          inflows = std::vector<FlowValueType>(num_inflows, 0.0);
          std::vector<FlowValueType>::size_type inflow_port_for_request = 0;
          inflows[inflow_port_for_request] = total_outflow_request;
          inflows_achieved = inflows;
        }
        else {
          // we're undersupplying
          const auto final_inflow_port = num_inflows - 1;
          if (highest_inflow_port_received < final_inflow_port) {
            std::vector<FlowValueType>::size_type inflow_port_for_request =
              static_cast<std::vector<FlowValueType>::size_type>(
                highest_inflow_port_received) + 1;
            set_report_outflow_achieved(false);
            set_report_inflow_request(true);
            inflows = inflows_achieved;
            inflows[inflow_port_for_request] = diff;
            inflows_achieved = inflows;
          }
          else {
            // We've requested to all inflow ports and are still
            // deficient on meeting outflow request. Update outflows.
            if constexpr (debug_level >= debug_level_high) {
              std::ostringstream oss{};
              oss << "outflow request > inflow available\n";
            }
            update_state(FlowState{total_inflow_achieved});
            inflows = inflows_achieved;
            switch (outflow_strategy) {
              case MuxerDispatchStrategy::InOrder:
                {
                  // We start satisfying loads one by one in port order until there
                  // is nothing left
                  if constexpr (debug_level >= debug_level_high) {
                    std::ostringstream oss{};
                    oss << "InOrder outflow_strategy\n";
                    oss << "get_report_inflow_request() = "
                        << get_report_inflow_request() << "\n";
                    oss << "get_report_outflow_achieved() = "
                        << get_report_outflow_achieved() << "\n";
                  }
                  FlowValueType remaining_inflow{total_inflow_achieved};
                  using size_type = std::vector<FlowValueType>::size_type;
                  auto num_ofs = outflows.size();
                  for (size_type of_idx{0}; of_idx < num_ofs; ++of_idx) {
                    if (outflow_requests[of_idx] >= remaining_inflow) {
                      outflows[of_idx] = remaining_inflow;
                    } else {
                      outflows[of_idx] = outflow_requests[of_idx];
                    }
                    remaining_inflow -= outflows[of_idx];
                  }
                  break;
                }
              case MuxerDispatchStrategy::Distribute:
                {
                  // The inflows are distributed evenly over the outflows
                  if (total_outflow_request != 0.0) {
                    FlowValueType reduction_factor{1.0};
                    reduction_factor = total_inflow_achieved / total_outflow_request;
                    for (auto& of_item: outflows) {
                      of_item *= reduction_factor;
                    }
                  }
                  break;
                }
              default:
                {
                  std::ostringstream oss;
                  oss << "unhandled output_strategy for Mux " << get_id() << "\n";
                  throw std::runtime_error(oss.str());
                }
            }
            if constexpr (debug_level >= debug_level_high) {
              std::cout
                << "muxer dispatch (in)   : "
                << muxer_dispatch_strategy_to_string(strategy) << "\n"
                << "muxer dispatch (out)  : "
                << muxer_dispatch_strategy_to_string(outflow_strategy) << "\n"
                << "inflow                : " << get_inflow() << "\n"
                << "outflow               : " << get_outflow() << "\n"
                << "highest_inflow_port_received: "
                << highest_inflow_port_received << "\n"
                << "highest_outflow_port_received: "
                << highest_outflow_port_received << "\n"
                << "num_inflows           : " << num_inflows << "\n"
                << "num_outflows          : " << num_outflows << "\n"
                << "total_inflow_achieved : " << total_inflow_achieved << "\n"
                << "total_outflow_request : " << total_outflow_request << "\n";
              int i{0};
              for (const auto& n : inflows) {
                std::cout << "inflows[" << i << "]=" << n << "\n";
                ++i;
              }
              i = 0;
              for (const auto& n : outflows) {
                std::cout << "outflows[" << i << "]=" << n << "\n";
                ++i;
              }
            }
          }
        }
      }
      else {
        std::ostringstream oss;
        oss << "unhandled dispatch strategy for Mux: \""
            << static_cast<int>(strategy) << "\"";
        throw std::runtime_error(oss.str());
      }
    }
    else if (outflow_provided && !inflow_provided) {
      set_report_inflow_request(true);
      set_report_outflow_achieved(true);
      if (strategy == MuxerDispatchStrategy::InOrder) {
        // Whenever the outflow request updates, we always start querying
        // inflows from the first inflow port and update the inflow_request.
        std::vector<FlowValueType>::size_type inflow_port_for_request = 0;
        inflows = std::vector<FlowValueType>(num_inflows, 0.0);
        inflows[inflow_port_for_request] = total_outflow_request;
        inflows_achieved = inflows;
        update_state(FlowState{total_outflow_request});
        outflows = outflow_requests;
      }
      else {
        std::ostringstream oss;
        oss << "unhandled dispatch strategy for Mux: \""
            << static_cast<int>(strategy) << "\"";
        throw std::runtime_error(oss.str());
      }
      if (get_outflow() > total_outflow_request) {
        std::ostringstream oss;
        oss << "AchievedMoreThanRequestedError\n";
        oss << "outflow > total_outflow_request\n";
        oss << "outflow: " << get_outflow() << "\n";
        oss << "total_outflow_request: " << total_outflow_request << "\n";
        throw std::runtime_error(oss.str());
      }
    }
    else if (inflow_provided && outflow_provided) {
      if (total_inflow_achieved > total_outflow_request) {
        std::ostringstream oss;
        oss << "SimultaneousIORequestError: assumption was we'd never get here...\n";
        oss << "... id=" << get_id() << "\n";
        oss << "... total_inflow_achieved=" << total_inflow_achieved << "\n";
        oss << "... total_outflow_request=" << total_outflow_request << "\n";
        throw std::runtime_error(oss.str());
      }
      if (total_inflow_achieved < total_outflow_request) {
        // Below is true but not used. Here for reference.
        // total_outflow_request = total_inflow_achieved;
        set_report_outflow_achieved(true);
      }
    }
    else {
      std::ostringstream oss;
      oss << "Unhandled port IO case...;"
          << "id=" << get_id() << "\n";
      throw std::runtime_error(oss.str());
    }
    if (get_report_inflow_request() || get_report_outflow_achieved()) {
      update_on_external_transition();
      check_flow_invariants();
    }
  }

  void
  Mux::output_func(std::vector<PortValue>& ys)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "Mux::output_func();id=" << get_id() << "\n";
    }
    if (get_report_inflow_request()) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... send=>outport_inflow_request;id="
                  << get_id() << "\n";
      }
      if (strategy == MuxerDispatchStrategy::InOrder) {
        for (int i{0}; i < num_inflows; ++i) {
          auto inflow_ = inflows[i];
          auto prev_inflow_ = prev_inflows[i];
          if (prev_inflow_ != inflow_) {
            ys.emplace_back(
                adevs::port_value<FlowValueType>{
                  outport_inflow_request + i,
                  inflow_});
            prev_inflows[i] = inflow_;
          }
        }
      }
      else {
        std::ostringstream oss;
        oss << "unhandled strategy \"" << static_cast<int>(strategy)
            << "\"";
        throw std::runtime_error(oss.str());
      }
    }
    if (get_report_outflow_achieved()) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... send=>outport_outflow_achieved;id="
                  << get_id() << "\n";
        std::cout << "... t = " << get_real_time() << ";id="
                  << get_id() << "\n";
      }
      for (int i{0}; i < num_outflows; ++i) {
        auto outflow_ = outflows[i];
        auto prev_outflow_ = prev_outflows[i];
        if constexpr (debug_level >= debug_level_high) {
          std::cout << "... outflow[" << i << "] = "
                    << outflow_ << "\n";
          std::cout << "... prev_outflow[" << i << "] = "
                    << prev_outflow_ << "\n";
        }
        if (prev_outflow_ != outflow_) {
          ys.emplace_back(
              adevs::port_value<FlowValueType>{
                outport_outflow_achieved + i,
                outflow_});
          prev_outflows[i] = outflow_;
        }
      }
    }
  }
}
