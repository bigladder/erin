/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */
#include "disco/disco.h"
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
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
    std::unordered_map<std::string, ::DISCO::StreamType> stream_types_map;
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
    std::unordered_map<
      std::string,
      std::shared_ptr<Component>> components{};
    for (const auto& c: toml_comps) {
      toml::value t = c.second;
      toml::table tt = toml::get<toml::table>(t);
      auto it = tt.find("type");
      std::string component_type;
      if (it != tt.end())
        component_type = toml::get<std::string>(it->second);
      // stream OR input_stream, output_stream
      std::string input_stream_id{""};
      std::string output_stream_id{""};
      it = tt.find("stream");
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
      if (component_type == "source") {
        std::shared_ptr<Component> source_comp =
          std::make_shared<SourceComponent>(
              c.first, stream_types_map.at(output_stream_id));
        components.insert(std::make_pair(c.first, source_comp));
      } else if (component_type == "load") {
        std::unordered_map<std::string,std::vector<LoadItem>> loads_by_scenario{};
        it = tt.find("load_profiles_by_scenario");
        if (it != tt.end()) {
          const auto& loads = toml::get<toml::table>(it->second);
          const auto num_profile_scenarios{loads.size()};
          if (DEBUG)
            std::cout << num_profile_scenarios << " load profile(s) by scenario"
              << " for component " << c.first << "\n";
          for (const auto& lp: loads) {
            std::vector<LoadItem> loads2{};
            for (const auto& li:
                toml::get<std::vector<toml::table>>(lp.second)) {
              RealTimeType t{};
              FlowValueType v{};
              auto it_for_t = li.find("t");
              if (it_for_t != li.end())
                t = toml::get<RealTimeType>(it_for_t->second);
              else
                t = -1;
              auto it_for_v = li.find("v");
              if (it_for_v != li.end()) {
                v = toml::get<FlowValueType>(it_for_v->second);
                loads2.push_back(LoadItem(t, v));
              } else {
                loads2.push_back(LoadItem(t));
              }
            }
            loads_by_scenario.insert(std::make_pair(lp.first, loads2));
          }
          if (DEBUG) {
            std::cout << loads_by_scenario.size() << " scenarios with loads\n";
            for (const auto& ls: loads_by_scenario) {
              std::cout << ls.first << ": [";
              for (const auto& li: ls.second) {
                std::cout << "(" << li.get_time();
                if (li.get_is_end())
                  std::cout << ")";
                else
                  std::cout << ", " << li.get_value() << "), ";
              }
              std::cout << "]\n";
            }
          }
          std::shared_ptr<Component> load_comp = std::make_shared<LoadComponent>(
              c.first,
              stream_types_map.at(input_stream_id),
              loads_by_scenario);
          components.insert(
              std::make_pair(c.first, load_comp));
        } else {
          throw BadInputError();
        }
      }
    }
    if (DEBUG)
      for (const auto& c: components) {
        std::cout << "comp[" << c.first << "]:\n";
        std::cout << "\t" << c.second->get_id() << "\n";
      }
    // [networks]
    std::unordered_map<
      std::string,
      std::unordered_map<std::string,std::vector<std::string>>> networks;
    const auto toml_nets = toml::find<toml::table>(data, "networks");
    const auto num_nets{toml_nets.size()};
    if (DEBUG)
      std::cout << num_nets << " networks found\n";
    for (const auto& n: toml_nets) {
      std::unordered_map<std::string, std::vector<std::string>> nw_map;
      toml::table toml_nw;
      auto nested_nw_table = toml::get<toml::table>(n.second);
      auto nested_nw_it = nested_nw_table.find("network");
      if (nested_nw_it != nested_nw_table.end())
        toml_nw = toml::get<toml::table>(nested_nw_it->second);
      for (const auto& nw_p: toml_nw) {
        auto nodes = toml::get<std::vector<std::string>>(nw_p.second);
        nw_map.insert(std::make_pair(nw_p.first, nodes));
      }
      networks.insert(std::make_pair(n.first, nw_map));
    }
    if (DEBUG)
      for (const auto& nw: networks) {
        std::cout << "network[" << nw.first << "]:\n";
        for (const auto& fan: nw.second) {
          for (const auto& node: fan.second) {
            std::cout << "\tedge: (" << fan.first << " ==> " << node << ")\n";
          }
        }
      }
    // [scenarios]
    std::unordered_map<std::string, std::shared_ptr<Scenario>> scenarios;
    const auto toml_scenarios = toml::find<toml::table>(data, "scenarios");
    const auto num_scenarios{toml_scenarios.size()};
    if (DEBUG)
      std::cout << num_scenarios << " scenarios found\n";
    for (const auto& s: toml_scenarios) {
      const auto occurrence_distribution = toml::find<toml::table>(s.second, "occurrence_distribution");
      const auto duration_distribution = toml::find<toml::table>(s.second, "duration_distribution");
      const auto max_times = toml::find<int>(s.second, "max_times");
      const auto network_id = toml::find<std::string>(s.second, "network");
      scenarios.insert(
          std::make_pair(
            s.first,
            std::make_shared<Scenario>(
              s.first,
              network_id,
              max_times)));
    }
    if (DEBUG)
      for (const auto& s: scenarios) {
        std::cout << "scenario[" << s.first << "]\n";
        auto scenario = *s.second;
        std::cout << "\tname      : " << scenario.get_name() << "\n";
        std::cout << "\tnetwork_id: " << scenario.get_network_id() << "\n";
        std::cout << "\tmax_times : " << scenario.get_max_times() << "\n";
      }
    // 1. Initial assumption: blue-sky scenario. Need to actually make a
    //    separate devs simulator to accommodate switching between scenarios
    // 2. Construct and Run Simulation
    adevs::Digraph<Stream> network;
    // 3. Instantiate the network and add the components
    //network.couple(
    //  sink, ::DISCO::Sink::outport_inflow_request,
    //  genset_meter, ::DISCO::FlowMeter::inport_outflow_request);
    adevs::Simulator<PortValue> sim;
    network.add(&sim);
    adevs::Time t;
    while (sim.next_event_time() < adevs_inf<adevs::Time>()) {
      sim.exec_next_event();
      t = sim.now();
      std::cout << "The current time is: (" << t.real << ", "
                << t.logical << ")\n";
    }
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
  // LoadItem
  LoadItem::LoadItem(RealTimeType t):
    time{t},
    value{-1},
    is_end{true}
  {
    if (!is_good()) throw BadInputError();
  }

  LoadItem::LoadItem(RealTimeType t, FlowValueType v):
    time{t},
    value{v},
    is_end{false}
  {
    if (!is_good()) throw BadInputError();
  }

  RealTimeType
  LoadItem::get_time_advance(const LoadItem& next) const
  {
    return (next.get_time() - time);
  }

  //////////////////////////////////////////////////////////// 
  // FlowState
  FlowState::FlowState(FlowValueType in):
    FlowState(in, in, 0.0, 0.0)
  {
  }

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
  StreamType::StreamType():
    StreamType("electricity") {}

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
  // Component
  Component::Component(
      const std::string& id_,
      const std::string& type_,
      const StreamType& input_stream_,
      const StreamType& output_stream_):
    id{id_},
    component_type{type_},
    input_stream{input_stream_},
    output_stream{output_stream_}
  {
  }

  void
  Component::add_input(std::shared_ptr<Component>& c)
  {
    inputs.push_back(c);
  }

  ////////////////////////////////////////////////////////////
  // LoadComponent
  LoadComponent::LoadComponent(
      const std::string& id_,
      const StreamType& input_stream_,
      const std::unordered_map<std::string,std::vector<LoadItem>>&
        loads_by_scenario_):
    Component(id_, "load", input_stream_, input_stream_),
    loads_by_scenario{loads_by_scenario_}
  {
  }

  void
  LoadComponent::add_to_network(adevs::Digraph<Stream>& nw)
  {
    // 
  }

  ////////////////////////////////////////////////////////////
  // SourceComponent
  SourceComponent::SourceComponent(
      const std::string& id_,
      const StreamType& output_stream_):
    Component(id_, "source", output_stream_, output_stream_)
  {
  }

  void
  SourceComponent::add_to_network(adevs::Digraph<Stream>& nw)
  {
    //
  }

  ////////////////////////////////////////////////////////////
  // Scenario
  Scenario::Scenario(
      const std::string& name_,
      const std::string& network_id_,
      long max_times_):
    name{name_},
    network_id{network_id_},
    max_times{max_times_}
  {
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
  FlowElement::calculate_time_advance()
  {
    return adevs_inf<adevs::Time>();
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
    return calculate_time_advance();
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
    add_additional_outputs(ys);
  }

  void
  FlowElement::add_additional_outputs(std::vector<PortValue>& ys)
  {
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
  Sink::Sink(
      std::string id,
      StreamType st,
      std::unordered_map<std::string, std::vector<LoadItem>> loads):
    Sink(id, st, loads, "")
  {
  }

  Sink::Sink(
      std::string id,
      StreamType st,
      std::unordered_map<std::string, std::vector<LoadItem>> loads,
      std::string scenario):
    FlowElement(id, st, st),
    loads_by_scenario{loads},
    active_scenario{scenario},
    idx{-1},
    num_loads{0},
    loads{}
  {
    check_loads_by_scenario();
    switch_scenario(active_scenario);
  }

  void
  Sink::update_on_internal_transition()
  {
    if (DEBUG)
      std::cout << "Sink::update_on_internal_transition()\n";
    // if there is no active scenario, return
    if (active_scenario == "")
      return;
    ++idx;
  }

  adevs::Time
  Sink::calculate_time_advance()
  {
    if (DEBUG)
      std::cout << "Sink::calculate_time_advance()\n";
    if (idx < 0) {
      if (DEBUG)
        std::cout << "... dt = infinity\n";
      return adevs::Time{0, 0};
    }
    int next_idx = idx + 1;
    if (next_idx < num_loads) {
      RealTimeType dt{loads[idx].get_time_advance(loads[next_idx])};
      if (DEBUG)
        std::cout << "... dt = (" << dt << ", 0)\n";
      return adevs::Time{dt, 0};
    }
    if (DEBUG)
      std::cout << "... dt = infinity\n";
    return adevs_inf<adevs::Time>();
  }

  const FlowState
  Sink::update_state_for_inflow_achieved(FlowValueType inflow_) const
  {
    return FlowState{inflow_};
  }

  void
  Sink::add_additional_outputs(std::vector<PortValue>& ys)
  {
    if (DEBUG)
      std::cout << "Sink::output_func()\n";
    if (active_scenario != "") {
      auto next_idx{idx + 1};
      auto max_idx{num_loads - 1};
      if (next_idx < max_idx)
        ys.push_back(
            adevs::port_value<Stream>{
            outport_inflow_request,
            Stream{get_inflow_type(), loads[next_idx].get_value()}});
    }
  }

  void
  Sink::check_loads(
      const std::string& scenario,
      const std::vector<LoadItem>& loads) const
  {
    auto N{loads.size()};
    auto last_idx{N - 1};
    if (N < 2) {
      std::ostringstream oss;
      oss << "Sink: must have at least two LoadItems per scenario, "
          << "but scenario \"" << scenario << "\", only has " << N;
      throw std::invalid_argument(oss.str());
    }
    RealTimeType t{-1};
    for (int idx{0}; idx < loads.size(); ++idx) {
      const auto& x{loads.at(idx)};
      auto t_{x.get_time()};
      if (idx == last_idx) {
        if (!x.get_is_end()) {
          std::ostringstream oss;
          oss << "Sink: LoadItem[" << idx << "] for scenario \"" << scenario
              << "\" must not specify a value but it does...";
          throw std::invalid_argument(oss.str());
        }
      } else {
        if (x.get_is_end()) {
          std::ostringstream oss;
          oss << "Sink: non-last LoadItem[" << idx << "] for scenario \""
              << scenario << "\" doesn't specify a value...";
          throw std::invalid_argument(oss.str());
        }
      }
      if ((t_ < 0) || (t_ <= t)) {
        std::ostringstream oss;
        oss << "Sink: LoadItems for scenario \"" << scenario
            << "\" must have time points that are everywhere increasing "
            << " and positive";
        throw std::invalid_argument(oss.str());
      }
    }
  }

  void
  Sink::check_loads_by_scenario() const
  {
    for (const auto& sl: loads_by_scenario)
      check_loads(sl.first, sl.second);
  }

  bool
  Sink::switch_scenario(const std::string& scenario)
  {
    for (const auto& sl: loads_by_scenario) {
      if (sl.first == scenario) {
        idx = -1;
        active_scenario = scenario;
        num_loads = sl.second.size();
        loads = sl.second;
        return true;
      }
    }
    return false;
  }
}
