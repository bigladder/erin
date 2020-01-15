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
  using fragility_map = std::unordered_map<
    std::string, // <-- the vulnerable_to identifier. E.g., wind_speed_mph,
                 //     inundation_depth_ft, etc.
    // vector of fragility curves that apply
    std::vector<std::unique_ptr<::erin::fragility::Curve>>>;

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
          StreamType output_stream,
          StreamType lossflow_stream);
      Component(
          std::string id,
          ComponentType type,
          StreamType input_stream,
          StreamType output_stream,
          StreamType lossflow_stream,
          fragility_map fragilities);
      Component& operator=(const Component&) = delete;
      Component(Component&&) = delete;
      Component& operator=(Component&&) = delete;
      virtual ~Component() = default;
      [[nodiscard]] virtual std::unique_ptr<Component> clone() const = 0;

      [[nodiscard]] const std::string& get_id() const { return id; }
      [[nodiscard]] ComponentType get_component_type() const { return component_type; }
      [[nodiscard]] const StreamType& get_input_stream() const { return input_stream; }
      [[nodiscard]] const StreamType& get_output_stream() const { return output_stream; }
      [[nodiscard]] const StreamType& get_lossflow_stream() const { return lossflow_stream; }
      [[nodiscard]] fragility_map clone_fragility_curves() const;
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
      void connect_source_to_sink_with_ports(
          adevs::Digraph<FlowValueType, Time>& nw,
          FlowElement* source,
          int source_port,
          FlowElement* sink,
          int sink_port,
          bool both_way) const;
      [[nodiscard]] bool base_is_equal(const Component& other) const;
      [[nodiscard]] std::string internals_to_string() const;

    private:
      std::string id;
      ComponentType component_type;
      StreamType input_stream;
      StreamType output_stream;
      StreamType lossflow_stream;
      fragility_map fragilities;
      bool has_fragilities;
  };

  bool operator==(const std::unique_ptr<Component>& a, const std::unique_ptr<Component>& b);
  bool operator!=(const std::unique_ptr<Component>& a, const std::unique_ptr<Component>& b);
  std::ostream& operator<<(std::ostream& os, const std::unique_ptr<Component>& n);

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
          fragility_map fragilities);
      PortsAndElements add_to_network(
          adevs::Digraph<FlowValueType, Time>& nw,
          const std::string& active_scenario,
          bool is_failed = false) const override;
      [[nodiscard]] std::unique_ptr<Component> clone() const override;

      friend bool operator==(const LoadComponent& a, const LoadComponent& b);
      friend bool operator!=(const LoadComponent& a, const LoadComponent& b);
      friend std::ostream& operator<<(std::ostream& os, const LoadComponent& n);

    private:
      std::unordered_map<std::string,std::vector<LoadItem>> loads_by_scenario;
  };

  bool operator==(const LoadComponent& a, const LoadComponent& b);
  bool operator!=(const LoadComponent& a, const LoadComponent& b);
  std::ostream& operator<<(std::ostream& os, const LoadComponent& n);

  ////////////////////////////////////////////////////////////
  // Limits
  class Limits
  {
    public:
      Limits();
      Limits(FlowValueType max);
      Limits(FlowValueType min, FlowValueType max);

      [[nodiscard]] bool get_is_limited() const { return is_limited; }
      [[nodiscard]] FlowValueType get_min() const { return minimum; }
      [[nodiscard]] FlowValueType get_max() const { return maximum; }

      friend bool operator==(const Limits& a, const Limits& b);
      friend bool operator!=(const Limits& a, const Limits& b);
      friend std::ostream& operator<<(std::ostream& os, const Limits& n);

    private:
      bool is_limited;
      FlowValueType minimum;
      FlowValueType maximum;
  };

  bool operator==(const Limits& a, const Limits& b);
  bool operator!=(const Limits& a, const Limits& b);
  std::ostream& operator<<(std::ostream& os, const Limits& n);

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
          fragility_map fragilities);
      SourceComponent(
          const std::string& id,
          const StreamType& output_stream,
          fragility_map fragilities,
          const FlowValueType max_output,
          const FlowValueType min_output = 0.0);
      SourceComponent(
          const std::string& id,
          const StreamType& output_stream,
          fragility_map fragilities,
          const Limits& limits);

      [[nodiscard]] std::unique_ptr<Component> clone() const override;
      PortsAndElements add_to_network(
          adevs::Digraph<FlowValueType, Time>& nw,
          const std::string& active_scenario,
          bool is_failed = false) const override;

      friend bool operator==(const SourceComponent& a, const SourceComponent& b);
      friend bool operator!=(const SourceComponent& a, const SourceComponent& b);
      friend std::ostream& operator<<(std::ostream& os, const SourceComponent& n);

    private:
      Limits limits;
  };

  bool operator==(const SourceComponent& a, const SourceComponent& b);
  bool operator!=(const SourceComponent& a, const SourceComponent& b);
  std::ostream& operator<<(std::ostream& os, const SourceComponent& n);

  ////////////////////////////////////////////////////////////
  // MuxerComponent
  class MuxerComponent : public Component
  {
    public:
      MuxerComponent(
          const std::string& id,
          const StreamType& stream,
          const int num_inflows,
          const int num_outflows,
          const MuxerDispatchStrategy strategy
            = MuxerDispatchStrategy::InOrder,
          const MuxerDispatchStrategy output_strategy
            = MuxerDispatchStrategy::Distribute);
      MuxerComponent(
          const std::string& id,
          const StreamType& stream,
          const int num_inflows,
          const int num_outflows,
          fragility_map fragilities,
          const MuxerDispatchStrategy strategy
            = MuxerDispatchStrategy::InOrder,
          const MuxerDispatchStrategy output_strategy
            = MuxerDispatchStrategy::InOrder);

      [[nodiscard]] int get_num_inflows() const { return num_inflows; }
      [[nodiscard]] int get_num_outflows() const { return num_outflows; }

      [[nodiscard]] std::unique_ptr<Component> clone() const override;
      PortsAndElements add_to_network(
          adevs::Digraph<FlowValueType, Time>& nw,
          const std::string& active_scenario,
          bool is_failed = false) const override;

      friend bool operator==(const MuxerComponent& a, const MuxerComponent& b);
      friend bool operator!=(const MuxerComponent& a, const MuxerComponent& b);
      friend std::ostream& operator<<(std::ostream& os, const MuxerComponent& n);

    private:
      int num_inflows;
      int num_outflows;
      MuxerDispatchStrategy strategy;
      MuxerDispatchStrategy output_strategy;
  };

  bool operator==(const MuxerComponent& a, const MuxerComponent& b);
  bool operator!=(const MuxerComponent& a, const MuxerComponent& b);
  std::ostream& operator<<(std::ostream& os, const MuxerComponent& n);

  ////////////////////////////////////////////////////////////
  // ConverterComponent
  class ConverterComponent : public Component
  {
    public:
      ConverterComponent(
          const std::string& id,
          const StreamType& input_stream,
          const StreamType& output_stream,
          const StreamType& lossflow_stream,
          const FlowValueType& const_eff);

      [[nodiscard]] std::unique_ptr<Component> clone() const override;
      PortsAndElements add_to_network(
          adevs::Digraph<FlowValueType, Time>& nw,
          const std::string& active_scenario,
          bool is_failed = false) const override;

      friend bool operator==(const ConverterComponent& a, const ConverterComponent& b);
      friend std::ostream& operator<<(std::ostream& os, const ConverterComponent& n);

    private:
      FlowValueType const_eff;
  };

  bool operator==(const ConverterComponent& a, const ConverterComponent& b);
  bool operator!=(const ConverterComponent& a, const ConverterComponent& b);
  std::ostream& operator<<(std::ostream& os, const ConverterComponent& n);
}

#endif // ERIN_COMPONENT_H
