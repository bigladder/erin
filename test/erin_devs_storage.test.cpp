/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin/devs.h"
#include "erin/devs/storage.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

namespace ED = erin::devs;

enum class TransitionType {
  InitialState = 0,
  InternalTransition,
  ExternalTransition,
  ConfluentTransition
};

struct TimeStateOutputs
{
  TransitionType transition_type{TransitionType::InternalTransition};
  ED::RealTimeType time_s{0};
  ED::StorageState state = ED::storage_make_state(ED::storage_make_data(100.0, 10.0), 1.0);
  std::vector<ED::PortValue> outputs{};
};

std::string
port_to_tag(int port)
{
  switch (port)
  {
    case ED::inport_inflow_achieved:
      return "inport_inflow_achieved";
    case ED::inport_outflow_request:
      return "inport_outflow_request";
    case ED::outport_inflow_request:
      return "outport_inflow_request";
    case ED::outport_outflow_achieved:
      return "outflow_outflow_achieved";
    default:
      return "unknown_port_" + std::to_string(port);
  }
}

std::string
port_values_to_string(const std::vector<ED::PortValue>& port_values)
{
  std::ostringstream oss{};
  bool first{true};
  for (const auto& pv : port_values) {
    oss << (first ? "" : ",")
        << "PortValue{" << port_to_tag(pv.port)
        << ", " << pv.value << "}";
    first = false;
  }
  return oss.str();
}

std::string
transition_type_to_tag(const TransitionType& tt)
{
  switch (tt)
  {
    case TransitionType::InitialState:
      return "init";
    case TransitionType::InternalTransition:
      return "int";
    case TransitionType::ExternalTransition:
      return "ext";
    case TransitionType::ConfluentTransition:
      return "conf";
    default:
      return "?";
  }
}

std::vector<TimeStateOutputs>
run_devs(
    const ED::StorageData& data,
    const ED::StorageState& state,
    const std::vector<ED::RealTimeType> times_s,
    const std::vector<std::vector<ED::PortValue>>& xss,
    const ED::RealTimeType& max_time_s)
{
  using size_type = std::vector<ED::RealTimeType>::size_type;
  std::vector<TimeStateOutputs> log{};
  /*
    time←0
    current state ← initial state
    last time ← −initial elapsed
    while not termination condition(current state, time) do
      next time ← last time+ta(current state)
      if time next event ≤ next time then
        elapsed←time next event−last time
        current state ← δext((current state,elapsed),next event) time ← time next event
      else
        time ← next time output(λ(current state))
        current state←δint(current state)
      end if
      last time ← time
    end while
   */
  ED::StorageState s{state};
  ED::RealTimeType t_last{0};
  ED::RealTimeType t_next{0};
  size_type ext_idx{0};
  ED::RealTimeType t_next_ext{ED::infinity};
  std::vector<ED::PortValue> ys{};
  log.emplace_back(
      TimeStateOutputs{TransitionType::InitialState, t_next, s, ys});
  while ((t_next != ED::infinity) || (t_next_ext != ED::infinity)) {
    auto dt = ED::storage_time_advance(data, s);
    if (dt == ED::infinity) {
      t_next = ED::infinity;
    }
    else {
      t_next = t_last + dt;
    }
    if (ext_idx < times_s.size()) {
      t_next_ext = times_s[ext_idx];
    }
    else {
      t_next_ext = ED::infinity;
    }
    if (((t_next == ED::infinity) || (t_next > max_time_s)) &&
        ((t_next_ext == ED::infinity) || (t_next_ext > max_time_s))) {
      break;
    }
    if ((t_next_ext == ED::infinity) || ((t_next != ED::infinity) && (t_next < t_next_ext))) {
      // internal transition
      ys = ED::storage_output_function(data, s);
      s = ED::storage_internal_transition(data, s);
      log.emplace_back(
          TimeStateOutputs{TransitionType::InternalTransition, t_next, s, ys});
    }
    else {
      const auto& xs = xss[ext_idx];
      ext_idx++;
      if (t_next == t_next_ext) {
        ys = ED::storage_output_function(data, s);
        s = ED::storage_confluent_transition(data, s, xs);
        log.emplace_back(
            TimeStateOutputs{TransitionType::ConfluentTransition, t_next, s, ys});
      }
      else {
        auto e{t_next_ext - t_last};
        t_next = t_last + e;
        s = ED::storage_external_transition(data, s, e, xs);
        ys.clear();
        log.emplace_back(
            TimeStateOutputs{TransitionType::ExternalTransition, t_next, s, ys});
      }
    }
    t_last = t_next;
  }
  return log;
}

void
write_details(const TimeStateOutputs& out)
{
  std::cout << "------------------------\n"
            << " transition type: "
            << transition_type_to_tag(out.transition_type) << "\n"
            << " time (seconds) : " << out.time_s << "\n"
            << " state          : " << out.state << "\n"
            << " outputs        : "
            << port_values_to_string(out.outputs) << "\n";
}

bool
test_undisturbed_discharge(bool show_details)
{
  using size_type = std::vector<double>::size_type;
  const std::vector<ED::RealTimeType> expected_times_s{0, 0, 0, 20, 40};
  const std::vector<ED::FlowValueType> expected_socs{1.0, 1.0, 1.0, 0.0, 1.0};
  const std::vector<ED::FlowValueType> expected_inflows_requested{0.0, 5.0, 5.0, 10.0, 5.0};
  const std::vector<ED::FlowValueType> expected_inflows_achieved{0.0, 5.0, 0.0, 10.0, 5.0};
  const std::vector<ED::FlowValueType> expected_outflows_requested{0.0, 5.0, 5.0, 5.0, 5.0};
  const std::vector<ED::FlowValueType> expected_outflows_achieved{0.0, 5.0, 5.0, 5.0, 5.0};
  const ED::FlowValueType tol{1e-6};
  auto data = ED::storage_make_data(100.0, 10.0);
  auto s0 = ED::storage_make_state(data, 1.0);
  const std::vector<ED::RealTimeType> times_s{
    0,
    0,
  };
  const std::vector<std::vector<ED::PortValue>> xss{
    {ED::PortValue{ED::inport_outflow_request, 5.0}},
    {ED::PortValue{ED::inport_inflow_achieved, 0.0}},
  };
  auto outputs = run_devs(data, s0, times_s, xss, 100);
  if (outputs.size() != expected_times_s.size()) {
    std::cout << "Expected outputs.size() == expected_times_s.size()\n"
              << "outputs.size() = " << outputs.size() << "\n"
              << "expected_times_s.size() = "
              << expected_times_s.size() << "\n";
    return false;
  }
  for (size_type idx{0}; idx < outputs.size(); idx++) {
    const auto& out = outputs[idx];
    const auto& t_expected = expected_times_s[idx];
    const auto& soc_expected = expected_socs[idx];
    const auto& ia_expected = expected_inflows_achieved[idx];
    const auto& ir_expected = expected_inflows_requested[idx];
    const auto& oa_expected = expected_outflows_achieved[idx];
    const auto& or_expected = expected_outflows_requested[idx];
    if (out.time_s != t_expected) {
      std::cout << "event times do not meet expectation\n"
                << "idx        : " << idx << "\n"
                << "out.time_s : " << out.time_s << "\n"
                << "t_expected : " << t_expected << "\n";
      return false;
    }
    if (std::abs(out.state.soc - soc_expected) > tol) {
      std::cout << "event SOCs do not meet expectation\n"
                << "idx        : " << idx << "\n"
                << "out.time_s : " << out.state.soc << "\n"
                << "t_expected : " << soc_expected << "\n";
      return false;
    }
    if (std::abs(out.state.inflow_port.get_achieved() - ia_expected) > tol) {
      std::cout << "inflow achieved does not meet expectation\n"
                << "idx: " << idx << "\n"
                << "out.state.inflow_port.get_achieved(): "
                << out.state.inflow_port.get_achieved() << "\n"
                << "ia_expected: "
                << ia_expected << "\n";
      return false;
    }
    if (std::abs(out.state.inflow_port.get_requested() - ir_expected) > tol) {
      std::cout << "inflow request does not meet expectation\n"
                << "idx: " << idx << "\n"
                << "out.state.inflow_port.get_requested(): "
                << out.state.inflow_port.get_requested() << "\n"
                << "ir_expected: "
                << ir_expected << "\n";
      return false;
    }
    if (std::abs(out.state.outflow_port.get_achieved() - oa_expected) > tol) {
      std::cout << "outflow achieved does not meet expectation\n"
                << "idx: " << idx << "\n"
                << "out.state.outflow_port.get_achieved(): "
                << out.state.outflow_port.get_achieved() << "\n"
                << "oa_expected: " << oa_expected << "\n";
      return false;
    }
    if (std::abs(out.state.outflow_port.get_requested() - or_expected) > tol) {
      std::cout << "outflow requested does not meet expectation\n"
                << "idx: " << idx << "\n"
                << "out.state.outflow_port.get_requested(): "
                << out.state.outflow_port.get_requested() << "\n"
                << "or_expected: " << or_expected << "\n";
      return false;
    }
    if (show_details) {
      write_details(out);
    }
  }
  return true;
}

int
main() {
  const bool show_details{true};
  int num_tests{0};
  int num_passed{0};
  bool current_test{true};
  std::cout << "test_undisturbed_discharge:\n";
  current_test = test_undisturbed_discharge(show_details);
  num_tests++;
  if (current_test) {
    num_passed++;
  }
  std::cout << "=> " << (current_test ? "PASSED" : "FAILED!!!!!") << "\n";
  ////
  std::cout << "Summary: " << num_passed << " / " << num_tests << " passed.\n";
  return 0;
}
