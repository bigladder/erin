/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/element.h"
#include "debug_utils.h"

namespace ERIN
{
  ////////////////////////////////////////////////////////////
  // FlowElement
  const int FlowElement::inport_inflow_achieved = 0;
  const int FlowElement::inport_outflow_request = 1;
  const int FlowElement::outport_inflow_request = 2;
  const int FlowElement::outport_outflow_achieved = 3;

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
    adevs::Atomic<PortValue>(),
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
    if (inflow_type.get_rate_units() != outflow_type.get_rate_units())
      throw InconsistentStreamUnitsError();
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
  FlowElement::delta_ext(adevs::Time e, std::vector<PortValue>& xs)
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
          throw BadPortError();
      }
    }
    if (inflow_provided && !outflow_provided) {
      report_outflow_achieved = true;
      if (inflow >= 0.0 && inflow_achieved > inflow) {
        throw AchievedMoreThanRequestedError();
      }
      if (inflow <= 0.0 && inflow_achieved < inflow) {
        throw AchievedMoreThanRequestedError();
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
      if (outflow >= 0.0 && outflow > outflow_request) {
        throw AchievedMoreThanRequestedError();
      }
      if (outflow <= 0.0 && outflow < outflow_request) {
        throw AchievedMoreThanRequestedError();
      }
    }
    else if (inflow_provided && outflow_provided) {
      // assumption: we'll never get here...
      throw SimultaneousIORequestError();
    }
    else {
      throw BadPortError();
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
    auto e = adevs::Time{0,0};
    delta_int();
    delta_ext(e, xs);
  }

  adevs::Time
  FlowElement::calculate_time_advance()
  {
    return adevs_inf<adevs::Time>();
  }

  adevs::Time
  FlowElement::ta()
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "FlowElement::ta();id=" << id << "\n";
    }
    if (report_inflow_request || report_outflow_achieved) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... dt = (0,1)\n";
      }
      return adevs::Time{0, 1};
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
      std::string id,
      ComponentType component_type,
      StreamType stream_type,
      FlowValueType low_lim,
      FlowValueType up_lim) :
    FlowElement(std::move(id), component_type, stream_type),
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
      throw AchievedMoreThanRequestedError();
    }
    else if (in < lower_limit) {
      throw AchievedMoreThanRequestedError();
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
    if ((requested_flows.size() != num_events)
        || (achieved_flows.size() != num_events)) {
      throw InvariantError();
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
    if (num_events == 0 || (num_events > 0 && event_times.back() != real_time)) {
      event_times.push_back(real_time);
      ++num_events;
    }
    auto num_achieved{achieved_flows.size()};
    if (get_report_inflow_request()) {
      requested_flows.push_back(get_inflow());
      if (num_achieved == num_events && num_achieved > 0) {
        auto &v = achieved_flows.back();
        v = get_inflow();
      }
      else
        achieved_flows.push_back(get_inflow());
    }
    else if (get_report_outflow_achieved()) {
      if (num_achieved == num_events && num_achieved > 0) {
        auto &v = achieved_flows.back();
        v = get_outflow();
      }
      else
        achieved_flows.push_back(get_outflow());
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

  adevs::Time
  Sink::calculate_time_advance()
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "Sink::calculate_time_advance()\n";
    }
    if (idx < 0) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... dt = infinity\n";
      }
      return adevs::Time{0, 0};
    }
    std::vector<LoadItem>::size_type next_idx = idx + 1;
    if (next_idx < num_loads) {
      RealTimeType dt{loads[idx].get_time_advance(loads[next_idx])};
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... dt = (" << dt << ", 0)\n";
      }
      return adevs::Time{dt, 0};
    }
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "... dt = infinity\n";
    }
    return adevs_inf<adevs::Time>();
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
}
