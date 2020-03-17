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
    is_final{false},
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

  DefaultFlowWriter::DefaultFlowWriter(
      bool is_final_,
      RealTimeType current_time_,
      int next_id_,
      const std::vector<Datum>& current_status_,
      const std::unordered_map<std::string, int>& element_tag_to_id_,
      const std::unordered_map<int, std::string>& element_id_to_tag_,
      const std::unordered_map<int, std::string>& element_id_to_stream_tag_,
      const std::vector<bool>& recording_flags_,
      std::vector<std::vector<Datum>>&& history_):
    FlowWriter(),
    is_final{is_final_},
    current_time{current_time_},
    next_id{next_id_},
    current_status{current_status_},
    element_tag_to_id{element_tag_to_id_},
    element_id_to_tag{element_id_to_tag_},
    element_id_to_stream_tag{element_id_to_stream_tag_},
    recording_flags{recording_flags_},
    history{std::move(history_)}
  {
  }

  std::unique_ptr<FlowWriter>
  DefaultFlowWriter::clone() const
  {
    std::vector<std::vector<Datum>> new_history(num_elements());
    for (size_type_H i{0}; i < static_cast<size_type_H>(num_elements()); ++i) {
      std::vector<Datum> xs{history[i]};
      new_history[i] = std::move(xs);
    }
    std::unique_ptr<FlowWriter> p = std::make_unique<DefaultFlowWriter>(
        is_final,
        current_time,
        next_id,
        current_status,
        element_tag_to_id,
        element_id_to_tag,
        element_id_to_stream_tag,
        recording_flags,
        std::move(new_history));
    return p;
  }

  int
  DefaultFlowWriter::register_id(
      const std::string& element_tag,
      const std::string& stream_tag,
      bool record_history)
  {
    ensure_not_final();
    auto element_id = get_next_id();
    ensure_element_tag_is_unique(element_tag);
    current_status.emplace_back(Datum{current_time,0.0,0.0});
    element_tag_to_id.emplace(std::make_pair(element_tag, element_id));
    element_id_to_tag.emplace(std::make_pair(element_id, element_tag));
    element_id_to_stream_tag.emplace(std::make_pair(element_id, stream_tag));
    recording_flags.emplace_back(record_history);
    history.emplace_back(std::vector<Datum>{});
    return element_id;
  }

  void
  DefaultFlowWriter::ensure_not_final() const
  {
    if (is_final) {
      throw std::runtime_error("invalid operation on a finalized FlowWriter");
    }
  }

  void
  DefaultFlowWriter::ensure_element_tag_is_unique(
      const std::string& element_tag) const
  {
    auto it = element_tag_to_id.find(element_tag);
    if (it != element_tag_to_id.end()) {
      std::ostringstream oss;
      oss << "element_tag '" << element_tag << "' already registered!";
      throw std::invalid_argument(oss.str());
    }
  }

  void
  DefaultFlowWriter::ensure_element_id_is_valid(int element_id) const
  {
    auto num = num_elements();
    if ((element_id >= num) || (element_id < 0)) {
      std::ostringstream oss;
      oss << "element_id is invalid. "
          << "element_id must be >= 0 and < " << num_elements() << "\n"
          << "and element_id cannot be larger than the number of elements - 1\n"
          << "element_id: " << element_id << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  void
  DefaultFlowWriter::ensure_time_is_valid(RealTimeType time) const
  {
    if (time < current_time) {
      std::ostringstream oss;
      oss << "time is invalid. "
          << "This value indicates time is flowing backward!\n"
          << "time: " << time << "\n"
          << "current_time: " << current_time << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  void
  DefaultFlowWriter::record_history_and_update_current_time(RealTimeType time)
  {
    auto num = num_elements();
    auto num_st = static_cast<size_type_D>(num);
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

  void
  DefaultFlowWriter::write_data(
      int element_id,
      RealTimeType time,
      FlowValueType requested_flow,
      FlowValueType achieved_flow)
  {
    ensure_not_final();
    ensure_element_id_is_valid(element_id);
    ensure_time_is_valid(time);
    if (time > current_time) {
      record_history_and_update_current_time(time);
    }
    current_status[element_id] = Datum{time,requested_flow,achieved_flow};
  }

  void
  DefaultFlowWriter::finalize_at_time(RealTimeType time)
  {
    is_final = true;
    auto num = num_elements();
    auto num_st = static_cast<size_type_D>(num);
    auto d_final = Datum{time,0.0,0.0};
    for (size_type_D i{0}; i < num_st; ++i) {
      if (recording_flags[i]) {
        if (current_time < time) {
          history[i].emplace_back(current_status[i]);
        }
        history[i].emplace_back(d_final);
      }
    }
    current_time = time;
    current_status.clear();
  }

  std::unordered_map<std::string, std::vector<Datum>>
  DefaultFlowWriter::get_results() const
  {
    std::unordered_map<std::string, std::vector<Datum>> out{};
    for (const auto& tag_id: element_tag_to_id) {
      auto element_tag = tag_id.first;
      auto element_id = tag_id.second;
      if (recording_flags[element_id]) {
        out[element_tag] = history[element_id];
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
    is_final = false;
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
    state{erin::devs::make_flow_limits_state(lower_limit, upper_limit)}
  {
  }

  void
  FlowLimits::delta_int()
  {
    state = erin::devs::flow_limits_internal_transition(state);
  }

  void
  FlowLimits::delta_ext(Time dt, std::vector<PortValue>& xs)
  {
    state = erin::devs::flow_limits_external_transition(
        state, dt.real, xs);
  }

  void
  FlowLimits::delta_conf(std::vector<PortValue>& xs)
  {
    state = erin::devs::flow_limits_confluent_transition(state, xs);
  }

  Time
  FlowLimits::ta()
  {
    auto dt = erin::devs::flow_limits_time_advance(state);
    if (dt == erin::devs::infinity)
      return inf;
    return Time{dt, 1};
  }

  void
  FlowLimits::output_func(std::vector<PortValue>& ys)
  {
    erin::devs::flow_limits_output_function_mutable(state, ys);
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
    flow_writer{nullptr},
    element_id{-1}
  {
  }

  void
  FlowMeter::set_flow_writer(const std::shared_ptr<FlowWriter>& writer)
  {
    flow_writer = writer;
    bool record_history{true};
    element_id = flow_writer->register_id(
        get_id(),
        get_outflow_type().get_type(),
        record_history);
    flow_writer->write_data(
        element_id,
        get_real_time(),
        get_outflow_request(),
        get_outflow());
  }

  void
  FlowMeter::update_on_external_transition()
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "FlowMeter::update_on_external_transition()\n";
      print_state("... ");
      std::cout << "... element_id = " << element_id << "\n";
    }
    auto real_time{get_real_time()};
    if (element_id >= 0) {
      flow_writer->write_data(
          element_id,
          real_time,
          get_outflow_request(),
          get_outflow());
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
    state{
      erin::devs::make_converter_state(
          calc_output_from_input, calc_input_from_output)},
    output_from_input{std::move(calc_output_from_input)},
    input_from_output{std::move(calc_input_from_output)}
  {
  }

  void
  Converter::delta_int()
  {
    state = erin::devs::converter_internal_transition(state);
  }

  void
  Converter::delta_ext(Time e, std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "Converter::detla_ext("
                << "e=Time{" << e.real << "," << e.logical << "}, "
                << "xs=" << vec_to_string<PortValue>(xs) << ")\n";
      std::cout << "id = " << get_id() << "\n";
    }
    state = erin::devs::converter_external_transition(state, e.real, xs);
  }

  void
  Converter::delta_conf(std::vector<PortValue>& xs)
  {
    state = erin::devs::converter_confluent_transition(state, xs);
  }

  Time
  Converter::ta()
  {
    auto dt = erin::devs::converter_time_advance(state);
    if (dt == erin::devs::infinity)
      return inf;
    return Time{dt, 1};
  }

  void
  Converter::output_func(std::vector<PortValue>& ys)
  {
    erin::devs::converter_output_function_mutable(state, ys);
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
    state{erin::devs::make_load_state(loads_)}
  {
  }

  void
  Sink::delta_int()
  {
    state = erin::devs::load_internal_transition(state);
  }

  void
  Sink::delta_ext(Time e, std::vector<PortValue>& xs)
  {
    state = erin::devs::load_external_transition(state, e.real, xs);
  }

  void
  Sink::delta_conf(std::vector<PortValue>& xs)
  {
    state = erin::devs::load_confluent_transition(state, xs);
  }

  Time
  Sink::ta()
  {
    auto dt = erin::devs::load_time_advance(state);
    if (dt == erin::devs::infinity)
      return inf;
    return Time{dt, 1};
  }

  void
  Sink::output_func(std::vector<PortValue>& ys)
  {
    erin::devs::load_output_function_mutable(state, ys);
  }

  ////////////////////////////////////////////////////////////
  // MuxerDispatchStrategy
  //MuxerDispatchStrategy
  //tag_to_muxer_dispatch_strategy(const std::string& tag)
  //{
  //  if (tag == "in_order") {
  //    return MuxerDispatchStrategy::InOrder;
  //  }
  //  else if (tag == "distribute") {
  //    return MuxerDispatchStrategy::Distribute;
  //  }
  //  std::ostringstream oss;
  //  oss << "unhandled tag '" << tag << "' for Muxer_dispatch_strategy\n";
  //  throw std::runtime_error(oss.str());
  //}

  //std::string
  //muxer_dispatch_strategy_to_string(MuxerDispatchStrategy mds)
  //{
  //  switch (mds) {
  //    case MuxerDispatchStrategy::InOrder:
  //      return std::string{"in_order"};
  //    case MuxerDispatchStrategy::Distribute:
  //      return std::string{"distribute"};
  //  }
  //  std::ostringstream oss;
  //  oss << "unhandled Muxer_dispatch_strategy '"
  //    << static_cast<int>(mds) << "'\n";
  //  throw std::runtime_error(oss.str());
  //}

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
    state{erin::devs::make_mux_state(
        num_inflows_,
        num_outflows_,
        outflow_strategy_)},
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
  Mux::delta_int()
  {
    state = erin::devs::mux_internal_transition(state);
  }

  void
  Mux::update_outflows_using_inorder_dispatch(FlowValueType remaining_inflow)
  {
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
  }

  void
  Mux::delta_ext(Time e, std::vector<PortValue>& xs)
  {
    state = erin::devs::mux_external_transition(state, e.real, xs);
    //if constexpr (debug_level >= debug_level_high) {
    //  std::cout << "Mux::delta_ext();"
    //            << "id=" << get_id() << ";"
    //            << "time={" << get_real_time() << ", " << get_logical_time() << "}\n";
    //  bool first{true};
    //  std::cout << "xs=";
    //  for (const auto& x : xs) {
    //    if (first) {
    //      first = false;
    //      std::cout << "[";
    //    } else {
    //      std::cout << ", ";
    //    }
    //    std::cout << "{port=" << x.port << ", value=" << x.value << "}";
    //  }
    //  if (xs.size() > 0)
    //    std::cout << "]\n";
    //  else
    //    std::cout << "[]\n";
    //}
    //update_time(e);
    //bool inflow_provided{false};
    //bool outflow_provided{false};
    //int highest_inflow_port_received{-1};
    //int highest_outflow_port_received{-1};
    //for (const auto& x : xs) {
    //  int port = x.port;
    //  int port_n_ia = port - inport_inflow_achieved;
    //  int port_n_or = port - inport_outflow_request;
    //  int port_n;
    //  if ((port_n_ia >= 0) && (port_n_ia < num_inflows)) {
    //    port_n = port_n_ia;
    //    if (port_n > highest_inflow_port_received) {
    //      highest_inflow_port_received = port_n;
    //    }
    //    prev_inflows[port_n] = x.value;
    //    inflows_achieved[port_n] = x.value;
    //    inflow_provided = true;
    //    if constexpr (debug_level >= debug_level_high) {
    //      std::cout << "... receive<=inport_inflow_achieved(" << port_n << ");id="
    //                << get_id() << "\n";
    //      std::cout << "... prev_inflows = " << vec_to_string(prev_inflows) << "\n";
    //      std::cout << "... inflows_achieved = " << vec_to_string(inflows_achieved) << "\n";
    //    }
    //  }
    //  else if ((port_n_or >= 0) && (port_n_or < num_outflows)) {
    //    port_n = port_n_or;
    //    if (port_n > highest_outflow_port_received) {
    //      highest_outflow_port_received = port_n;
    //    }
    //    // by setting both prev_outflows and outflow_requests we prevent
    //    // reporting back downstream to the component. If, however, our
    //    // outflows end up differing from requests, we will report.
    //    prev_outflows[port_n] = x.value;
    //    outflow_requests[port_n] = x.value;
    //    outflow_provided = true;
    //    if constexpr (debug_level >= debug_level_high) {
    //      std::cout << "... receive<=inport_outflow_request(" << port_n << ");id="
    //                << get_id() << "\n";
    //      std::cout << "... prev_outflows    = " << vec_to_string(prev_outflows) << "\n";
    //      std::cout << "... outflow_requests = " << vec_to_string(outflow_requests) << "\n";
    //    }
    //  }
    //  else {
    //    std::ostringstream oss;
    //    oss << "BadPortError: unhandled port: \"" << x.port << "\"";
    //    throw std::runtime_error(oss.str());
    //  }
    //}
    //FlowValueType total_inflow_achieved{0.0};
    //for (const auto x: inflows_achieved) {
    //  total_inflow_achieved += x;
    //}
    //FlowValueType total_outflow_request{0.0};
    //for (const auto x: outflow_requests) {
    //  total_outflow_request += x;
    //}
    //if constexpr (debug_level >= debug_level_high) {
    //  std::cout << "... PRIOR TO PROCESSING:\n";
    //  std::cout << "... total_inflow_achieved = " << total_inflow_achieved << "\n";
    //  std::cout << "... total_outflow_request = " << total_outflow_request << "\n";
    //  std::cout << "... inflows = " << vec_to_string(inflows) << "\n";
    //  std::cout << "... prev_inflows = " << vec_to_string(prev_inflows) << "\n";
    //  std::cout << "... inflows_achieved = " << vec_to_string(inflows_achieved) << "\n";
    //  std::cout << "... outflows = " << vec_to_string(outflows) << "\n";
    //  std::cout << "... prev_outflows = " << vec_to_string(prev_outflows) << "\n";
    //  std::cout << "... outflow_requests = " << vec_to_string(outflow_requests) << "\n";
    //}
    //if (inflow_provided && !outflow_provided) {
    //  if constexpr (debug_level >= debug_level_high) {
    //    std::cout << "inflow_provided && !outflow_provided\n";
    //  }
    //  set_report_outflow_achieved(true);
    //  if (total_inflow_achieved > total_outflow_request) {
    //    std::ostringstream oss;
    //    oss << "AchievedMoreThanRequestedError\n";
    //    oss << "total_outflow_request > inflow\n";
    //    oss << "inflow: " << get_inflow() << "\n";
    //    oss << "total_inflow_achieved: " << total_inflow_achieved << "\n";
    //    oss << "total_outflow_request: " << total_outflow_request << "\n";
    //    oss << "id: " << get_id() << "\n";
    //    oss << "time: (" << get_real_time() << ", "
    //                     << get_logical_time() << ")\n";
    //    throw std::runtime_error(oss.str());
    //  }
    //  if (strategy == MuxerDispatchStrategy::InOrder) {
    //    auto diff = total_outflow_request - total_inflow_achieved;
    //    if constexpr (debug_level >= debug_level_high) {
    //      std::cout << "... diff = " << diff << "\n";
    //    }
    //    if (std::abs(diff) <= flow_value_tolerance) {
    //      if constexpr (debug_level >= debug_level_high) {
    //        std::cout << "... we met the loads\n";
    //      }
    //      // We met the loads.
    //      update_state(FlowState{total_inflow_achieved});
    //      inflows = inflows_achieved;
    //      update_outflows_using_inorder_dispatch(total_inflow_achieved);
    //    }
    //    else if (diff < neg_flow_value_tol) {
    //      if constexpr (debug_level >= debug_level_high) {
    //        std::cout << "... we're oversupplying; reset\n";
    //      }
    //      // we're oversupplying... reset
    //      set_report_outflow_achieved(false);
    //      set_report_inflow_request(true);
    //      inflows = std::vector<FlowValueType>(num_inflows, 0.0);
    //      std::vector<FlowValueType>::size_type inflow_port_for_request = 0;
    //      inflows[inflow_port_for_request] = total_outflow_request;
    //      inflows_achieved = inflows;
    //    }
    //    else {
    //      if constexpr (debug_level >= debug_level_high) {
    //        std::cout << "... we're undersupplying\n";
    //      }
    //      // we're undersupplying
    //      const auto final_inflow_port = num_inflows - 1;
    //      if (highest_inflow_port_received < final_inflow_port) {
    //        std::vector<FlowValueType>::size_type inflow_port_for_request =
    //          static_cast<std::vector<FlowValueType>::size_type>(
    //            highest_inflow_port_received) + 1;
    //        set_report_outflow_achieved(false);
    //        set_report_inflow_request(true);
    //        inflows = inflows_achieved;
    //        inflows[inflow_port_for_request] = diff;
    //        inflows_achieved = inflows;
    //      }
    //      else {
    //        // We've requested to all inflow ports and are still
    //        // deficient on meeting outflow request. Update outflows.
    //        if constexpr (debug_level >= debug_level_high) {
    //          std::ostringstream oss{};
    //          oss << "outflow request > inflow available\n";
    //        }
    //        update_state(FlowState{total_inflow_achieved});
    //        inflows = inflows_achieved;
    //        switch (outflow_strategy) {
    //          case MuxerDispatchStrategy::InOrder:
    //            {
    //              // We start satisfying loads one by one in port order until there
    //              // is nothing left
    //              if constexpr (debug_level >= debug_level_high) {
    //                std::ostringstream oss{};
    //                oss << "InOrder outflow_strategy\n";
    //                oss << "get_report_inflow_request() = "
    //                    << get_report_inflow_request() << "\n";
    //                oss << "get_report_outflow_achieved() = "
    //                    << get_report_outflow_achieved() << "\n";
    //              }
    //              update_outflows_using_inorder_dispatch(total_inflow_achieved);
    //              break;
    //            }
    //          case MuxerDispatchStrategy::Distribute:
    //            {
    //              // The inflows are distributed evenly over the outflows
    //              if (total_outflow_request != 0.0) {
    //                FlowValueType reduction_factor{1.0};
    //                reduction_factor = total_inflow_achieved / total_outflow_request;
    //                for (auto& of_item: outflows) {
    //                  of_item *= reduction_factor;
    //                }
    //              }
    //              break;
    //            }
    //          default:
    //            {
    //              std::ostringstream oss;
    //              oss << "unhandled output_strategy for Mux " << get_id() << "\n";
    //              throw std::runtime_error(oss.str());
    //            }
    //        }
    //        if constexpr (debug_level >= debug_level_high) {
    //          std::cout
    //            << "muxer dispatch (in)   : "
    //            << muxer_dispatch_strategy_to_string(strategy) << "\n"
    //            << "muxer dispatch (out)  : "
    //            << muxer_dispatch_strategy_to_string(outflow_strategy) << "\n"
    //            << "inflow                : " << get_inflow() << "\n"
    //            << "outflow               : " << get_outflow() << "\n"
    //            << "highest_inflow_port_received: "
    //            << highest_inflow_port_received << "\n"
    //            << "highest_outflow_port_received: "
    //            << highest_outflow_port_received << "\n"
    //            << "num_inflows           : " << num_inflows << "\n"
    //            << "num_outflows          : " << num_outflows << "\n"
    //            << "total_inflow_achieved : " << total_inflow_achieved << "\n"
    //            << "total_outflow_request : " << total_outflow_request << "\n";
    //          int i{0};
    //          for (const auto& n : inflows) {
    //            std::cout << "inflows[" << i << "]=" << n << "\n";
    //            ++i;
    //          }
    //          i = 0;
    //          for (const auto& n : outflows) {
    //            std::cout << "outflows[" << i << "]=" << n << "\n";
    //            ++i;
    //          }
    //        }
    //      }
    //    }
    //  }
    //  else {
    //    std::ostringstream oss;
    //    oss << "unhandled dispatch strategy for Mux: \""
    //        << static_cast<int>(strategy) << "\"";
    //    throw std::runtime_error(oss.str());
    //  }
    //}
    //else if (outflow_provided && !inflow_provided) {
    //  if constexpr (debug_level >= debug_level_high) {
    //    std::cout << "outflow_provided && !inflow_provided\n";
    //  }
    //  set_report_inflow_request(true);
    //  set_report_outflow_achieved(true);
    //  if (strategy == MuxerDispatchStrategy::InOrder) {
    //    // Whenever the outflow request updates, we always start querying
    //    // inflows from the first inflow port and update the inflow_request.
    //    std::vector<FlowValueType>::size_type inflow_port_for_request = 0;
    //    inflows = std::vector<FlowValueType>(num_inflows, 0.0);
    //    inflows[inflow_port_for_request] = total_outflow_request;
    //    inflows_achieved = inflows;
    //    update_state(FlowState{total_outflow_request});
    //    outflows = outflow_requests;
    //  }
    //  else {
    //    std::ostringstream oss;
    //    oss << "unhandled dispatch strategy for Mux: \""
    //        << static_cast<int>(strategy) << "\"";
    //    throw std::runtime_error(oss.str());
    //  }
    //  if (get_outflow() > total_outflow_request) {
    //    std::ostringstream oss;
    //    oss << "AchievedMoreThanRequestedError\n";
    //    oss << "outflow > total_outflow_request\n";
    //    oss << "outflow: " << get_outflow() << "\n";
    //    oss << "total_outflow_request: " << total_outflow_request << "\n";
    //    throw std::runtime_error(oss.str());
    //  }
    //}
    //else if (inflow_provided && outflow_provided) {
    //  if constexpr (debug_level >= debug_level_high) {
    //    std::cout << "inflow_provided && outflow_provided\n";
    //  }
    //  if (total_inflow_achieved > total_outflow_request) {
    //    std::ostringstream oss;
    //    oss << "SimultaneousIORequestError: assumption was we'd never get here...\n";
    //    oss << "... id=" << get_id() << "\n";
    //    oss << "... total_inflow_achieved=" << total_inflow_achieved << "\n";
    //    oss << "... total_outflow_request=" << total_outflow_request << "\n";
    //    throw std::runtime_error(oss.str());
    //  }
    //  if (total_inflow_achieved < total_outflow_request) {
    //    // Below is true but not used. Here for reference.
    //    // total_outflow_request = total_inflow_achieved;
    //    set_report_outflow_achieved(true);
    //  }
    //}
    //else {
    //  std::ostringstream oss;
    //  oss << "Unhandled port IO case...;"
    //      << "id=" << get_id() << "\n";
    //  throw std::runtime_error(oss.str());
    //}
    //if (get_report_inflow_request() || get_report_outflow_achieved()) {
    //  update_on_external_transition();
    //  check_flow_invariants();
    //}
    //if constexpr (debug_level >= debug_level_high) {
    //  std::cout << "... AT ALGORITHM END:\n";
    //  std::cout << "... total_inflow_achieved = " << total_inflow_achieved << "\n";
    //  std::cout << "... total_outflow_request = " << total_outflow_request << "\n";
    //  std::cout << "... inflows = " << vec_to_string(inflows) << "\n";
    //  std::cout << "... prev_inflows = " << vec_to_string(prev_inflows) << "\n";
    //  std::cout << "... inflows_achieved = " << vec_to_string(inflows_achieved) << "\n";
    //  std::cout << "... outflows = " << vec_to_string(outflows) << "\n";
    //  std::cout << "... prev_outflows = " << vec_to_string(prev_outflows) << "\n";
    //  std::cout << "... outflow_requests = " << vec_to_string(outflow_requests) << "\n";
    //}
  }

  void
  Mux::delta_conf(std::vector<PortValue>& xs)
  {
    state = erin::devs::mux_confluent_transition(state, xs);
  }

  Time
  Mux::ta()
  {
    auto dt = erin::devs::mux_time_advance(state);
    if (dt == erin::devs::infinity)
      return inf;
    return Time{dt, 1};
  }

  void
  Mux::output_func(std::vector<PortValue>& ys)
  {
    erin::devs::mux_output_function_mutable(state, ys);
    //if constexpr (debug_level >= debug_level_high) {
    //  std::cout << "Mux::output_func();"
    //            << "id=" << get_id() << ";"
    //            << "time={" << get_real_time() << ", " << get_logical_time() << "}\n";
    //}
    //if (get_report_inflow_request()) {
    //  if constexpr (debug_level >= debug_level_high) {
    //    std::cout << "... send=>outport_inflow_request;id="
    //              << get_id() << "\n";
    //  }
    //  if (strategy == MuxerDispatchStrategy::InOrder) {
    //    for (int i{0}; i < num_inflows; ++i) {
    //      auto inflow_ = inflows[i];
    //      auto prev_inflow_ = prev_inflows[i];
    //      if (prev_inflow_ != inflow_) {
    //        ys.emplace_back(
    //            adevs::port_value<FlowValueType>{
    //              outport_inflow_request + i,
    //              inflow_});
    //        prev_inflows[i] = inflow_;
    //      }
    //    }
    //  }
    //  else {
    //    std::ostringstream oss;
    //    oss << "unhandled strategy \"" << static_cast<int>(strategy)
    //        << "\"";
    //    throw std::runtime_error(oss.str());
    //  }
    //}
    //if (get_report_outflow_achieved()) {
    //  if constexpr (debug_level >= debug_level_high) {
    //    std::cout << "... send=>outport_outflow_achieved;id="
    //              << get_id() << "\n";
    //    std::cout << "... t = " << get_real_time() << ";id="
    //              << get_id() << "\n";
    //  }
    //  for (int i{0}; i < num_outflows; ++i) {
    //    auto outflow_ = outflows[i];
    //    auto prev_outflow_ = prev_outflows[i];
    //    if constexpr (debug_level >= debug_level_high) {
    //      std::cout << "... outflow[" << i << "] = "
    //                << outflow_ << "\n";
    //      std::cout << "... prev_outflow[" << i << "] = "
    //                << prev_outflow_ << "\n";
    //    }
    //    if (prev_outflow_ != outflow_) {
    //      ys.emplace_back(
    //          adevs::port_value<FlowValueType>{
    //            outport_outflow_achieved + i,
    //            outflow_});
    //      prev_outflows[i] = outflow_;
    //    }
    //  }
    //}
    //if constexpr (debug_level >= debug_level_high) {
    //  std::cout << "... ys = ";
    //  bool first{true};
    //  for (const auto y : ys) {
    //    if (first) {
    //      std::cout << "[";
    //      first = false;
    //    } else {
    //      std::cout << ", ";
    //    }
    //    std::cout << "{port=" << y.port << ", value=" << y.value << "}";
    //  }
    //  if (ys.size() > 0)
    //    std::cout << "]\n";
    //  else
    //    std::cout << "[]\n";
    //}
  }
}
