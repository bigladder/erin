/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/component.h"
#include "erin/network.h"
#include "erin/port.h"
#include "debug_utils.h"
#include <sstream>
#include <stdexcept>
#include <string>

namespace ERIN
{
  ////////////////////////////////////////////////////////////
  // Component
  Component::Component(
      std::string id_,
      ComponentType type_,
      std::string input_stream_,
      std::string output_stream_,
      std::string lossflow_stream_):
    Component(
        std::move(id_),
        type_,
        std::move(input_stream_),
        std::move(output_stream_),
        std::move(lossflow_stream_),
        {})
  {
  }

  Component::Component(
      std::string id_,
      ComponentType type_,
      std::string input_stream_,
      std::string output_stream_,
      std::string lossflow_stream_,
      fragility_map fragilities_):
    id{std::move(id_)},
    component_type{type_},
    input_stream{std::move(input_stream_)},
    output_stream{std::move(output_stream_)},
    lossflow_stream{std::move(lossflow_stream_)},
    fragilities{std::move(fragilities_)},
    has_fragilities{!fragilities.empty()},
    has_reliability{false},
    schedule_times{},
    schedule_values{}
  {
  }

  fragility_map
  Component::clone_fragility_curves() const
  {
    namespace ef = erin::fragility;
    fragility_map frags;
    for (const auto& pair : fragilities) {
      const auto& curves = pair.second;
      auto num_curves = curves.size();
      std::vector<std::unique_ptr<ef::Curve>> vs(num_curves);
      for (decltype(num_curves) i{0}; i < num_curves; ++i) {
        const auto& c = curves[i];
        vs[i] = c->clone();
      }
      frags.insert(std::make_pair(pair.first, std::move(vs)));
    }
    return frags;
  }

  std::vector<double>
  Component::apply_intensities(
      const std::unordered_map<std::string, double>& intensities)
  {
    std::vector<double> failure_probabilities{};
    if (!has_fragilities) {
      return failure_probabilities;
    }
    for (const auto& intensity_pair: intensities) {
      auto intensity_tag = intensity_pair.first;
      auto it = fragilities.find(intensity_tag);
      if (it == fragilities.end()) {
        continue;
      }
      auto intensity = intensity_pair.second;
      for (const auto& c : it->second) {
        auto probability = c->apply(intensity);
        if (probability > 0.0) {
          failure_probabilities.emplace_back(probability);
        }
      }
    }
    return failure_probabilities;
  }

  void
  Component::connect_source_to_sink(
      adevs::Digraph<FlowValueType, Time>& network,
      FlowElement* source,
      FlowElement* sink,
      bool both_way,
      const std::string& stream) const
  {
    connect_source_to_sink_with_ports(
        network, source, 0, sink, 0, both_way, stream);
  }

  void
  Component::connect_source_to_sink_with_ports(
      adevs::Digraph<FlowValueType, Time>& network,
      FlowElement* source,
      int source_port,
      FlowElement* sink,
      int sink_port,
      bool both_way,
      const std::string& stream) const
  {
    erin::network::connect_source_to_sink_with_ports(
        network,
        source,
        source_port,
        sink,
        sink_port,
        both_way,
        stream);
  }

  bool
  Component::base_is_equal(const Component& other) const
  {
    // TODO: add comparison of fragilities
    return (id == other.id)
      && (component_type == other.component_type)
      && (input_stream == other.input_stream)
      && (output_stream == other.output_stream)
      && (has_fragilities == other.has_fragilities)
      && (has_reliability == other.has_reliability)
      && (schedule_times == other.schedule_times)
      && (schedule_values == other.schedule_values);
  }

  std::string
  Component::internals_to_string() const
  {
    std::ostringstream oss;
    oss << "id=" << id << ", "
        << "component_type=" << component_type_to_tag(component_type) << ", "
        << "input_stream=\"" << input_stream << "\", "
        << "output_stream=\"" << output_stream << "\", "
        << "fragilities=..., "
        << "has_fragilities=" << (has_fragilities ? "true" : "false");
    return oss.str();
  }

  void
  Component::set_reliability_schedule(
      const std::vector<RealTimeType>& schedule_times_,
      const std::vector<FlowValueType>& schedule_values_
      )
  {
    if (schedule_times_.size() != schedule_values_.size()) {
      std::ostringstream oss{};
      oss << "schedule_times and schedule_values sizes must equal!\n"
          << "schedule_times.size() = " << schedule_times_.size() << "\n"
          << "schedule_values.size() = " << schedule_values.size() << "\n";
      throw std::invalid_argument(oss.str());
    }
    has_reliability = (schedule_times_.size() > 0);
    schedule_times = schedule_times_;
    schedule_values = schedule_values_;
  }

  bool
  operator==(const std::unique_ptr<Component>& a, const std::unique_ptr<Component>& b)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "operator==(unique_ptr<Component> a, unique_ptr<Component> b)\n";
      std::cout << "a = " << a << "\n";
      std::cout << "b = " << b << "\n";
    }
    if ((a.get() == nullptr) || (b.get() == nullptr)) {
      return false;
    }
    auto a_type = a->get_component_type();
    auto b_type = b->get_component_type();
    if (a_type != b_type) {
      return false;
    }
    switch (a_type) {
      case ComponentType::Load:
        {
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "LoadComponent compare...\n";
          }
          const auto& a_load = dynamic_cast<const LoadComponent&>(*a);
          const auto& b_load = dynamic_cast<const LoadComponent&>(*b);
          return a_load == b_load;
        }
      case ComponentType::Source:
        {
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "SourceComponent compare...\n";
          }
          const auto& a_source = dynamic_cast<const SourceComponent&>(*a);
          const auto& b_source = dynamic_cast<const SourceComponent&>(*b);
          return a_source == b_source;
        }
      case ComponentType::Converter:
        {
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "ConverterComponent compare...\n";
          }
          const auto& a_conv = dynamic_cast<const ConverterComponent&>(*a);
          const auto& b_conv = dynamic_cast<const ConverterComponent&>(*b);
          return a_conv == b_conv;
        }
      case ComponentType::Muxer:
        {
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "MuxerComponent compare...\n";
          }
          const auto& a_mux = dynamic_cast<const MuxerComponent&>(*a);
          const auto& b_mux = dynamic_cast<const MuxerComponent&>(*b);
          return a_mux == b_mux;
        }
      case ComponentType::Storage:
        {
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "StorageComponent compare...\n";
          }
          const auto& a_store = dynamic_cast<const StorageComponent&>(*a);
          const auto& b_store = dynamic_cast<const StorageComponent&>(*b);
          return a_store == b_store;
        }
      default:
        {
          std::ostringstream oss;
          oss << "unhandled component type " << static_cast<int>(a_type) << "\n";
          throw std::invalid_argument(oss.str());
        }
    }
  }

  bool
  operator!=(const std::unique_ptr<Component>& a, const std::unique_ptr<Component>& b)
  {
    return !(a == b);
  }

  std::ostream& operator<<(std::ostream& os, const std::unique_ptr<Component>& a)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "operator<<(std::ostream& os, std::unique_ptr<Component>& a)\n";
    }
    if (a.get() == nullptr) {
      return os;
    }
    auto a_type = a->get_component_type();
    switch (a_type) {
      case ComponentType::Load:
        {
          const auto& a_load = dynamic_cast<const LoadComponent&>(*a);
          os << a_load;
          break;
        }
      case ComponentType::Source:
        {
          const auto& a_source = dynamic_cast<const SourceComponent&>(*a);
          os << a_source;
          break;
        }
      case ComponentType::Converter:
        {
          const auto& a_conv = dynamic_cast<const ConverterComponent&>(*a);
          os << a_conv;
          break;
        }
      case ComponentType::Muxer:
        {
          const auto& a_mux = dynamic_cast<const MuxerComponent&>(*a);
          os << a_mux;
          break;
        }
      case ComponentType::Storage:
        {
          const auto& a_store = dynamic_cast<const StorageComponent&>(*a);
          os << a_store;
          break;
        }
      default:
        {
          std::ostringstream oss;
          oss << "unhandled component type " << static_cast<int>(a_type) << "\n";
          throw std::invalid_argument(oss.str());
        }
    }
    return os;
  }

  ////////////////////////////////////////////////////////////
  // LoadComponent
  LoadComponent::LoadComponent(
      const std::string& id_,
      const std::string& input_stream_,
      std::unordered_map<
        std::string,std::vector<LoadItem>> loads_by_scenario_):
    LoadComponent(id_, input_stream_, std::move(loads_by_scenario_), {})
  {
  }

  LoadComponent::LoadComponent(
      const std::string& id_,
      const std::string& input_stream_,
      std::unordered_map<
        std::string, std::vector<LoadItem>> loads_by_scenario_,
      fragility_map fragilities):
    Component(
        id_,
        ComponentType::Load,
        input_stream_,
        input_stream_,
        input_stream_,
        std::move(fragilities)),
    loads_by_scenario{std::move(loads_by_scenario_)}
  {
  }

  std::unique_ptr<Component>
  LoadComponent::clone() const
  {
    auto the_id = get_id();
    auto in_strm = get_input_stream();
    auto lbs = loads_by_scenario;
    auto fcs = clone_fragility_curves();
    std::unique_ptr<Component> p =
      std::make_unique<LoadComponent>(the_id, in_strm, lbs, std::move(fcs));
    return p;
  }

  PortsAndElements
  LoadComponent::add_to_network(
      adevs::Digraph<FlowValueType, Time>& network,
      const std::string& active_scenario,
      bool is_failed) const
  {
    namespace ep = erin::port;
    std::unordered_set<FlowElement*> elements;
    std::unordered_map<ep::Type, std::vector<ElementPort>> ports{};
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "LoadComponent::add_to_network(...)\n";
    }
    auto the_id = get_id();
    auto stream = get_input_stream();
    const auto& stream_name{stream};
    auto loads = loads_by_scenario.at(active_scenario);
    auto sink = new Sink(the_id, ComponentType::Load, stream, loads);
    sink->set_recording_on();
    elements.emplace(sink);
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "sink = " << sink << "\n";
    }
    if (is_failed) {
      auto lim = new FlowLimits(
          the_id + "-limits",
          ComponentType::Source,
          stream,
          0.0,
          0.0);
      elements.emplace(lim);
      connect_source_to_sink(network, lim, sink, true, stream_name);
      ports[ep::Type::Inflow] = std::vector<ElementPort>{{lim, 0}};
    }
    else {
      ports[ep::Type::Inflow] = std::vector<ElementPort>{{sink, 0}};
    }
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "LoadComponent::add_to_network(...) exit\n";
    }
    return PortsAndElements{ports, elements};
  }

  bool
  operator==(const LoadComponent& a, const LoadComponent& b)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "entering operator==(a, b)\n"
                << "... a.id = " << a.get_id() << "\n"
                << "... b.id = " << b.get_id() << "\n";
    }
    if (!a.base_is_equal(b)) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... not equal because a.base_is_equal(b) is false\n";
      }
      return false;
    }
    const auto& a_lbs = a.loads_by_scenario;
    const auto& b_lbs = b.loads_by_scenario;
    const auto num_a_items = a_lbs.size();
    const auto num_b_items = b_lbs.size();
    if (num_a_items != num_b_items) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... not equal because num_a_items "
                  << "!= num_b_items\n"
                  << "num_a_items: " << num_a_items << "\n"
                  << "num_b_items: " << num_b_items << "\n";
      }
      return false;
    }
    for (const auto& a_pair : a_lbs) {
      const auto& tag = a_pair.first;
      auto b_it = b_lbs.find(tag);
      if (b_it == b_lbs.end()) {
        if constexpr (debug_level >= debug_level_high) {
          std::cout << "... not equal because tag '" << tag << "' "
            << "doesn't appear in b_lbs\n";
        }
        return false;
      }
      const auto& a_load_items = a_pair.second;
      const auto& b_load_items = b_it->second;
      const auto a_num_loads = a_load_items.size();
      const auto b_num_loads = b_load_items.size();
      if (a_num_loads != b_num_loads) {
        if constexpr (debug_level >= debug_level_high) {
          std::cout << "... not equal because a_num_loads != b_num_loads\n"
            << "tag:         " << tag << "\n"
            << "a_num_loads: " << a_num_loads << "\n"
            << "b_num_loads: " << b_num_loads << "\n";
        }
        return false;
      }
      for (std::vector<LoadItem>::size_type i{0}; i < a_num_loads; ++i) {
        const auto& a_load_item = a_load_items[i];
        const auto& b_load_item = b_load_items[i];
        if (a_load_item != b_load_item) {
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "... not equal because a_load_item != b_load_item\n"
              << "tag:         " << tag << "\n"
              << "i:           " << i << "\n"
              << "a_load_item: " << a_load_item << "\n"
              << "b_load_item: " << b_load_item << "\n";
          }
          return false;
        }
      }
    }
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "... is equal\n";
    }
    return true;
  }

  bool
  operator!=(const LoadComponent& a, const LoadComponent& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const LoadComponent& n)
  {
    os << "LoadComponent(" << n.internals_to_string() << ", "
       << "loads_by_scenario=...)";
    return os;
  }

  ////////////////////////////////////////////////////////////
  // Limits
  Limits::Limits():
    is_limited{false},
    minimum{0.0},
    maximum{0.0}
  {
  }

  Limits::Limits(FlowValueType max_):
    Limits(0.0, max_)
  {
  }

  Limits::Limits(FlowValueType min_, FlowValueType max_):
    is_limited{true},
    minimum{min_},
    maximum{max_}
  {
    if (minimum > maximum) {
      std::ostringstream oss;
      oss << "invariant error: minimum cannot be greater "
          << "than maximum in Limits\n"
          << "minimum: " << minimum << "\n"
          << "maximum: " << maximum << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  bool
  operator==(const Limits& a, const Limits& b)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "entering operator==(a, b) for Limits\n"
                << "a = " << a << "\n"
                << "b = " << b << "\n";
    }
    if (a.is_limited != b.is_limited) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... not equal because a.is_limited != b.is_limited\n";
      }
      return false;
    }
    if (a.is_limited) {
      return (a.minimum == b.minimum) && (b.maximum == b.maximum);
    }
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "... is equal\n";
    }
    return true;
  }

  bool
  operator!=(const Limits& a, const Limits& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const Limits& n)
  {
    std::string b = n.is_limited ? "true" : "false";
    os << "Limits("
       << "is_limited=" << b << ", "
       << "minimum=" << n.minimum << ", "
       << "maximum=" << n.maximum << ")";
    return os;
  }

  ////////////////////////////////////////////////////////////
  // SourceComponent
  SourceComponent::SourceComponent(
      const std::string& id_,
      const std::string& output_stream_):
    SourceComponent(id_, output_stream_, {}, Limits{})
  {
  }

  SourceComponent::SourceComponent(
      const std::string& id_,
      const std::string& output_stream_,
      const FlowValueType max_output_,
      const FlowValueType min_output_):
    SourceComponent(
        id_, output_stream_, {}, Limits{min_output_, max_output_})
  {
  }

  SourceComponent::SourceComponent(
      const std::string& id_,
      const std::string& output_stream_,
      const Limits& limits_):
    SourceComponent(id_, output_stream_, {}, limits_)
  {
  }

  SourceComponent::SourceComponent(
      const std::string& id_,
      const std::string& output_stream_,
      fragility_map fragilities_):
    SourceComponent(
        id_, output_stream_, std::move(fragilities_), Limits{})
  {
  }

  SourceComponent::SourceComponent(
      const std::string& id_,
      const std::string& output_stream_,
      fragility_map fragilities_,
      const FlowValueType max_output_,
      const FlowValueType min_output_): 
    SourceComponent(
        id_, output_stream_, std::move(fragilities_),
        Limits{min_output_, max_output_})
  {
  }

  SourceComponent::SourceComponent(
      const std::string& id_,
      const std::string& output_stream_,
      fragility_map fragilities_,
      const Limits& limits_):
    Component(
        id_,
        ComponentType::Source,
        output_stream_,
        output_stream_,
        output_stream_,
        std::move(fragilities_)),
    limits{limits_}
  {
  }

  std::unique_ptr<Component>
  SourceComponent::clone() const
  {
    auto the_id = get_id();
    auto out_strm = get_output_stream();
    auto frags = clone_fragility_curves();
    std::unique_ptr<Component> p =
      std::make_unique<SourceComponent>(
          the_id, out_strm, std::move(frags), limits);
    return p;
  }

  PortsAndElements
  SourceComponent::add_to_network(
      adevs::Digraph<FlowValueType, Time>& /* network */,
      const std::string&,
      bool is_failed) const
  {
    namespace ep = erin::port;
    std::unordered_map<ep::Type, std::vector<ElementPort>> ports{};
    std::unordered_set<FlowElement*> elements{};
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "SourceComponent::add_to_network("
                << "adevs::Digraph<FlowValueType>& network)\n";
    }
    auto the_id = get_id();
    auto stream = get_output_stream();
    if (is_failed || limits.get_is_limited()) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "is_failed || is_limited\n";
      }
      auto min_output = limits.get_min();
      auto max_output = limits.get_max();
      if (is_failed) {
        if constexpr (debug_level >= debug_level_high) {
          std::cout << "is_failed = true; setting min/max output to 0.0\n";
        }
        min_output = 0.0;
        max_output = 0.0;
      }
      auto lim = new FlowLimits(
          the_id, ComponentType::Source, stream, min_output, max_output,
          PortRole::SourceOutflow);
      elements.emplace(lim);
      lim->set_recording_on();
      ports[ep::Type::Outflow] = std::vector<ElementPort>{ElementPort{lim, 0}};
    }
    else {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "!is_failed\n";
      }
      auto meter = new FlowMeter(the_id, ComponentType::Source, stream,
          PortRole::SourceOutflow);
      elements.emplace(meter);
      meter->set_recording_on();
      ports[ep::Type::Outflow] = std::vector<ElementPort>{ElementPort{meter, 0}};
    }
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "SourceComponent::add_to_network(...) exit\n";
    }
    return PortsAndElements{ports, elements};
  }

  bool
  operator==(const SourceComponent& a, const SourceComponent& b)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "entering operator==(a, b) for SourceComponent\n"
                << "a = " << a << "\n"
                << "b = " << b << "\n";
    }
    if (!a.base_is_equal(b)) {
      return false;
    }
    return (a.limits == b.limits);
  }

  bool
  operator!=(const SourceComponent& a, const SourceComponent& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const SourceComponent& n)
  {
    os << "SourceComponent(" << n.internals_to_string() << ", "
       << "limits=" << n.limits << ")";
    return os;
  }

  ////////////////////////////////////////////////////////////
  // MuxerComponent
  MuxerComponent::MuxerComponent(
      const std::string& id_,
      const std::string& stream_,
      const int num_inflows_,
      const int num_outflows_,
      const MuxerDispatchStrategy output_strategy_):
    MuxerComponent(
        id_,
        stream_,
        num_inflows_,
        num_outflows_,
        {},
        output_strategy_)
  {
  }

  MuxerComponent::MuxerComponent(
      const std::string& id_,
      const std::string& stream_,
      const int num_inflows_,
      const int num_outflows_,
      fragility_map fragilities_,
      const MuxerDispatchStrategy output_strategy_):
    Component(
        id_,
        ComponentType::Muxer,
        stream_,
        stream_,
        stream_,
        std::move(fragilities_)),
    num_inflows{num_inflows_},
    num_outflows{num_outflows_},
    output_strategy{output_strategy_}
  {
    const int min_ports{1};
    const int max_ports{FlowElement::max_port_numbers};
    if ((num_inflows < min_ports) || (num_inflows > max_ports)) {
      std::ostringstream oss;
      oss << "num_inflows must be >= " << min_ports
          << " and <= " << max_ports << "; num_inflows="
          << num_inflows << "\n";
      throw std::invalid_argument(oss.str());
    }
    if ((num_outflows < min_ports) || (num_outflows > max_ports)) {
      std::ostringstream oss;
      oss << "num_outflows must be >= " << min_ports
          << " and <= " << max_ports << "; num_outflows="
          << num_outflows << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  std::unique_ptr<Component>
  MuxerComponent::clone() const
  {
    auto the_id = get_id();
    auto the_stream = get_input_stream();
    auto the_fcs = clone_fragility_curves();
    std::unique_ptr<Component> p =
      std::make_unique<MuxerComponent>(
          the_id,
          the_stream,
          num_inflows,
          num_outflows,
          std::move(the_fcs),
          output_strategy);
    return p;
  }

  PortsAndElements
  MuxerComponent::add_to_network(
      adevs::Digraph<FlowValueType, Time>& /* nw */,
      const std::string&, // active_scenario
      bool is_failed) const
  {
    namespace ep = erin::port;
    std::unordered_set<FlowElement*> elements{};
    std::unordered_map<ep::Type, std::vector<ElementPort>> ports{};
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "MuxerComponent::add_to_network(...)\n";
    }
    auto the_id = get_id();
    auto the_stream = get_input_stream();
    auto the_ct = ComponentType::Muxer;
    std::vector<ElementPort> inflow_meters{};
    std::vector<ElementPort> outflow_meters{};
    if (!is_failed) {
      auto mux = new Mux(
          the_id,
          the_ct,
          the_stream,
          num_inflows,
          num_outflows,
          output_strategy);
      mux->set_recording_on();
      elements.emplace(mux);
      for (int i{0}; i < num_inflows; ++i) {
        inflow_meters.emplace_back(ElementPort{mux, i});
      }
      for (int i{0}; i < num_outflows; ++i) {
        outflow_meters.emplace_back(ElementPort{mux, i});
      }
    }
    else {
      for (int i{0}; i < num_inflows; ++i) {
        auto lim = new FlowLimits(
            the_id + "-inflow(" + std::to_string(i) + ")",
            the_ct,
            the_stream,
            0.0,
            0.0);
        elements.emplace(lim);
        lim->set_recording_on();
        inflow_meters.emplace_back(ElementPort{lim, 0});
      }
      for (int i{0}; i < num_outflows; ++i) {
        auto lim = new FlowLimits(
            the_id + "-outflow(" + std::to_string(i) + ")",
            the_ct,
            the_stream,
            0.0,
            0.0);
        elements.emplace(lim);
        lim->set_recording_on();
        outflow_meters.emplace_back(ElementPort{lim, 0});
      }
    }
    ports[ep::Type::Inflow] = inflow_meters;
    ports[ep::Type::Outflow] = outflow_meters;
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "MuxerComponent::add_to_network(...) exit\n";
    }
    return PortsAndElements{ports, elements};
  }

  bool
  operator==(const MuxerComponent& a, const MuxerComponent& b)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "enter operator==(a, b)\n"
                << "a.id = " << a.get_id() << "\n"
                << "b.id = " << b.get_id() << "\n";
    }
    if (!a.base_is_equal(b)) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... not equal because a.base_is_equal(b) doesn't pass\n";
      }
      return false;
    }
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "... a.num_inflows  = " << a.num_inflows << "\n";
      std::cout << "... b.num_inflows  = " << b.num_inflows << "\n";
      std::cout << "... a.num_outflows = " << a.num_outflows << "\n";
      std::cout << "... b.num_outflows = " << b.num_outflows << "\n";
      std::cout << "... a.output_strategy = "
                << muxer_dispatch_strategy_to_string(a.output_strategy) << "\n";
      std::cout << "... b.output_strategy = "
                << muxer_dispatch_strategy_to_string(b.output_strategy) << "\n";
    }
    return (a.num_inflows == b.num_inflows)
      && (a.num_outflows == b.num_outflows)
      && (a.output_strategy == b.output_strategy);
  }

  bool
  operator!=(const MuxerComponent& a, const MuxerComponent& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const MuxerComponent& n)
  {
    os << "MuxerComponent(" << n.internals_to_string() << ", "
       << "num_inflows=" << n.num_inflows << ", "
       << "num_outflows=" << n.num_outflows << ", "
       << "output_strategy=" << muxer_dispatch_strategy_to_string(n.output_strategy) << ")";
    return os;
  }

  ////////////////////////////////////////////////////////////
  // ConverterComponent
  ConverterComponent::ConverterComponent(
      const std::string& id_,
      const std::string& input_stream_,
      const std::string& output_stream_,
      const std::string& lossflow_stream_,
      const FlowValueType& const_eff_):
    ConverterComponent(
        id_,
        input_stream_,
        output_stream_,
        lossflow_stream_,
        const_eff_,
        {})
  {
  }

  ConverterComponent::ConverterComponent(
      const std::string& id_,
      const std::string& input_stream_,
      const std::string& output_stream_,
      const std::string& lossflow_stream_,
      const FlowValueType& const_eff_,
      fragility_map fragilities_):
    Component(
        id_,
        ComponentType::Converter,
        input_stream_,
        output_stream_,
        lossflow_stream_,
        std::move(fragilities_)),
    const_eff{const_eff_}
  {
    if ((const_eff > 1.0) || (const_eff <= 0.0)) {
      std::ostringstream oss;
      oss << "const_eff not in the proper range (0 < const_eff <= 1.0)\n"
          << "const_eff = " << const_eff << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  std::unique_ptr<Component>
  ConverterComponent::clone() const
  {
    auto fcs = clone_fragility_curves();
    std::unique_ptr<Component> p = std::make_unique<ConverterComponent>(
        get_id(),
        get_input_stream(),
        get_output_stream(),
        get_lossflow_stream(),
        const_eff,
        std::move(fcs));
    return p;
  }

  PortsAndElements
  ConverterComponent::add_to_network(
      adevs::Digraph<FlowValueType, Time>&, // nw
      const std::string&, // active_scenario
      bool is_failed) const
  {
    namespace ep = erin::port;
    std::unordered_map<ep::Type, std::vector<ElementPort>> ports{};
    std::unordered_set<FlowElement*> elements{};
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "ConverterComponent::add_to_network(...)\n";
    }
    auto the_id = get_id();
    auto the_type = ComponentType::Converter;
    auto in_stream = get_input_stream();
    auto out_stream = get_output_stream();
    auto loss_stream = get_lossflow_stream();
    if (is_failed) {
      auto lim_in = new FlowLimits(the_id + "-inflow", the_type, in_stream, 0.0, 0.0);
      elements.emplace(lim_in);
      lim_in->set_recording_on();
      auto lim_out = new FlowLimits(the_id + "-outflow", the_type, out_stream, 0.0, 0.0);
      elements.emplace(lim_out);
      lim_out->set_recording_on();
      auto lim_loss = new FlowLimits(the_id + "-lossflow", the_type, loss_stream, 0.0, 0.0);
      lim_loss->set_recording_on();
      elements.emplace(lim_loss);
      auto lim_waste = new FlowLimits(
          the_id + "-wasteflow", the_type, loss_stream, 0.0, 0.0, PortRole::WasteInflow);
      lim_waste->set_recording_on();
      elements.emplace(lim_waste);
      ports[ep::Type::Inflow] = std::vector<ElementPort>{{lim_in, 0}};
      ports[ep::Type::Outflow] = std::vector<ElementPort>{{lim_out, 0}, {lim_loss, 0}};
    }
    else {
      auto eff{const_eff};
      auto out_from_in = [eff](FlowValueType in) -> FlowValueType {
        return in * eff;
      };
      auto in_from_out = [eff](FlowValueType out) -> FlowValueType {
        return out / eff;
      };
      auto conv = new Converter(
          the_id, the_type, in_stream, out_stream, out_from_in, in_from_out,
          loss_stream);
      elements.emplace(conv);
      conv->set_recording_on();
      ports[ep::Type::Inflow] = std::vector<ElementPort>{{conv, 0}};
      ports[ep::Type::Outflow] = std::vector<ElementPort>{{conv, 0}, {conv, 1}};
    }
    return PortsAndElements{ports, elements};
  }

  bool
  operator==(const ConverterComponent& a, const ConverterComponent& b)
  {
    return a.base_is_equal(b) && (a.const_eff == b.const_eff);
  }

  bool
  operator!=(const ConverterComponent& a, const ConverterComponent& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const ConverterComponent& n)
  {
    os << "ConverterComponent(" << n.internals_to_string()
       << ", const_eff=" << n.const_eff << ")";
    return os;
  }

  ////////////////////////////////////////////////////////////
  // PassThroughComponent
  PassThroughComponent::PassThroughComponent(
      const std::string& id_,
      const std::string& stream_):
    PassThroughComponent(
        id_,
        stream_,
        Limits{},
        {})
  {
  }

  PassThroughComponent::PassThroughComponent(
      const std::string& id_,
      const std::string& stream_,
      const Limits& limits_):
    PassThroughComponent(
        id_,
        stream_,
        limits_,
        {})
  {
  }

  PassThroughComponent::PassThroughComponent(
      const std::string& id_,
      const std::string& stream_,
      fragility_map fragilities):
    PassThroughComponent(
        id_,
        stream_,
        Limits{},
        std::move(fragilities))
  {
  }

  PassThroughComponent::PassThroughComponent(
      const std::string& id_,
      const std::string& stream_,
      const Limits& limits_,
      fragility_map fragilities):
    Component(
        id_,
        ComponentType::PassThrough,
        stream_,
        stream_,
        stream_,
        std::move(fragilities)),
    limits{limits_}
  {
  }

  std::unique_ptr<Component>
  PassThroughComponent::clone() const
  {
    auto fcs = clone_fragility_curves();
    std::unique_ptr<Component> p = std::make_unique<PassThroughComponent>(
        get_id(),
        get_input_stream(),
        limits,
        std::move(fcs));
    return p;
  }

  PortsAndElements
  PassThroughComponent::add_to_network(
      adevs::Digraph<FlowValueType, Time>& /* nw */,
      const std::string&, // active_scenario
      bool is_failed) const
  {
    namespace ep = erin::port;
    std::unordered_map<ep::Type, std::vector<ElementPort>> ports{};
    std::unordered_set<FlowElement*> elements{};
    auto the_id = get_id();
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "PassThroughComponent::add_to_network(...);id="
                << the_id << "\n";
    }
    auto the_type = ComponentType::PassThrough;
    auto stream = get_input_stream();
    if (is_failed || limits.get_is_limited()) {
      auto min_limit = limits.get_min();
      auto max_limit = limits.get_max();
      if (is_failed) {
        min_limit = 0.0;
        max_limit = 0.0;
      }
      auto the_limits = new FlowLimits(the_id, the_type, stream, min_limit, max_limit);
      elements.emplace(the_limits);
      the_limits->set_recording_on();
      ports[ep::Type::Inflow] = std::vector<ElementPort>{{the_limits, 0}};
      ports[ep::Type::Outflow] = std::vector<ElementPort>{{the_limits, 0}};
    }
    else {
      auto meter = new FlowMeter(the_id, the_type, stream);
      elements.emplace(meter);
      meter->set_recording_on();
      ports[ep::Type::Inflow] = std::vector<ElementPort>{{meter, 0}};
      ports[ep::Type::Outflow] = std::vector<ElementPort>{{meter, 0}};
    }
    return PortsAndElements{ports, elements};
  }

  std::ostream&
  operator<<(std::ostream& os, const PassThroughComponent& n)
  {
    os << "PassThroughComponent(" << n.internals_to_string() << ")";
    return os;
  }

  bool
  operator==(const PassThroughComponent& a, const PassThroughComponent& b)
  {
    return a.base_is_equal(b);
  }

  bool
  operator!=(const PassThroughComponent& a, const PassThroughComponent& b)
  {
    return !(a == b);
  }

  ////////////////////////////////////////////////////////////
  // StorageComponent
  StorageComponent::StorageComponent(
      const std::string& id_,
      const std::string& stream_,
      FlowValueType capacity_,
      FlowValueType max_charge_rate_):
    StorageComponent(
        id_,
        stream_,
        capacity_,
        max_charge_rate_,
        {})
  {
  }

  StorageComponent::StorageComponent(
      const std::string& id_,
      const std::string& stream_,
      FlowValueType capacity_,
      FlowValueType max_charge_rate_,
      fragility_map fragilities):
    Component(
        id_,
        ComponentType::Storage,
        stream_,
        stream_,
        stream_,
        std::move(fragilities)),
    capacity{capacity_},
    max_charge_rate{max_charge_rate_}
  {
    if (capacity <= 0.0)
      throw std::invalid_argument(
          "capacity cannot be less than or equal to 0.0: capacity = "
          + std::to_string(capacity));
    if (max_charge_rate < 0.0)
      throw std::invalid_argument(
          "max_charge_rate cannot be less than 0.0: max_charge_rate = "
          + std::to_string(max_charge_rate));
  }

  std::unique_ptr<Component>
  StorageComponent::clone() const
  {
    auto fcs = clone_fragility_curves();
    std::unique_ptr<Component> p = std::make_unique<StorageComponent>(
        get_id(),
        get_input_stream(),
        capacity,
        max_charge_rate,
        std::move(fcs));
    return p;
  }

  PortsAndElements
  StorageComponent::add_to_network(
      adevs::Digraph<FlowValueType,Time>& /* nw */,
      const std::string& /* active_scenario */,
      bool is_failed) const
  {
    namespace ep = erin::port;
    std::unordered_map<ep::Type, std::vector<ElementPort>> ports{};
    std::unordered_set<FlowElement*> elements{};
    auto the_id = get_id();
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "StorageComponent::add_to_network(...);id="
                << the_id << "\n";
    }
    auto the_type = ComponentType::Storage;
    auto stream = get_input_stream();
    if (is_failed) {
      FlowValueType min_limit{0.0};
      FlowValueType max_limit{0.0};
      auto the_limits = new FlowLimits(the_id, the_type, stream, min_limit, max_limit);
      elements.emplace(the_limits);
      the_limits->set_recording_on();
      ports[ep::Type::Inflow] = std::vector<ElementPort>{{the_limits, 0}};
      ports[ep::Type::Outflow] = std::vector<ElementPort>{{the_limits, 0}};
    }
    else {
      auto store = new Storage(
          the_id, the_type, stream, capacity, max_charge_rate);
      elements.emplace(store);
      store->set_recording_on();
      ports[ep::Type::Inflow] = std::vector<ElementPort>{{store, 0}};
      ports[ep::Type::Outflow] = std::vector<ElementPort>{{store, 0}};
    }
    return PortsAndElements{ports, elements};
  }

  bool
  operator==(const StorageComponent& a, const StorageComponent& b)
  {
    return a.base_is_equal(b) && (a.capacity == b.capacity);
  }

  bool
  operator!=(const StorageComponent& a, const StorageComponent& b)
  {
    return !(a == b);
  }

  std::ostream& operator<<(std::ostream& os, const StorageComponent& n)
  {
    os << "StorageComponent("
       << n.internals_to_string() << ", "
       << "capacity=" << n.capacity << ")";
    return os;
  }
}
