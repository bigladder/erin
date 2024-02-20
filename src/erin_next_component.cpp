#include "erin_next/erin_next_component.h"
#include "erin_next/erin_next_toml.h"
#include <map>
#include <optional>

namespace erin_next
{
	// TODO: remove Model& m as a separate parameter; it is now part of
	// Simulation& s (i.e., s.TheModel)
	Result
	ParseSingleComponent(
		Simulation& s,
		Model& m,
		toml::table const& table,
		std::string const& tag)
	{
		std::string fullTableName = "components." + tag;
		if (!table.contains("type"))
		{
			std::cout << "[" << fullTableName << "] "
				<< "required field 'type' not present" << std::endl;
			return Result::Failure;
		}
		std::optional<ComponentType> maybeCompType =
			TagToComponentType(table.at("type").as_string());
		if (!maybeCompType.has_value())
		{
			std::cout << "[" << fullTableName << "] "
				<< "unable to parse table type from '"
				<< table.at("type").as_string() << "'" << std::endl;
			return Result::Failure;
		}
		ComponentType ct = maybeCompType.value();
		size_t id = {};
		std::string inflow{};
		size_t inflowId = {};
		std::string outflow{};
		size_t outflowId = {};
		if (table.contains("outflow"))
		{
			auto maybe = TOMLTable_ParseString(table, "outflow", fullTableName);
			if (maybe.has_value())
			{
				outflow = maybe.value();
				outflowId = Simulation_RegisterFlow(s, outflow);
			}
		}
		if (table.contains("inflow"))
		{
			auto maybe = TOMLTable_ParseString(table, "inflow", fullTableName);
			if (maybe.has_value())
			{
				inflow = maybe.value();
				inflowId = Simulation_RegisterFlow(s, inflow);
			}
		}
		if (table.contains("flow"))
		{
			auto maybe = TOMLTable_ParseString(table, "flow", fullTableName);
			if (maybe.has_value())
			{
				inflow = maybe.value();
				inflowId = Simulation_RegisterFlow(s, inflow);
				outflow = maybe.value();
				outflowId = Simulation_RegisterFlow(s, outflow);
			}
		}
		// TODO: parse failure modes

		switch (ct)
		{
			case (ComponentType::ConstantSourceType):
			{
				// TODO: need to get the max available from constant source
				// for now, we can use uint32_t max?
				id = Model_AddConstantSource(
					m, std::numeric_limits<uint32_t>::max(),
					outflowId, tag);
			} break;
			case (ComponentType::ScheduleBasedLoadType):
			{
				if (!table.contains("loads_by_scenario"))
				{
					std::cout << "[" << fullTableName << "] "
						<< "missing required field 'loads_by_scenario'"
						<< std::endl;
					return Result::Failure;
				}
				if (!table.at("loads_by_scenario").is_table())
				{
					std::cout << "[" << fullTableName << "] "
						<< "'loads_by_scenario' exists but is not a table"
						<< std::endl;
					return Result::Failure;
				}
				toml::table const& lbs =
					table.at("loads_by_scenario").as_table();
				std::map<size_t, size_t> scenarioIdToLoadId = {};
				for (auto it = lbs.cbegin(); it != lbs.cend(); ++it)
				{
					std::string const& scenarioTag = it->first;
					size_t scenarioId =
						Simulation_RegisterScenario(s, scenarioTag);
					if (it->second.is_string())
					{
						std::string const& loadTag = it->second.as_string();
						std::optional<size_t> loadId =
							Simulation_GetLoadIdByTag(s, loadTag);
						if (loadId.has_value())
						{
							scenarioIdToLoadId.insert(
								{ scenarioId, loadId.value() });
						}
					}
				}
				std::vector<TimeAndAmount> emptyLoads = {};
				id = Model_AddScheduleBasedLoad(
					m, emptyLoads, scenarioIdToLoadId, inflowId, tag);
			} break;
			case (ComponentType::MuxType):
			{
				auto numInflows =
					TOMLTable_ParseInteger(
						table, "num_inflows", fullTableName);
				auto numOutflows =
					TOMLTable_ParseInteger(
						table, "num_outflows", fullTableName);
				if (!numInflows.has_value())
				{
					std::cout << "[" << fullTableName << "] "
						<< "num_inflows doesn't appear or is not an integer"
						<< std::endl;
					return Result::Failure;
				}
				if (!numOutflows.has_value())
				{
					std::cout << "[" << fullTableName << "] "
						<< "num_outflows doesn't appear or is not an integer"
						<< std::endl;
					return Result::Failure;
				}
				// TODO: add flowId -- check inflowId == outflowId
				if (inflowId != outflowId)
				{
					std::cout << "[" << fullTableName << "] "
						<< "a mux component must have the same inflow type "
						<< "as outflow type; we have inflow = '"
						<< s.FlowTypeMap.Type[inflowId] << "'; outflow = '"
						<< s.FlowTypeMap.Type[outflowId] << "'"
						<< std::endl;
					return Result::Failure;
				}
				id = Model_AddMux(
					m, numInflows.value(), numOutflows.value(), outflowId, tag);
			} break;
			default:
			{
				std::cout << "Unhandled component type: "
					<< ToString(ct) << std::endl;
				throw std::runtime_error{ "Unhandled component type" };
			}
		}
		if (table.contains("fragility_modes"))
		{
			if (!table.at("fragility_modes").is_array())
			{
				std::cout << "[" << fullTableName << "] "
					<< "fragility_modes must be an array of string"
					<< std::endl;
				return Result::Failure;
			}
			std::vector<toml::value> const& fms =
				table.at("fragility_modes").as_array();
			for (size_t fmIdx = 0; fmIdx < fms.size(); ++fmIdx)
			{
				if (!fms[fmIdx].is_string())
				{
					std::cout << "[" << fullTableName << "] "
						<< "fragility_modes[" << fmIdx << "] must be string"
						<< std::endl;
					return Result::Failure;
				}
				std::string const& fmTag = fms[fmIdx].as_string();
				bool existingFragilityMode = false;
				size_t fmId;
				for (fmId = 0; fmId < s.FragilityModes.Tags.size(); ++fmId)
				{
					if (s.FragilityModes.Tags[fmId] == fmTag)
					{
						existingFragilityMode = true;
						break;
					}
				}
				if (!existingFragilityMode)
				{
					fmId = s.FragilityModes.Tags.size();
					s.FragilityModes.Tags.push_back(fmTag);
					// NOTE: add placeholder default data
					s.FragilityModes.FragilityCurveId.push_back(0);
					s.FragilityModes.RepairDistIds.push_back({});
				}
				s.ComponentFragilities.ComponentIds.push_back(id);
				s.ComponentFragilities.FragilityModeIds.push_back(fmId);
			}
		}
		return Result::Success;
	}

	// TODO: remove model as a parameter -- it is part of Simulation
	Result
	ParseComponents(Simulation& s, Model& m, toml::table const& table)
	{
		for (auto it = table.cbegin(); it != table.cend(); ++it)
		{
			auto result =
				ParseSingleComponent(s, m, it->second.as_table(), it->first);
			if (result == Result::Failure)
			{
				return Result::Failure;
			}
		}
		return Result::Success;
	}
}