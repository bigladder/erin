/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef ERIN_COMPONENT_H
#define ERIN_COMPONENT_H
#include "erin/type.h"
#include "erin/element.h"
#include "erin/fragility.h"
#include "erin/port.h"
#include "erin/reliability.h"
#include "erin/stream.h"
#include "adevs.h"
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ERIN
{
  struct FragilityCurveAndRepair
  {
    std::unique_ptr<erin::fragility::Curve> curve{};
    std::int64_t repair_dist_id{erin::fragility::no_repair_distribution};
  };

  std::ostream& operator<<(std::ostream& os, const FragilityCurveAndRepair& fcar);

  using fragility_map = std::unordered_map<
    std::string, // <-- the vulnerable_to identifier. E.g., wind_speed_mph,
                 //     inundation_depth_ft, etc.
    // vector of fragility curves that apply
    std::vector<FragilityCurveAndRepair>>;

  /**
   * Holds a FlowElement and the Port it should be connected on
   */
  struct ElementPort
  {
    FlowElement* element{nullptr};
    int port{0};
  };

  /**
   * Holds Ports and FlowElement pointers from a return of Component.add_to_network(...)
   */
  struct PortsAndElements
  {
    std::unordered_map<::erin::port::Type, std::vector<ElementPort>> port_map{};
    std::unordered_set<FlowElement*> elements_added{};
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
          std::string input_stream,
          std::string output_stream,
          std::string lossflow_stream);
      Component(
          std::string id,
          ComponentType type,
          std::string input_stream,
          std::string output_stream,
          std::string lossflow_stream,
          fragility_map fragilities);
      Component& operator=(const Component&) = delete;
      Component(Component&&) = delete;
      Component& operator=(Component&&) = delete;
      virtual ~Component() = default;
      [[nodiscard]] virtual std::unique_ptr<Component> clone() const = 0;

      [[nodiscard]] const std::string& get_id() const { return id; }
      [[nodiscard]] ComponentType get_component_type() const { return component_type; }
      [[nodiscard]] const std::string& get_input_stream() const { return input_stream; }
      [[nodiscard]] const std::string& get_output_stream() const { return output_stream; }
      [[nodiscard]] const std::string& get_lossflow_stream() const { return lossflow_stream; }
      [[nodiscard]] fragility_map clone_fragility_curves() const;
      [[nodiscard]] bool is_fragile() const { return has_fragilities; }
      // TODO: consider moving this elsewhere
      std::vector<erin::fragility::FailureProbAndRepair> apply_intensities(
          const std::unordered_map<std::string, double>& intensities);

      virtual PortsAndElements add_to_network(
          adevs::Digraph<FlowValueType, Time>& nw,
          const std::string& active_scenario,
          bool is_failed = false,
          const std::vector<TimeState>& reliability_schedule = {}) const = 0;

    protected:
      void connect_source_to_sink(
          adevs::Digraph<FlowValueType, Time>& nw,
          FlowElement* source,
          FlowElement* sink,
          bool both_way,
          const std::string& stream) const;
      void connect_source_to_sink_with_ports(
          adevs::Digraph<FlowValueType, Time>& nw,
          FlowElement* source,
          int source_port,
          FlowElement* sink,
          int sink_port,
          bool both_way,
          const std::string& stream) const;
      [[nodiscard]] bool base_is_equal(const Component& other) const;
      [[nodiscard]] std::string internals_to_string() const;

    private:
      std::string id;
      ComponentType component_type;
      std::string input_stream;
      std::string output_stream;
      std::string lossflow_stream;
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
          const std::string& input_stream,
          std::unordered_map<std::string, std::vector<LoadItem>>
            loads_by_scenario);
      LoadComponent(
          const std::string& id,
          const std::string& input_stream,
          std::unordered_map<std::string, std::vector<LoadItem>>
            loads_by_scenario,
          fragility_map fragilities);
      PortsAndElements add_to_network(
          adevs::Digraph<FlowValueType, Time>& nw,
          const std::string& active_scenario,
          bool is_failed = false,
          const std::vector<TimeState>& reliability_schedule = {}) const override;
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
      explicit Limits(FlowValueType max);
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
          const std::string& output_stream);
      SourceComponent(
          const std::string& id,
          const std::string& output_stream,
          const FlowValueType max_output,
          const FlowValueType min_output = 0.0);
      SourceComponent(
          const std::string& id,
          const std::string& output_stream,
          const Limits& limits);
      SourceComponent(
          const std::string& id,
          const std::string& output_stream,
          fragility_map fragilities);
      SourceComponent(
          const std::string& id,
          const std::string& output_stream,
          fragility_map fragilities,
          const FlowValueType max_output,
          const FlowValueType min_output = 0.0);
      SourceComponent(
          const std::string& id,
          const std::string& output_stream,
          fragility_map fragilities,
          const Limits& limits);

      [[nodiscard]] std::unique_ptr<Component> clone() const override;
      PortsAndElements add_to_network(
          adevs::Digraph<FlowValueType, Time>& nw,
          const std::string& active_scenario,
          bool is_failed = false,
          const std::vector<TimeState>& reliability_schedule = {}) const override;

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
          const std::string& stream,
          const int num_inflows,
          const int num_outflows,
          const MuxerDispatchStrategy output_strategy
            = MuxerDispatchStrategy::InOrder);
      MuxerComponent(
          const std::string& id,
          const std::string& stream,
          const int num_inflows,
          const int num_outflows,
          fragility_map fragilities,
          const MuxerDispatchStrategy output_strategy
            = MuxerDispatchStrategy::InOrder);

      [[nodiscard]] int get_num_inflows() const { return num_inflows; }
      [[nodiscard]] int get_num_outflows() const { return num_outflows; }

      [[nodiscard]] std::unique_ptr<Component> clone() const override;
      PortsAndElements add_to_network(
          adevs::Digraph<FlowValueType, Time>& nw,
          const std::string& active_scenario,
          bool is_failed = false,
          const std::vector<TimeState>& reliability_schedule = {}) const override;

      friend bool operator==(const MuxerComponent& a, const MuxerComponent& b);
      friend bool operator!=(const MuxerComponent& a, const MuxerComponent& b);
      friend std::ostream& operator<<(std::ostream& os, const MuxerComponent& n);

    private:
      int num_inflows;
      int num_outflows;
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
          const std::string& input_stream,
          const std::string& output_stream,
          const std::string& lossflow_stream,
          const FlowValueType& const_eff);

      ConverterComponent(
          const std::string& id,
          const std::string& input_stream,
          const std::string& output_stream,
          const std::string& lossflow_stream,
          const FlowValueType& const_eff,
          fragility_map fragilities);

      [[nodiscard]] std::unique_ptr<Component> clone() const override;
      PortsAndElements add_to_network(
          adevs::Digraph<FlowValueType, Time>& nw,
          const std::string& active_scenario,
          bool is_failed = false,
          const std::vector<TimeState>& reliability_schedule = {}) const override;

      friend bool operator==(const ConverterComponent& a, const ConverterComponent& b);
      friend std::ostream& operator<<(std::ostream& os, const ConverterComponent& n);

    private:
      FlowValueType const_eff;
  };

  bool operator==(const ConverterComponent& a, const ConverterComponent& b);
  bool operator!=(const ConverterComponent& a, const ConverterComponent& b);
  std::ostream& operator<<(std::ostream& os, const ConverterComponent& n);

  ////////////////////////////////////////////////////////////
  // PassThroughComponent
  class PassThroughComponent : public Component
  {
    public:
      PassThroughComponent(
          const std::string& id,
          const std::string& stream);
      PassThroughComponent(
          const std::string& id,
          const std::string& stream,
          const Limits& limits);
      PassThroughComponent(
          const std::string& id,
          const std::string& stream,
          fragility_map fragilities);
      PassThroughComponent(
          const std::string& id,
          const std::string& stream,
          const Limits& limits,
          fragility_map fragilities);

      [[nodiscard]] std::unique_ptr<Component> clone() const override;
      PortsAndElements add_to_network(
          adevs::Digraph<FlowValueType, Time>& nw,
          const std::string& active_scenario,
          bool is_failed = false,
          const std::vector<TimeState>& reliability_schedule = {}) const override;

      friend bool operator==(const PassThroughComponent& a, const PassThroughComponent& b);
      friend std::ostream& operator<<(std::ostream& os, const PassThroughComponent& n);

    private:
      Limits limits;
  };

  bool operator==(const PassThroughComponent& a, const PassThroughComponent& b);
  bool operator!=(const PassThroughComponent& a, const PassThroughComponent& b);
  std::ostream& operator<<(std::ostream& os, const PassThroughComponent& n);

  ////////////////////////////////////////////////////////////
  // StorageComponent
  class StorageComponent : public Component
  {
    public:
      StorageComponent(
          const std::string& id,
          const std::string& stream,
          FlowValueType capacity,
          FlowValueType max_charge_rate);
      StorageComponent(
          const std::string& id,
          const std::string& stream,
          FlowValueType capacity,
          FlowValueType max_charge_rate,
          fragility_map fragilities);
      StorageComponent(
          const std::string& id,
          const std::string& stream,
          FlowValueType capacity,
          FlowValueType max_charge_rate,
          fragility_map fragilities,
          FlowValueType init_soc);

      [[nodiscard]] std::unique_ptr<Component> clone() const override;
      PortsAndElements add_to_network(
          adevs::Digraph<FlowValueType, Time>& nw,
          const std::string& active_scenario,
          bool is_failed = false,
          const std::vector<TimeState>& reliability_schedule = {}) const override;

      friend bool operator==(const StorageComponent& a, const StorageComponent& b);
      friend std::ostream& operator<<(std::ostream& os, const StorageComponent& n);

    private:
      FlowValueType capacity;
      FlowValueType max_charge_rate;
      FlowValueType init_soc;
  };

  bool operator==(const StorageComponent& a, const StorageComponent& b);
  bool operator!=(const StorageComponent& a, const StorageComponent& b);
  std::ostream& operator<<(std::ostream& os, const StorageComponent& n);

  ////////////////////////////////////////////////////////////
  // UncontrolledSourceComponent
  class UncontrolledSourceComponent : public Component
  {
    public:
      UncontrolledSourceComponent(
          const std::string& id,
          const std::string& outflow,
          std::unordered_map<std::string, std::vector<LoadItem>>
            supply_by_scenario);
      UncontrolledSourceComponent(
          const std::string& id,
          const std::string& output_stream,
          std::unordered_map<std::string, std::vector<LoadItem>>
            supply_by_scenario,
          fragility_map fragilities);
      PortsAndElements add_to_network(
          adevs::Digraph<FlowValueType, Time>& nw,
          const std::string& active_scenario,
          bool is_failed = false,
          const std::vector<TimeState>& reliability_schedule = {}) const override;
      [[nodiscard]] std::unique_ptr<Component> clone() const override;

      friend bool operator==(
          const UncontrolledSourceComponent& a,
          const UncontrolledSourceComponent& b);
      friend bool operator!=(
          const UncontrolledSourceComponent& a,
          const UncontrolledSourceComponent& b);
      friend std::ostream& operator<<(
          std::ostream& os,
          const UncontrolledSourceComponent& n);

    private:
      std::unordered_map<std::string,std::vector<LoadItem>> supply_by_scenario;
  };

  bool operator==(
      const UncontrolledSourceComponent& a,
      const UncontrolledSourceComponent& b);
  bool operator!=(
      const UncontrolledSourceComponent& a,
      const UncontrolledSourceComponent& b);
  std::ostream& operator<<(
      std::ostream& os,
      const UncontrolledSourceComponent& n);

  ////////////////////////////////////////////////////////////
  // MoverComponent
  class MoverComponent : public Component
  {
    public:
      //
      //    id, inflow0, inflow1, outflow, COP, std::move(frags));
      MoverComponent(
          const std::string& id,
          const std::string& inflow0,
          const std::string& inflow1,
          const std::string& outflow,
          FlowValueType COP);
      MoverComponent(
          const std::string& id,
          const std::string& inflow0,
          const std::string& inflow1,
          const std::string& outflow,
          FlowValueType COP,
          fragility_map fragilities);
      PortsAndElements add_to_network(
          adevs::Digraph<FlowValueType, Time>& nw,
          const std::string& active_scenario,
          bool is_failed = false,
          const std::vector<TimeState>& reliability_schedule = {}) const override;
      [[nodiscard]] std::unique_ptr<Component> clone() const override;

      friend bool operator==(
          const MoverComponent& a,
          const MoverComponent& b);
      friend bool operator!=(
          const MoverComponent& a,
          const MoverComponent& b);
      friend std::ostream& operator<<(
          std::ostream& os,
          const MoverComponent& n);

    private:
      std::string inflow1;
      FlowValueType COP;
  };

  bool operator==(const MoverComponent& a, const MoverComponent& b);
  bool operator!=(const MoverComponent& a, const MoverComponent& b);
  std::ostream& operator<<(std::ostream& os, const MoverComponent& n);
}

#endif // ERIN_COMPONENT_H
