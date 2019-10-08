/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */
#include "disco/disco.h"
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <map>
#include "../vendor/toml11/toml.hpp"

namespace DISCO
{
  const bool DEBUG{true};
  const FlowValueType TOL{1e-6};

  //////////////////////////////////////////////////////////// 
  // Main
  // main class that runs the simulation from file
  Main::Main(std::string in_path, std::string out_path):
    input_file_path{in_path},
    output_file_path{out_path}
  {
  }

  bool
  Main::run()
  {
    const auto data = toml::parse(input_file_path);
    // [stream_info]
    const auto stream_info = toml::find(data, "stream_info");
    const std::string stream_info_rate_unit(
        toml::find_or(stream_info, "rate_unit", "kW"));
    const std::string stream_info_quantity_unit(
        toml::find_or(stream_info, "quantity_unit", "kJ"));
    double default_seconds_per_time_unit{1.0};
    if (stream_info_rate_unit == "kW" && stream_info_quantity_unit == "kJ")
      default_seconds_per_time_unit = 1.0;
    else if (stream_info_rate_unit == "kW"
             && stream_info_quantity_unit == "kWh")
      default_seconds_per_time_unit = 3600.0;
    else
      default_seconds_per_time_unit = -1.0;
    const double stream_info_seconds_per_time_unit(
        toml::find_or(
          stream_info, "seconds_per_time_unit",
          default_seconds_per_time_unit));
    if (stream_info_seconds_per_time_unit < 0.0)
      throw new BadInputError();
    if (DEBUG) {
      std::cout << "stream_info.rate_unit = " << stream_info_rate_unit << "\n";
      std::cout << "stream_info.quantity_unit = "
                << stream_info_quantity_unit << "\n";
      std::cout << "stream_info.seconds_per_time_unit = "
                << stream_info_seconds_per_time_unit << "\n";
    }
    // [streams]
    const auto toml_streams = toml::find<toml::table>(data, "streams");
    const auto num_streams = toml_streams.size();
    if (DEBUG)
      std::cout << num_streams << " streams found\n";
    std::map<std::string, ::DISCO::StreamType> stream_types_map;
    for (const auto& s: toml_streams) {
      toml::value t = s.second;
      toml::table tt = toml::get<toml::table>(t);
      auto other_rate_units = std::unordered_map<std::string,FlowValueType>();
      auto other_quantity_units = std::unordered_map<std::string,FlowValueType>();
      auto it1 = tt.find("other_rate_units");
      if (it1 != tt.end()) {
        const auto oru = toml::get<toml::table>(it1->second);
        for (const auto& p: oru)
          other_rate_units.insert(std::pair<std::string,FlowValueType>(
                p.first,
                toml::get<FlowValueType>(p.second)));
      }
      auto it2  = tt.find("other_quantity_units");
      if (it2 != tt.end()) {
        const auto oqu = toml::get<toml::table>(it2->second);
        for (const auto& p: oqu)
          other_quantity_units.insert(std::pair<std::string,FlowValueType>(
                p.first,
                toml::get<FlowValueType>(p.second)));
      }
      const std::string stream_type{toml::find<std::string>(t, "type")};
      stream_types_map.insert(std::make_pair(
            s.first,
            ::DISCO::StreamType(
              stream_type,
              stream_info_rate_unit,
              stream_info_quantity_unit,
              stream_info_seconds_per_time_unit,
              other_rate_units,
              other_quantity_units)));
    }
    if (DEBUG)
      for (const auto& x: stream_types_map)
        std::cout << "stream type: " << x.first << ", " << x.second << "\n";
    // [components]
    const auto toml_comps = toml::find<toml::table>(data, "components");
    const auto num_comps = toml_comps.size();
    if (DEBUG)
      std::cout << num_comps << " components found\n";
    std::map<std::string, std::vector<::DISCO::FlowElement>> components{};
    for (const auto& c: toml_comps) {
      toml::value t = c.second;
      toml::table tt = toml::get<toml::table>(t);
      // stream OR input_stream, output_stream
      std::string input_stream_id{""};
      std::string output_stream_id{""};
      auto it = tt.find("stream");
      if (it != tt.end()) {
        input_stream_id = toml::get<std::string>(it->second);
        output_stream_id = input_stream_id;
      } else {
        it = tt.find("input_stream");
        if (it != tt.end())
          input_stream_id = toml::get<std::string>(it->second);
        it = tt.find("output_stream");
        if (it != tt.end())
          output_stream_id = toml::get<std::string>(it->second);
      }
      if (DEBUG) {
        std::cout << "comp: " << c.first
                  << ".input_stream_id  = " << input_stream_id << "\n";
        std::cout << "comp: " << c.first
                  << ".output_stream_id = " << output_stream_id << "\n";
      }
      // load_priority ... ignore for now
      // load_profiles_by_scenario :: toml::table
      // ... need to refactor FlowSink objects to change load profiles by the active scenario
      it = tt.find("load_profiles_by_scenario");
      if (it != tt.end()) {
        if (DEBUG)
          std::cout << "load_profiles_by_scenario found\n";
      }
    }
    // [networks]
    std::map<std::string, std::vector<::DISCO::FlowElement>> networks{};
    // [scenarios]
    std::map<std::string, std::vector<::DISCO::FlowElement>> scenarios{};
    return true;
  }

  ////////////////////////////////////////////////////////////
  // Utility Functions
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

  std::string
  map_to_string(const std::unordered_map<std::string,FlowValueType>& m)
  {
    auto max_idx{m.size() - 1};
    std::ostringstream oss;
    oss << "{";
    int idx{0};
    for (const auto& p: m) {
      oss << "{" << p.first << ", " << p.second << "}";
      if (idx != max_idx)
        oss << ", ";
      ++idx;
    }
    oss << "}";
    return oss.str();
  }

  //////////////////////////////////////////////////////////// 
  // FlowState
  FlowState::FlowState(FlowValueType in, FlowValueType out):
    FlowState(in, out, 0.0, std::fabs(in - out))
  {
  }

  FlowState::FlowState(FlowValueType in, FlowValueType out, FlowValueType store):
    FlowState(in, out, store, std::fabs(in - (out + store)))
  {
  }

  FlowState::FlowState(
      FlowValueType in,
      FlowValueType out,
      FlowValueType store,
      FlowValueType loss):
    inflow{in},
    outflow{out},
    storeflow{store},
    lossflow{loss}
  {
    checkInvariants();
  }

  void
  FlowState::checkInvariants() const
  {
    auto diff{inflow - (outflow + storeflow + lossflow)};
    if (std::fabs(diff) > TOL) {
      if (DEBUG) {
        std::cout << "FlowState.inflow   : " << inflow << "\n";
        std::cout << "FlowState.outflow  : " << outflow << "\n";
        std::cout << "FlowState.storeflow: " << storeflow << "\n";
        std::cout << "FlowState.lossflow : " << lossflow << "\n";
        std::cout << "FlowState ERROR! " << inflow << " != " << outflow << " + "
          << storeflow << " + " << lossflow << "\n";
      }
      throw FlowInvariantError();
    }
  }

  ////////////////////////////////////////////////////////////
  // StreamType
  StreamType::StreamType(const std::string& stream_type):
    StreamType(stream_type, "kW", "kJ", 1.0)
  {
  }

  StreamType::StreamType(
      const std::string& stream_type,
      const std::string& rate_units,
      const std::string& quantity_units,
      FlowValueType seconds_per_time_unit):
    StreamType(
        stream_type,
        rate_units,
        quantity_units,
        seconds_per_time_unit,
        std::unordered_map<std::string,FlowValueType>{},
        std::unordered_map<std::string,FlowValueType>{})
  {
  }

  StreamType::StreamType(
      const std::string& stream_type,
      const std::string& r_units,
      const std::string& q_units,
      FlowValueType s_per_time_unit,
      const std::unordered_map<std::string, FlowValueType>& other_r_units,
      const std::unordered_map<std::string, FlowValueType>& other_q_units
      ):
    type{stream_type},
    rate_units{r_units},
    quantity_units{q_units},
    seconds_per_time_unit{s_per_time_unit},
    other_rate_units{other_r_units},
    other_quantity_units{other_q_units}
  {
  }

  bool
  StreamType::operator==(const StreamType& other) const
  {
    if (this == &other) return true;
    return (type != other.type)   ? false :
           (rate_units != other.rate_units) ? false :
           (quantity_units != other.quantity_units) ? false :
           (seconds_per_time_unit != other.seconds_per_time_unit) ? false :
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
              << "\", rate_units=\"" << st.get_rate_units()
              << "\", quantity_units=\"" << st.get_quantity_units()
              << "\", seconds_per_time_unit=" << st.get_seconds_per_time_unit()
              << ", other_rate_units="
              << map_to_string(st.get_other_rate_units())
              << ", other_quantity_units="
              << map_to_string(st.get_other_quantity_units())
              << ")";
  }

  ///////////////////////////////////////////////////////////////////
  // Stream
  Stream::Stream(StreamType s_type, FlowValueType r):
    type{s_type},
    rate{r}
  {
  }

  std::ostream&
  operator<<(std::ostream& os, const Stream& s)
  {
    return os << "Stream(stream_type=" << s.get_type()
              << ", rate=" << s.get_rate() << ")";
  }

  ////////////////////////////////////////////////////////////
  // FlowElement
  const int FlowElement::inport_inflow_achieved = 0;
  const int FlowElement::inport_outflow_request = 1;
  const int FlowElement::outport_inflow_request = 2;
  const int FlowElement::outport_outflow_achieved = 3;

  FlowElement::FlowElement(std::string id_, StreamType st) :
    FlowElement(id_, st, st)
  {
  }

  FlowElement::FlowElement(
      std::string id_,
      StreamType in,
      StreamType out):
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
    if (inflow_type.get_rate_units() != outflow_type.get_rate_units())
      throw InconsistentStreamUnitsError();
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
          inflow_achieved += x.value.get_rate();
          break;
        case inport_outflow_request:
          if (DEBUG)
            std::cout << "... <=inport_outflow_request\n";
          if (x.value.get_type() != outflow_type)
            throw MixedStreamsError();
          outflow_provided = true;
          outflow_request += x.value.get_rate();
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
      const FlowState& fs = update_state_for_inflow_achieved(inflow_achieved);
      update_state(fs);
    }
    else if (outflow_provided && !inflow_provided) {
      report_inflow_request = true;
      const FlowState fs = update_state_for_outflow_request(outflow_request);
      if (std::fabs(fs.getOutflow() - outflow_request) > TOL)
        report_outflow_achieved = true;
      update_state(fs);
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

  const FlowState
  FlowElement::update_state_for_outflow_request(FlowValueType outflow_) const
  {
    if (DEBUG)
      std::cout << "FlowElement::update_state_for_outflow_request();id=" << id << "\n";
    return FlowState{outflow_, outflow_};
  }

  const FlowState
  FlowElement::update_state_for_inflow_achieved(FlowValueType inflow_) const
  {
    if (DEBUG)
      std::cout << "FlowElement::update_state_for_inflow_achieved();id=" << id << "\n";
    return FlowState{inflow_, inflow_};
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
    auto diff{inflow - (outflow + storeflow + lossflow)};
    if (std::fabs(diff) > TOL) {
      std::cout << "FlowElement ERROR! " << inflow << " != " << outflow << " + "
        << storeflow << " + " << lossflow << "!\n";
      throw FlowInvariantError();
    }
  }

  ///////////////////////////////////////////////////////////////////
  // FlowLimits
  FlowLimits::FlowLimits(
      std::string id,
      StreamType stream_type,
      FlowValueType low_lim,
      FlowValueType up_lim) :
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

  const FlowState
  FlowLimits::update_state_for_outflow_request(FlowValueType out) const
  {
    if (DEBUG) {
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
    else
      out_ = out;
    if (DEBUG) {
      print_state("... ");
      std::cout << "end FlowLimits::update_state_for_outflow_request\n";
    }
    return FlowState{out_, out_};
  }

  const FlowState
  FlowLimits::update_state_for_inflow_achieved(FlowValueType in) const
  {
    if (DEBUG) {
      std::cout << "FlowLimits::update_state_for_inflow_achieved(" << in << ")\n";
      print_state("... ");
    }
    FlowValueType in_{0.0};
    if (in > upper_limit)
      throw AchievedMoreThanRequestedError();
    else if (in < lower_limit)
      throw AchievedMoreThanRequestedError();
    else
      in_ = in;
    if (DEBUG) {
      print_state("... ");
      std::cout << "end FlowLimits::update_state_for_inflow_achieved\n";
    }
    return FlowState{in_, in_};
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
  Transformer::Transformer(
      std::string id,
      StreamType input_stream_type,
      StreamType output_stream_type,
      std::function<FlowValueType(FlowValueType)> calc_output_from_input,
      std::function<FlowValueType(FlowValueType)> calc_input_from_output
      ) :
    FlowElement(id, input_stream_type, output_stream_type),
    output_from_input{calc_output_from_input},
    input_from_output{calc_input_from_output}
  {
  }

  const FlowState
  Transformer::update_state_for_outflow_request(FlowValueType outflow_) const
  {
    return FlowState{input_from_output(outflow_), outflow_};
  }

  const FlowState
  Transformer::update_state_for_inflow_achieved(FlowValueType inflow_) const
  {
    return FlowState{inflow_, output_from_input(inflow_)};
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
