// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#include <map>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "erin/component.h"
#include "erin/erin.h"
#include "erin/logging.h"
#include "erin/simulation.h"
#include "erin/time_and_amount.h"
#include "erin/toml.h"
#include "erin/units.h"
#include "erin/utils.h"
#include "erin/validation.h"

namespace erin
{

Result parse_single_component(Simulation& s,
                              toml::table const& table,
                              std::string const& tag,
                              ComponentValidationMap const& compValids,
                              Log const& log)
{
    std::string fullTableName = "components." + tag;
    if (!table.contains("type"))
    {
        Log_error(log, fullTableName, "required field 'type' not present");
        return Result::failure;
    }
    std::optional<ComponentType> maybeCompType = TagToComponentType(table.at("type").as_string());
    if (!maybeCompType.has_value())
    {
        Log_error(log,
                  fullTableName,
                  fmt::format("unable to parse component type '{}'",
                              std::string {table.at("type").as_string()}));
        return Result::failure;
    }
    // TODO: move this section into another function?
    ComponentType ct = maybeCompType.value();
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::unordered_map<std::string, InputValue> input;
    switch (ct)
    {
    case ComponentType::constant_efficiency_converter_type:
    {
        input = TOMLTable_parse_with_validation(
            table, compValids.constant_efficiency_converter, fullTableName, errors, warnings);
    }
    break;
    case ComponentType::variable_efficiency_converter_type:
    {
        input = TOMLTable_parse_with_validation(
            table, compValids.variable_efficiency_converter, fullTableName, errors, warnings);
    }
    break;
    case ComponentType::constant_load_type:
    {
        input = TOMLTable_parse_with_validation(
            table, compValids.constant_load, fullTableName, errors, warnings);
    }
    break;
    case ComponentType::constant_source_type:
    {
        input = TOMLTable_parse_with_validation(
            table, compValids.constant_source, fullTableName, errors, warnings);
    }
    break;
    case ComponentType::mux_type:
    {
        input =
            TOMLTable_parse_with_validation(table, compValids.mux, fullTableName, errors, warnings);
    }
    break;
    case ComponentType::pass_through_type:
    {
        input = TOMLTable_parse_with_validation(
            table, compValids.pass_through, fullTableName, errors, warnings);
    }
    break;
    case ComponentType::schedule_based_load_type:
    {
        input = TOMLTable_parse_with_validation(
            table, compValids.schedule_based_load, fullTableName, errors, warnings);
    }
    break;
    case ComponentType::schedule_based_source_type:
    {
        input = TOMLTable_parse_with_validation(
            table, compValids.schedule_based_source, fullTableName, errors, warnings);
    }
    break;
    case ComponentType::store_type:
    {
        input = TOMLTable_parse_with_validation(
            table, compValids.store, fullTableName, errors, warnings);
    }
    break;
    case ComponentType::mover_type:
    {
        input = TOMLTable_parse_with_validation(
            table, compValids.mover, fullTableName, errors, warnings);
    }
    break;
    case ComponentType::variable_efficiency_mover_type:
    {
        input = TOMLTable_parse_with_validation(
            table, compValids.variable_efficiency_mover, fullTableName, errors, warnings);
    }
    break;
    case ComponentType::switch_type:
    {
        input = TOMLTable_parse_with_validation(
            table, compValids.transfer_switch, fullTableName, errors, warnings);
    }
    break;
    default:
    {
        write_error_message(fullTableName, "unhandled component type: " + ToString(ct));
        std::exit(1);
    }
    break;
    }
    if (!errors.empty())
    {
        Log_error(log, tag, "errors parsing component");
        for (auto const& err : errors)
        {
            Log_error(log, tag, err);
        }
        return Result::failure;
    }
    if (!warnings.empty())
    {
        for (auto const& w : warnings)
        {
            Log_warning(log, tag, w);
        }
    }
    size_t id = {};
    std::string inflow {};
    size_t inflowId = {};
    std::string outflow {};
    size_t outflowId = 0;
    std::string lossflow {};
    size_t lossflowId = 0;
    if (input.contains("outflow"))
    {
        outflow = std::get<std::string>(input.at("outflow").value);
        outflowId = Simulation_RegisterFlow(s, outflow);
    }
    if (input.contains("inflow"))
    {
        inflow = std::get<std::string>(input.at("inflow").value);
        inflowId = Simulation_RegisterFlow(s, inflow);
    }
    if (input.contains("flow"))
    {
        inflow = std::get<std::string>(input.at("flow").value);
        inflowId = Simulation_RegisterFlow(s, inflow);
        outflow = inflow;
        outflowId = inflowId;
    }
    if (input.contains("lossflow"))
    {
        lossflow = std::get<std::string>(input.at("lossflow").value);
        lossflowId = Simulation_RegisterFlow(s, lossflow);
    }
    PowerUnit rateUnit = s.info.RateUnit;
    if (input.contains("rate_unit"))
    {
        auto const& rateUnitStr = std::get<std::string>(input.at("rate_unit").value);
        auto maybeRateUnit = tag_to_power_unit(rateUnitStr);
        if (!maybeRateUnit.has_value())
        {
            Log_error(log, fullTableName, fmt::format("unhandled rate_unit '{}'", rateUnitStr));
            return Result::failure;
        }
        rateUnit = maybeRateUnit.value();
    }
    bool report = true;
    if (table.contains("report"))
    {
        std::optional<bool> maybeReport = TOML_parse_value_as_bool(table.at("report"));
        if (maybeReport.has_value())
        {
            report = maybeReport.value();
        }
        else
        {
            Log_error(log, fullTableName, "unable to parse 'report' as bool");
            return Result::failure;
        }
    }
    switch (ct)
    {
    case ComponentType::constant_load_type:
    {
        flow_t loadRequest_W = 0;
        PowerUnit localPowerUnit = rateUnit;
        if (input.contains("rate_unit"))
        {
            std::string localRateUnit = std::get<std::string>(input.at("rate_unit").value);
            std::optional<PowerUnit> maybePowerUnit = tag_to_power_unit(localRateUnit);
            if (!maybePowerUnit.has_value())
            {
                write_error_message(fullTableName,
                                    "unhandled rate_unit value: '" + localRateUnit + "'");
                return Result::failure;
            }
            localPowerUnit = maybePowerUnit.value();
        }
        if (!input.contains("constant_request"))
        {
            write_error_message(fullTableName, "required field 'constant_request' not found");
            return Result::failure;
        }
        double loadRequest = std::get<double>(input.at("constant_request").value);
        loadRequest_W = static_cast<flow_t>(power_to_watts(loadRequest, localPowerUnit));
        id = Model_AddConstantLoad(s.the_model, loadRequest_W, inflowId, tag, report);
    }
    break;
    case ComponentType::constant_source_type:
    {
        flow_t maxAvailable = max_flow_W;
        if (table.contains("max_outflow"))
        {
            auto maybe = TOMLTable_parse_double(table, "max_outflow", fullTableName);
            if (!maybe.has_value())
            {
                Log_error(log, fullTableName, "unable to parse 'max_outflow' as number");
                return Result::failure;
            }
            double maxAvailableReal = maybe.value();
            maxAvailable = static_cast<flow_t>(power_to_watts(maxAvailableReal, rateUnit));
        }
        id = Model_AddConstantSource(s.the_model, maxAvailable, outflowId, tag);
    }
    break;
    case ComponentType::schedule_based_load_type:
    {
        if (!table.contains("loads_by_scenario"))
        {
            Log_error(log, fullTableName, "missing required field 'loads_by_scenario'");
            return Result::failure;
        }
        if (!table.at("loads_by_scenario").is_table())
        {
            Log_error(log, fullTableName, "'loads_by_scenario' must be a table");
            return Result::failure;
        }
        toml::table const& lbs = table.at("loads_by_scenario").as_table();
        std::map<size_t, size_t> scenarioIdToLoadId = {};
        for (auto it = lbs.cbegin(); it != lbs.cend(); ++it)
        {
            std::string const& scenarioTag = it->first;
            size_t scenarioId = Simulation_RegisterScenario(s, scenarioTag);
            if (it->second.is_string())
            {
                std::string const& loadTag = it->second.as_string();
                std::optional<size_t> loadId = Simulation_GetLoadIdByTag(s, loadTag);
                if (loadId.has_value())
                {
                    scenarioIdToLoadId.insert({scenarioId, loadId.value()});
                }
                else
                {
                    Log_error(log, tag, fmt::format("missing supply for tag '{}'", loadTag));
                    return Result::failure;
                }
            }
        }
        std::vector<TimeAndAmount> emptyLoads = {};
        id = Model_AddScheduleBasedLoad(s.the_model, emptyLoads, scenarioIdToLoadId, inflowId, tag);
    }
    break;
    case ComponentType::schedule_based_source_type:
    {
        std::unordered_map<std::string, std::string> const& sbs =
            std::get<std::unordered_map<std::string, std::string>>(
                input.at("supply_by_scenario").value);
        std::map<size_t, size_t> scenarioIdToSupplyId = {};
        for (auto it = sbs.cbegin(); it != sbs.cend(); ++it)
        {
            std::string const& scenarioTag = it->first;
            size_t scenarioId = Simulation_RegisterScenario(s, scenarioTag);
            std::string const& loadTag = it->second;
            std::optional<size_t> loadId = Simulation_GetLoadIdByTag(s, loadTag);
            if (loadId.has_value())
            {
                scenarioIdToSupplyId.insert({scenarioId, loadId.value()});
            }
            else
            {
                Log_error(log, tag, fmt::format("missing supply for tag '{}'", loadTag));
                return Result::failure;
            }
        }
        std::vector<TimeAndAmount> timesAndAmounts;
        double initialAge_s = 0.0;
        auto const compIdAndWasteConn = Model_AddScheduleBasedSource(
            s.the_model, timesAndAmounts, scenarioIdToSupplyId, outflowId, tag, initialAge_s);
        id = compIdAndWasteConn.id;
        if (input.contains("max_outflow"))
        {
            double rawMaxOutflow = std::get<double>(input.at("max_outflow").value);
            flow_t maxOutflow_W = static_cast<flow_t>(power_to_watts(rawMaxOutflow, rateUnit));
            s.the_model.scheduled_source[s.the_model.component.subtype_index[id]].max_outflow_W =
                maxOutflow_W;
        }
    }
    break;
    case ComponentType::mux_type:
    {
        auto numInflowsTemp = std::get<int64_t>(input.at("num_inflows").value);
        auto numOutflowsTemp = std::get<int64_t>(input.at("num_outflows").value);
        if (numInflowsTemp <= 0)
        {
            write_error_message(fullTableName, "num_inflows must be a positive integer");
            return Result::failure;
        }
        if (numOutflowsTemp <= 0)
        {
            write_error_message(fullTableName, "num_outflows must be a positive integer");
            return Result::failure;
        }
        size_t numInflows = static_cast<size_t>(numInflowsTemp);
        size_t numOutflows = static_cast<size_t>(numOutflowsTemp);
        if (inflowId != outflowId)
        {
            write_error_message(fullTableName,
                                "a mux component must have the same inflow type "
                                "as outflow type; we have inflow = '" +
                                    s.flow_type_map.flow_type[inflowId] + "'; outflow = '" +
                                    s.flow_type_map.flow_type[outflowId] + "'");
            return Result::failure;
        }
        id = Model_AddMux(s.the_model, numInflows, numOutflows, outflowId, tag);
        if (input.contains("max_outflows"))
        {
            std::vector<flow_t> maxOutflows_W(numOutflows, 0);
            std::vector<double> maxOutflowsRaw =
                std::get<std::vector<double>>(input.at("max_outflows").value);
            // TODO: add en error for size mismatch on max outflows
            assert(maxOutflowsRaw.size() == numOutflows);
            for (size_t i = 0; i < numOutflows; ++i)
            {
                maxOutflows_W[i] = static_cast<flow_t>(power_to_watts(maxOutflowsRaw[i], rateUnit));
            }
            s.the_model.mux[s.the_model.component.subtype_index[id]].max_outflows_W =
                std::move(maxOutflows_W);
        }
    }
    break;
    case ComponentType::constant_efficiency_converter_type:
    {
        PowerUnit localRateUnit = rateUnit;
        double efficiency = std::get<double>(input.at("constant_efficiency").value);
        if (efficiency <= 0.0)
        {
            errors.push_back(write_error_to_string(fullTableName, "efficiency must be > 0.0"));
            return Result::failure;
        }
        if (efficiency > 1.0)
        {
            errors.push_back(write_error_to_string(fullTableName,
                                                   "efficiency must be <= 1.0; "
                                                   "if you need efficiencies (COPs) > 1, "
                                                   "consider using a mover"));
            return Result::failure;
        }
        auto const compIdAndWasteConn = Model_AddConstantEfficiencyConverter(
            s.the_model, efficiency, inflowId, outflowId, lossflowId, tag, report);
        id = compIdAndWasteConn.id;
        if (input.contains("rate_unit"))
        {
            std::string localRateUnitStr = std::get<std::string>(input.at("rate_unit").value);
            auto maybeRateUnit = tag_to_power_unit(localRateUnitStr);
            if (!maybeRateUnit.has_value())
            {
                errors.push_back(write_error_to_string(
                    fullTableName, "unhandled rate unit '" + localRateUnitStr + "'"));
                return Result::failure;
            }
            localRateUnit = maybeRateUnit.value();
        }
        if (input.contains("max_outflow"))
        {
            double maxOutflow_W =
                power_to_watts(std::get<double>(input.at("max_outflow").value), localRateUnit);
            size_t constEffIdx = s.the_model.component.subtype_index[id];
            s.the_model.constant_efficiency_converter[constEffIdx].max_outflow_W =
                static_cast<flow_t>(maxOutflow_W);
        }
        if (input.contains("max_lossflow"))
        {
            double maxLossflow_W =
                power_to_watts(std::get<double>(input.at("max_lossflow").value), localRateUnit);
            size_t constEffIdx = s.the_model.component.subtype_index[id];
            s.the_model.constant_efficiency_converter[constEffIdx].max_lossflow_W =
                static_cast<flow_t>(maxLossflow_W);
        }
    }
    break;
    case ComponentType::variable_efficiency_converter_type:
    {
        PowerUnit localRateUnit = rateUnit;

        std::vector<std::vector<double>> rawEfficiency = std::get<std::vector<std::vector<double>>>(
            input.at("efficiency_by_fraction_out").value);
        std::unordered_map<double, double> effByOutfrac {};
        for (std::vector<double> const& fracEffPair : rawEfficiency)
        {
            assert(fracEffPair.size() == 2);
            double frac = fracEffPair[0];
            double eff = fracEffPair[1];
            if (frac < 0.0 || frac > 1.0)
            {
                errors.push_back(write_error_to_string(fullTableName,
                                                       "Output power fraction must be "
                                                       "in range [0.0, 1.0]; got " +
                                                           std::to_string(frac)));
                return Result::failure;
            }
            if (eff <= 0.0 || eff > 1.0)
            {
                errors.push_back(write_error_to_string(fullTableName,
                                                       "Efficiency must be "
                                                       "in range (0.0, 1.0]; got " +
                                                           std::to_string(eff)));
                return Result::failure;
            }
            if (effByOutfrac.contains(frac))
            {
                warnings.push_back(write_warning_to_string(
                    fullTableName,
                    "Found duplicate value of output fraction: " + std::to_string(frac) +
                        "; overwriting previous"));
            }
            effByOutfrac[frac] = eff;
        }
        if (input.contains("rate_unit"))
        {
            std::string localRateUnitStr = std::get<std::string>(input.at("rate_unit").value);
            auto maybeRateUnit = tag_to_power_unit(localRateUnitStr);
            if (!maybeRateUnit.has_value())
            {
                errors.push_back(write_error_to_string(
                    fullTableName, "unhandled rate unit '" + localRateUnitStr + "'"));
                return Result::failure;
            }
            localRateUnit = maybeRateUnit.value();
        }
        double maxOutflow_W =
            power_to_watts(std::get<double>(input.at("max_outflow").value), localRateUnit);
        std::vector<double> fracsForEff;
        fracsForEff.reserve(effByOutfrac.size());
        for (auto const& fracEffPair : effByOutfrac)
        {
            fracsForEff.push_back(fracEffPair.first);
        }
        std::sort(fracsForEff.begin(), fracsForEff.end());
        std::vector<double> outflowsForEff_W;
        std::vector<double> efficiencyFracs;
        outflowsForEff_W.reserve(effByOutfrac.size());
        efficiencyFracs.reserve(effByOutfrac.size());
        for (auto const& frac : fracsForEff)
        {
            outflowsForEff_W.push_back(frac * maxOutflow_W);
            efficiencyFracs.push_back(effByOutfrac.at(frac));
        }
        auto const compIdAndWasteConn =
            Model_AddVariableEfficiencyConverter(s.the_model,
                                                 std::move(outflowsForEff_W),
                                                 std::move(efficiencyFracs),
                                                 inflowId,
                                                 outflowId,
                                                 lossflowId,
                                                 tag,
                                                 report);
        id = compIdAndWasteConn.id;
        size_t varEffIdx = s.the_model.component.subtype_index[id];
        s.the_model.variable_efficiency_converter[varEffIdx].max_outflow_W =
            static_cast<flow_t>(maxOutflow_W);
        if (input.contains("max_lossflow"))
        {
            double maxLossflow_W =
                power_to_watts(std::get<double>(input.at("max_lossflow").value), localRateUnit);
            s.the_model.variable_efficiency_converter[varEffIdx].max_lossflow_W =
                static_cast<flow_t>(maxLossflow_W);
        }
    }
    break;
    case ComponentType::pass_through_type:
    {
        if (inflowId != outflowId)
        {
            write_error_message(fullTableName,
                                "inflow type must equal outflow type for pass-through");
            return Result::failure;
        }
        id = Model_AddPassThrough(s.the_model, inflowId, tag);
        if (input.contains("max_outflow"))
        {
            double rawMaxOutflow = std::get<double>(input.at("max_outflow").value);
            flow_t maxOutflow_W = static_cast<flow_t>(power_to_watts(rawMaxOutflow, rateUnit));
            s.the_model.pass_through[s.the_model.component.subtype_index[id]].max_outflow_W =
                maxOutflow_W;
        }
    }
    break;
    case ComponentType::store_type:
    {
        if (inflowId != outflowId)
        {
            write_error_message(fullTableName, "inflow type must equal outflow type for store");
            return Result::failure;
        }
        EnergyUnit capacityUnit = EnergyUnit::Joule;
        if (input.contains("capacity_unit"))
        {
            std::string capacityUnitStr = std::get<std::string>(input.at("capacity_unit").value);
            auto maybeCapacityUnit = tag_to_energy_unit(capacityUnitStr);
            if (!maybeCapacityUnit.has_value())
            {
                write_error_message(fullTableName,
                                    "unhandled capacity unit '" + capacityUnitStr + "'");
                return Result::failure;
            }
            capacityUnit = maybeCapacityUnit.value();
        }
        flow_t capacity_J = static_cast<flow_t>(
            energy_to_joules(std::get<double>(input.at("capacity").value), capacityUnit));
        if (capacity_J == 0)
        {
            write_error_message(fullTableName, "capacity must be greater than 0");
            return Result::failure;
        }
        flow_t maxCharge_W = static_cast<flow_t>(
            power_to_watts(std::get<double>(input.at("max_charge").value), rateUnit));
        flow_t maxDischarge_W = static_cast<flow_t>(
            power_to_watts(std::get<double>(input.at("max_discharge").value), rateUnit));
        double chargeAtSoc = std::get<double>(input.at("charge_at_soc").value);
        if (chargeAtSoc < 0.0 || chargeAtSoc > 1.0)
        {
            write_error_message(fullTableName, "charge_at_soc must be in range [0.0, 1.0]");
            return Result::failure;
        }
        flow_t noChargeAmount_J = static_cast<flow_t>(chargeAtSoc * capacity_J);
        if (noChargeAmount_J == capacity_J)
        {
            // NOTE: noChargeAmount must be at
            // least 1 unit less than capacity
            noChargeAmount_J = capacity_J - 1;
        }
        double initSoc = std::get<double>(input.at("init_soc").value);
        if (initSoc < 0.0 || initSoc > 1.0)
        {
            write_error_message(fullTableName, "init_soc must be in range [0.0, 1.0]");
            return Result::failure;
        }
        flow_t initialStorage_J = static_cast<flow_t>(capacity_J * initSoc);
        double rtEff = 1.0;
        if (input.contains("roundtrip_efficiency"))
        {
            rtEff = std::get<double>(input.at("roundtrip_efficiency").value);
            if (rtEff <= 0.0 || rtEff > 1.0)
            {
                write_error_message(fullTableName, "roundtrip efficiency must be (0.0, 1.0]");
                return Result::failure;
            }
        }
        if (rtEff == 1.0)
        {
            id = Model_AddStore(s.the_model,
                                capacity_J,
                                maxCharge_W,
                                maxDischarge_W,
                                noChargeAmount_J,
                                initialStorage_J,
                                inflowId,
                                tag);
        }
        else
        {
            auto compIdAndWasteConn = Model_AddStoreWithWasteflow(s.the_model,
                                                                  capacity_J,
                                                                  maxCharge_W,
                                                                  maxDischarge_W,
                                                                  noChargeAmount_J,
                                                                  initialStorage_J,
                                                                  inflowId,
                                                                  rtEff,
                                                                  tag);
            id = compIdAndWasteConn.id;
        }
        if (input.contains("max_outflow"))
        {
            flow_t maxOutflow_W = static_cast<flow_t>(
                power_to_watts(std::get<double>(input.at("max_outflow").value), rateUnit));
            s.the_model.store[s.the_model.component.subtype_index[id]].max_outflow_W = maxOutflow_W;
        }
    }
    break;
    case ComponentType::mover_type:
    {
        double cop = std::get<double>(input.at("cop").value);
        auto compIdAndConns = Model_AddMover(s.the_model, cop, inflowId, outflowId, tag, report);
        id = compIdAndConns.id;
        if (input.contains("max_outflow"))
        {
            flow_t maxOutflow_W = static_cast<flow_t>(
                power_to_watts(std::get<double>(input.at("max_outflow").value), rateUnit));
            size_t moverIdx = s.the_model.component.subtype_index[id];
            s.the_model.mover[moverIdx].max_outflow_W = maxOutflow_W;
        }
    }
    break;
    case ComponentType::variable_efficiency_mover_type:
    {
        PowerUnit localRateUnit = rateUnit;

        std::vector<std::vector<double>> copsByLoadFrac =
            std::get<std::vector<std::vector<double>>>(input.at("cop_by_fraction_out").value);
        std::unordered_map<double, double> copByOutFrac {};
        // TODO(mok): extract this and the one in var-eff conv
        // into a separate processing step
        for (std::vector<double> const& fracCopPair : copsByLoadFrac)
        {
            assert(fracCopPair.size() == 2);
            double frac = fracCopPair[0];
            double cop = fracCopPair[1];
            if (frac < 0.0 || frac > 1.0)
            {
                errors.push_back(write_error_to_string(fullTableName,
                                                       "Output power fraction must be "
                                                       "in range [0.0, 1.0]; got " +
                                                           std::to_string(frac)));
                return Result::failure;
            }
            if (cop <= 0.0)
            {
                errors.push_back(write_error_to_string(fullTableName,
                                                       "COP must be > 0.0" + std::to_string(frac)));
                return Result::failure;
            }
            if (copByOutFrac.contains(frac))
            {
                warnings.push_back(write_warning_to_string(
                    fullTableName,
                    "Found duplicate value of output fraction: " + std::to_string(frac) +
                        "; overwriting previous"));
            }
            copByOutFrac[frac] = cop;
        }
        if (input.contains("rate_unit"))
        {
            std::string localRateUnitStr = std::get<std::string>(input.at("rate_unit").value);
            auto maybeRateUnit = tag_to_power_unit(localRateUnitStr);
            if (!maybeRateUnit.has_value())
            {
                errors.push_back(write_error_to_string(
                    fullTableName, "unhandled rate unit '" + localRateUnitStr + "'"));
                return Result::failure;
            }
            localRateUnit = maybeRateUnit.value();
        }
        double maxOutflow_W =
            power_to_watts(std::get<double>(input.at("max_outflow").value), localRateUnit);
        std::vector<double> fracsForCop;
        fracsForCop.reserve(copByOutFrac.size());
        for (auto const& fracCopPair : copByOutFrac)
        {
            fracsForCop.push_back(fracCopPair.first);
        }
        std::sort(fracsForCop.begin(), fracsForCop.end());
        std::vector<double> outflowsForCop_W;
        std::vector<double> copByOutflow;
        outflowsForCop_W.reserve(copByOutFrac.size());
        copByOutflow.reserve(copByOutFrac.size());
        for (auto const& frac : fracsForCop)
        {
            outflowsForCop_W.push_back(frac * maxOutflow_W);
            copByOutflow.push_back(copByOutFrac.at(frac));
        }
        auto const compIdAndConns = Model_AddVariableEfficiencyMover(s.the_model,
                                                                     std::move(outflowsForCop_W),
                                                                     std::move(copByOutflow),
                                                                     inflowId,
                                                                     outflowId,
                                                                     tag,
                                                                     report);
        id = compIdAndConns.id;
        size_t moverIdx = s.the_model.component.subtype_index[id];
        s.the_model.variable_efficiency_mover[moverIdx].max_outflow_W =
            static_cast<flow_t>(maxOutflow_W);
    }
    break;
    case ComponentType::switch_type:
    {
        PowerUnit localRateUnit = rateUnit;
        id = Model_AddSwitch(s.the_model, inflowId, tag);
        if (input.contains("max_outflow"))
        {
            if (input.contains("rate_unit"))
            {
                std::string localRateUnitStr = std::get<std::string>(input.at("rate_unit").value);
                auto maybeRateUnit = tag_to_power_unit(localRateUnitStr);
                if (!maybeRateUnit.has_value())
                {
                    errors.push_back(write_error_to_string(
                        fullTableName, "unhandled rate unit '" + localRateUnitStr + "'"));
                    return Result::failure;
                }
                localRateUnit = maybeRateUnit.value();
            }
            double maxOutflow_W =
                power_to_watts(std::get<double>(input.at("max_outflow").value), localRateUnit);
            size_t switchIdx = s.the_model.component.subtype_index[id];
            s.the_model.transfer_switch[switchIdx].max_outflow_W =
                static_cast<flow_t>(maxOutflow_W);
        }
    }
    break;
    default:
    {
        write_error_message(fullTableName, "unhandled component type: " + ToString(ct));
        std::exit(1);
    }
    }
    s.the_model.component.report[id] = report;
    if (table.contains("failure_modes"))
    {
        if (!table.at("failure_modes").is_array())
        {
            write_error_message(fullTableName, "failure_modes must be an array of string");
            return Result::failure;
        }
        std::vector<toml::value> const& fms = table.at("failure_modes").as_array();
        for (size_t fmIdx = 0; fmIdx < fms.size(); ++fmIdx)
        {
            if (!fms[fmIdx].is_string())
            {
                write_error_message(fullTableName,
                                    "failure_modes[" + std::to_string(fmIdx) + "] must be string");
                return Result::failure;
            }
            std::string const& fmTag = fms[fmIdx].as_string();
            bool existingFailureMode = false;
            size_t fmId;
            for (fmId = 0; fmId < s.failure_modes.tag.size(); ++fmId)
            {
                if (s.failure_modes.tag[fmId] == fmTag)
                {
                    existingFailureMode = true;
                    break;
                }
            }
            if (!existingFailureMode)
            {
                fmId = s.failure_modes.tag.size();
                s.failure_modes.tag.push_back(fmTag);
                // NOTE: add placeholder default data
                s.failure_modes.failure_distribution_id.push_back(0);
                s.failure_modes.repair_distribution_id.push_back(0);
            }
            s.component_failure_modes.component_id.push_back(id);
            s.component_failure_modes.failure_mode_id.push_back(fmId);
        }
    }
    if (table.contains("fragility_modes"))
    {
        if (!table.at("fragility_modes").is_array())
        {
            write_error_message(fullTableName, "fragility_modes must be an array of string");
            return Result::failure;
        }
        std::vector<toml::value> const& fms = table.at("fragility_modes").as_array();
        for (size_t fmIdx = 0; fmIdx < fms.size(); ++fmIdx)
        {
            if (!fms[fmIdx].is_string())
            {
                write_error_message(
                    fullTableName, "fragility_modes[" + std::to_string(fmIdx) + "] must be string");
                return Result::failure;
            }
            std::string const& fmTag = fms[fmIdx].as_string();
            bool existingFragilityMode = false;
            size_t fmId;
            for (fmId = 0; fmId < s.fragility_modes.tag.size(); ++fmId)
            {
                if (s.fragility_modes.tag[fmId] == fmTag)
                {
                    existingFragilityMode = true;
                    break;
                }
            }
            if (!existingFragilityMode)
            {
                fmId = s.fragility_modes.tag.size();
                s.fragility_modes.tag.push_back(fmTag);
                // NOTE: add placeholder default data
                s.fragility_modes.fragility_curve_id.push_back(0);
                s.fragility_modes.repair_distribution_id.push_back({});
            }
            s.component_fragilities.component_id.push_back(id);
            s.component_fragilities.fragility_mode_id.push_back(fmId);
        }
    }
    if (table.contains("initial_age"))
    {
        TimeUnit timeUnit = TimeUnit::second;
        if (table.contains("time_unit"))
        {
            auto maybeTimeUnitStr = TOMLTable_parse_string(table, "time_unit", fullTableName);
            if (!maybeTimeUnitStr.has_value())
            {
                write_error_message(fullTableName, "unable to parse 'time_unit' as string");
                return Result::failure;
            }
            auto maybeTimeUnit = tag_to_time_unit(maybeTimeUnitStr.value());
            if (!maybeTimeUnit.has_value())
            {
                write_error_message(fullTableName,
                                    "could not interpret '" + maybeTimeUnitStr.value() +
                                        "' as time unit");
                return Result::failure;
            }
            timeUnit = maybeTimeUnit.value();
        }
        auto maybeInitialAge = TOMLTable_parse_double(table, "initial_age", fullTableName);
        if (!maybeInitialAge.has_value())
        {
            write_error_message(fullTableName, "unable to parse initial age as a number");
            return Result::failure;
        }
        double initialAge_s = time_to_seconds(maybeInitialAge.value(), timeUnit);
        ComponentDict_SetInitialAge(s.the_model.component, id, initialAge_s);
    }
    if (table.contains("group"))
    {
        auto maybeGroup = TOMLTable_parse_string(table, "group", fullTableName);
        if (!maybeGroup.has_value())
        {
            write_error_message(fullTableName, "unable to parse 'group' as a string");
            return Result::failure;
        }
        std::string group = maybeGroup.value();
        AddComponentToGroup(s.the_model, id, group);
    }
    return Result::success;
}

Result parse_components(Simulation& s,
                        toml::table const& table,
                        ComponentValidationMap const& compValids,
                        std::unordered_set<std::string> const& componentTagsInUse,
                        Log const& log)
{
    for (auto it = table.cbegin(); it != table.cend(); ++it)
    {
        std::string const& compTag = it->first;
        if (!componentTagsInUse.contains(compTag))
        {
            std::string tag = "components." + compTag;
            Log_warning(log,
                        tag,
                        "component is declared but does not appear in network "
                        "connections");
            continue;
        }
        toml::table const& compTable = it->second.as_table();
        auto result = parse_single_component(s, compTable, compTag, compValids, log);
        if (result == Result::failure)
        {
            std::string tag = "components." + compTag;
            Log_error(log, tag, "could not parse component");
            return Result::failure;
        }
    }
    return Result::success;
}
} // namespace erin
