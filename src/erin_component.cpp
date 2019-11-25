/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/component.h"
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
      StreamType output_stream_):
    Component(
        std::move(id_),
        type_,
        std::move(input_stream_),
        std::move(output_stream_),
        {})
  {
  }

  Component::Component(
      std::string id_,
      ComponentType type_,
      StreamType input_stream_,
      StreamType output_stream_,
      std::unordered_map<
        std::string,
        std::unique_ptr<::erin::fragility::Curve>> fragilities_):
    id{std::move(id_)},
    component_type{type_},
    input_stream{std::move(input_stream_)},
    output_stream{std::move(output_stream_)},
    fragilities{std::move(fragilities_)},
    has_fragilities{!fragilities.empty()}
  {
  }

  std::unordered_map<std::string, std::unique_ptr<::erin::fragility::Curve>>
  Component::clone_fragility_curves() const
  {
    std::unordered_map<
      std::string, std::unique_ptr<::erin::fragility::Curve>> frags;
    for (const auto& pair : fragilities) {
      frags.insert(
          std::make_pair(pair.first, pair.second->clone()));
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
      const auto& curve = it->second;
      auto intensity = intensity_pair.second;
      auto probability = curve->apply(intensity);
      failure_probabilities.emplace_back(probability);
    }
    return failure_probabilities;
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
        sink, FlowMeter::outport_inflow_request,
        source, FlowMeter::inport_outflow_request);
    if (both_way) {
      network.couple(
          source, FlowMeter::outport_outflow_achieved,
          sink, FlowMeter::inport_inflow_achieved);
    }
  }

  ////////////////////////////////////////////////////////////
  // LoadComponent
  LoadComponent::LoadComponent(
      const std::string& id_,
      const StreamType& input_stream_,
      std::unordered_map<
        std::string,std::vector<LoadItem>> loads_by_scenario_):
    LoadComponent(id_, input_stream_, loads_by_scenario_, {})
  {
  }

  LoadComponent::LoadComponent(
      const std::string& id_,
      const StreamType& input_stream_,
      std::unordered_map<
        std::string, std::vector<LoadItem>> loads_by_scenario_,
      std::unordered_map<
        std::string, std::unique_ptr<::erin::fragility::Curve>> fragilities):
    Component(
        id_, ComponentType::Load, input_stream_, input_stream_, std::move(fragilities)),
    loads_by_scenario{std::move(loads_by_scenario_)}
  {
  }

  std::unique_ptr<Component>
  LoadComponent::clone() const
  {
    auto the_id = get_id();
    auto in_strm = get_input_stream();
    auto lbs = loads_by_scenario;
    std::unique_ptr<Component> p =
      std::make_unique<LoadComponent>(the_id, in_strm, lbs);
    return p;
  }

  PortsAndElements
  LoadComponent::add_to_network(
      adevs::Digraph<FlowValueType>& network,
      const std::string& active_scenario,
      bool is_failed) const
  {
    namespace ep = ::erin::port;
    std::unordered_set<FlowElement*> elements;
    std::unordered_map<std::string, FlowElement*> ports;
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "LoadComponent::add_to_network("
                   "adevs::Digraph<FlowValueType>& network)\n";
    }
    // TODO: implement is_failed=true semantics
    if (is_failed) {
      std::ostringstream oss;
      oss << "semantics for is_failed=true not yet implemented "
             "for LoadComponent";
      throw std::runtime_error(oss.str());
    }
    auto sink = new Sink(
        get_id(),
        ComponentType::Load,
        get_input_stream(),
        loads_by_scenario.at(active_scenario));
    elements.emplace(sink);
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "sink = " << sink << "\n";
    }
    auto meter = new FlowMeter(get_id(), ComponentType::Load, get_input_stream());
    elements.emplace(meter);
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "meter = " << meter << "\n";
    }
    connect_source_to_sink(network, meter, sink, false);
    ports[ep::inflow] = meter;
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "LoadComponent::add_to_network(...) exit\n";
    }
    return PortsAndElements{ports, elements};
  }

  ////////////////////////////////////////////////////////////
  // SourceComponent
  SourceComponent::SourceComponent(
      const std::string& id_,
      const StreamType& output_stream_):
    Component(id_, ComponentType::Source, output_stream_, output_stream_)
  {
  }

  SourceComponent::SourceComponent(
      const std::string& id_,
      const StreamType& output_stream_,
      std::unordered_map<std::string,std::unique_ptr<::erin::fragility::Curve>>
        fragilities_):
    Component(
        id_,
        ComponentType::Source,
        output_stream_,
        output_stream_,
        std::move(fragilities_))
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
          the_id, out_strm, std::move(frags));
    return p;
  }

  PortsAndElements
  SourceComponent::add_to_network(
      adevs::Digraph<FlowValueType>&,
      const std::string&,
      bool is_failed) const
  {
    namespace ep = ::erin::port;
    std::unordered_map<std::string, FlowElement*> ports;
    std::unordered_set<FlowElement*> elements;
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "SourceComponent::add_to_network("
                   "adevs::Digraph<FlowValueType>& network)\n";
    }
    if (is_failed) {
      std::ostringstream oss;
      oss << "is_failed semantics not yet implemented for SourceComponent";
      throw std::runtime_error(oss.str());
    }
    auto meter = new FlowMeter(get_id(), ComponentType::Source, get_output_stream());
    elements.emplace(meter);
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "SourceComponent::add_to_network(...) exit\n";
    }
    ports[ep::outflow] = meter;
    return PortsAndElements{ports, elements};
  }
}
