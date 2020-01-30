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
      StreamType input_stream_,
      StreamType output_stream_,
      StreamType lossflow_stream_):
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
      StreamType input_stream_,
      StreamType output_stream_,
      StreamType lossflow_stream_,
      fragility_map fragilities_):
    id{std::move(id_)},
    component_type{type_},
    input_stream{std::move(input_stream_)},
    output_stream{std::move(output_stream_)},
    lossflow_stream{std::move(lossflow_stream_)},
    fragilities{std::move(fragilities_)},
    has_fragilities{!fragilities.empty()}
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
      bool both_way) const
  {
    connect_source_to_sink_with_ports(network, source, 0, sink, 0, both_way);
  }

  void
  Component::connect_source_to_sink_with_ports(
      adevs::Digraph<FlowValueType, Time>& network,
      FlowElement* source,
      int source_port,
      FlowElement* sink,
      int sink_port,
      bool both_way) const
  {
    auto src_out = source->get_outflow_type();
    auto sink_in = sink->get_inflow_type();
    if (src_out != sink_in) {
      std::ostringstream oss;
      oss << "MixedStreamsError:\n";
      oss << "source output stream != sink input stream for component ";
      oss << component_type_to_tag(get_component_type()) << "\n";
      oss << "source output stream: " << src_out.get_type() << "\n";
      oss << "sink input stream: " << sink_in.get_type() << "\n";
      throw std::runtime_error(oss.str());
    }
    network.couple(
        sink, FlowMeter::outport_inflow_request + sink_port,
        source, FlowMeter::inport_outflow_request + source_port);
    if (both_way) {
      network.couple(
          source, FlowMeter::outport_outflow_achieved + source_port,
          sink, FlowMeter::inport_inflow_achieved + sink_port);
    }
  }

  bool
  Component::base_is_equal(const Component& other) const
  {
    // TODO: add comparison of fragilities
    return (id == other.id)
      && (component_type == other.component_type)
      && (input_stream == other.input_stream)
      && (output_stream == other.output_stream)
      && (has_fragilities == other.has_fragilities);
  }

  std::string
  Component::internals_to_string() const
  {
    std::ostringstream oss;
    oss << "id=" << id << ", "
        << "component_type=" << component_type_to_tag(component_type) << ", "
        << "input_stream=" << input_stream << ", "
        << "output_stream=" << output_stream << ", "
        << "fragilities=..., "
        << "has_fragilities=" << (has_fragilities ? "true" : "false");
    return oss.str();
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
      const StreamType& input_stream_,
      std::unordered_map<
        std::string,std::vector<LoadItem>> loads_by_scenario_):
    LoadComponent(id_, input_stream_, std::move(loads_by_scenario_), {})
  {
  }

  LoadComponent::LoadComponent(
      const std::string& id_,
      const StreamType& input_stream_,
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
    namespace ep = ::erin::port;
    std::unordered_set<FlowElement*> elements;
    std::unordered_map<ep::Type, std::vector<FlowElement*>> ports;
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "LoadComponent::add_to_network(...)\n";
    }
    auto the_id = get_id();
    auto stream = get_input_stream();
    auto loads = loads_by_scenario.at(active_scenario);
    auto sink = new Sink(the_id, ComponentType::Load, stream, loads);
    elements.emplace(sink);
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "sink = " << sink << "\n";
    }
    auto meter = new FlowMeter(the_id, ComponentType::Load, stream);
    elements.emplace(meter);
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "meter = " << meter << "\n";
    }
    connect_source_to_sink(network, meter, sink, false);
    if (is_failed) {
      auto lim = new FlowLimits(the_id, ComponentType::Source, stream, 0.0, 0.0);
      elements.emplace(lim);
      connect_source_to_sink(network, lim, meter, true);
      ports[ep::Type::Inflow] = std::vector<FlowElement*>{lim};
    }
    else {
      ports[ep::Type::Inflow] = std::vector<FlowElement*>{meter};
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
      const StreamType& output_stream_):
    SourceComponent(id_, output_stream_, {}, Limits{})
  {
  }

  SourceComponent::SourceComponent(
      const std::string& id_,
      const StreamType& output_stream_,
      const FlowValueType max_output_,
      const FlowValueType min_output_):
    SourceComponent(
        id_, output_stream_, {}, Limits{min_output_, max_output_})
  {
  }

  SourceComponent::SourceComponent(
      const std::string& id_,
      const StreamType& output_stream_,
      const Limits& limits_):
    SourceComponent(id_, output_stream_, {}, limits_)
  {
  }

  SourceComponent::SourceComponent(
      const std::string& id_,
      const StreamType& output_stream_,
      fragility_map fragilities_):
    SourceComponent(
        id_, output_stream_, std::move(fragilities_), Limits{})
  {
  }

  SourceComponent::SourceComponent(
      const std::string& id_,
      const StreamType& output_stream_,
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
      const StreamType& output_stream_,
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
      adevs::Digraph<FlowValueType, Time>& network,
      const std::string&,
      bool is_failed) const
  {
    namespace ep = ::erin::port;
    std::unordered_map<ep::Type, std::vector<FlowElement*>> ports;
    std::unordered_set<FlowElement*> elements;
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "SourceComponent::add_to_network("
                   "adevs::Digraph<FlowValueType>& network)\n";
    }
    auto the_id = get_id();
    auto stream = get_output_stream();
    auto meter = new FlowMeter(the_id, ComponentType::Source, stream);
    elements.emplace(meter);
    if (is_failed || limits.get_is_limited()) {
      auto min_output = limits.get_min();
      auto max_output = limits.get_max();
      if (is_failed) {
        min_output = 0.0;
        max_output = 0.0;
      }
      auto lim = new FlowLimits(
          the_id, ComponentType::Source, stream, min_output, max_output);
      elements.emplace(lim);
      connect_source_to_sink(network, lim, meter, true);
    }
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "SourceComponent::add_to_network(...) exit\n";
    }
    ports[ep::Type::Outflow] = std::vector<FlowElement*>{meter};
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
      const StreamType& stream_,
      const int num_inflows_,
      const int num_outflows_,
      const MuxerDispatchStrategy strategy_,
      const MuxerDispatchStrategy output_strategy_):
    MuxerComponent(
        id_,
        stream_,
        num_inflows_,
        num_outflows_,
        {},
        strategy_,
        output_strategy_)
  {
  }

  MuxerComponent::MuxerComponent(
      const std::string& id_,
      const StreamType& stream_,
      const int num_inflows_,
      const int num_outflows_,
      fragility_map fragilities_,
      const MuxerDispatchStrategy strategy_,
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
    strategy{strategy_},
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
          strategy,
          output_strategy);
    return p;
  }

  PortsAndElements
  MuxerComponent::add_to_network(
      adevs::Digraph<FlowValueType, Time>& nw,
      const std::string&, // active_scenario
      bool is_failed) const
  {
    namespace ep = ::erin::port;
    std::unordered_set<FlowElement*> elements;
    std::unordered_map<ep::Type, std::vector<FlowElement*>> ports;
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "MuxerComponent::add_to_network(...)\n";
    }
    auto the_id = get_id();
    auto the_stream = get_input_stream();
    FlowElement* mux = nullptr;
    if (!is_failed) {
      mux = new Mux(
          the_id,
          ComponentType::Muxer,
          the_stream,
          num_inflows,
          num_outflows,
          strategy,
          output_strategy);
      elements.emplace(mux);
    }
    auto the_ct = ComponentType::Muxer;
    std::vector<FlowElement*> inflow_meters(num_inflows, nullptr);
    std::vector<FlowElement*> outflow_meters(num_outflows, nullptr);
    for (int i{0}; i < num_inflows; ++i) {
      std::ostringstream oss;
      oss << the_id << "-inflow(" << i << ")";
      auto m = new FlowMeter(oss.str(), the_ct, the_stream);
      inflow_meters[i] = m;
      elements.emplace(m);
      if (is_failed) {
        auto lim = new FlowLimits(the_id, the_ct, the_stream, 0.0, 0.0);
        elements.emplace(lim);
        connect_source_to_sink(nw, m, lim, true);
      }
      else {
        connect_source_to_sink_with_ports(nw, m, 0, mux, i, true);
      }
    }
    for (int i{0}; i < num_outflows; ++i) {
      std::ostringstream oss;
      oss << the_id << "-outflow(" << i << ")";
      auto m = new FlowMeter(oss.str(), the_ct, the_stream);
      outflow_meters[i] = m;
      elements.emplace(m);
      if (is_failed) {
        auto lim = new FlowLimits(the_id, the_ct, the_stream, 0.0, 0.0);
        elements.emplace(lim);
        connect_source_to_sink(nw, lim, m, true);
      }
      else {
        connect_source_to_sink_with_ports(nw, mux, i, m, 0, true);
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
      std::cout << "... a.strategy = "
                << muxer_dispatch_strategy_to_string(a.strategy) << "\n";
      std::cout << "... b.strategy = "
                << muxer_dispatch_strategy_to_string(a.strategy) << "\n";
    }
    return (a.num_inflows == b.num_inflows)
      && (a.num_outflows == b.num_outflows)
      && (a.strategy == b.strategy);
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
       << "strategy=" << muxer_dispatch_strategy_to_string(n.strategy) << ")";
    return os;
  }

  ////////////////////////////////////////////////////////////
  // ConverterComponent
  ConverterComponent::ConverterComponent(
      const std::string& id_,
      const StreamType& input_stream_,
      const StreamType& output_stream_,
      const StreamType& lossflow_stream_,
      const FlowValueType& const_eff_):
    ConverterComponent(
        std::move(id_),
        std::move(input_stream_),
        std::move(output_stream_),
        std::move(lossflow_stream_),
        const_eff_,
        {})
  {
  }

  ConverterComponent::ConverterComponent(
      const std::string& id_,
      const StreamType& input_stream_,
      const StreamType& output_stream_,
      const StreamType& lossflow_stream_,
      const FlowValueType& const_eff_,
      fragility_map fragilities_):
    Component(
        std::move(id_),
        ComponentType::Converter,
        std::move(input_stream_),
        std::move(output_stream_),
        std::move(lossflow_stream_),
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
      adevs::Digraph<FlowValueType, Time>& nw,
      const std::string&, // active_scenario
      bool is_failed) const
  {
    namespace ep = ::erin::port;
    std::unordered_set<FlowElement*> elements;
    std::unordered_map<ep::Type, std::vector<FlowElement*>> ports;
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "ConverterComponent::add_to_network(...)\n";
    }
    if (is_failed) {
      throw std::invalid_argument(
          "unimplemented functionality for is_failed=true");
    }
    auto the_id = get_id();
    auto the_type = ComponentType::Converter;
    auto in_stream = get_input_stream();
    auto out_stream = get_output_stream();
    auto loss_stream = get_lossflow_stream();
    auto in_meter = new FlowMeter(the_id + ":inflow", the_type, in_stream);
    elements.emplace(in_meter);
    ports[ep::Type::Inflow] = std::vector<FlowElement*>{in_meter};
    auto loss_meter = new FlowMeter(the_id + ":lossflow", the_type, loss_stream);
    elements.emplace(loss_meter);
    auto out_meter = new FlowMeter(the_id + ":outflow", the_type, out_stream);
    elements.emplace(out_meter);
    ports[ep::Type::Outflow] = std::vector<FlowElement*>{out_meter, loss_meter};
    auto out_from_in = [this](FlowValueType in) -> FlowValueType {
      return in * const_eff;
    };
    auto in_from_out = [this](FlowValueType out) -> FlowValueType {
      return out / const_eff;
    };
    auto conv = new Converter(
        the_id, the_type, in_stream, out_stream, out_from_in, in_from_out);
    elements.emplace(conv);
    connect_source_to_sink(nw, in_meter, conv, true);
    connect_source_to_sink(nw, conv, out_meter, true);
    erin::network::couple_source_loss_to_sink(nw, conv, loss_meter, true);
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
      const StreamType& stream_):
    PassThroughComponent(
        std::move(id_),
        std::move(stream_),
        Limits{},
        {})
  {
  }

  PassThroughComponent::PassThroughComponent(
      const std::string& id_,
      const StreamType& stream_,
      const Limits& limits_):
    PassThroughComponent(
        std::move(id_),
        std::move(stream_),
        std::move(limits_),
        {})
  {
  }

  PassThroughComponent::PassThroughComponent(
      const std::string& id_,
      const StreamType& stream_,
      fragility_map fragilities):
    PassThroughComponent(
        std::move(id_),
        std::move(stream_),
        Limits{},
        std::move(fragilities))
  {
  }

  PassThroughComponent::PassThroughComponent(
      const std::string& id_,
      const StreamType& stream_,
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
      adevs::Digraph<FlowValueType, Time>& nw,
      const std::string&, // active_scenario
      bool is_failed) const
  {
    namespace ep = ::erin::port;
    std::unordered_set<FlowElement*> elements;
    std::unordered_map<ep::Type, std::vector<FlowElement*>> ports;
    auto the_id = get_id();
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "PassThroughComponent::add_to_network(...);id="
                << the_id << "\n";
    }
    auto the_type = ComponentType::PassThrough;
    auto stream = get_input_stream();
    auto meter = new FlowMeter(the_id, the_type, stream);
    elements.emplace(meter);
    ports[ep::Type::Outflow] = std::vector<FlowElement*>{meter};
    if (is_failed) {
      auto the_limits = new FlowLimits(the_id, the_type, stream, 0.0, 0.0);
      elements.emplace(the_limits);
      connect_source_to_sink(nw, the_limits, meter, true);
      ports[ep::Type::Inflow] = std::vector<FlowElement*>{the_limits};
    }
    else {
      if (limits.get_is_limited()) {
        auto lims = new FlowLimits(
            the_id, the_type, stream, limits.get_min(), limits.get_max());
        elements.emplace(lims);
        connect_source_to_sink(nw, lims, meter, true);
        ports[ep::Type::Inflow] = std::vector<FlowElement*>{lims};
      }
      else {
        ports[ep::Type::Inflow] = std::vector<FlowElement*>{meter};
      }
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
}
