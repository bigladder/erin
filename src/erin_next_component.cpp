#include "erin_next/erin_next_component.h"
#include "erin_next/erin_next.h"
#include "erin_next/erin_next_time_and_amount.h"
#include "erin_next/erin_next_toml.h"
#include "erin_next/erin_next_units.h"
#include "erin_next/erin_next_utils.h"
#include "erin_next/erin_next_validation.h"
#include <map>
#include <optional>
#include <sstream>
#include <unordered_set>
#include <unordered_map>

namespace erin_next
{

	// TODO: pass in default rate_unit and quantity_unit from
	// simulation_info
	Result
	ParseSingleComponent(
		Simulation& s,
		toml::table const& table,
		std::string const& tag,
		ComponentValidationMap const& compValids
	)
	{
		std::string fullTableName = "components." + tag;
		if (!table.contains("type"))
		{
			WriteErrorMessage(
				fullTableName, "required field 'type' not present"
			);
			return Result::Failure;
		}
		std::optional<ComponentType> maybeCompType =
			TagToComponentType(table.at("type").as_string());
		if (!maybeCompType.has_value())
		{
			WriteErrorMessage(
				fullTableName,
				"unable to parse component type '"
					+ std::string{table.at("type").as_string()} + "'"
			);
			return Result::Failure;
		}
		ComponentType ct = maybeCompType.value();
		std::vector<std::string> errors;
		std::vector<std::string> warnings;
		std::unordered_map<std::string, InputValue> input;
		switch (ct)
		{
			case ComponentType::ConstantEfficiencyConverterType:
			{
				input = TOMLTable_ParseWithValidation(
					table,
					compValids.ConstantEfficiencyConverter,
					fullTableName,
					errors,
					warnings
				);
			} break;
			case ComponentType::ConstantLoadType:
			{
				input = TOMLTable_ParseWithValidation(
					table,
					compValids.ConstantLoad,
					fullTableName,
					errors,
					warnings
				);
			} break;
			case ComponentType::ConstantSourceType:
			{
				input = TOMLTable_ParseWithValidation(
					table,
					compValids.ConstantSource,
					fullTableName,
					errors,
					warnings
				);
			} break;
			case ComponentType::MuxType:
			{
				input = TOMLTable_ParseWithValidation(
					table,
					compValids.Mux,
					fullTableName,
					errors,
					warnings
				);
			} break;
			case ComponentType::PassThroughType:
			{
				input = TOMLTable_ParseWithValidation(
					table,
					compValids.PassThrough,
					fullTableName,
					errors,
					warnings
				);
			} break;
			case ComponentType::ScheduleBasedLoadType:
			{
				input = TOMLTable_ParseWithValidation(
					table,
					compValids.ScheduleBasedLoad,
					fullTableName,
					errors,
					warnings
				);
			} break;
			case ComponentType::ScheduleBasedSourceType:
			{
				input = TOMLTable_ParseWithValidation(
					table,
					compValids.ScheduleBasedSource,
					fullTableName,
					errors,
					warnings
				);
			} break;
			case ComponentType::StoreType:
			{
				input = TOMLTable_ParseWithValidation(
					table,
					compValids.Store,
					fullTableName,
					errors,
					warnings
				);
			} break;
			default:
			{
				WriteErrorMessage(
					fullTableName,
					"Unhandled component type");
				std::exit(1);
			} break;
		}
		if (errors.size() > 0)
		{
			std::cout << "ERRORS Parsing Component " << tag << ":" << std::endl;
			for (auto const& err : errors)
			{
				std::cout << "- " << err << std::endl;
			}
			return Result::Failure;
		}
		if (warnings.size() > 0)
		{
			std::cout << "WARNINGS:" << std::endl;
			for (auto const& w : warnings)
			{
				std::cout << "- " << w << std::endl;
			}
		}
		size_t id = {};
		std::string inflow{};
		size_t inflowId = {};
		std::string outflow{};
		size_t outflowId = 0;
		std::string lossflow{};
		size_t lossflowId = 0;
		if (input.contains("outflow"))
		{
			outflow = std::get<std::string>(input.at("outflow").Value);
			outflowId = Simulation_RegisterFlow(s, outflow);
		}
		if (input.contains("inflow"))
		{
			inflow = std::get<std::string>(input.at("inflow").Value);
			inflowId = Simulation_RegisterFlow(s, inflow);
		}
		if (table.contains("flow"))
		{
			inflow = std::get<std::string>(input.at("flow").Value);
			inflowId = Simulation_RegisterFlow(s, inflow);
			outflow = inflow;
			outflowId = inflowId;
		}
		if (table.contains("lossflow"))
		{
			lossflow = std::get<std::string>(input.at("lossflow").Value);
			lossflowId = Simulation_RegisterFlow(s, lossflow);
		}
		PowerUnit rateUnit = PowerUnit::Watt;
		if (input.contains("rate_unit"))
		{
			auto const& rateUnitStr =
				std::get<std::string>(input.at("rate_unit").Value);
			auto maybeRateUnit = TagToPowerUnit(rateUnitStr);
			if (!maybeRateUnit.has_value())
			{
				WriteErrorMessage(
					fullTableName,
					"unable to understand rate_unit '" + rateUnitStr + "'"
				);
				return Result::Failure;
			}
			rateUnit = maybeRateUnit.value();
		}
		switch (ct)
		{
			case ComponentType::ConstantSourceType:
			{
				flow_t maxAvailable = max_flow_W;
				if (table.contains("max_outflow"))
				{
					auto maybe = TOMLTable_ParseDouble(
						table, "max_outflow", fullTableName
					);
					if (!maybe.has_value())
					{
						WriteErrorMessage(
							fullTableName,
							"unable to parse 'max_outflow' as number"
						);
						return Result::Failure;
					}
					double maxAvailableReal = maybe.value();
					maxAvailable = static_cast<uint32_t>(
						Power_ToWatt(maxAvailableReal, rateUnit)
					);
				}
				id = Model_AddConstantSource(
					s.TheModel, maxAvailable, outflowId, tag
				);
			} break;
			case ComponentType::ScheduleBasedLoadType:
			{
				if (!table.contains("loads_by_scenario"))
				{
					WriteErrorMessage(
						fullTableName,
						"missing required field 'loads_by_scenario'"
					);
					return Result::Failure;
				}
				if (!table.at("loads_by_scenario").is_table())
				{
					WriteErrorMessage(
						fullTableName, "'loads_by_scenario' must be a table"
					);
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
								{scenarioId, loadId.value()}
							);
						}
						else
						{
							// TODO: pass in warnings and error std::vector<std::string>
							std::ostringstream oss;
							oss << "missing supply for tag '" << loadTag << "'";
							std::cout << WriteErrorToString(tag, oss.str()) << std::endl;
							return Result::Failure;
						}
					}
				}
				std::vector<TimeAndAmount> emptyLoads = {};
				id = Model_AddScheduleBasedLoad(
					s.TheModel, emptyLoads, scenarioIdToLoadId, inflowId, tag
				);
			} break;
			case ComponentType::ScheduleBasedSourceType:
			{
				// TODO: read other items
				if (!table.contains("supply_by_scenario"))
				{
					WriteErrorMessage(
						fullTableName,
						"missing required field 'supply_by_scenario'"
					);
					return Result::Failure;
				}
				if (!table.at("supply_by_scenario").is_table())
				{
					WriteErrorMessage(
						fullTableName, "'supply_by_scenario' must be a table"
					);
					return Result::Failure;
				}
				toml::table const& sbs =
					table.at("supply_by_scenario").as_table();
				std::map<size_t, size_t> scenarioIdToSupplyId = {};
				for (auto it = sbs.cbegin(); it != sbs.cend(); ++it)
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
							scenarioIdToSupplyId.insert(
								{scenarioId, loadId.value()}
							);
						}
						else
						{
							// TODO: pass in warnings and error std::vector<std::string>
							std::ostringstream oss;
							oss << "missing supply for tag '" << loadTag << "'";
							std::cout << WriteErrorToString(tag, oss.str()) << std::endl;
							return Result::Failure;
						}
					}
				}
				std::vector<TimeAndAmount> timesAndAmounts;
				double initialAge_s = 0.0;
				auto const compIdAndWasteConn = Model_AddScheduleBasedSource(
					s.TheModel,
					timesAndAmounts,
					scenarioIdToSupplyId,
					outflowId,
					tag,
					initialAge_s
				);
				id = compIdAndWasteConn.Id;
			} break;
			case ComponentType::MuxType:
			{
				auto numInflows =
					TOMLTable_ParseInteger(table, "num_inflows", fullTableName);
				auto numOutflows = TOMLTable_ParseInteger(
					table, "num_outflows", fullTableName
				);
				if (!numInflows.has_value())
				{
					WriteErrorMessage(
						fullTableName,
						"num_inflows doesn't appear or is not an integer"
					);
					return Result::Failure;
				}
				if (!numOutflows.has_value())
				{
					WriteErrorMessage(
						fullTableName,
						"num_outflows doesn't appear or is not an integer"
					);
					return Result::Failure;
				}
				if (inflowId != outflowId)
				{
					WriteErrorMessage(
						fullTableName,
						"a mux component must have the same inflow type "
						"as outflow type; we have inflow = '"
							+ s.FlowTypeMap.Type[inflowId] + "'; outflow = '"
							+ s.FlowTypeMap.Type[outflowId] + "'"
					);
					return Result::Failure;
				}
				id = Model_AddMux(
					s.TheModel,
					numInflows.value(),
					numOutflows.value(),
					outflowId,
					tag
				);
			} break;
			case ComponentType::ConstantEfficiencyConverterType:
			{
				PowerUnit localRateUnit = rateUnit;
				double efficiency = std::get<double>(
					input.at("constant_efficiency").Value
				);
				if (efficiency <= 0.0)
				{
					WriteErrorMessage(
						fullTableName, "efficiency must be > 0.0"
					);
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
						s.TheModel,
						efficiency,
						inflowId,
						outflowId,
						lossflowId,
						tag
					);
				id = compIdAndWasteConn.Id;
				if (input.contains("rate_unit"))
				{
					std::string localRateUnitStr =
						std::get<std::string>(input.at("rate_unit").Value);
					auto maybeRateUnit =
						TagToPowerUnit(localRateUnitStr);
					if (!maybeRateUnit.has_value())
					{
						WriteErrorMessage(
							fullTableName,
							"unhandled rate unit '" + localRateUnitStr + "'"
						);
						return Result::Failure;
					}
					localRateUnit = maybeRateUnit.value();
				}
				if (input.contains("max_outflow"))
				{
					double maxOutflow_W = Power_ToWatt(
						std::get<double>(input.at("max_outflow").Value),
						localRateUnit
					);
					size_t constEffIdx =
						s.TheModel.ComponentMap.Idx[id];
					s.TheModel.ConstEffConvs[constEffIdx].MaxOutflow_W =
						maxOutflow_W;
				}
				if (input.contains("max_lossflow"))
				{
					double maxLossflow_W = Power_ToWatt(
						std::get<double>(input.at("max_lossflow").Value),
						localRateUnit
					);
					size_t constEffIdx =
						s.TheModel.ComponentMap.Idx[id];
					s.TheModel.ConstEffConvs[constEffIdx].MaxLossflow_W =
						maxLossflow_W;
				}
			} break;
			case ComponentType::PassThroughType:
			{
				if (inflowId != outflowId)
				{
					WriteErrorMessage(
						fullTableName,
						"inflow type must equal outflow type for pass-through"
					);
					return Result::Failure;
				}
				id = Model_AddPassThrough(s.TheModel, inflowId, tag);
			} break;
			case ComponentType::StoreType:
			{
				std::unordered_map<std::string, std::string> defaultStoreFields{
					{"charge_at_soc", "0.8"},
					{"flow", ""},
					{"init_soc", "1.0"},
				};
				if (inflowId != outflowId)
				{
					WriteErrorMessage(
						fullTableName,
						"inflow type must equal outflow type for store"
					);
					return Result::Failure;
				}
				auto maybeCapacity =
					TOMLTable_ParseDouble(table, "capacity", fullTableName);
				if (!maybeCapacity.has_value())
				{
					WriteErrorMessage(
						fullTableName, "unable to parse 'capacity' as double"
					);
					return Result::Failure;
				}
				double capacityReal = maybeCapacity.value();
				if (capacityReal <= 0.0)
				{
					WriteErrorMessage(
						fullTableName, "capacity must be greater than 0"
					);
					return Result::Failure;
				}
				auto maybeCapacityUnitStr = TOMLTable_ParseString(
					table, "capacity_unit", fullTableName
				);
				if (!maybeCapacityUnitStr.has_value())
				{
					WriteErrorMessage(
						fullTableName,
						"unable to parse 'capacity_unit' as string"
					);
					return Result::Failure;
				}
				std::string capacityUnitStr = maybeCapacityUnitStr.value();
				auto maybeCapacityUnit = TagToEnergyUnit(capacityUnitStr);
				if (!maybeCapacityUnit.has_value())
				{
					WriteErrorMessage(
						fullTableName,
						"unable to understand capacity_unit of '"
							+ capacityUnitStr + "'"
					);
					// TODO: list available capacity units
					return Result::Failure;
				}
				EnergyUnit capacityUnit = maybeCapacityUnit.value();
				uint32_t capacity = static_cast<uint32_t>(
					Energy_ToJoules(capacityReal, capacityUnit)
				);
				if (capacity == 0)
				{
					WriteErrorMessage(
						fullTableName, "capacity must be greater than 0"
					);
					return Result::Failure;
				}
				auto maybeMaxCharge =
					TOMLTable_ParseDouble(table, "max_charge", fullTableName);
				if (!maybeMaxCharge.has_value())
				{
					WriteErrorMessage(
						fullTableName, "unable to parse 'max_charge' as double"
					);
					return Result::Failure;
				}
				double maxChargeReal = maybeMaxCharge.value();
				uint32_t maxCharge =
					static_cast<uint32_t>(Power_ToWatt(maxChargeReal, rateUnit)
					);
				auto maybeMaxDischarge = TOMLTable_ParseDouble(
					table, "max_discharge", fullTableName
				);
				if (!maybeMaxDischarge.has_value())
				{
					WriteErrorMessage(
						fullTableName,
						"unable to parse 'max_discharge' as double"
					);
					return Result::Failure;
				}
				double maxDischargeReal = maybeMaxDischarge.value();
				uint32_t maxDischarge = static_cast<uint32_t>(
					Power_ToWatt(maxDischargeReal, rateUnit)
				);
				double chargeAtSoc =
					std::stod(defaultStoreFields.at("charge_at_soc"));
				if (table.contains("charge_at_soc"))
				{
					auto maybe = TOMLTable_ParseDouble(
						table, "charge_at_soc", fullTableName
					);
					if (!maybe.has_value())
					{
						WriteErrorMessage(
							fullTableName,
							"unable to parse 'charge_at_soc' as double"
						);
						return Result::Failure;
					}
					chargeAtSoc = maybe.value();
				}
				if (chargeAtSoc < 0.0 || chargeAtSoc > 1.0)
				{
					WriteErrorMessage(
						fullTableName,
						"charge_at_soc must be in range [0.0, 1.0]"
					);
					return Result::Failure;
				}
				uint32_t noChargeAmount =
					static_cast<uint32_t>(chargeAtSoc * capacity);
				if (noChargeAmount == capacity)
				{
					// NOTE: noChargeAmount must be at
					// least 1 unit less than capacity
					noChargeAmount = capacity - 1;
				}
				double initSoc = std::stod(defaultStoreFields.at("init_soc"));
				if (table.contains("init_soc"))
				{
					auto maybe =
						TOMLTable_ParseDouble(table, "init_soc", fullTableName);
					if (!maybe.has_value())
					{
						WriteErrorMessage(
							fullTableName,
							"unable to parse 'init_soc' as double"
						);
						return Result::Failure;
					}
					initSoc = maybe.value();
				}
				if (initSoc < 0.0 || initSoc > 1.0)
				{
					WriteErrorMessage(
						fullTableName, "init_soc must be in range [0.0, 1.0]"
					);
					return Result::Failure;
				}
				uint32_t initialStorage =
					static_cast<uint32_t>(capacity * initSoc);
				id = Model_AddStore(
					s.TheModel,
					capacity,
					maxCharge,
					maxDischarge,
					noChargeAmount,
					initialStorage,
					inflowId,
					tag
				);
			} break;
			default:
			{
				WriteErrorMessage(
					fullTableName, "unhandled component type: " + ToString(ct)
				);
				throw std::runtime_error{"Unhandled component type"};
			}
		}
		if (table.contains("failure_modes"))
		{
			if (!table.at("failure_modes").is_array())
			{
				WriteErrorMessage(
					fullTableName, "failure_modes must be an array of string"
				);
				return Result::Failure;
			}
			std::vector<toml::value> const& fms =
				table.at("failure_modes").as_array();
			for (size_t fmIdx = 0; fmIdx < fms.size(); ++fmIdx)
			{
				if (!fms[fmIdx].is_string())
				{
					WriteErrorMessage(
						fullTableName,
						"failure_modes[" + std::to_string(fmIdx)
							+ "] must be string"
					);
					return Result::Failure;
				}
				std::string const& fmTag = fms[fmIdx].as_string();
				bool existingFailureMode = false;
				size_t fmId;
				for (fmId = 0; fmId < s.FailureModes.Tags.size(); ++fmId)
				{
					if (s.FailureModes.Tags[fmId] == fmTag)
					{
						existingFailureMode = true;
						break;
					}
				}
				if (!existingFailureMode)
				{
					fmId = s.FailureModes.Tags.size();
					s.FailureModes.Tags.push_back(fmTag);
					// NOTE: add placeholder default data
					s.FailureModes.FailureDistIds.push_back(0);
					s.FailureModes.RepairDistIds.push_back(0);
				}
				s.ComponentFailureModes.ComponentIds.push_back(id);
				s.ComponentFailureModes.FailureModeIds.push_back(fmId);
			}
		}
		if (table.contains("fragility_modes"))
		{
			if (!table.at("fragility_modes").is_array())
			{
				WriteErrorMessage(
					fullTableName, "fragility_modes must be an array of string"
				);
				return Result::Failure;
			}
			std::vector<toml::value> const& fms =
				table.at("fragility_modes").as_array();
			for (size_t fmIdx = 0; fmIdx < fms.size(); ++fmIdx)
			{
				if (!fms[fmIdx].is_string())
				{
					WriteErrorMessage(
						fullTableName,
						"fragility_modes[" + std::to_string(fmIdx)
							+ "] must be string"
					);
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
		if (table.contains("initial_age"))
		{
			TimeUnit timeUnit = TimeUnit::Second;
			if (table.contains("time_unit"))
			{
				auto maybeTimeUnitStr =
					TOMLTable_ParseString(table, "time_unit", fullTableName);
				if (!maybeTimeUnitStr.has_value())
				{
					WriteErrorMessage(
						fullTableName, "unable to parse 'time_unit' as string"
					);
					return Result::Failure;
				}
				auto maybeTimeUnit = TagToTimeUnit(maybeTimeUnitStr.value());
				if (!maybeTimeUnit.has_value())
				{
					WriteErrorMessage(
						fullTableName,
						"could not interpret '" + maybeTimeUnitStr.value()
							+ "' as time unit"
					);
					return Result::Failure;
				}
				timeUnit = maybeTimeUnit.value();
			}
			auto maybeInitialAge =
				TOMLTable_ParseDouble(table, "initial_age", fullTableName);
			if (!maybeInitialAge.has_value())
			{
				WriteErrorMessage(
					fullTableName, "unable to parse initial age as a number"
				);
				return Result::Failure;
			}
			double initialAge_s =
				Time_ToSeconds(maybeInitialAge.value(), timeUnit);
			ComponentDict_SetInitialAge(
				s.TheModel.ComponentMap, id, initialAge_s
			);
		}
		return Result::Success;
	}

	Result
	ParseComponents(
		Simulation& s,
		toml::table const& table,
		ComponentValidationMap const& compValids)
	{
		for (auto it = table.cbegin(); it != table.cend(); ++it)
		{
			auto result =
				ParseSingleComponent(s, it->second.as_table(), it->first, compValids);
			if (result == Result::Failure)
			{
				std::string tag = "components." + it->first;
				WriteErrorMessage(tag, "could not parse component");
				return Result::Failure;
			}
		}
		return Result::Success;
	}
}
