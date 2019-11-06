/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */
#include <algorithm>
#include <cmath>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include "../vendor/toml11/toml.hpp"
#include "debug_utils.h"
#include "erin/erin.h"

namespace ERIN
{
  const FlowValueType TOL{1e-6};

  ////////////////////////////////////////////////////////////
  // StreamInfo
  StreamInfo::StreamInfo():
    StreamInfo(std::string{"kW"}, std::string{"kJ"}, 1.0)
  {
  }

  StreamInfo::StreamInfo(
      std::string rate_unit_,
      std::string  quantity_unit_):
    rate_unit{std::move(rate_unit_)},
    quantity_unit{std::move(quantity_unit_)},
    seconds_per_time_unit{1.0}
  {
    if ((rate_unit == "kW") && (quantity_unit == "kJ"))
      seconds_per_time_unit = 1.0;
    else if ((rate_unit == "kW") && (quantity_unit == "kWh"))
      seconds_per_time_unit = 3600.0;
    else
      throw BadInputError();
  }
  StreamInfo::StreamInfo(
      std::string rate_unit_,
      std::string quantity_unit_,
      double seconds_per_time_unit_):
    rate_unit{std::move(rate_unit_)},
    quantity_unit{std::move(quantity_unit_)},
    seconds_per_time_unit{seconds_per_time_unit_}
  {
  }

  bool
  StreamInfo::operator==(const StreamInfo& other) const
  {
    if (this == &other) return true;
    return (rate_unit == other.rate_unit) &&
           (quantity_unit == other.quantity_unit) &&
           (seconds_per_time_unit == other.seconds_per_time_unit);
  }

  ////////////////////////////////////////////////////////////
  // TomlInputReader
  TomlInputReader::TomlInputReader(toml::value data_):
    InputReader(),
    data{std::move(data_)}
  {
  }

  TomlInputReader::TomlInputReader(const std::string& path):
    InputReader(),
    data{}
  {
    data = toml::parse(path);
  }

  TomlInputReader::TomlInputReader(std::istream& in):
    InputReader(),
    data{}
  {
    data = toml::parse(in, "input_from_string.toml");
  }

  StreamInfo
  TomlInputReader::read_stream_info()
  {
    const auto stream_info = toml::find(data, "stream_info");
    const std::string rate_unit(
        toml::find_or(stream_info, "rate_unit", "kW"));
    const std::string quantity_unit(
        toml::find_or(stream_info, "quantity_unit", "kJ"));
    double default_seconds_per_time_unit{1.0};
    if (rate_unit == "kW" && quantity_unit == "kJ")
      default_seconds_per_time_unit = 1.0;
    else if (rate_unit == "kW" && quantity_unit == "kWh")
      default_seconds_per_time_unit = 3600.0;
    else
      default_seconds_per_time_unit = -1.0;
    const double seconds_per_time_unit(
        toml::find_or(
          stream_info, "seconds_per_time_unit",
          default_seconds_per_time_unit));
    if (seconds_per_time_unit < 0.0)
      throw BadInputError();
    StreamInfo si{rate_unit, quantity_unit, seconds_per_time_unit};
    DB_PUTS2("stream_info.rate_unit = ", si.get_rate_unit());
    DB_PUTS2("stream_info.quantity_unit = ", si.get_quantity_unit());
    DB_PUTS2("stream_info.seconds_per_time_unit = ",
        si.get_seconds_per_time_unit());
    return si;
  }

  std::unordered_map<std::string, StreamType>
  TomlInputReader::read_streams(const StreamInfo& si)
  {
    const auto toml_streams = toml::find<toml::table>(data, "streams");
    std::unordered_map<std::string, StreamType> stream_types_map;
    for (const auto& s: toml_streams) {
      toml::value t = s.second;
      toml::table tt = toml::get<toml::table>(t);
      auto other_rate_units = std::unordered_map<std::string,FlowValueType>();
      auto other_quantity_units =
        std::unordered_map<std::string,FlowValueType>();
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
            StreamType(
              stream_type,
              si.get_rate_unit(),
              si.get_quantity_unit(),
              si.get_seconds_per_time_unit(),
              other_rate_units,
              other_quantity_units)));
    }
#ifdef DEBUG_PRINT
      for (const auto& x: stream_types_map)
        std::cerr << "stream type: " << x.first << ", " << x.second << "\n";
#endif
    return stream_types_map;
  }

  std::unordered_map<std::string, std::vector<LoadItem>>
  TomlInputReader::read_loads()
  {
    std::unordered_map<std::string, std::vector<LoadItem>> loads_by_id;
    const auto toml_loads = toml::find<toml::table>(data, "loads");
    DB_PUTS2(toml_loads.size(), " load entries found");
    for (const auto& load_item: toml_loads) {
      toml::value tv = load_item.second;
      toml::table tt = toml::get<toml::table>(tv);
      auto it = tt.find("loads");
      if (it == tt.end())
        throw BadInputError();
      const auto load_array = toml::get<std::vector<toml::table>>(it->second);
      std::vector<LoadItem> the_loads{};
      for (const auto& item: load_array) {
        RealTimeType the_time{};
        FlowValueType the_value{};
        auto it_for_t = item.find("t");
        if (it_for_t != item.end())
          the_time = toml::get<RealTimeType>(it_for_t->second);
        else
          the_time = -1;
        auto it_for_v = item.find("v");
        if (it_for_v != item.end()) {
          the_value = toml::get<FlowValueType>(it_for_v->second);
          the_loads.emplace_back(LoadItem(the_time, the_value));
        } else {
          the_loads.emplace_back(LoadItem(the_time));
        }
      }
      loads_by_id.insert(std::make_pair(load_item.first, the_loads));
    }
    return loads_by_id;
  }

  std::unordered_map<std::string, std::shared_ptr<Component>>
  TomlInputReader::read_components(
      const std::unordered_map<std::string, StreamType>& stream_types_map,
      const std::unordered_map<std::string, std::vector<LoadItem>>& loads_by_id)
  {
    const auto toml_comps = toml::find<toml::table>(data, "components");
    DB_PUTS2(toml_comps.size(), " components found");
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
      std::string input_stream_id;
      std::string output_stream_id;
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
      DB_PUTS4("comp: ", c.first, ".input_stream_id  = ", input_stream_id);
      DB_PUTS4("comp: ", c.first, ".output_stream_id = ", output_stream_id);
      if (component_type == "source") {
        std::shared_ptr<Component> source_comp =
          std::make_shared<SourceComponent>(
              c.first, stream_types_map.at(output_stream_id));
        components.insert(std::make_pair(c.first, source_comp));
      } else if (component_type == "load") {
        std::unordered_map<std::string,std::vector<LoadItem>>
          loads_by_scenario{};
        it = tt.find("load_profiles_by_scenario");
        if (it != tt.end()) {
          const auto& loads = toml::get<toml::table>(it->second);
          DB_PUTS4(loads.size(), " load profile(s) by scenario",
              " for component ", c.first);
          for (const auto& lp: loads) {
            const std::string load_id{toml::get<toml::string>(lp.second)}; 
            auto the_loads_it = loads_by_id.find(load_id);
            if (the_loads_it != loads_by_id.end())
              loads_by_scenario.insert(
                  std::make_pair(lp.first, the_loads_it->second));
            else
              throw BadInputError();
          }
          DB_PUTS2(loads_by_scenario.size(), " scenarios with loads");
#ifdef DEBUG_PRINT
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
#endif
          std::shared_ptr<Component> load_comp =
            std::make_shared<LoadComponent>(
                c.first,
                stream_types_map.at(input_stream_id),
                loads_by_scenario);
          components.insert(
              std::make_pair(c.first, load_comp));
        }
        else
          throw BadInputError();
      }
    }
#ifdef DEBUG_PRINT
      for (const auto& c: components) {
        std::cout << "comp[" << c.first << "]:\n";
        std::cout << "\t" << c.second->get_id() << "\n";
      }
#endif
    return components;
  }

  std::unordered_map<std::string,
    std::unordered_map<std::string, std::vector<std::string>>>
  TomlInputReader::read_networks()
  {
    std::unordered_map<
      std::string,
      std::unordered_map<std::string,std::vector<std::string>>> networks;
    const auto toml_nets = toml::find<toml::table>(data, "networks");
    DB_PUTS2(toml_nets.size(), " networks found");
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
#ifdef DEBUG_PRINT    
      for (const auto& nw: networks) {
        std::cout << "network[" << nw.first << "]:\n";
        for (const auto& fan: nw.second) {
          for (const auto& node: fan.second) {
            std::cout << "\tedge: (" << fan.first << " ==> " << node << ")\n";
          }
        }
      }
#endif
    return networks;
  }

  std::unordered_map<std::string, std::shared_ptr<Scenario>>
  TomlInputReader::read_scenarios()
  {
    std::unordered_map<std::string, std::shared_ptr<Scenario>> scenarios;
    const auto toml_scenarios = toml::find<toml::table>(data, "scenarios");
    DB_PUTS2(toml_scenarios.size(), " scenarios found");
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
#ifdef DEBUG_PRINT
      for (const auto& s: scenarios) {
        std::cout << "scenario[" << s.first << "]\n";
        auto scenario = *s.second;
        std::cout << "\tname      : " << scenario.get_name() << "\n";
        std::cout << "\tnetwork_id: " << scenario.get_network_id() << "\n";
        std::cout << "\tmax_times : " << scenario.get_max_times() << "\n";
      }
#endif
    return scenarios;
  }

  ////////////////////////////////////////////////////////////
  // ScenarioResults
  ScenarioResults::ScenarioResults():
    is_good{false},
    results{}
  {
  }

  ScenarioResults::ScenarioResults(
      bool is_good_,
      std::unordered_map<std::string, std::vector<Datum>> results_):
    is_good{is_good_},
    results{std::move(results_)}
  {
  }

  std::string
  ScenarioResults::to_csv(const RealTimeType& max_time) const
  {
    if (!is_good) return std::string{};
    std::set<RealTimeType> times_set{max_time};
    std::unordered_map<std::string, std::vector<FlowValueType>> values;
    std::unordered_map<std::string, FlowValueType> last_values;
    std::vector<std::string> keys;
    for (const auto p: results) {
      keys.emplace_back(p.first);
      values[p.first] = std::vector<FlowValueType>();
      last_values[p.first] = 0.0;
      for (const auto d: p.second)
        times_set.emplace(d.time);
    }
    std::vector<RealTimeType> times{times_set.begin(), times_set.end()};
    std::sort(keys.begin(), keys.end());
    for (const auto p: results) {
      for (const auto t: times) {
        auto k{p.first};
        if (t == max_time)
          values[k].emplace_back(0.0);
        else {
          bool found{false};
          FlowValueType last = last_values[k];
          for (const auto d: p.second) {
            if (d.time == t) {
              found = true;
              values[k].emplace_back(d.value);
              last_values[k] = d.value;
              break;
            }
            if (d.time > t)
              break;
          }
          if (!found)
            values[k].emplace_back(last);
        }
      }
    }
    std::ostringstream oss;
    oss << "time";
    for (const auto k: keys)
      oss << "," << k;
    oss << "\n";
    for (std::vector<RealTimeType>::size_type i{0}; i < times.size(); ++i) {
      oss << times[i];
      for (const auto k: keys)
        oss << "," << values[k][i];
      oss << "\n";
    }
    return oss.str();
  }

  //////////////////////////////////////////////////////////// 
  // Main
  // main class that runs the simulation from file
  Main::Main(const std::string& input_file_path)
  {
    auto reader = TomlInputReader{input_file_path};
    // Read data into private class fields
    stream_info = reader.read_stream_info();
    stream_types_map = reader.read_streams(stream_info);
    auto loads_by_id = reader.read_loads();
    components = reader.read_components(stream_types_map, loads_by_id);
    networks = reader.read_networks();
    scenarios = reader.read_scenarios();
    check_data();
  }

  Main::Main(
      StreamInfo stream_info_,
      std::unordered_map<std::string, StreamType> streams_,
      std::unordered_map<std::string, std::shared_ptr<Component>> components_,
      std::unordered_map<
        std::string,
        std::unordered_map<
          std::string, std::vector<std::string>>> networks_,
      std::unordered_map<std::string, std::shared_ptr<Scenario>> scenarios_):
    stream_info{std::move(stream_info_)},
    stream_types_map{std::move(streams_)},
    components{std::move(components_)},
    networks{std::move(networks_)},
    scenarios{std::move(scenarios_)}
  {
    check_data();
  }

  ScenarioResults
  Main::run(const std::string& scenario_id)
  {
    // TODO: check input structure to ensure that keys are available in maps that
    //       should be there. If not, provide a good error message about what's
    //       wrong.
    // The Run Algorithm
    // 0. Check if we have a valid scenario_id
    const auto it = scenarios.find(scenario_id);
    if (it == scenarios.end()) {
      std::ostringstream oss;
      oss << "scenario_id -- \"" << scenario_id << "\"" 
             " is not in available scenarios\n"; 
      oss << "possible choices: ";
      for (const auto& item: scenarios)
        oss << "\"" << item.first << "\", ";
      oss << "\n";
      throw std::invalid_argument(oss.str());
    }
    // 1. Switch to reading the scenario_id from the input
    const auto the_scenario = scenarios[scenario_id];
    // 2. Construct and Run Simulation
    // 2.1. Instantiate a devs network
    adevs::Digraph<FlowValueType> network;
    // 2.2. Interconnect components based on the network definition
    const auto network_id = the_scenario->get_network_id();
    const auto the_nw = networks[network_id];
    std::unordered_set<std::string> comps_in_use{};
    for (const auto& nw: the_nw) {
      comps_in_use.emplace(nw.first);
      auto src = components[nw.first];
      for (const auto& sink_id: nw.second) {
        auto sink = components[sink_id];
        comps_in_use.emplace(sink_id);
        sink->add_input(src);
      }
    }
    // 2.2. Add the components on the network
    std::unordered_set<FlowElement*> elements;
    for (const auto& comp_id: comps_in_use) {
      auto c = components[comp_id];
      auto es = c->add_to_network(network, scenario_id);
      for (auto e: es) elements.emplace(e);
    }
    adevs::Simulator<PortValue> sim;
    network.add(&sim);
    adevs::Time t;
    const auto max_non_advance{comps_in_use.size() * 10};
    auto non_advance_count{max_non_advance*0};
    auto t_last_real = sim.now().real;
    bool sim_good{true};
    while (sim.next_event_time() < adevs_inf<adevs::Time>()) {
      sim.exec_next_event();
      t = sim.now();
      if (t.real == t_last_real)
        ++non_advance_count;
      else {
        non_advance_count = 0;
        t_last_real = t.real;
      }
      if (non_advance_count >= max_non_advance) {
        sim_good = false;
        break;
      }
      DB_PUTS("The current time is:");
      DB_PUTS2("... real   : ", t.real);
      DB_PUTS2("... logical: ", t.logical);
    }
    // 3. Return outputs.
    std::unordered_map<std::string, std::vector<Datum>> results;
    for (const auto& e: elements) {
      auto vals = e->get_results();
      if (!vals.empty())
        results.insert(
            std::pair<std::string,std::vector<Datum>>(e->get_id(), vals));
    }
    return ScenarioResults{sim_good, results};
  }

  RealTimeType
  Main::max_time_for_scenario(const std::string& scenario_id)
  {
    // TODO: implement after we get loads broken out
    // - get the network for the scenario_id
    // - for each component on that network, query it for its max time
    // - return the largest max time
    return 8760;
  }

  void
  Main::check_data() const
  {
    // TODO: implement this
  }

  ////////////////////////////////////////////////////////////
  // Utility Functions
  FlowValueType
  clamp_toward_0(FlowValueType value, FlowValueType lower, FlowValueType upper)
  {
    if (lower > upper) {
      std::ostringstream oss;
      oss << "ERIN::clamp_toward_0 error: lower (" << lower
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

  std::string
  map_to_string(const std::unordered_map<std::string, FlowValueType>& m)
  {
    auto max_idx{m.size() - 1};
    std::ostringstream oss;
    oss << "{";
    std::unordered_map<std::string, FlowValueType>::size_type idx{0};
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
      DB_PUTS2("FlowState.inflow   : ", inflow);
      DB_PUTS2("FlowState.outflow  : ", outflow);
      DB_PUTS2("FlowState.storeflow: ", storeflow);
      DB_PUTS2("FlowState.lossflow : ", lossflow);
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
      std::string stream_type,
      std::string  r_units,
      std::string  q_units,
      FlowValueType s_per_time_unit,
      std::unordered_map<std::string, FlowValueType>  other_r_units,
      std::unordered_map<std::string, FlowValueType>  other_q_units
      ):
    type{std::move(stream_type)},
    rate_units{std::move(r_units)},
    quantity_units{std::move(q_units)},
    seconds_per_time_unit{s_per_time_unit},
    other_rate_units{std::move(other_r_units)},
    other_quantity_units{std::move(other_q_units)}
  {
  }

  bool
  StreamType::operator==(const StreamType& other) const
  {
    if (this == &other) return true;
    return (type == other.type) &&
           (rate_units == other.rate_units) &&
           (quantity_units == other.quantity_units) &&
           (seconds_per_time_unit == other.seconds_per_time_unit);
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

  ////////////////////////////////////////////////////////////
  // Component
  Component::Component(
      std::string id_,
      std::string type_,
      StreamType input_stream_,
      StreamType output_stream_):
    id{std::move(id_)},
    component_type{std::move(type_)},
    input_stream{std::move(input_stream_)},
    output_stream{std::move(output_stream_)},
    inputs{},
    connecting_element{nullptr}
  {
  }

  void
  Component::add_input(std::shared_ptr<Component>& c)
  {
    inputs.push_back(c);
  }

  FlowElement*
  Component::get_connecting_element()
  {
    if (connecting_element == nullptr)
      connecting_element = create_connecting_element();
    return connecting_element;
  }

  void
  Component::connect_source_to_sink(
      adevs::Digraph<FlowValueType>& network,
      FlowElement* source,
      FlowElement* sink,
      bool both_way) const
  {
    auto src_out = source->get_outflow_type();
    auto sink_in = sink->get_inflow_type();
    if (src_out != sink_in)
      throw MixedStreamsError();
    network.couple(
        sink, FlowMeter::outport_inflow_request,
        source, FlowMeter::inport_outflow_request);
    if (both_way)
      network.couple(
          source, FlowMeter::outport_outflow_achieved,
          sink, FlowMeter::inport_inflow_achieved);
  }

  ////////////////////////////////////////////////////////////
  // LoadComponent
  LoadComponent::LoadComponent(
      const std::string& id_,
      const StreamType& input_stream_,
      std::unordered_map<
        std::string,std::vector<LoadItem>> loads_by_scenario_):
    Component(id_, "load", input_stream_, input_stream_),
    loads_by_scenario{std::move(loads_by_scenario_)}
  {
  }

  FlowElement*
  LoadComponent::create_connecting_element()
  {
    DB_PUTS("LoadComponent::create_connecting_element()");
    auto p = new FlowMeter(get_id(), get_input_stream());
    DB_PUTS2("LoadComponent.connecting_element = ", p);
    return p;
  }

  std::unordered_set<FlowElement*>
  LoadComponent::add_to_network(
      adevs::Digraph<FlowValueType>& network,
      const std::string& active_scenario)
  {
    std::unordered_set<FlowElement*> elements;
    DB_PUTS("LoadComponent::add_to_network(adevs::Digraph<FlowValueType>& network)");
    auto sink = new Sink(
        get_id(),
        get_input_stream(),
        loads_by_scenario[active_scenario]);
    elements.emplace(sink);
    DB_PUTS2("sink = ", sink);
    auto meter = get_connecting_element();
    elements.emplace(meter);
    DB_PUTS2("meter = ", meter);
    connect_source_to_sink(network, meter, sink, false);
    for (const auto& in: get_inputs()) {
      auto p = in->get_connecting_element();
      elements.emplace(p);
      DB_PUTS2("p = ", p);
      if (p != nullptr)
        connect_source_to_sink(network, p, meter, true);
    }
    DB_PUTS("LoadComponent::add_to_network(...) exit");
    return elements;
  }

  ////////////////////////////////////////////////////////////
  // SourceComponent
  SourceComponent::SourceComponent(
      const std::string& id_,
      const StreamType& output_stream_):
    Component(id_, "source", output_stream_, output_stream_)
  {
  }

  std::unordered_set<FlowElement*> 
  SourceComponent::add_to_network(
      adevs::Digraph<FlowValueType>&,
      const std::string&)
  {
    std::unordered_set<FlowElement*> elements;
    DB_PUTS("SourceComponent::add_to_network(adevs::Digraph<FlowValueType>& network)");
    // do nothing in this case. There is only the connecting element. If nobody
    // connects to it, then it doesn't exist. If someone DOES connect, then the
    // pointer eventually gets passed into the network coupling model
    // TODO: what we need are wrappers for Element classes that track if
    // they've been added to the simulation or not. If NOT, then we need to
    // delete the resource at deletion time... otherwise, the simulator will
    // delete them.
    DB_PUTS("SourceComponent::add_to_network(...) exit");
    return elements;
  }

  FlowElement*
  SourceComponent::create_connecting_element()
  {
    DB_PUTS("SourceComponent::create_connecting_element()");
    auto p = new FlowMeter(get_id(), get_output_stream());
    DB_PUTS2("SourceComponent.p = ", p);
    return p;
  }

  ////////////////////////////////////////////////////////////
  // Scenario
  Scenario::Scenario(
      std::string name_,
      std::string network_id_,
      long max_times_):
    name{std::move(name_)},
    network_id{std::move(network_id_)},
    max_times{max_times_}
  {
  }

  bool
  Scenario::operator==(const Scenario& other) const
  {
    if (this == &other) return true;
    return (name == other.get_name()) &&
           (network_id == other.get_network_id()) &&
           (max_times == other.get_max_times());
  }

  ////////////////////////////////////////////////////////////
  // FlowElement
  const int FlowElement::inport_inflow_achieved = 0;
  const int FlowElement::inport_outflow_request = 1;
  const int FlowElement::outport_inflow_request = 2;
  const int FlowElement::outport_outflow_achieved = 3;

  FlowElement::FlowElement(std::string id_, StreamType st) :
    FlowElement(std::move(id_), st, st)
  {
  }

  FlowElement::FlowElement(
      std::string id_,
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
    report_outflow_achieved{false}
  {
    if (inflow_type.get_rate_units() != outflow_type.get_rate_units())
      throw InconsistentStreamUnitsError();
  }

  void
  FlowElement::delta_int()
  {
    DB_PUTS2("FlowElement::delta_int();id=", id);
    update_on_internal_transition();
    report_inflow_request = false;
    report_outflow_achieved = false;
  }

  void
  FlowElement::delta_ext(adevs::Time e, std::vector<PortValue>& xs)
  {
    DB_PUTS2("FlowElement::delta_ext();id=", id);
    time = time + e;
    bool inflow_provided{false};
    bool outflow_provided{false};
    FlowValueType inflow_achieved{0};
    FlowValueType outflow_request{0};
    for (const auto &x : xs) {
      switch (x.port) {
        case inport_inflow_achieved:
          DB_PUTS("... <=inport_inflow_achieved");
          inflow_provided = true;
          inflow_achieved += x.value;
          break;
        case inport_outflow_request:
          DB_PUTS("... <=inport_outflow_request");
          outflow_provided = true;
          outflow_request += x.value;
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
    DB_PUTS2("FlowElement::delta_conf();id=", id);
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
    DB_PUTS2("FlowElement::ta();id=", id);
    if (report_inflow_request || report_outflow_achieved) {
      DB_PUTS("... dt = (0,1)");
      return adevs::Time{0, 1};
    }
    DB_PUTS("... dt = infinity");
    return calculate_time_advance();
  }

  void
  FlowElement::output_func(std::vector<PortValue>& ys)
  {
    DB_PUTS2("FlowElement::output_func();id=", id);
    if (report_inflow_request) {
      DB_PUTS("... send=>outport_inflow_request");
      ys.push_back(
          adevs::port_value<FlowValueType>{outport_inflow_request, inflow});
    }
    if (report_outflow_achieved) {
      DB_PUTS("... send=>outport_outflow_achieved");
      ys.push_back(
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
    DB_PUTS2("FlowElement::update_state_for_outflow_request();id=", id);
    return FlowState{outflow_, outflow_};
  }

  FlowState
  FlowElement::update_state_for_inflow_achieved(FlowValueType inflow_) const
  {
    DB_PUTS2("FlowElement::update_state_for_inflow_achieved();id=", id);
    return FlowState{inflow_, inflow_};
  }

  void
  FlowElement::update_on_internal_transition()
  {
    DB_PUTS2("FlowElement::update_on_internal_transition();id=", id);
  }

  void
  FlowElement::update_on_external_transition()
  {
    DB_PUTS2("FlowElement::update_on_external_transition();id=", id);
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
    FlowElement(std::move(id), stream_type),
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
#ifdef DEBUG_PRINT
      std::cout << "FlowLimits::update_state_for_outflow_request(" << out << ")\n";
      print_state("... ");
#endif
    FlowValueType out_{0.0};
    if (out > upper_limit) {
      out_ = upper_limit;
    }
    else if (out < lower_limit) {
      out_ = lower_limit;
    }
    else
      out_ = out;
#ifdef DEBUG_PRINT
      print_state("... ");
      std::cout << "end FlowLimits::update_state_for_outflow_request\n";
#endif
    return FlowState{out_, out_};
  }

  FlowState
  FlowLimits::update_state_for_inflow_achieved(FlowValueType in) const
  {
#ifdef DEBUG_PRINT
      std::cout << "FlowLimits::update_state_for_inflow_achieved(" << in << ")\n";
      print_state("... ");
#endif
    FlowValueType in_{0.0};
    if (in > upper_limit)
      throw AchievedMoreThanRequestedError();
    else if (in < lower_limit)
      throw AchievedMoreThanRequestedError();
    else
      in_ = in;
#ifdef DEBUG_PRINT
      print_state("... ");
      std::cout << "end FlowLimits::update_state_for_inflow_achieved\n";
#endif
    return FlowState{in_, in_};
  }

  ////////////////////////////////////////////////////////////
  // FlowMeter
  FlowMeter::FlowMeter(std::string id, StreamType stream_type) :
    FlowElement(std::move(id), stream_type),
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

  std::vector<Datum>
  FlowMeter::get_results() const
  {
    const auto ts = get_actual_output_times();
    const auto vs = get_actual_output();
    std::vector<Datum> results(ts.size());
    for (std::vector<Datum>::size_type i=0; i < ts.size(); ++i) {
      results[i] = Datum{ts[i], vs[i]};
    }
    return results;
  }

  void
  FlowMeter::update_on_external_transition()
  {
#ifdef DEBUG_PRINT
      std::cout << "FlowMeter::update_on_external_transition()\n";
      print_state("... ");
      print_vec<RealTimeType>("... event_times", event_times);
      print_vec<FlowValueType>("... requested_flows", requested_flows);
      print_vec<FlowValueType>("... achieved_flows", achieved_flows);
#endif
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
#ifdef DEBUG_PRINT
      print_state("... ");
      print_vec<RealTimeType>("... event_times", event_times);
      print_vec<FlowValueType>("... requested_flows", requested_flows);
      print_vec<FlowValueType>("... achieved_flows", achieved_flows);
      std::cout << "end FlowMeter::update_on_external_transition()\n";
#endif
  }

  ////////////////////////////////////////////////////////////
  // Converter
  Converter::Converter(
      std::string id,
      StreamType input_stream_type,
      StreamType output_stream_type,
      std::function<FlowValueType(FlowValueType)> calc_output_from_input,
      std::function<FlowValueType(FlowValueType)> calc_input_from_output
      ) :
    FlowElement(std::move(id), input_stream_type, output_stream_type),
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
      const StreamType& st,
      const std::vector<LoadItem>& loads_):
    FlowElement(std::move(id), st, st),
    loads{loads_},
    idx{-1},
    num_loads{loads_.size()}
  {
    check_loads();
  }

  void
  Sink::update_on_internal_transition()
  {
    DB_PUTS("Sink::update_on_internal_transition()");
    ++idx;
  }

  adevs::Time
  Sink::calculate_time_advance()
  {
    DB_PUTS("Sink::calculate_time_advance()");
    if (idx < 0) {
      DB_PUTS("... dt = infinity");
      return adevs::Time{0, 0};
    }
    std::vector<LoadItem>::size_type next_idx = idx + 1;
    if (next_idx < num_loads) {
      RealTimeType dt{loads[idx].get_time_advance(loads[next_idx])};
      DB_PUTS3("... dt = (", dt, ", 0)");
      return adevs::Time{dt, 0};
    }
    DB_PUTS("... dt = infinity");
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
    DB_PUTS("Sink::output_func()");
    std::vector<LoadItem>::size_type next_idx = idx + 1;
    auto max_idx{num_loads - 1};
    if (next_idx < max_idx)
      ys.push_back(
          adevs::port_value<FlowValueType>{
          outport_inflow_request, loads[next_idx].get_value()});
  }

  void
  Sink::check_loads() const
  {
    DB_PUTS("Sink::check_loads");
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
