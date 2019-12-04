/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_COMPONENT_H
#define ERIN_COMPONENT_H
#include "erin/type.h"
#include "erin/element.h"
#include "erin/fragility.h"
#include "erin/port.h"
#include "erin/stream.h"
#include "adevs.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ERIN
{
  /**
   * Holds Ports and FlowElement pointers from a return of Component.add_to_network(...)
   */
  struct PortsAndElements
  {
    std::unordered_map<::erin::port::Type, std::vector<FlowElement*>> port_map;
    std::unordered_set<FlowElement*> elements_added;
  };

  ////////////////////////////////////////////////////////////
  // Component
  // base class for all types of components
  class Component
  {
    public:
      Component(const Component&) = delete;
      Component(
          std::string id,
          ComponentType type,
          StreamType input_stream,
          StreamType output_stream);
      Component(
          std::string id,
          ComponentType type,
          StreamType input_stream,
          StreamType output_stream,
          std::unordered_map<
            std::string,
            std::unique_ptr<erin::fragility::Curve>> fragilities);
      Component& operator=(const Component&) = delete;
      Component(Component&&) = delete;
      Component& operator=(Component&&) = delete;
      virtual ~Component() = default;
      [[nodiscard]] virtual std::unique_ptr<Component> clone() const = 0;

      [[nodiscard]] const std::string& get_id() const { return id; }
      [[nodiscard]] ComponentType get_component_type() const { return component_type; }
      [[nodiscard]] const StreamType& get_input_stream() const { return input_stream; }
      [[nodiscard]] const StreamType& get_output_stream() const { return output_stream; }
      [[nodiscard]] std::unordered_map<std::string, std::unique_ptr<erin::fragility::Curve>>
        clone_fragility_curves() const;
      [[nodiscard]] bool is_fragile() const { return has_fragilities; }
      // TODO: consider moving this elsewhere
      std::vector<double> apply_intensities(
          const std::unordered_map<std::string, double>& intensities);

      virtual PortsAndElements add_to_network(
          adevs::Digraph<FlowValueType, Time>& nw,
          const std::string& active_scenario,
          bool is_failed = false) const = 0;

    protected:
      void connect_source_to_sink(
          adevs::Digraph<FlowValueType, Time>& nw,
          FlowElement* source,
          FlowElement* sink,
          bool both_way) const;

    private:
      std::string id;
      ComponentType component_type;
      StreamType input_stream;
      StreamType output_stream;
      std::unordered_map<
        std::string, std::unique_ptr<erin::fragility::Curve>> fragilities;
      bool has_fragilities;
  };

  ////////////////////////////////////////////////////////////
  // LoadComponent
  class LoadComponent : public Component
  {
    public:
      LoadComponent(
          const std::string& id,
          const StreamType& input_stream,
          std::unordered_map<std::string, std::vector<LoadItem>>
            loads_by_scenario);
      LoadComponent(
          const std::string& id,
          const StreamType& input_stream,
          std::unordered_map<std::string, std::vector<LoadItem>>
            loads_by_scenario,
          std::unordered_map<
            std::string,
            std::unique_ptr<erin::fragility::Curve>> fragilities);
      PortsAndElements add_to_network(
          adevs::Digraph<FlowValueType, Time>& nw,
          const std::string& active_scenario,
          bool is_failed = false) const override;
      [[nodiscard]] std::unique_ptr<Component> clone() const override;

    private:
      std::unordered_map<std::string,std::vector<LoadItem>> loads_by_scenario;
  };

  ////////////////////////////////////////////////////////////
  // Limits
  struct Limits
  {
    bool is_limited;
    FlowValueType minimum;
    FlowValueType maximum;
  };

  ////////////////////////////////////////////////////////////
  // SourceComponent
  class SourceComponent : public Component
  {
    public:
      SourceComponent(
          const std::string& id,
          const StreamType& output_stream);
      SourceComponent(
          const std::string& id,
          const StreamType& output_stream,
          const FlowValueType max_output,
          const FlowValueType min_output = 0.0);
      SourceComponent(
          const std::string& id,
          const StreamType& output_stream,
          const Limits& limits);
      SourceComponent(
          const std::string& id,
          const StreamType& output_stream,
          std::unordered_map<
            std::string,
            std::unique_ptr<erin::fragility::Curve>> fragilities);
      SourceComponent(
          const std::string& id,
          const StreamType& output_stream,
          std::unordered_map<
            std::string,
            std::unique_ptr<erin::fragility::Curve>> fragilities,
          const FlowValueType max_output,
          const FlowValueType min_output = 0.0);
      SourceComponent(
          const std::string& id,
          const StreamType& output_stream,
          std::unordered_map<
            std::string,
            std::unique_ptr<erin::fragility::Curve>> fragilities,
          const Limits& limits);

      [[nodiscard]] std::unique_ptr<Component> clone() const override;
      PortsAndElements add_to_network(
          adevs::Digraph<FlowValueType, Time>& nw,
          const std::string& active_scenario,
          bool is_failed = false) const override;

    private:
      Limits limits;
  };

  ////////////////////////////////////////////////////////////
  // MuxerComponent
  class MuxerComponent : public Component
  {
    public:
      MuxerComponent(
          const std::string& id,
          const StreamType& stream,
          const std::vector<std::string>& input_ports,
          const std::vector<std::string>& output_ports,
          const MuxerDispatchStrategy strategy = MuxerDispatchStrategy::InOrder);
      MuxerComponent(
          const std::string& id,
          const StreamType& stream,
          const std::vector<std::string>& input_ports,
          const std::vector<std::string>& output_ports,
          std::unordered_map<
            std::string,
            std::unique_ptr<erin::fragility::Curve>> fragilities,
          const MuxerDispatchStrategy strategy = MuxerDispatchStrategy::InOrder);

      [[nodiscard]] std::unique_ptr<Component> clone() const override;
      PortsAndElements add_to_network(
          adevs::Digraph<FlowValueType, Time>& nw,
          const std::string& active_scenario,
          bool is_failed = false) const override;

    private:
      std::vector<std::string> input_ports;
      std::vector<std::string> output_ports;
      MuxerDispatchStrategy strategy;
  };
}

#endif // ERIN_COMPONENT_H
