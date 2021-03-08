/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include "erin/element.h"
#include "debug_utils.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <string>

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
    else if (tag == "on_off_switch") {
      return ElementType::OnOffSwitch;
    }
    else if (tag == "uncontrolled_source") {
      return ElementType::UncontrolledSource;
    }
    else if (tag == "mover") {
      return ElementType::Mover;
    }
    else {
      std::ostringstream oss{};
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
      case ElementType::OnOffSwitch:
        return std::string{"on_off_switch"};
      case ElementType::UncontrolledSource:
        return std::string{"uncontrolled_source"};
      case ElementType::Mover:
        return std::string{"mover"};
      default:
        {
          std::ostringstream oss{};
          oss << "unhandled ElementType '" << static_cast<int>(et) << "'\n";
          throw std::invalid_argument(oss.str());
        }
    }
  }

  ////////////////////////////////////////////////////////////
  // DefaultFlowWriter
  DefaultFlowWriter::DefaultFlowWriter():
    FlowWriter(),
    recording_started{false},
    is_final{false},
    current_time{0},
    next_id{0},
    current_requests{},
    current_achieved{},
    element_tag_to_id{},
    element_id_to_tag{},
    element_id_to_stream_tag{},
    element_id_to_comp_type{},
    element_id_to_port_role{},
    recording_flags{},
    time_history{},
    request_history{},
    achieved_history{}
  {
  }

  int
  DefaultFlowWriter::register_id(
      const std::string& element_tag,
      const std::string& stream_tag,
      ComponentType comp_type,
      PortRole port_role,
      bool record_history)
  {
    ensure_not_final();
    ensure_not_recording();
    auto element_id = get_next_id();
    ensure_element_tag_is_unique(element_tag);
    current_requests.emplace_back(0.0);
    current_achieved.emplace_back(0.0);
    element_tag_to_id.emplace(std::make_pair(element_tag, element_id));
    element_id_to_tag.emplace(std::make_pair(element_id, element_tag));
    element_id_to_stream_tag.emplace(std::make_pair(element_id, stream_tag));
    element_id_to_comp_type.emplace(std::make_pair(element_id, comp_type));
    element_id_to_port_role.emplace(std::make_pair(element_id, port_role));
    recording_flags.emplace_back(record_history);
    return element_id;
  }

  void
  DefaultFlowWriter::ensure_not_final() const
  {
    if (is_final)
      throw std::runtime_error("invalid operation on a finalized FlowWriter");
  }

  void
  DefaultFlowWriter::ensure_not_recording() const
  {
    if (recording_started)
      throw std::runtime_error("invalid operation on a recording FlowWriter");
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
      std::ostringstream oss{};
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
      std::ostringstream oss{};
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
    namespace E = ERIN;
    if constexpr (E::debug_level >= E::debug_level_high) {
      std::cout << "DefaultFlowWriter::record_history_and_update_current_time(...)\n"
                << "time = " << time << "\n";
    }
    recording_started = true;
    time_history.emplace_back(current_time);
    for (size_type_D i{0}; i < static_cast<size_type_D>(num_elements()); ++i) {
      if (recording_flags[i]) {
        request_history.emplace_back(current_requests[i]);
        achieved_history.emplace_back(current_achieved[i]);
      }
      else {
        std::cout << "WARNING! Element " << i << " not recorded!\n";
        auto idx = static_cast<const int>(i);
        std::cout << "Element " << i << " is " << element_id_to_tag[idx] << "\n";
      }
    }
    if constexpr (E::debug_level >= E::debug_level_high) {
      for (size_type_D i{0}; i < request_history.size(); ++i) {
        std::cout << "r[" << i << "] = " << request_history[i] << "\n";
        std::cout << "a[" << i << "] = " << achieved_history[i] << "\n";
      }
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
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "DefaultFlowWriter::write_data(...)\n"
                << "element_id     = " << element_id << " ("
                << element_id_to_tag[element_id] << ")\n"
                << "time           = " << time << "\n"
                << "requested_flow = " << requested_flow << "\n"
                << "achieved_flow  = " << achieved_flow << "\n";
    }
    ensure_not_final();
    ensure_element_id_is_valid(element_id);
    ensure_time_is_valid(time);
    if (time > current_time) {
      record_history_and_update_current_time(time);
    }
    current_requests[element_id] = requested_flow;
    current_achieved[element_id] = achieved_flow;
    if constexpr (debug_level >= debug_level_high) {
      using size_type = std::vector<FlowValueType>::size_type;
      for (size_type i{0}; i < current_requests.size(); ++i) {
        if (i == static_cast<size_type>(element_id)) {
          std::cout << "current_requests[*] = " << current_requests[i] << "\n";
          std::cout << "current_achieved[*] = " << current_achieved[i] << "\n";
        }
        else {
          std::cout << "current_requests[" << i << "] = " << current_requests[i] << "\n";
          std::cout << "current_achieved[" << i << "] = " << current_achieved[i] << "\n";
        }
      }
    }
  }

  void
  DefaultFlowWriter::finalize_at_time(RealTimeType time)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "DefaultFlowWriter::finalize_at_time(" << time << ")\n";
    }
    is_final = true;
    auto num = num_elements();
    auto num_st = static_cast<size_type_D>(num);
    if (time > current_time) {
      record_history_and_update_current_time(time);
    }
    time_history.emplace_back(time);
    for (size_type_D i{0}; i < num_st; ++i) {
      if (recording_flags[i]) {
        request_history.emplace_back(0.0);
        achieved_history.emplace_back(0.0);
      }
    }
    current_requests.clear();
    current_achieved.clear();
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "FINAL HISTORY:\n";
      using size_type = std::vector<FlowValueType>::size_type;
      int N{0};
      for (size_type i{0}; i < recording_flags.size(); ++i) {
        if (recording_flags[i]) {
          N++;
        }
      }
      for (size_type i{0}; i < time_history.size(); ++i) {
        std::cout << "time[" << i << "] = " << time_history[i] << "\n";
        size_type k{0};
        for (size_type j{0}; j < recording_flags.size(); ++j) {
          auto idx = (i * N) + k;
          if (recording_flags[j]) {
            std::cout << "request_history[("
                      << i << " * " << N << ") + " << k << " = " << idx
                      << "] = " << request_history[idx] << "\n";
            std::cout << "achieved_history[("
                      << i << " * " << N << ") + " << k << " = " << idx
                      << "] = " << achieved_history[(i*N)+k] << "\n";
            k++;
          }
          else {
            std::cout << "NOT RECORDING\n"
                      << "i = " << i << "\n"
                      << "N = " << N << "\n"
                      << "k = " << k << "\n"
                      << "idx = " << idx << "\n";
          }
        }
      }
    }
  }

  std::unordered_map<std::string, std::vector<Datum>>
  DefaultFlowWriter::get_results() const
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "DefaultFlowWriter::get_results()\n";
    }
    auto num_elem = num_elements();
    auto num_events = time_history.size();
    std::unordered_map<std::string, std::vector<Datum>> out{};
    for (const auto& tag_id: element_tag_to_id) {
      auto element_tag = tag_id.first;
      auto element_id = tag_id.second;
      std::vector<Datum> data(num_events);
      for (decltype(num_events) i{0}; i < num_events; ++i) {
        auto idx = (i * num_elem) + element_id;
        if constexpr (debug_level >= debug_level_high) {
          std::cout << "... element_id = " << element_id
                    << " (" << element_tag << ")\n"
                    << "... i = " << i << "\n"
                    << "... idx = " << idx << "\n"
                    << "... time = " << time_history[i] << "\n"
                    << "... req = " << request_history[idx] << "\n"
                    << "... ach = " << achieved_history[idx] << "\n";
        }
        data[i] = Datum{
          time_history[i],
          request_history[idx],
          achieved_history[idx]};
      }
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "pulling datums\n"
                  << "num_elem = " << num_elem << "\n"
                  << "num_events = " << num_events << "\n"
                  << "element_tag = " << element_tag << "\n"
                  << "element_id = " << element_id << "\n";
        for (const auto& d : data) {
          std::cout << "- " << d << "\n";
        }
      }
      out[element_tag] = std::move(data);
    }
    return out;
  }

  std::unordered_map<std::string,std::string>
  DefaultFlowWriter::get_stream_ids() const
  {
    std::unordered_map<std::string, std::string> out{};
    for (const auto& pair : element_id_to_tag) {
      const auto& id = pair.first;
      const auto& tag = pair.second;
      auto it = element_id_to_stream_tag.find(id);
      std::string stream{};
      if (it != element_id_to_stream_tag.end())
        stream = it->second;
      else
        throw std::runtime_error(
            "id '" + std::to_string(id) +
            "' not found in element_id_to_stream_tag");
      out.emplace(std::make_pair(tag, stream));
    }
    return out;
  }

  std::unordered_map<std::string,ComponentType>
  DefaultFlowWriter::get_component_types() const
  {
    std::unordered_map<std::string, ComponentType> out{};
    for (const auto& pair : element_id_to_tag) {
      const auto& id = pair.first;
      const auto& tag = pair.second;
      auto it = element_id_to_comp_type.find(id);
      ComponentType type{ComponentType::Informational};
      if (it != element_id_to_comp_type.end()) {
        type = it->second;
      }
      else {
        throw std::runtime_error(
            "element id '" + std::to_string(id) +
            "' not found in element_id_to_comp_type for tag '" + tag + "'");
      }
      out.emplace(std::make_pair(tag, type));
    }
    return out;
  }

  std::unordered_map<std::string, PortRole>
  DefaultFlowWriter::get_port_roles() const
  {
    std::unordered_map<std::string, PortRole> out{};
    for (const auto& pair : element_id_to_tag) {
      const auto& id = pair.first;
      const auto& tag = pair.second;
      auto it = element_id_to_port_role.find(id);
      PortRole role{PortRole::Inflow};
      if (it != element_id_to_port_role.end()) {
        role = it->second;
      }
      else {
        throw std::runtime_error(
            "element id '" + std::to_string(id) +
            "' not found in element_id_to_port_role for tag '" + tag + "'");
      }
      out.emplace(std::make_pair(tag, role));
    }
    return out;
  }

  void
  DefaultFlowWriter::clear()
  {
    recording_started = false;
    is_final = false;
    current_time = 0;
    next_id = 0;
    current_requests.clear();
    current_achieved.clear();
    element_tag_to_id.clear();
    element_id_to_tag.clear();
    element_id_to_stream_tag.clear();
    element_id_to_comp_type.clear();
    recording_flags.clear();
    time_history.clear();
    request_history.clear();
    achieved_history.clear();
  }

  ////////////////////////////////////////////////////////////
  // FlowElement
  FlowElement::FlowElement(
      std::string id_,
      ComponentType component_type_,
      ElementType element_type_,
      const std::string& st) :
    FlowElement(std::move(id_), component_type_, element_type_, st, st)
  {
  }

  FlowElement::FlowElement(
      std::string id_,
      ComponentType component_type_,
      ElementType element_type_,
      std::string in,
      std::string out):
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
  }

  void
  FlowElement::delta_int()
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "FlowElement::delta_int()::" << id << "\n";
    }
    update_on_internal_transition();
    report_inflow_request = false;
    report_outflow_achieved = false;
    report_lossflow_achieved = false;
  }

  void
  FlowElement::delta_ext(Time e, std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_ext::" << id << "::FlowElement\n"
                << "- xs = " << vec_to_string<PortValue>(xs) << "\n";
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
        inflow = inflow_request;
        the_inflow_achieved = inflow_request;
        report_inflow_request = true;
        report_outflow_achieved = false;
      }
      if (the_inflow_achieved < neg_flow_value_tol) {
        std::ostringstream oss;
        oss << "FlowReversalError!!!\n";
        oss << "inflow should never be below 0.0!!!\n";
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
    else if (outflow_provided) {
      if (lossflow_provided) {
        if constexpr (debug_level >= debug_level_high) {
          std::cout << "outflow_provided "
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
        throw std::runtime_error(oss.str());
      }
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
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_conf::" << id << "::FlowElement\n"
                << "- xs = " << vec_to_string<PortValue>(xs) << "\n";
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
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "ta::" << id << "::FlowElement\n"
                << "- dt = ";
    }
    bool flag{report_inflow_request
              || report_outflow_achieved
              || report_lossflow_achieved};
    if (flag) {
      if constexpr (debug_level >= debug_level_medium) {
        std::cout << "0\n";
      }
      return Time{0, 1};
    }
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "infinity\n";
    }
    return calculate_time_advance();
  }

  void
  FlowElement::output_func(std::vector<PortValue>& ys)
  {
    if (report_inflow_request) {
      ys.emplace_back(
          adevs::port_value<FlowValueType>{outport_inflow_request, inflow});
    }
    if (report_outflow_achieved) {
      ys.emplace_back(
          adevs::port_value<FlowValueType>{outport_outflow_achieved, outflow});
    }
    add_additional_outputs(ys);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "output_func::" << id << "::FlowElement\n"
       << "- ys = " << vec_to_string<PortValue>(ys) << "\n";
    }
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
      std::ostringstream oss{};
      oss << "FlowElement ERROR! " << inflow << " != " << outflow << " + "
          << storeflow << " + " << lossflow << "!\n";
      throw std::runtime_error(oss.str());
    }
  }

  ///////////////////////////////////////////////////////////////////
  // FlowLimits
  FlowLimits::FlowLimits(
      std::string id_,
      ComponentType component_type_,
      const std::string& stream_type_,
      FlowValueType lower_limit,
      FlowValueType upper_limit,
      PortRole port_role_):
    FlowElement(
        std::move(id_),
        component_type_,
        ElementType::FlowLimits,
        stream_type_),
    state{erin::devs::make_flow_limits_state(lower_limit, upper_limit)},
    flow_writer{nullptr},
    element_id{-1},
    record_history{false},
    port_role{port_role_}
  {
  }

  void
  FlowLimits::delta_int()
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_int::" << get_id() << "::FlowLimits\n"
                << "- s = " << state << "\n";
    }
    state = erin::devs::flow_limits_internal_transition(state);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s*= " << state << "\n";
    }
    if (flow_writer && record_history && (element_id != -1)) {
      flow_writer->write_data(
          element_id,
          state.time,
          state.outflow_port.get_requested(),
          state.outflow_port.get_achieved());
    }
  }

  void
  FlowLimits::delta_ext(Time dt, std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_ext::" << get_id() << "::FlowLimits\n"
                << "- xs = " << vec_to_string<PortValue>(xs) << "\n"
                << "- s  = " << state << "\n";
    }
    state = erin::devs::flow_limits_external_transition(
        state, dt.real, xs);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* = " << state << "\n";
    }
    if (flow_writer && record_history && (element_id != -1)) {
      flow_writer->write_data(
          element_id,
          state.time,
          state.outflow_port.get_requested(),
          state.outflow_port.get_achieved());
    }
  }

  void
  FlowLimits::delta_conf(std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_conf::" << get_id() << "::FlowLimits\n"
                << "- xs = " << vec_to_string<PortValue>(xs) << "\n"
                << "- s = " << state << "\n";
    }
    state = erin::devs::flow_limits_confluent_transition(state, xs);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s*= " << state << "\n";
    }
    if (flow_writer && record_history && (element_id != -1)) {
      flow_writer->write_data(
          element_id,
          state.time,
          state.outflow_port.get_requested(),
          state.outflow_port.get_achieved());
    }
  }

  Time
  FlowLimits::ta()
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "ta::" << get_id() << "::FlowLimits\n"
                << "- dt = ";
    }
    auto dt = erin::devs::flow_limits_time_advance(state);
    if (dt == erin::devs::infinity) {
      if constexpr (debug_level >= debug_level_medium) {
        std::cout << "infinity\n";
      }
      return inf;
    }
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << dt << "\n";
    }
    return Time{dt, 1};
  }

  void
  FlowLimits::output_func(std::vector<PortValue>& ys)
  {
    erin::devs::flow_limits_output_function_mutable(state, ys);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "output_func::" << get_id() << "::FlowLimits\n"
                << "- ys = " << vec_to_string<PortValue>(ys) << "\n";
    }
  }

  void
  FlowLimits::set_flow_writer(const std::shared_ptr<FlowWriter>& writer)
  {
    flow_writer = writer;
    if (flow_writer && record_history && (element_id == -1)) {
      element_id = flow_writer->register_id(
          get_id(),
          get_outflow_type(),
          get_component_type(),
          port_role,
          record_history);
      flow_writer->write_data(
          element_id,
          state.time,
          state.outflow_port.get_requested(),
          state.outflow_port.get_achieved());
    }
  }

  void
  FlowLimits::set_recording_on()
  {
    record_history = true;
    set_flow_writer(flow_writer);
  }

  ////////////////////////////////////////////////////////////
  // FlowMeter
  FlowMeter::FlowMeter(
      std::string id,
      ComponentType component_type,
      const std::string& stream_type,
      PortRole port_role_) :
    FlowElement(
        std::move(id),
        component_type,
        ElementType::FlowMeter,
        stream_type),
    flow_writer{nullptr},
    element_id{-1},
    record_history{true},
    port_role{port_role_}
  {
  }

  void
  FlowMeter::set_flow_writer(const std::shared_ptr<FlowWriter>& writer)
  {
    flow_writer = writer;
    update_on_external_transition();
  }

  void
  FlowMeter::set_recording_on()
  {
    record_history = true;
    update_on_external_transition();
  }

  void
  FlowMeter::update_on_external_transition()
  {
    if ((element_id == -1) && flow_writer && record_history) {
      element_id = flow_writer->register_id(
          get_id(),
          get_outflow_type(),
          get_component_type(),
          port_role,
          record_history);
    }
    if ((element_id != -1) && flow_writer) {
      flow_writer->write_data(
          element_id,
          get_real_time(),
          get_outflow_request(),
          get_outflow());
    }
  }

  ////////////////////////////////////////////////////////////
  // Converter
  Converter::Converter(
      std::string id,
      ComponentType component_type,
      std::string input_stream_type,
      std::string output_stream_type,
      std::function<FlowValueType(FlowValueType)> calc_output_from_input,
      std::function<FlowValueType(FlowValueType)> calc_input_from_output,
      std::string lossflow_stream_):
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
    input_from_output{std::move(calc_input_from_output)},
    flow_writer{nullptr},
    inflow_element_id{-1},
    outflow_element_id{-1},
    lossflow_element_id{-1},
    wasteflow_element_id{-1},
    record_history{false},
    record_wasteflow_history{false},
    lossflow_stream{std::move(lossflow_stream_)}
  {
  }

  void
  Converter::delta_int()
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_int::" << get_id() << "::Converter\n";
      std::cout << "- s = " << state << "\n";
    }
    state = erin::devs::converter_internal_transition(state);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s*= " << state << "\n";
    }
    log_ports();
  }

  void
  Converter::delta_ext(Time e, std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_ext::" << get_id() << "::Converter\n"
                << "- xs = " << vec_to_string<PortValue>(xs) << "\n"
                << "- s  = " << state << "\n";
    }
    state = erin::devs::converter_external_transition(state, e.real, xs);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* = " << state << "\n";
    }
    log_ports();
  }

  void
  Converter::delta_conf(std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_conf::" << get_id() << "::Converter\n"
                << "- xs = " << vec_to_string<PortValue>(xs) << "\n"
                << "- s  = " << state << "\n";
    }
    state = erin::devs::converter_confluent_transition(state, xs);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* = " << state << "\n";
    }
    log_ports();
  }

  Time
  Converter::ta()
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "ta::" << get_id() << "::Converter\n";
    }
    auto dt = erin::devs::converter_time_advance(state);
    if (dt == erin::devs::infinity) {
      if constexpr (debug_level >= debug_level_medium) {
        std::cout << "- dt = infinity\n";
      }
      return inf;
    }
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- dt = " << dt << "\n";
    }
    return Time{dt, 1};
  }

  void
  Converter::output_func(std::vector<PortValue>& ys)
  {
    erin::devs::converter_output_function_mutable(state, ys);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "output_func::" << get_id() << "::Converter\n"
                << "- ys = " << vec_to_string<PortValue>(ys) << "\n";
    }
  }

  void
  Converter::set_flow_writer(const std::shared_ptr<FlowWriter>& writer)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "Converter::set_flow_writer(); id = " << get_id() << "\n";
    }
    flow_writer = writer;
    log_ports();
  }

  void
  Converter::set_recording_on()
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "Converter::set_recording_on(); id = " << get_id() << "\n";
    }
    record_history = true;
    log_ports();
  }

  void
  Converter::set_wasteflow_recording_on()
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "Converter::set_wasteflow_recording_on(); id = " << get_id() << "\n";
    }
    record_wasteflow_history = true;
    log_ports();
  }

  void
  Converter::log_ports()
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "Converter::log_ports(); id = " << get_id() << "\n"
                << "flow_writer == nullptr   = " << (flow_writer == nullptr) << "\n"
                << "record_history           = " << record_history << "\n"
                << "record_wasteflow_history = " << record_wasteflow_history << "\n";
    }
    if (flow_writer && (record_history || record_wasteflow_history)) {
      if ((inflow_element_id == -1) && record_history) {
        inflow_element_id = flow_writer->register_id(
            get_id() + "-inflow",
            get_inflow_type(),
            get_component_type(),
            PortRole::Inflow,
            record_history);
      }
      if ((outflow_element_id == -1) && record_history) {
        outflow_element_id = flow_writer->register_id(
            get_id() + "-outflow",
            get_outflow_type(),
            get_component_type(),
            PortRole::Outflow,
            record_history);
      }
      if ((lossflow_element_id == -1) && record_history) {
        lossflow_element_id = flow_writer->register_id(
            get_id() + "-lossflow",
            lossflow_stream,
            get_component_type(),
            PortRole::Outflow,
            record_history);
      }
      if (wasteflow_element_id == -1) {
        wasteflow_element_id = flow_writer->register_id(
            get_id() + "-wasteflow",
            lossflow_stream,
            get_component_type(),
            PortRole::WasteInflow,
            (record_history || record_wasteflow_history));
      }
      if (record_history) {
        flow_writer->write_data(
            inflow_element_id,
            state.time,
            state.inflow_port.get_requested(),
            state.inflow_port.get_achieved());
        flow_writer->write_data(
            outflow_element_id,
            state.time,
            state.outflow_port.get_requested(),
            state.outflow_port.get_achieved());
        flow_writer->write_data(
            lossflow_element_id,
            state.time,
            state.lossflow_port.get_requested(),
            state.lossflow_port.get_achieved());
      }
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "ID = " << get_id() << "-wasteflow\n"
                  << "time = " << state.time << "\n"
                  << "wasteflow-request  = "
                  << state.wasteflow_port.get_requested() << "\n"
                  << "wasteflow-achieved = "
                  << state.wasteflow_port.get_achieved() << "\n";
      }
      flow_writer->write_data(
          wasteflow_element_id,
          state.time,
          state.wasteflow_port.get_requested(),
          state.wasteflow_port.get_achieved());
    }
  }

  ///////////////////////////////////////////////////////////////////
  // Sink
  Sink::Sink(
      std::string id,
      ComponentType component_type,
      const std::string& st,
      const std::vector<LoadItem>& loads_):
    FlowElement(
        std::move(id),
        component_type,
        ElementType::Sink,
        st),
    data{erin::devs::make_load_data(loads_)},
    state{erin::devs::make_load_state()},
    flow_writer{nullptr},
    element_id{-1},
    record_history{false}
  {
  }

  void
  Sink::delta_int()
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_int::" << get_id() << "::Sink\n"
                << "- s  = " << state << "\n";
    }
    state = erin::devs::load_internal_transition(data, state);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* " << state << "\n";
    }
    if (flow_writer && record_history && (element_id != -1))
      flow_writer->write_data(
          element_id,
          state.time,
          state.inflow_port.get_requested(),
          state.inflow_port.get_achieved());
  }

  void
  Sink::delta_ext(Time e, std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_ext::" << get_id() << "::Sink\n"
                << "- xs = " << vec_to_string<PortValue>(xs) << "\n"
                << "- s  = " << state << "\n";
    }
    state = erin::devs::load_external_transition(state, e.real, xs);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* = " << state << "\n"; 
    }
    if (flow_writer && record_history && (element_id != -1))
      flow_writer->write_data(
          element_id,
          state.time,
          state.inflow_port.get_requested(),
          state.inflow_port.get_achieved());
  }

  void
  Sink::delta_conf(std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_conf::" << get_id() << "::Sink\n"
                << "- xs = " << vec_to_string<PortValue>(xs) << "\n" 
                << "- s  = " << state << "\n"; 
    }
    state = erin::devs::load_confluent_transition(data, state, xs);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* = " << state << "\n"; 
    }
    if (flow_writer && record_history && (element_id != -1))
      flow_writer->write_data(
          element_id,
          state.time,
          state.inflow_port.get_requested(),
          state.inflow_port.get_achieved());
  }

  Time
  Sink::ta()
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "ta::" << get_id() << "::Sink\n"
                << "- dt = ";
    }
    auto dt = erin::devs::load_time_advance(data, state);
    if (dt == erin::devs::infinity) {
      if constexpr (debug_level >= debug_level_medium) {
        std::cout << "infinity\n"; 
      }
      return inf;
    }
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << dt << "\n";
    }
    return Time{dt, 1};
  }

  void
  Sink::output_func(std::vector<PortValue>& ys)
  {
    erin::devs::load_output_function_mutable(data, state, ys);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "output_func::" << get_id() << "::Sink\n"
                << "- ys = " << vec_to_string<PortValue>(ys) << "\n";
    }
  }

  void
  Sink::set_flow_writer(const std::shared_ptr<FlowWriter>& writer)
  {
    flow_writer = writer;
    if (flow_writer && record_history && (element_id == -1)) {
      element_id = flow_writer->register_id(
          get_id(),
          get_outflow_type(),
          get_component_type(),
          PortRole::LoadInflow,
          record_history);
      flow_writer->write_data(
          element_id,
          state.time,
          state.inflow_port.get_requested(),
          state.inflow_port.get_achieved());
    }
  }

  void
  Sink::set_recording_on()
  {
    record_history = true;
    set_flow_writer(flow_writer);
  }

  ////////////////////////////////////////////////////////////
  // Mux
  Mux::Mux(
      std::string id,
      ComponentType ct,
      const std::string& st,
      int num_inflows_,
      int num_outflows_,
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
    flow_writer{nullptr},
    outflow_element_ids{},
    inflow_element_ids{},
    record_history{false}
  {
  }

  void
  Mux::delta_int()
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_int::" << get_id() << "::Mux\n"
                << "- s  = " << state << "\n";
    }
    state = erin::devs::mux_internal_transition(state);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* = " << state << "\n";
    }
    log_ports();
  }

  void
  Mux::delta_ext(Time e, std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_ext::" << get_id() << "::Mux\n"
                << "- xs = " << vec_to_string<PortValue>(xs) << "\n"
                << "- s  = " << state << "\n";
    }
    state = erin::devs::mux_external_transition(state, e.real, xs);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* = " << state << "\n";
    }
    log_ports();
  }

  void
  Mux::delta_conf(std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_conf::" << get_id() << "::Mux\n"
                << "- xs = " << vec_to_string<PortValue>(xs) << "\n"
                << "- s  = " << state << "\n";
    }
    state = erin::devs::mux_confluent_transition(state, xs);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* = " << state << "\n";
    }
    log_ports();
  }

  Time
  Mux::ta()
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "ta::" << get_id() << "::Mux\n"
                << "- dt = ";
    }
    auto dt = erin::devs::mux_time_advance(state);
    if (dt == erin::devs::infinity) {
      if constexpr (debug_level >= debug_level_medium) {
        std::cout << "infinity\n";
      }
      return inf;
    }
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << dt << "\n";
    }
    return Time{dt, 1};
  }

  void
  Mux::output_func(std::vector<PortValue>& ys)
  {
    erin::devs::mux_output_function_mutable(state, ys);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "output_func::" << get_id() << "::Mux\n"
                << "- ys = " << vec_to_string<PortValue>(ys) << "\n";
    }
  }

  void
  Mux::set_flow_writer(const std::shared_ptr<FlowWriter>& writer)
  {
    flow_writer = writer;
    log_ports();
  }

  void
  Mux::set_recording_on()
  {
    record_history = true;
    log_ports();
  }

  void
  Mux::log_ports()
  {
    using size_type = std::vector<int>::size_type;
    auto ni = static_cast<size_type>(state.num_inflows);
    auto no = static_cast<size_type>(state.num_outflows);
    if (flow_writer && record_history) {
      if (ni != inflow_element_ids.size()) {
        auto the_id = get_id();
        for (size_type i{0}; i < ni; ++i) {
          inflow_element_ids.emplace_back(
              flow_writer->register_id(
                the_id + "-inflow(" + std::to_string(i) + ")",
                get_inflow_type(),
                get_component_type(),
                PortRole::Inflow,
                record_history));
        }
      }
      if (no != outflow_element_ids.size()) {
        auto the_id = get_id();
        for (size_type i{0}; i < no; ++i) {
          outflow_element_ids.emplace_back(
              flow_writer->register_id(
                the_id + "-outflow(" + std::to_string(i) + ")",
                get_outflow_type(),
                get_component_type(),
                PortRole::Outflow,
                record_history));
        }
      }
      for (size_type i{0}; i < outflow_element_ids.size(); ++i) {
        const auto& op = state.outflow_ports[i];
        flow_writer->write_data(
            outflow_element_ids[i],
            state.time,
            op.get_requested(),
            op.get_achieved());
      }
      for (size_type i{0}; i < inflow_element_ids.size(); ++i) {
        const auto& ip = state.inflow_ports[i];
        flow_writer->write_data(
            inflow_element_ids[i],
            state.time,
            ip.get_requested(),
            ip.get_achieved());
      }
    }
  }

  ////////////////////////////////////////////////////////////
  // Storage
  Storage::Storage(
      std::string id,
      ComponentType ct,
      const std::string& st,
      FlowValueType capacity,
      FlowValueType max_charge_rate):
    FlowElement(
        std::move(id),
        ct,
        ElementType::Store,
        st),
    data{erin::devs::storage_make_data(capacity, max_charge_rate)},
    state{erin::devs::storage_make_state(data)},
    flow_writer{nullptr},
    record_history{false},
    record_storeflow_and_discharge{false},
    inflow_element_id{-1},
    outflow_element_id{-1},
    storeflow_element_id{-1},
    discharge_element_id{-1}
  {
  }

  void
  Storage::delta_int()
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_int::" << get_id() << "::Storage\n"
                << "- s  = {:t " << state.time
                << " :soc " << state.soc
                << " :report-inflow-request? " << (state.report_inflow_request ? "true" : "false")
                << " :report-outflow-achieved? " << (state.report_outflow_achieved ? "true" : "false") << "\n"
                << "        :inflow-port" << state.inflow_port << "\n"
                << "        :outflow-port" << state.outflow_port << "}\n";
    }
    state = erin::devs::storage_internal_transition(data, state);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* = {:t " << state.time
                << " :soc " << state.soc
                << " :report-inflow-request? " << (state.report_inflow_request ? "true" : "false")
                << " :report-outflow-achieved? " << (state.report_outflow_achieved ? "true" : "false") << "\n"
                << "        :inflow-port" << state.inflow_port << "\n"
                << "        :outflow-port" << state.outflow_port << "}\n";
    }
    log_ports();
  }

  void
  Storage::delta_ext(Time e, std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_ext::" << get_id() << "::Storage\n"
                << "- s  = " << state << "\n";
    }
    state = erin::devs::storage_external_transition(data, state, e.real, xs);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* = " << state << "\n";
    }
    log_ports();
  }

  void
  Storage::delta_conf(std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_conf::" << get_id() << "::Storage\n"
                << "- xs = " << vec_to_string<PortValue>(xs) << "\n"
                << "- s  = " << state << "\n";
    }
    state = erin::devs::storage_confluent_transition(data, state, xs);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* = " << state << "\n";
    }
    log_ports();
  }

  Time
  Storage::ta()
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "ta::" << get_id() << "::Storage\n"
                << "- dt = ";
    }
    auto dt = erin::devs::storage_time_advance(data, state);
    if (dt == erin::devs::infinity) {
      if constexpr (debug_level >= debug_level_medium) {
        std::cout << "infinity\n";
      }
      return inf;
    }
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << dt << "\n";
    }
    return Time{dt, 1};
  }

  void
  Storage::output_func(std::vector<PortValue>& ys)
  {
    erin::devs::storage_output_function_mutable(data, state, ys);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "output_func::" << get_id() << "::Storage\n"
                << "- ys = " << vec_to_string<PortValue>(ys) << "\n";
    }
  }

  void
  Storage::set_flow_writer(const std::shared_ptr<FlowWriter>& writer)
  {
    flow_writer = writer;
    log_ports();
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "Storage::set_flow_writer();id = " << get_id() << "\n";
      std::cout << "record_history = " << record_history << "\n";
      std::cout << "flow_writer == nullptr: " << (flow_writer == nullptr) << "\n";
      std::cout << "inflow_element_id = " << inflow_element_id << "\n";
      std::cout << "outflow_element_id = " << outflow_element_id << "\n";
      std::cout << "state = " << state << "\n";
    }
  }

  void
  Storage::set_recording_on()
  {
    record_history = true;
    log_ports();
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "Storage::set_recording_on();id = " << get_id() << "\n";
      std::cout << "data = " << data << "\n";
      std::cout << "state = " << state << "\n";
      std::cout << "flow_writer == nullptr: " << (flow_writer == nullptr) << "\n";
      std::cout << "record_history = " << record_history << "\n";
      std::cout << "inflow_element_id = " << inflow_element_id << "\n";
      std::cout << "outflow_element_id = " << outflow_element_id << "\n";
      std::cout << "storeflow_element_id = " << storeflow_element_id << "\n";
      std::cout << "discharge_element_id = " << discharge_element_id << "\n";
    }
  }

  void
  Storage::set_storeflow_discharge_recording_on()
  {
    record_storeflow_and_discharge = true;
    log_ports();
  }

  void
  Storage::log_ports()
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "Storage::log_ports();id = " << get_id() << "\n";
      std::cout << "data = " << data << "\n";
      std::cout << "state = " << state << "\n";
      std::cout << "record_history = " << record_history << "\n";
      std::cout << "inflow_element_id = " << inflow_element_id << "\n";
      std::cout << "outflow_element_id = " << outflow_element_id << "\n";
      std::cout << "storeflow_element_id = " << storeflow_element_id << "\n";
      std::cout << "discharge_element_id = " << discharge_element_id << "\n";
    }
    if (flow_writer && (record_history || record_storeflow_and_discharge)) {
      if (record_history) {
        if (inflow_element_id == -1) {
          inflow_element_id = flow_writer->register_id(
              get_id() + "-inflow",
              get_inflow_type(),
              get_component_type(),
              PortRole::Inflow,
              true);
        }
        if (outflow_element_id == -1) {
          outflow_element_id = flow_writer->register_id(
              get_id() + "-outflow",
              get_outflow_type(),
              get_component_type(),
              PortRole::Outflow,
              true);
        }
      }
      if (storeflow_element_id == -1) {
        storeflow_element_id = flow_writer->register_id(
            get_id() + "-storeflow",
            get_outflow_type(),
            get_component_type(),
            PortRole::StorageInflow,
            true);
      }
      if (discharge_element_id == -1) {
        discharge_element_id = flow_writer->register_id(
            get_id() + "-discharge",
            get_outflow_type(),
            get_component_type(),
            PortRole::StorageOutflow,
            true);
      }
      if (record_history) {
        flow_writer->write_data(
            inflow_element_id,
            state.time,
            state.inflow_port.get_requested(),
            state.inflow_port.get_achieved());
        flow_writer->write_data(
            outflow_element_id,
            state.time,
            state.outflow_port.get_requested(),
            state.outflow_port.get_achieved());
      }
      auto storeflow_requested{
        state.inflow_port.get_requested()
        - state.outflow_port.get_requested()};
      auto storeflow_achieved{
        state.inflow_port.get_achieved()
        - state.outflow_port.get_achieved()};
      flow_writer->write_data(
          storeflow_element_id,
          state.time,
          storeflow_requested >= 0.0 ? storeflow_requested : 0.0,
          storeflow_achieved >= 0.0 ? storeflow_achieved : 0.0);
      flow_writer->write_data(
          discharge_element_id,
          state.time,
          storeflow_requested < 0.0 ? (-1.0 * storeflow_requested) : 0.0,
          storeflow_achieved < 0.0 ? (-1.0 * storeflow_achieved) : 0.0);
    }
  }

  ////////////////////////////////////////////////////////////
  // OnOffSwitch
  OnOffSwitch::OnOffSwitch(
      std::string id,
      ComponentType component_type,
      const std::string& stream_type,
      const std::vector<TimeState>& schedule,
      PortRole port_role_):
    FlowElement(
        std::move(id),
        component_type,
        ElementType::OnOffSwitch,
        stream_type),
    data{erin::devs::make_on_off_switch_data(schedule)},
    state{erin::devs::make_on_off_switch_state(data)},
    flow_writer{nullptr},
    record_history{false},
    element_id{-1},
    port_role{port_role_}
  {
  }

  void
  OnOffSwitch::delta_int()
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_int::" << get_id() << "::OnOffSwitch\n"
                << "- s  = " << state << "\n";
    }
    state = erin::devs::on_off_switch_internal_transition(data, state);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* = " << state << "\n";
    }
    log_ports();
  }

  void
  OnOffSwitch::delta_ext(Time e, std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_ext::" << get_id() << "::OnOffSwitch\n"
                << "- xs = " << vec_to_string<PortValue>(xs) << "\n"
                << "- s  = " << state << "\n";
    }
    state = erin::devs::on_off_switch_external_transition(state, e.real, xs);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* = " << state << "\n";
    }
    log_ports();
  }

  void
  OnOffSwitch::delta_conf(std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_conf::" << get_id() << "::OnOffSwitch\n"
                << "- xs = " << vec_to_string<PortValue>(xs) << "\n"
                << "- s  = " << state << "\n";
    }
    state = erin::devs::on_off_switch_confluent_transition(data, state, xs);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* = " << state << "\n";
    }
    log_ports();
  }

  Time
  OnOffSwitch::ta()
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "ta::" << get_id() << "::OnOffSwitch\n"
                << "- dt = ";
    }
    auto dt = erin::devs::on_off_switch_time_advance(data, state);
    if (dt == erin::devs::infinity) {
      if constexpr (debug_level >= debug_level_medium) {
        std::cout << "infinity\n";
      }
      return inf; 
    }
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << dt << "\n";
    }
    return Time{dt, 1};
  }

  void
  OnOffSwitch::output_func(std::vector<PortValue>& ys)
  {
    erin::devs::on_off_switch_output_function_mutable(data, state, ys);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "output_func::" << get_id() << "::OnOffSwitch\n"
                << "- ys = " << vec_to_string<PortValue>(ys) << "\n";
    }
  }

  void
  OnOffSwitch::set_flow_writer(const std::shared_ptr<FlowWriter>& writer)
  {
    flow_writer = writer;
    log_ports();
  }

  void
  OnOffSwitch::set_recording_on()
  {
    record_history = true;
    log_ports();
  }

  void
  OnOffSwitch::log_ports()
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "OnOffSwitch::log_ports(...); id = " << get_id() << "\n";
      std::cout << "flow_writer == nullptr = " << (flow_writer == nullptr) << "\n";
      std::cout << "record_history = " << record_history << "\n";
    }
    if (flow_writer && record_history) {
      if (element_id == -1) {
        element_id = flow_writer->register_id(
            get_id(),
            get_inflow_type(),
            get_component_type(),
            port_role,
            record_history);
      }
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "elementid= " << element_id << "\n"
                  << "time     = " << state.time << "\n"
                  << "request  = " << state.outflow_port.get_requested() << "\n"
                  << "achieved = " << state.outflow_port.get_achieved() << "\n";
      }
      flow_writer->write_data(
          element_id,
          state.time,
          state.outflow_port.get_requested(),
          state.outflow_port.get_achieved());
    }
  }

  ////////////////////////////////////////////////////////////
  // UncontrolledSource
  UncontrolledSource::UncontrolledSource(
      std::string id,
      ComponentType component_type,
      const std::string& stream_type,
      const std::vector<LoadItem>& profile):
    FlowElement(
        std::move(id),
        component_type,
        ElementType::UncontrolledSource,
        stream_type),
    data{erin::devs::make_uncontrolled_source_data(profile)},
    state{erin::devs::make_uncontrolled_source_state()},
    flow_writer{nullptr},
    element_id{-1},
    inflow_element_id{-1},
    lossflow_element_id{-1},
    record_history{false}
  {
  }

  void
  UncontrolledSource::delta_int()
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "UncontrolledSource::delta_int::" << get_id() << "\n"
                << "- s  = " << state << "\n";
    }
    state = erin::devs::uncontrolled_src_internal_transition(data, state);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* = " << state << "\n";
    }
    log_ports();
  }

  void
  UncontrolledSource::delta_ext(Time e, std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "UncontrolledSource::delta_ext::" << get_id() << "\n"
                << "- xs = " << vec_to_string<PortValue>(xs) << "\n"
                << "- s  = " << state << "\n";
    }
    state = erin::devs::uncontrolled_src_external_transition(
        data, state, e.real, xs);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* = " << state << "\n";
    }
    log_ports();
  }

  void
  UncontrolledSource::delta_conf(std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_conf::" << get_id() << "::UncontrolledSource\n"
                << "- xs = " << vec_to_string<PortValue>(xs) << "\n"
                << "- s  = " << state << "\n";
    }
    state = erin::devs::uncontrolled_src_confluent_transition(
        data, state, xs);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* = " << state << "\n";
    }
    log_ports();
  }

  Time
  UncontrolledSource::ta()
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "UncontrolledSource::ta::" << get_id() << "\n"
                << "- dt = ";
    }
    auto dt = erin::devs::uncontrolled_src_time_advance(data, state);
    if (dt == erin::devs::infinity) {
      if constexpr (debug_level >= debug_level_medium) {
        std::cout << "infinity\n";
      }
      return inf;
    }
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << dt << "\n";
    }
    return Time{dt, 1};
  }

  void
  UncontrolledSource::output_func(std::vector<PortValue>& ys)
  {
    erin::devs::uncontrolled_src_output_function_mutable(data, state, ys);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "output_func::" << get_id() << "::UncontrolledSource\n"
                << "- ys = " << vec_to_string<PortValue>(ys) << "\n";
    }
    log_ports();
  }

  void
  UncontrolledSource::log_ports()
  {
    if (flow_writer && record_history) {
      if (element_id == -1) {
        element_id = flow_writer->register_id(
            get_id() + "-outflow",
            get_outflow_type(),
            get_component_type(),
            PortRole::Outflow,
            record_history);
      }
      if (inflow_element_id == -1) {
        inflow_element_id = flow_writer->register_id(
            get_id() + "-inflow",
            get_inflow_type(),
            get_component_type(),
            PortRole::SourceOutflow,
            record_history);
      }
      if (lossflow_element_id == -1) {
        lossflow_element_id = flow_writer->register_id(
            get_id() + "-lossflow",
            get_outflow_type(),
            get_component_type(),
            PortRole::WasteInflow,
            record_history);
      }
      flow_writer->write_data(
          element_id,
          state.time,
          state.outflow_port.get_requested(),
          state.outflow_port.get_achieved());
      flow_writer->write_data(
          inflow_element_id,
          state.time,
          state.inflow_port.get_requested(),
          state.inflow_port.get_achieved());
      flow_writer->write_data(
          lossflow_element_id,
          state.time,
          state.spill_port.get_requested(),
          state.spill_port.get_achieved());
    }
  }

  void
  UncontrolledSource::set_flow_writer(const std::shared_ptr<FlowWriter>& writer)
  {
    flow_writer = writer;
    log_ports();
  }

  void
  UncontrolledSource::set_recording_on()
  {
    record_history = true;
    log_ports();
  }

  ////////////////////////////////////////////////////////////
  // Mover
  Mover::Mover(
      std::string id,
      ComponentType component_type,
      const std::string& inflow0_,
      const std::string& inflow1_,
      const std::string& outflow,
      FlowValueType COP):
    FlowElement(
        std::move(id),
        component_type,
        ElementType::Mover,
        inflow0_,
        outflow),
    inflow0{inflow0_},
    inflow1{inflow1_},
    data{erin::devs::make_mover_data(COP)},
    state{erin::devs::make_mover_state()},
    flow_writer{nullptr},
    inflow0_element_id{-1},
    inflow1_element_id{-1},
    outflow_element_id{-1},
    record_history{false}
  {
  }

  void
  Mover::delta_int()
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "Mover::delta_int::" << get_id() << "\n"
                << "- s  = " << state << "\n";
    }
    state = erin::devs::mover_internal_transition(data, state);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* = " << state << "\n";
    }
    log_ports();
  }

  void
  Mover::delta_ext(Time e, std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_ext::" << get_id() << "::Mover\n"
                << "- xs = " << vec_to_string<PortValue>(xs) << "\n"
                << "- s  = " << state << "\n";
    }
    state = erin::devs::mover_external_transition(data, state, e.real, xs);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* = " << state << "\n";
    }
    log_ports();
  }

  void
  Mover::delta_conf(std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "delta_conf::" << get_id() << "::Mover\n"
                << "- xs = " << vec_to_string<PortValue>(xs) << "\n"
                << "- s  = " << state << "\n";
    }
    state = erin::devs::mover_confluent_transition(data, state, xs);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "- s* = " << state << "\n";
    }
    log_ports();
  }

  Time
  Mover::ta()
  {
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "ta::" << get_id() << "::Mover\n"
                << "- dt = ";
    }
    auto dt = erin::devs::mover_time_advance(data, state);
    if (dt == erin::devs::infinity) {
      if constexpr (debug_level >= debug_level_medium) {
        std::cout << "infinity\n";
      }
      return inf;
    }
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << dt << "\n";
    }
    return Time{dt, 1};
  }

  void
  Mover::output_func(std::vector<PortValue>& ys)
  {
    erin::devs::mover_output_function_mutable(data, state, ys);
    if constexpr (debug_level >= debug_level_medium) {
      std::cout << "output_func::" << get_id() << "::Mover\n"
                << "- ys = " << vec_to_string<PortValue>(ys) << "\n";
    }
  }

  void
  Mover::set_flow_writer(const std::shared_ptr<FlowWriter>& writer)
  {
    flow_writer = writer;
    log_ports();
  }

  void
  Mover::set_recording_on()
  {
    record_history = true;
    log_ports();
  }

  std::string
  Mover::get_inflow_type_by_port(int inflow_port) const
  {
    std::string the_inflow{};
    if (inflow_port == 0) {
      the_inflow = inflow0;
    }
    else if (inflow_port == 1) {
      the_inflow = inflow1;
    }
    else {
    }
    return the_inflow;
  }

  void
  Mover::log_ports()
  {
    if (flow_writer && record_history) {
      if (inflow0_element_id == -1) {
        inflow0_element_id = flow_writer->register_id(
            get_id() + "-inflow(0)",
            inflow0,
            get_component_type(),
            PortRole::Inflow,
            record_history);
      }
      if (inflow1_element_id == -1) {
        inflow1_element_id = flow_writer->register_id(
            get_id() + "-inflow(1)",
            inflow1,
            get_component_type(),
            PortRole::Inflow,
            record_history);
      }
      if (outflow_element_id == -1) {
        outflow_element_id = flow_writer->register_id(
            get_id() + "-outflow",
            get_outflow_type(),
            get_component_type(),
            PortRole::Outflow,
            record_history);
      }
      flow_writer->write_data(
          inflow0_element_id,
          state.time,
          state.inflow0_port.get_requested(),
          state.inflow0_port.get_achieved());
      flow_writer->write_data(
          inflow1_element_id,
          state.time,
          state.inflow1_port.get_requested(),
          state.inflow1_port.get_achieved());
      flow_writer->write_data(
          outflow_element_id,
          state.time,
          state.outflow_port.get_requested(),
          state.outflow_port.get_achieved());
    }
  }

  ////////////////////////////////////////////////////////////
  // Helper
  void
  print_ports(
      const std::vector<erin::devs::Port>& ports,
      const std::string& tag)
  {
    int idx{0};
    for (const auto& p : ports) {
      std::cout << tag << "(" << idx << "): " << p << "\n";
      ++idx;
    }
  }
}
