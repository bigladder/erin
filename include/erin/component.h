/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_COMPONENT_H
#define ERIN_COMPONENT_H
#include "erin/type.h"
#include "erin/element.h"
#include "erin/fragility.h"
#include "erin/stream.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ERIN
{
  ////////////////////////////////////////////////////////////
  // Component
  // base class for all types of components
  class Component
  {
    public:
      Component(const Component&) = delete;
      Component& operator=(const Component&) = delete;
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
      virtual ~Component() = default;
      virtual std::unique_ptr<Component> clone() const = 0;

      void add_input(std::shared_ptr<Component>& c);
      [[nodiscard]] const std::string& get_id() const { return id; }
      [[nodiscard]] ComponentType get_component_type() const { return component_type; }
      [[nodiscard]] const StreamType& get_input_stream() const { return input_stream; }
      [[nodiscard]] const StreamType& get_output_stream() const { return output_stream; }
      std::unordered_map<std::string, std::unique_ptr<erin::fragility::Curve>>
        clone_fragility_curves() const;
      [[nodiscard]] bool is_fragile() const { return has_fragilities; }
      std::vector<double> apply_intensities(
          const std::unordered_map<std::string, double>& intensities);

      virtual std::unordered_set<FlowElement*>
        add_to_network(
            adevs::Digraph<FlowValueType>& nw,
            const std::string& active_scenario,
            bool is_failed = false) = 0;
      FlowElement* get_connecting_element();

    protected:
      virtual FlowElement* create_connecting_element() = 0;
      [[nodiscard]] const std::vector<std::shared_ptr<Component>>& get_inputs() const {
        return inputs;
      }
      void connect_source_to_sink(
          adevs::Digraph<FlowValueType>& nw,
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
      std::vector<std::shared_ptr<Component>> inputs;
      FlowElement* connecting_element;
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
            std::string, std::unique_ptr<erin::fragility::Curve>>
            fragilities);
      std::unordered_set<FlowElement*>
        add_to_network(
            adevs::Digraph<FlowValueType>& nw,
            const std::string& active_scenario,
            bool is_failed = false) override;
      std::unique_ptr<Component> clone() const override;

    protected:
      FlowElement* create_connecting_element() override;

    private:
      std::unordered_map<std::string,std::vector<LoadItem>> loads_by_scenario;
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
          std::unordered_map<
            std::string,
            std::unique_ptr<erin::fragility::Curve>> fragilities); 
      std::unique_ptr<Component> clone() const override;
      std::unordered_set<FlowElement*>
        add_to_network(
            adevs::Digraph<FlowValueType>& nw,
            const std::string& active_scenario,
            bool is_failed = false) override;

    protected:
      FlowElement* create_connecting_element() override;
  };
}

#endif // ERIN_COMPONENT_H
