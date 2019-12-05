/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/element.h"
#include "debug_utils.h"
#include <algorithm>
#include <cmath>
#include <set>

namespace ERIN
{
  ////////////////////////////////////////////////////////////
  // FlowElement
  FlowElement::FlowElement(
      std::string id_,
      ComponentType component_type_,
      StreamType st) :
    FlowElement(std::move(id_), component_type_, st, st)
  {
  }

  FlowElement::FlowElement(
      std::string id_,
      ComponentType component_type_,
      StreamType in,
      StreamType out):
    adevs::Atomic<PortValue, Time>(),
    id{std::move(id_)},
    time{0,0},
    inflow_type{std::move(in)},
    outflow_type{std::move(out)},
    inflow{0},
    outflow{0},
    storeflow{0},
    lossflow{0},
    report_inflow_request{false},
    report_outflow_achieved{false},
    component_type{component_type_}
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
    FlowValueType inflow_achieved{0};
    FlowValueType outflow_request{0};
    for (const auto &x : xs) {
      switch (x.port) {
        case inport_inflow_achieved:
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "... <=inport_inflow_achieved\n";
          }
          inflow_provided = true;
          inflow_achieved += x.value;
          break;
        case inport_outflow_request:
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "... <=inport_outflow_request\n";
          }
          outflow_provided = true;
          outflow_request += x.value;
          break;
        default:
          std::ostringstream oss;
          oss << "BadPortError: unhandled port: \"" << x.port << "\"";
          throw std::runtime_error(oss.str());
      }
    }
    run_checks_after_receiving_inputs(
        inflow_provided, inflow_achieved, outflow_provided, outflow_request);
  }

  void
  FlowElement::run_checks_after_receiving_inputs(
      bool inflow_provided,
      FlowValueType inflow_achieved,
      bool outflow_provided,
      FlowValueType outflow_request)
  {
    if (inflow_provided && !outflow_provided) {
      report_outflow_achieved = true;
      if (inflow > 0.0 && inflow_achieved > inflow) {
        std::ostringstream oss;
        oss << "AchievedMoreThanRequestedError\n";
        oss << "inflow >= 0.0 and inflow_achieved > inflow\n";
        oss << "inflow: " << inflow << "\n";
        oss << "inflow_achieved: " << inflow_achieved << "\n";
        throw std::runtime_error(oss.str());
      }
      if (inflow < 0.0 && inflow_achieved < inflow) {
        std::ostringstream oss;
        oss << "AchievedMoreThanRequestedError\n";
        oss << "inflow <= 0.0 and inflow_achieved < inflow\n";
        oss << "inflow: " << inflow << "\n";
        oss << "inflow_achieved: " << inflow_achieved << "\n";
        throw std::runtime_error(oss.str());
      }
      const FlowState& fs = update_state_for_inflow_achieved(inflow_achieved);
      update_state(fs);
    }
    else if (outflow_provided && !inflow_provided) {
      report_inflow_request = true;
      const FlowState fs = update_state_for_outflow_request(outflow_request);
      if (std::fabs(fs.getOutflow() - outflow_request) > flow_value_tolerance) {
        report_outflow_achieved = true;
      }
      update_state(fs);
      if (outflow > 0.0 && outflow > outflow_request) {
        std::ostringstream oss;
        oss << "AchievedMoreThanRequestedError\n";
        oss << "outflow >= 0.0 && outflow > outflow_request\n";
        oss << "outflow: " << outflow << "\n";
        oss << "outflow_request: " << outflow_request << "\n";
        throw std::runtime_error(oss.str());
      }
      if (outflow < 0.0 && outflow < outflow_request) {
        std::ostringstream oss;
        oss << "AchievedMoreThanRequestedError\n";
        oss << "outflow <= 0.0 && outflow < outflow_request\n";
        oss << "outflow: " << outflow << "\n";
        oss << "outflow_request: " << outflow_request << "\n";
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
      StreamType stream_type_,
      FlowValueType low_lim,
      FlowValueType up_lim):
    FlowElement(std::move(id_), component_type_, stream_type_),
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
      StreamType stream_type) :
    FlowElement(std::move(id), component_type, stream_type),
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
    std::vector<Datum> results(num_events);
    for (std::vector<Datum>::size_type i=0; i < num_events; ++i) {
      results[i] = Datum{event_times[i], requested_flows[i], achieved_flows[i]};
    }
    if (results[num_events-1].time != max_time) {
      results.emplace_back(Datum{max_time, 0.0, 0.0});
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
        input_stream_type,
        output_stream_type),
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
    FlowElement(std::move(id), component_type, st, st),
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
    }
    if (idx < 0) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... dt = infinity\n";
      }
      return Time{0, 0};
    }
    std::vector<LoadItem>::size_type next_idx = idx + 1;
    if (next_idx < num_loads) {
      RealTimeType dt{loads[idx].get_time_advance(loads[next_idx])};
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... dt = (" << dt << ", 0)\n";
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
    std::vector<LoadItem>::size_type next_idx = idx + 1;
    auto max_idx{num_loads - 1};
    if (next_idx < max_idx)
      ys.emplace_back(
          adevs::port_value<FlowValueType>{
          outport_inflow_request, loads[next_idx].get_value()});
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
  // Mux
  Mux::Mux(
      std::string id,
      ComponentType ct,
      const StreamType& st,
      int num_inflows_,
      int num_outflows_,
      MuxerDispatchStrategy strategy_):
    FlowElement(std::move(id), ct, st, st),
    num_inflows{num_inflows_},
    num_outflows{num_outflows_},
    strategy{strategy_},
    inflow_request{0.0},
    inflow_port_for_request{0},
    inflows{},
    prev_inflows{},
    outflows{},
    prev_outflows{}
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
    outflows = std::vector<FlowValueType>(num_outflows, 0.0);
    prev_outflows = std::vector<FlowValueType>(num_outflows, 0.0);
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
    FlowValueType inflow_achieved{0};
    int highest_inflow_port_received{-1};
    int highest_outflow_port_received{-1};
    FlowValueType outflow_request{0};
    auto new_inflows = inflows;
    auto new_outflows = outflows;
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
        new_inflows[port_n] = x.value;
        prev_inflows[port_n] = x.value;
        inflow_achieved += x.value;
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
        new_outflows[port_n] = x.value;
        prev_outflows[port_n] = x.value;
        outflow_request += x.value;
        outflow_provided = true;
      }
      else {
        std::ostringstream oss;
        oss << "BadPortError: unhandled port: \"" << x.port << "\"";
        throw std::runtime_error(oss.str());
      }
    }
    FlowValueType total_inflow_achieved{0.0};
    for (const auto x: new_inflows) {
      total_inflow_achieved += x;
    }
    FlowValueType total_outflow_request{0.0};
    for (const auto x: new_outflows) {
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
        auto of = get_outflow();
        auto diff = std::abs(of - total_inflow_achieved);
        if (diff < flow_value_tolerance) {
          update_state(FlowState{total_inflow_achieved});
          inflows = new_inflows;
        }
        else {
          const auto final_inflow_port = num_inflows - 1;
          if (highest_inflow_port_received < final_inflow_port) {
            inflow_request = of - total_inflow_achieved;
            inflow_port_for_request = highest_inflow_port_received + 1;
            set_report_outflow_achieved(false);
            set_report_inflow_request(true);
            inflows = new_inflows;
            inflows[inflow_port_for_request] = inflow_request;
          }
          else {
            // We've requested to all inflow ports and are still deficient on
            // meeting outflow request. Update outflows.
            auto desired_outflow = of;
            update_state(FlowState{total_inflow_achieved});
            inflows = new_inflows;
            FlowValueType reduction_factor{1.0};
            if (desired_outflow != 0.0) {
              reduction_factor = total_inflow_achieved / desired_outflow;
              auto the_prev_outflows = outflows;
              for (auto& of_item: outflows) {
                of_item *= reduction_factor;
              }
              if constexpr (debug_level >= debug_level_low) {
                std::cout
                  << "inflow                : " << get_inflow() << "\n"
                  << "outflow               : " << get_outflow() << "\n"
                  << "highest_inflow_port_received: "
                  << highest_inflow_port_received << "\n"
                  << "highest_outflow_port_received: "
                  << highest_outflow_port_received << "\n"
                  << "num_inflows           : " << num_inflows << "\n"
                  << "num_outflows          : " << num_outflows << "\n"
                  << "desired_outflow       : " << desired_outflow << "\n"
                  << "total_inflow_achieved : " << total_inflow_achieved << "\n"
                  << "reduction_factor      : " << reduction_factor << "\n";
                int i{0};
                for (const auto& n : inflows) {
                  std::cout << "inflows[" << i << "]=" << n << "\n";
                  ++i;
                }
                i = 0;
                for (const auto& n : the_prev_outflows) {
                  std::cout << "prev_outflows[" << i << "]=" << n << "\n";
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
      if (strategy == MuxerDispatchStrategy::InOrder) {
        // Whenever the outflow request updates, we always start querying
        // inflows from the first inflow port and update the inflow_request.
        inflow_port_for_request = 0;
        inflow_request = total_outflow_request;
        inflows = std::vector<FlowValueType>(num_inflows, 0.0);
        inflows[inflow_port_for_request] = inflow_request;
        update_state(FlowState{inflow_request});
        outflows = new_outflows;
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
    if constexpr (debug_level >= debug_level_low) {
      std::cout << "FlowElement::output_func();id=" << get_id() << "\n";
    }
    if (get_report_inflow_request()) {
      if constexpr (debug_level >= debug_level_low) {
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
      if constexpr (debug_level >= debug_level_low) {
        std::cout << "... send=>outport_outflow_achieved\n";
      }
      for (int i{0}; i < num_outflows; ++i) {
        auto outflow_ = outflows[i];
        auto prev_outflow_ = prev_outflows[i];
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

  void
  Mux::update_on_internal_transition()
  {
    inflow_request = 0.0;
    inflow_port_for_request = 0;
  }
}
