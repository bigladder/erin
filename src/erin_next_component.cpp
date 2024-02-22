#include "erin_next/erin_next_component.h"
#include "erin_next/erin_next_toml.h"
#include "erin_next/erin_next_utils.h"
#include <map>
#include <optional>
#include <unordered_set>
#include <unordered_map>

namespace erin_next
{

	Result
	ParseSingleComponent(
		Simulation& s,
		toml::table const& table,
		std::string const& tag)
	{
		std::string fullTableName = "components." + tag;
		if (!table.contains("type"))
		{
			WriteErrorMessage(fullTableName,
				"required field 'type' not present");
			return Result::Failure;
		}
		std::optional<ComponentType> maybeCompType =
			TagToComponentType(table.at("type").as_string());
		if (!maybeCompType.has_value())
		{
			WriteErrorMessage(fullTableName,
				"unable to parse component type '"
				+ std::string{table.at("type").as_string()} + "'");
			return Result::Failure;
		}
		ComponentType ct = maybeCompType.value();
		size_t id = {};
		std::string inflow{};
		size_t inflowId = {};
		std::string outflow{};
		size_t outflowId = 0;
		std::string lossflow{};
		size_t lossflowId = 0;
		if (table.contains("outflow"))
		{
			auto maybe = TOMLTable_ParseString(table, "outflow", fullTableName);
			if (!maybe.has_value())
			{
				WriteErrorMessage(fullTableName,
					"unable to parse 'outflow' as string");
				return Result::Failure;
			}
			outflow = maybe.value();
			outflowId = Simulation_RegisterFlow(s, outflow);
		}
		if (table.contains("inflow"))
		{
			auto maybe = TOMLTable_ParseString(table, "inflow", fullTableName);
			if (!maybe.has_value())
			{
				WriteErrorMessage(fullTableName,
					"unable to parse 'inflow' as string");
				return Result::Failure;
			}
			inflow = maybe.value();
			inflowId = Simulation_RegisterFlow(s, inflow);
		}
		if (table.contains("flow"))
		{
			auto maybe = TOMLTable_ParseString(table, "flow", fullTableName);
			if (!maybe.has_value())
			{
				WriteErrorMessage(fullTableName,
					"unable to parse 'lossflow' as string");
				return Result::Failure;
			}
			inflow = maybe.value();
			inflowId = Simulation_RegisterFlow(s, inflow);
			outflow = maybe.value();
			outflowId = Simulation_RegisterFlow(s, outflow);
		}
		if (table.contains("lossflow"))
		{
			auto maybe =
				TOMLTable_ParseString(table, "lossflow", fullTableName);
			if (!maybe.has_value())
			{
				WriteErrorMessage(fullTableName,
					"unable to parse 'lossflow' as string");
				return Result::Failure;
			}
			lossflow = maybe.value();
			lossflowId = Simulation_RegisterFlow(s, lossflow);
		}
		// TODO: parse optional rate_unit up here; default to W?
		PowerUnit rateUnit = PowerUnit::Watt;
		if (table.contains("rate_unit"))
		{
			auto maybeRateUnitStr = TOMLTable_ParseString(
				table, "rate_unit", fullTableName);
			if (!maybeRateUnitStr.has_value())
			{
				WriteErrorMessage(fullTableName,
					"unable to parse field 'rate_unit' as string");
				return Result::Failure;
			}
			std::string rateUnitStr = maybeRateUnitStr.value();
			auto maybeRateUnit =
				TagToPowerUnit(rateUnitStr);
			if (!maybeRateUnit.has_value())
			{
				WriteErrorMessage(fullTableName,
					"unable to understand rate_unit '" + rateUnitStr + "'");
				return Result::Failure;
			}
			rateUnit = maybeRateUnit.value();
		}
		// TODO: parse failure modes
		switch (ct)
		{
			case (ComponentType::ConstantSourceType):
			{
				uint32_t maxAvailable =
					std::numeric_limits<uint32_t>::max();
				if (table.contains("max_outflow"))
				{
					auto maybe = TOMLTable_ParseDouble(
						table, "max_outflow", fullTableName);
					if (!maybe.has_value())
					{
						WriteErrorMessage(fullTableName,
							"unable to parse 'max_outflow' as number");
						return Result::Failure;
					}
					double maxAvailableReal = maybe.value();
					maxAvailable = static_cast<uint32_t>(
						Power_ToWatt(maxAvailableReal, rateUnit));
				}
				id = Model_AddConstantSource(
					s.TheModel, maxAvailable, outflowId, tag);
			} break;
			case (ComponentType::ScheduleBasedLoadType):
			{
				if (!table.contains("loads_by_scenario"))
				{
					WriteErrorMessage(fullTableName,
						"missing required field 'loads_by_scenario'");
					return Result::Failure;
				}
				if (!table.at("loads_by_scenario").is_table())
				{
					WriteErrorMessage(fullTableName,
						"'loads_by_scenario' must be a table");
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
					s.TheModel, emptyLoads, scenarioIdToLoadId, inflowId, tag);
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
					WriteErrorMessage(fullTableName,
						"num_inflows doesn't appear or is not an integer");
					return Result::Failure;
				}
				if (!numOutflows.has_value())
				{
					WriteErrorMessage(fullTableName,
						"num_outflows doesn't appear or is not an integer");
					return Result::Failure;
				}
				if (inflowId != outflowId)
				{
					WriteErrorMessage(fullTableName,
						"a mux component must have the same inflow type "
						"as outflow type; we have inflow = '"
						+ s.FlowTypeMap.Type[inflowId]
						+ "'; outflow = '"
						+ s.FlowTypeMap.Type[outflowId]
						+ "'");
					return Result::Failure;
				}
				id = Model_AddMux(
					s.TheModel, numInflows.value(), numOutflows.value(), outflowId, tag);
			} break;
			case (ComponentType::ConstantEfficiencyConverterType):
			{
				auto maybeEfficiency = TOMLTable_ParseDouble(
					table, "constant_efficiency", fullTableName);
				if (!maybeEfficiency.has_value())
				{
					WriteErrorMessage(fullTableName,
						"required field 'constant_efficiency' not found");
					return Result::Failure;
				}
				double efficiency = maybeEfficiency.value();
				if (efficiency <= 0.0)
				{
					WriteErrorMessage(
						fullTableName, "efficiency must be > 0.0");
					return Result::Failure;
				}
				// TODO: re-enable this after we have dedicated COP components
				// if (efficiency > 1.0)
				// {
				// 	WriteErrorMessage(
				// 		fullTableName, "efficiency must be <= 1.0");
				// 	return Result::Failure;
				// }
				auto const compIdAndWasteConn =
					Model_AddConstantEfficiencyConverter(
						s.TheModel, efficiency,
						inflowId, outflowId, lossflowId, tag);
				id = compIdAndWasteConn.Id;
			} break;
			case (ComponentType::PassThroughType):
			{
				if (inflowId != outflowId)
				{
					WriteErrorMessage(fullTableName,
						"inflow type must equal outflow type for pass-through");
					return Result::Failure;
				}
				id = Model_AddPassThrough(s.TheModel, inflowId, tag);
			} break;
			case (ComponentType::StoreType):
			{
				std::unordered_set<std::string> requiredStoreFields{
					"capacity", "capacity_unit", "rate_unit",
					"max_charge", "max_discharge", "type",
				};
				std::unordered_set<std::string> optionalStoreFields{};
				std::unordered_map<std::string, std::string> defaultStoreFields{
					{ "charge_at_soc", "0.8" },
					{ "flow", "" },
					{ "init_soc", "1.0" },
				};
				if (!TOMLTable_IsValid(
						table,
						requiredStoreFields,
						optionalStoreFields,
						defaultStoreFields,
						fullTableName,
						true))
				{
					return Result::Failure;
				}
				if (inflowId != outflowId)
				{
					WriteErrorMessage(fullTableName,
						"inflow type must equal outflow type for store");
					return Result::Failure;
				}
				auto maybeCapacity =
					TOMLTable_ParseDouble(table, "capacity", fullTableName);
				if (!maybeCapacity.has_value())
				{
					WriteErrorMessage(fullTableName,
						"unable to parse 'capacity' as double");
					return Result::Failure;
				}
				double capacityReal = maybeCapacity.value();
				if (capacityReal <= 0.0)
				{
					WriteErrorMessage(fullTableName,
						"capacity must be greater than 0");
					return Result::Failure;
				}
				auto maybeCapacityUnitStr =
					TOMLTable_ParseString(
						table, "capacity_unit", fullTableName);
				if (!maybeCapacityUnitStr.has_value())
				{
					WriteErrorMessage(fullTableName,
						"unable to parse 'capacity_unit' as string");
					return Result::Failure;
				}
				std::string capacityUnitStr = maybeCapacityUnitStr.value();
				auto maybeCapacityUnit = TagToEnergyUnit(capacityUnitStr);
				if (!maybeCapacityUnit.has_value())
				{
					WriteErrorMessage(fullTableName,
						"unable to understand capacity_unit of '"
						+ capacityUnitStr + "'");
					// TODO: list available capacity units
					return Result::Failure;
				}
				EnergyUnit capacityUnit = maybeCapacityUnit.value();
				uint32_t capacity = static_cast<uint32_t>(
					Energy_ToJoules(capacityReal, capacityUnit));
				if (capacity == 0)
				{
					WriteErrorMessage(fullTableName,
						"capacity must be greater than 0");
					return Result::Failure;
				}
				auto maybeMaxCharge =
					TOMLTable_ParseDouble(table, "max_charge", fullTableName);
				if (!maybeMaxCharge.has_value())
				{
					WriteErrorMessage(fullTableName,
						"unable to parse 'max_charge' as double");
					return Result::Failure;
				}
				double maxChargeReal = maybeMaxCharge.value();
				uint32_t maxCharge = static_cast<uint32_t>(
					Power_ToWatt(maxChargeReal, rateUnit));
				auto maybeMaxDischarge =
					TOMLTable_ParseDouble(
						table, "max_discharge", fullTableName);
				if (!maybeMaxDischarge.has_value())
				{
					WriteErrorMessage(fullTableName,
						"unable to parse 'max_discharge' as double");
					return Result::Failure;
				}
				double maxDischargeReal = maybeMaxDischarge.value();
				uint32_t maxDischarge = static_cast<uint32_t>(
					Power_ToWatt(maxDischargeReal, rateUnit));
				double chargeAtSoc =
					std::stod(defaultStoreFields.at("charge_at_soc"));
				if (table.contains("charge_at_soc"))
				{
					auto maybe = TOMLTable_ParseDouble(
						table, "charge_at_soc", fullTableName);
					if (!maybe.has_value())
					{
						WriteErrorMessage(fullTableName,
							"unable to parse 'charge_at_soc' as double");
						return Result::Failure;
					}
					chargeAtSoc = maybe.value();
				}
				if (chargeAtSoc < 0.0 || chargeAtSoc > 1.0)
				{
					WriteErrorMessage(fullTableName,
						"charge_at_soc must be in range [0.0, 1.0]");
					return Result::Failure;
				}
				uint32_t noChargeAmount = static_cast<uint32_t>(
					chargeAtSoc * capacity);
				if (noChargeAmount == capacity)
				{
					// NOTE: noChargeAmount must be at
					// least 1 unit less than capacity
					noChargeAmount = capacity - 1;
				}
				double initSoc = std::stod(defaultStoreFields.at("init_soc"));
				if (table.contains("init_soc"))
				{
					auto maybe = TOMLTable_ParseDouble(
						table, "init_soc", fullTableName);
					if (!maybe.has_value())
					{
						WriteErrorMessage(fullTableName,
							"unable to parse 'init_soc' as double");
						return Result::Failure;
					}
					initSoc = maybe.value();
				}
				if (initSoc < 0.0 || initSoc > 1.0)
				{
					WriteErrorMessage(fullTableName,
						"init_soc must be in range [0.0, 1.0]");
					return Result::Failure;
				}
				uint32_t initialStorage = static_cast<uint32_t>(
					capacity * initSoc);
				id = Model_AddStore(
					s.TheModel, capacity, maxCharge, maxDischarge,
					noChargeAmount, initialStorage, inflowId, tag);
			} break;
			default:
			{
				WriteErrorMessage(fullTableName,
					"unhandled component type: " + ToString(ct));
				throw std::runtime_error{ "Unhandled component type" };
			}
		}
		if (table.contains("fragility_modes"))
		{
			if (!table.at("fragility_modes").is_array())
			{
				WriteErrorMessage(
					fullTableName,
					"fragility_modes must be an array of string");
				return Result::Failure;
			}
			std::vector<toml::value> const& fms =
				table.at("fragility_modes").as_array();
			for (size_t fmIdx = 0; fmIdx < fms.size(); ++fmIdx)
			{
				if (!fms[fmIdx].is_string())
				{
					WriteErrorMessage(fullTableName,
						"fragility_modes[" + std::to_string(fmIdx)
						+ "] must be string");
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
	ParseComponents(Simulation& s, toml::table const& table)
	{
		for (auto it = table.cbegin(); it != table.cend(); ++it)
		{
			auto result =
				ParseSingleComponent(s, it->second.as_table(), it->first);
			if (result == Result::Failure)
			{
				return Result::Failure;
			}
		}
		return Result::Success;
	}
}