/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
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
    report_inflow_request{false},
    report_outflow_achieved{false},
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
    FlowValueType the_inflow_achieved{0};
    FlowValueType the_outflow_request{0};
    for (const auto &x : xs) {
      switch (x.port) {
        case inport_inflow_achieved:
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "... <=inport_the_inflow_achieved\n";
          }
          inflow_provided = true;
          the_inflow_achieved += x.value;
          break;
        case inport_outflow_request:
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "... <=inport_the_outflow_request\n";
          }
          outflow_provided = true;
          the_outflow_request += x.value;
          break;
        default:
          std::ostringstream oss;
          oss << "BadPortError: unhandled port: \"" << x.port << "\"";
          throw std::runtime_error(oss.str());
      }
    }
    run_checks_after_receiving_inputs(
        inflow_provided, the_inflow_achieved,
        outflow_provided, the_outflow_request);
  }

  void
  FlowElement::run_checks_after_receiving_inputs(
      bool inflow_provided,
      FlowValueType the_inflow_achieved,
      bool outflow_provided,
      FlowValueType the_outflow_request)
  {
    if (inflow_provided && !outflow_provided) {
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
      update_state(fs);
    }
    else if (outflow_provided && !inflow_provided) {
      report_inflow_request = true;
      outflow_request = the_outflow_request;
      const FlowState fs = update_state_for_outflow_request(outflow_request);
      // update what the inflow_request is based on all outflow_requests
      inflow_request = fs.getInflow();
      auto diff = std::fabs(fs.getOutflow() - outflow_request);
      if (diff > flow_value_tolerance) {
        report_outflow_achieved = true;
      }
      update_state(fs);
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
      oss << "SimultaneousIORequestError: assumption was we'd never get here...";
      throw std::runtime_error(oss.str());
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
    inflow = fs.getInflow();
    outflow = fs.getOutflow();
    storeflow = fs.getStoreflow();
    lossflow = fs.getLossflow();
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
    if (report_inflow_request || report_outflow_achieved) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... dt = (0,1)\n";
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
        std::cout << "... send=>outport_inflow_request\n";
      }
      ys.emplace_back(
          adevs::port_value<FlowValueType>{outport_inflow_request, inflow});
    }
    if (report_outflow_achieved) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... send=>outport_outflow_achieved\n";
      }
      ys.emplace_back(
          adevs::port_value<FlowValueType>{outport_outflow_achieved, outflow});
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
      FlowValueType low_lim,
      FlowValueType up_lim):
    FlowElement(
        std::move(id_),
        component_type_,
        ElementType::FlowLimits,
        stream_type_),
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

  FlowState
  FlowLimits::update_state_for_outflow_request(FlowValueType out) const
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "FlowLimits::update_state_for_outflow_request(" << out << ")\n";
      print_state("... ");
    }
    FlowValueType out_{0.0};
    if (out > upper_limit) {
      out_ = upper_limit;
    }
    else if (out < lower_limit) {
      out_ = lower_limit;
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
    if (in > upper_limit) {
      std::ostringstream oss;
      oss << "AchievedMoreThanRequestedError\n";
      oss << "in > upper_limit\n";
      oss << "in: " << in << "\n";
      oss << "upper_limit: " << upper_limit << "\n";
      throw std::runtime_error(oss.str());
    }
    else if (in < lower_limit) {
      std::ostringstream oss;
      oss << "AchievedMoreThanRequestedError\n";
      oss << "in < lower_limit\n";
      oss << "in: " << in << "\n";
      oss << "lower_limit: " << lower_limit << "\n";
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
    achieved_flows{}
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
      for (std::vector<RealTimeType>::size_type i{0}; i < num_events; ++i) {
        oss << "event_times[" << i << "]      = " << event_times[i] << "\n";
        if (i < num_rfs) {
          oss << "requested_flows[" << i << "]  = " << requested_flows[i] << "\n";
        }
        if (i < num_afs) {
          oss << "achieved_flows[" << i << "]   = " << achieved_flows[i] << "\n";
        }
      }
      throw std::runtime_error(oss.str());
    }
    std::vector<Datum>::size_type max_idx = 0;
    for (std::vector<Datum>::size_type i=0; i < num_events; ++i) {
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
    std::vector<Datum>::size_type num_datums = num_events;
    if (time_0_missing) {
      ++num_datums;
    }
    if (max_time_missing) {
      ++num_datums;
    }
    std::vector<Datum> results(num_datums);
    for (std::vector<Datum>::size_type i=0; i < num_datums; ++i) {
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
        results[i] = Datum{event_times[j], requested_flows[j], achieved_flows[j]};
      }
    }
    return results;
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
      event_times.push_back(real_time);
      ++num_events;
    }
    auto num_requested{requested_flows.size()};
    auto num_achieved{achieved_flows.size()};
    if (get_report_inflow_request()) {
      if (num_requested == (num_events - 1)) {
        requested_flows.push_back(get_inflow());
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
        achieved_flows.push_back(get_inflow());
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
        achieved_flows.push_back(of);
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
          requested_flows.push_back(requested_flows.back());
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
      std::function<FlowValueType(FlowValueType)> calc_input_from_output
      ) :
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
      default:
        break;
    }
    std::ostringstream oss;
    oss << "unhandled Muxer_dispatch_strategy \""
      << static_cast<int>(mds) << "\"\n";
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
      MuxerDispatchStrategy strategy_):
    FlowElement(
        std::move(id),
        ct,
        ElementType::Mux,
        st),
    num_inflows{num_inflows_},
    num_outflows{num_outflows_},
    strategy{strategy_},
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
    for (const auto &x : xs) {
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
          std::cout << "... <=inport_inflow_achieved(" << port_n << ")\n";
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
          std::cout << "... <=inport_outflow_request(" << port_n << ")\n";
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
      if (total_inflow_achieved > get_inflow()) {
        std::ostringstream oss;
        oss << "AchievedMoreThanRequestedError\n";
        oss << "total_inflow_achieved > inflow\n";
        oss << "inflow: " << get_inflow() << "\n";
        oss << "total_inflow_achieved: " << total_inflow_achieved << "\n";
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
            // We've (apparently) requested to all inflow ports and are still
            // deficient on meeting outflow request. Update outflows.
            // TODO: what really needs to happen is we need to track all the
            // ports we've sent inflow requests out on since *any* outflow
            // requests last changed. That could be a stack and we could start
            // with having all ports on the stack and just keep popping them
            // each time we request out. If we just heard back from a port,
            // we'd have to pop it from the stack until we have no more left.
            update_state(FlowState{total_inflow_achieved});
            inflows = inflows_achieved;
            FlowValueType reduction_factor{1.0};
            if (total_outflow_request != 0.0) {
              reduction_factor = total_inflow_achieved / total_outflow_request;
              for (auto& of_item: outflows) {
                of_item *= reduction_factor;
              }
              if constexpr (debug_level >= debug_level_high) {
                std::cout
                  << "inflow                : " << get_inflow() << "\n"
                  << "outflow               : " << get_outflow() << "\n"
                  << "highest_inflow_port_received: "
                  << highest_inflow_port_received << "\n"
                  << "highest_outflow_port_received: "
                  << highest_outflow_port_received << "\n"
                  << "num_inflows           : " << num_inflows << "\n"
                  << "num_outflows          : " << num_outflows << "\n"
                  << "total_inflow_achieved : " << total_inflow_achieved << "\n"
                  << "total_outflow_request : " << total_outflow_request << "\n"
                  << "reduction_factor      : " << reduction_factor << "\n";
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
      // assumption: we'll never get here...
      std::ostringstream oss;
      oss << "SimultaneousIORequestError: assumption was we'd never get here...";
      throw std::runtime_error(oss.str());
    }
    else {
      std::ostringstream oss;
      oss << "BadPortError: no relevant ports detected...";
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
      std::cout << "FlowElement::output_func();id=" << get_id() << "\n";
    }
    if (get_report_inflow_request()) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... send=>outport_inflow_request\n";
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
        std::cout << "... send=>outport_outflow_achieved\n";
        std::cout << "t = " << get_real_time() << "\n";
      }
      for (int i{0}; i < num_outflows; ++i) {
        auto outflow_ = outflows[i];
        auto prev_outflow_ = prev_outflows[i];
        if constexpr (debug_level >= debug_level_high) {
          std::cout << "outflow[" << i << "] = " << outflow_ << "\n";
          std::cout << "prev_outflow[" << i << "] = " << prev_outflow_ << "\n";
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
