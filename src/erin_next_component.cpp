/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_component.h"
#include "erin/logging.h"
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
#include <vector>

namespace erin
{

    Result
    ParseSingleComponent(
        Simulation& s,
        toml::table const& table,
        std::string const& tag,
        ComponentValidationMap const& compValids,
        Log const& log
    )
    {
        std::string fullTableName = "components." + tag;
        if (!table.contains("type"))
        {
            Log_Error(log, fullTableName, "required field 'type' not present");
            return Result::Failure;
        }
        std::optional<ComponentType> maybeCompType =
            TagToComponentType(table.at("type").as_string());
        if (!maybeCompType.has_value())
        {
            Log_Error(
                log,
                fullTableName,
                fmt::format(
                    "unable to parse component type '{}'",
                    std::string{table.at("type").as_string()}
                )
            );
            return Result::Failure;
        }
        // TODO: move this section into another function?
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
            }
            break;
            case ComponentType::VariableEfficiencyConverterType:
            {
                input = TOMLTable_ParseWithValidation(
                    table,
                    compValids.VariableEfficiencyConverter,
                    fullTableName,
                    errors,
                    warnings
                );
            }
            break;
            case ComponentType::ConstantLoadType:
            {
                input = TOMLTable_ParseWithValidation(
                    table,
                    compValids.ConstantLoad,
                    fullTableName,
                    errors,
                    warnings
                );
            }
            break;
            case ComponentType::ConstantSourceType:
            {
                input = TOMLTable_ParseWithValidation(
                    table,
                    compValids.ConstantSource,
                    fullTableName,
                    errors,
                    warnings
                );
            }
            break;
            case ComponentType::MuxType:
            {
                input = TOMLTable_ParseWithValidation(
                    table, compValids.Mux, fullTableName, errors, warnings
                );
            }
            break;
            case ComponentType::PassThroughType:
            {
                input = TOMLTable_ParseWithValidation(
                    table,
                    compValids.PassThrough,
                    fullTableName,
                    errors,
                    warnings
                );
            }
            break;
            case ComponentType::ScheduleBasedLoadType:
            {
                input = TOMLTable_ParseWithValidation(
                    table,
                    compValids.ScheduleBasedLoad,
                    fullTableName,
                    errors,
                    warnings
                );
            }
            break;
            case ComponentType::ScheduleBasedSourceType:
            {
                input = TOMLTable_ParseWithValidation(
                    table,
                    compValids.ScheduleBasedSource,
                    fullTableName,
                    errors,
                    warnings
                );
            }
            break;
            case ComponentType::StoreType:
            {
                input = TOMLTable_ParseWithValidation(
                    table, compValids.Store, fullTableName, errors, warnings
                );
            }
            break;
            case ComponentType::MoverType:
            {
                input = TOMLTable_ParseWithValidation(
                    table, compValids.Mover, fullTableName, errors, warnings
                );
            }
            break;
            case ComponentType::VariableEfficiencyMoverType:
            {
                input = TOMLTable_ParseWithValidation(
                    table,
                    compValids.VariableEfficiencyMover,
                    fullTableName,
                    errors,
                    warnings
                );
            }
            break;
            case ComponentType::SwitchType:
            {
                input = TOMLTable_ParseWithValidation(
                    table, compValids.Switch, fullTableName, errors, warnings
                );
            }
            break;
            default:
            {
                WriteErrorMessage(fullTableName, "Unhandled component type");
                std::exit(1);
            }
            break;
        }
        if (!errors.empty())
        {
            Log_Error(log, tag, "errors parsing component");
            for (auto const& err : errors)
            {
                Log_Error(log, tag, err);
            }
            return Result::Failure;
        }
        if (!warnings.empty())
        {
            for (auto const& w : warnings)
            {
                Log_Warning(log, tag, w);
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
        if (input.contains("flow"))
        {
            inflow = std::get<std::string>(input.at("flow").Value);
            inflowId = Simulation_RegisterFlow(s, inflow);
            outflow = inflow;
            outflowId = inflowId;
        }
        if (input.contains("lossflow"))
        {
            lossflow = std::get<std::string>(input.at("lossflow").Value);
            lossflowId = Simulation_RegisterFlow(s, lossflow);
        }
        PowerUnit rateUnit = s.Info.RateUnit;
        if (input.contains("rate_unit"))
        {
            auto const& rateUnitStr =
                std::get<std::string>(input.at("rate_unit").Value);
            auto maybeRateUnit = TagToPowerUnit(rateUnitStr);
            if (!maybeRateUnit.has_value())
            {
                Log_Error(
                    log,
                    fullTableName,
                    fmt::format("unhandled rate_unit '{}'", rateUnitStr)
                );
                return Result::Failure;
            }
            rateUnit = maybeRateUnit.value();
        }
        bool report = true;
        if (table.contains("report"))
        {
            std::optional<bool> maybeReport =
                TOML_ParseValueAsBool(table.at("report"));
            if (maybeReport.has_value())
            {
                report = maybeReport.value();
            }
            else
            {
                Log_Error(
                    log, fullTableName, "unable to parse 'report' as bool"
                );
                return Result::Failure;
            }
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
                        Log_Error(
                            log,
                            fullTableName,
                            "unable to parse 'max_outflow' as number"
                        );
                        return Result::Failure;
                    }
                    double maxAvailableReal = maybe.value();
                    maxAvailable = static_cast<flow_t>(
                        Power_ToWatt(maxAvailableReal, rateUnit)
                    );
                }
                id = Model_AddConstantSource(
                    s.TheModel, maxAvailable, outflowId, tag
                );
            }
            break;
            case ComponentType::ScheduleBasedLoadType:
            {
                if (!table.contains("loads_by_scenario"))
                {
                    Log_Error(
                        log,
                        fullTableName,
                        "missing required field 'loads_by_scenario'"
                    );
                    return Result::Failure;
                }
                if (!table.at("loads_by_scenario").is_table())
                {
                    Log_Error(
                        log,
                        fullTableName,
                        "'loads_by_scenario' must be a table"
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
                            Log_Error(
                                log,
                                tag,
                                fmt::format(
                                    "missing supply for tag '{}'", loadTag
                                )
                            );
                            return Result::Failure;
                        }
                    }
                }
                std::vector<TimeAndAmount> emptyLoads = {};
                id = Model_AddScheduleBasedLoad(
                    s.TheModel, emptyLoads, scenarioIdToLoadId, inflowId, tag
                );
            }
            break;
            case ComponentType::ScheduleBasedSourceType:
            {
                std::unordered_map<std::string, std::string> const& sbs =
                    std::get<std::unordered_map<std::string, std::string>>(
                        input.at("supply_by_scenario").Value
                    );
                std::map<size_t, size_t> scenarioIdToSupplyId = {};
                for (auto it = sbs.cbegin(); it != sbs.cend(); ++it)
                {
                    std::string const& scenarioTag = it->first;
                    size_t scenarioId =
                        Simulation_RegisterScenario(s, scenarioTag);
                    std::string const& loadTag = it->second;
                    std::optional<size_t> loadId =
                        Simulation_GetLoadIdByTag(s, loadTag);
                    if (loadId.has_value())
                    {
                        scenarioIdToSupplyId.insert({scenarioId, loadId.value()}
                        );
                    }
                    else
                    {
                        Log_Error(
                            log,
                            tag,
                            fmt::format("missing supply for tag '{}'", loadTag)
                        );
                        return Result::Failure;
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
                if (input.contains("max_outflow"))
                {
                    double rawMaxOutflow =
                        std::get<double>(input.at("max_outflow").Value);
                    flow_t maxOutflow_W = static_cast<flow_t>(
                        Power_ToWatt(rawMaxOutflow, rateUnit)
                    );
                    s.TheModel.ScheduledSrcs[s.TheModel.ComponentMap.Idx[id]]
                        .MaxOutflow_W = maxOutflow_W;
                }
            }
            break;
            case ComponentType::MuxType:
            {
                auto numInflowsTemp =
                    std::get<int64_t>(input.at("num_inflows").Value);
                auto numOutflowsTemp =
                    std::get<int64_t>(input.at("num_outflows").Value);
                if (numInflowsTemp <= 0)
                {
                    WriteErrorMessage(
                        fullTableName, "num_inflows must be a positive integer"
                    );
                    return Result::Failure;
                }
                if (numOutflowsTemp <= 0)
                {
                    WriteErrorMessage(
                        fullTableName, "num_outflows must be a positive integer"
                    );
                    return Result::Failure;
                }
                size_t numInflows = static_cast<size_t>(numInflowsTemp);
                size_t numOutflows = static_cast<size_t>(numOutflowsTemp);
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
                    s.TheModel, numInflows, numOutflows, outflowId, tag
                );
                if (input.contains("max_outflows"))
                {
                    std::vector<flow_t> maxOutflows_W(numOutflows, 0);
                    std::vector<double> maxOutflowsRaw =
                        std::get<std::vector<double>>(
                            input.at("max_outflows").Value
                        );
                    // TODO: add en error for size mismatch on max outflows
                    assert(maxOutflowsRaw.size() == numOutflows);
                    for (size_t i = 0; i < numOutflows; ++i)
                    {
                        maxOutflows_W[i] = static_cast<flow_t>(
                            Power_ToWatt(maxOutflowsRaw[i], rateUnit)
                        );
                    }
                    s.TheModel.Muxes[s.TheModel.ComponentMap.Idx[id]]
                        .MaxOutflows_W = std::move(maxOutflows_W);
                }
            }
            break;
            case ComponentType::ConstantEfficiencyConverterType:
            {
                PowerUnit localRateUnit = rateUnit;
                double efficiency =
                    std::get<double>(input.at("constant_efficiency").Value);
                if (efficiency <= 0.0)
                {
                    errors.push_back(WriteErrorToString(
                        fullTableName, "efficiency must be > 0.0"
                    ));
                    return Result::Failure;
                }
                if (efficiency > 1.0)
                {
                    errors.push_back(WriteErrorToString(
                        fullTableName,
                        "efficiency must be <= 1.0; "
                        "if you need efficiencies (COPs) > 1, "
                        "consider using a mover"
                    ));
                    return Result::Failure;
                }
                auto const compIdAndWasteConn =
                    Model_AddConstantEfficiencyConverter(
                        s.TheModel,
                        efficiency,
                        inflowId,
                        outflowId,
                        lossflowId,
                        tag,
                        report
                    );
                id = compIdAndWasteConn.Id;
                if (input.contains("rate_unit"))
                {
                    std::string localRateUnitStr =
                        std::get<std::string>(input.at("rate_unit").Value);
                    auto maybeRateUnit = TagToPowerUnit(localRateUnitStr);
                    if (!maybeRateUnit.has_value())
                    {
                        errors.push_back(WriteErrorToString(
                            fullTableName,
                            "unhandled rate unit '" + localRateUnitStr + "'"
                        ));
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
                    size_t constEffIdx = s.TheModel.ComponentMap.Idx[id];
                    s.TheModel.ConstEffConvs[constEffIdx].MaxOutflow_W =
                        static_cast<flow_t>(maxOutflow_W);
                }
                if (input.contains("max_lossflow"))
                {
                    double maxLossflow_W = Power_ToWatt(
                        std::get<double>(input.at("max_lossflow").Value),
                        localRateUnit
                    );
                    size_t constEffIdx = s.TheModel.ComponentMap.Idx[id];
                    s.TheModel.ConstEffConvs[constEffIdx].MaxLossflow_W =
                        static_cast<flow_t>(maxLossflow_W);
                }
            }
            break;
            case ComponentType::VariableEfficiencyConverterType:
            {
                PowerUnit localRateUnit = rateUnit;

                std::vector<std::vector<double>> rawEfficiency =
                    std::get<std::vector<std::vector<double>>>(
                        input.at("efficiency_by_fraction_out").Value
                    );
                std::unordered_map<double, double> effByOutfrac{};
                for (std::vector<double> const& fracEffPair : rawEfficiency)
                {
                    assert(fracEffPair.size() == 2);
                    double frac = fracEffPair[0];
                    double eff = fracEffPair[1];
                    if (frac < 0.0 || frac > 1.0)
                    {
                        errors.push_back(WriteErrorToString(
                            fullTableName,
                            "Output power fraction must be "
                            "in range [0.0, 1.0]; got "
                                + std::to_string(frac)
                        ));
                        return Result::Failure;
                    }
                    if (eff <= 0.0 || eff > 1.0)
                    {
                        errors.push_back(WriteErrorToString(
                            fullTableName,
                            "Efficiency must be "
                            "in range (0.0, 1.0]; got "
                                + std::to_string(eff)
                        ));
                        return Result::Failure;
                    }
                    if (effByOutfrac.contains(frac))
                    {
                        warnings.push_back(WriteWarningToString(
                            fullTableName,
                            "Found duplicate value of output fraction: "
                                + std::to_string(frac)
                                + "; overwriting previous"
                        ));
                    }
                    effByOutfrac[frac] = eff;
                }
                if (input.contains("rate_unit"))
                {
                    std::string localRateUnitStr =
                        std::get<std::string>(input.at("rate_unit").Value);
                    auto maybeRateUnit = TagToPowerUnit(localRateUnitStr);
                    if (!maybeRateUnit.has_value())
                    {
                        errors.push_back(WriteErrorToString(
                            fullTableName,
                            "unhandled rate unit '" + localRateUnitStr + "'"
                        ));
                        return Result::Failure;
                    }
                    localRateUnit = maybeRateUnit.value();
                }
                double maxOutflow_W = Power_ToWatt(
                    std::get<double>(input.at("max_outflow").Value),
                    localRateUnit
                );
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
                    Model_AddVariableEfficiencyConverter(
                        s.TheModel,
                        std::move(outflowsForEff_W),
                        std::move(efficiencyFracs),
                        inflowId,
                        outflowId,
                        lossflowId,
                        tag,
                        report
                    );
                id = compIdAndWasteConn.Id;
                size_t varEffIdx = s.TheModel.ComponentMap.Idx[id];
                s.TheModel.VarEffConvs[varEffIdx].MaxOutflow_W =
                    static_cast<flow_t>(maxOutflow_W);
                if (input.contains("max_lossflow"))
                {
                    double maxLossflow_W = Power_ToWatt(
                        std::get<double>(input.at("max_lossflow").Value),
                        localRateUnit
                    );
                    s.TheModel.VarEffConvs[varEffIdx].MaxLossflow_W =
                        static_cast<flow_t>(maxLossflow_W);
                }
            }
            break;
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
                if (input.contains("max_outflow"))
                {
                    double rawMaxOutflow =
                        std::get<double>(input.at("max_outflow").Value);
                    flow_t maxOutflow_W = static_cast<flow_t>(
                        Power_ToWatt(rawMaxOutflow, rateUnit)
                    );
                    s.TheModel.PassThroughs[s.TheModel.ComponentMap.Idx[id]]
                        .MaxOutflow_W = maxOutflow_W;
                }
            }
            break;
            case ComponentType::StoreType:
            {
                if (inflowId != outflowId)
                {
                    WriteErrorMessage(
                        fullTableName,
                        "inflow type must equal outflow type for store"
                    );
                    return Result::Failure;
                }
                EnergyUnit capacityUnit = EnergyUnit::Joule;
                if (input.contains("capacity_unit"))
                {
                    std::string capacityUnitStr =
                        std::get<std::string>(input.at("capacity_unit").Value);
                    auto maybeCapacityUnit = TagToEnergyUnit(capacityUnitStr);
                    if (!maybeCapacityUnit.has_value())
                    {
                        WriteErrorMessage(
                            fullTableName,
                            "unhandled capacity unit '" + capacityUnitStr + "'"
                        );
                        return Result::Failure;
                    }
                    capacityUnit = maybeCapacityUnit.value();
                }
                flow_t capacity_J = static_cast<flow_t>(Energy_ToJoules(
                    std::get<double>(input.at("capacity").Value), capacityUnit
                ));
                if (capacity_J == 0)
                {
                    WriteErrorMessage(
                        fullTableName, "capacity must be greater than 0"
                    );
                    return Result::Failure;
                }
                flow_t maxCharge_W = static_cast<flow_t>(Power_ToWatt(
                    std::get<double>(input.at("max_charge").Value), rateUnit
                ));
                flow_t maxDischarge_W = static_cast<flow_t>(Power_ToWatt(
                    std::get<double>(input.at("max_discharge").Value), rateUnit
                ));
                double chargeAtSoc =
                    std::get<double>(input.at("charge_at_soc").Value);
                if (chargeAtSoc < 0.0 || chargeAtSoc > 1.0)
                {
                    WriteErrorMessage(
                        fullTableName,
                        "charge_at_soc must be in range [0.0, 1.0]"
                    );
                    return Result::Failure;
                }
                flow_t noChargeAmount_J =
                    static_cast<flow_t>(chargeAtSoc * capacity_J);
                if (noChargeAmount_J == capacity_J)
                {
                    // NOTE: noChargeAmount must be at
                    // least 1 unit less than capacity
                    noChargeAmount_J = capacity_J - 1;
                }
                double initSoc = std::get<double>(input.at("init_soc").Value);
                if (initSoc < 0.0 || initSoc > 1.0)
                {
                    WriteErrorMessage(
                        fullTableName, "init_soc must be in range [0.0, 1.0]"
                    );
                    return Result::Failure;
                }
                flow_t initialStorage_J =
                    static_cast<flow_t>(capacity_J * initSoc);
                double rtEff = 1.0;
                if (input.contains("roundtrip_efficiency"))
                {
                    rtEff =
                        std::get<double>(input.at("roundtrip_efficiency").Value
                        );
                    if (rtEff <= 0.0 || rtEff > 1.0)
                    {
                        WriteErrorMessage(
                            fullTableName,
                            "roundtrip efficiency must be (0.0, 1.0]"
                        );
                        return Result::Failure;
                    }
                }
                if (rtEff == 1.0)
                {
                    id = Model_AddStore(
                        s.TheModel,
                        capacity_J,
                        maxCharge_W,
                        maxDischarge_W,
                        noChargeAmount_J,
                        initialStorage_J,
                        inflowId,
                        tag
                    );
                }
                else
                {
                    auto compIdAndWasteConn = Model_AddStoreWithWasteflow(
                        s.TheModel,
                        capacity_J,
                        maxCharge_W,
                        maxDischarge_W,
                        noChargeAmount_J,
                        initialStorage_J,
                        inflowId,
                        rtEff,
                        tag
                    );
                    id = compIdAndWasteConn.Id;
                }
                if (input.contains("max_outflow"))
                {
                    flow_t maxOutflow_W = static_cast<flow_t>(Power_ToWatt(
                        std::get<double>(input.at("max_outflow").Value),
                        rateUnit
                    ));
                    s.TheModel.Stores[s.TheModel.ComponentMap.Idx[id]]
                        .MaxOutflow_W = maxOutflow_W;
                }
            }
            break;
            case ComponentType::MoverType:
            {
                double cop = std::get<double>(input.at("cop").Value);
                auto compIdAndConns = Model_AddMover(
                    s.TheModel, cop, inflowId, outflowId, tag, report
                );
                id = compIdAndConns.Id;
                if (input.contains("max_outflow"))
                {
                    flow_t maxOutflow_W = static_cast<flow_t>(Power_ToWatt(
                        std::get<double>(input.at("max_outflow").Value),
                        rateUnit
                    ));
                    size_t moverIdx = s.TheModel.ComponentMap.Idx[id];
                    s.TheModel.Movers[moverIdx].MaxOutflow_W = maxOutflow_W;
                }
            }
            break;
            case ComponentType::VariableEfficiencyMoverType:
            {
                PowerUnit localRateUnit = rateUnit;

                std::vector<std::vector<double>> copsByLoadFrac =
                    std::get<std::vector<std::vector<double>>>(
                        input.at("cop_by_fraction_out").Value
                    );
                std::unordered_map<double, double> copByOutFrac{};
                // TODO(mok): extract this and the one in var-eff conv
                // into a separate processing step
                for (std::vector<double> const& fracCopPair : copsByLoadFrac)
                {
                    assert(fracCopPair.size() == 2);
                    double frac = fracCopPair[0];
                    double cop = fracCopPair[1];
                    if (frac < 0.0 || frac > 1.0)
                    {
                        errors.push_back(WriteErrorToString(
                            fullTableName,
                            "Output power fraction must be "
                            "in range [0.0, 1.0]; got "
                                + std::to_string(frac)
                        ));
                        return Result::Failure;
                    }
                    if (cop <= 0.0)
                    {
                        errors.push_back(WriteErrorToString(
                            fullTableName,
                            "COP must be > 0.0" + std::to_string(frac)
                        ));
                        return Result::Failure;
                    }
                    if (copByOutFrac.contains(frac))
                    {
                        warnings.push_back(WriteWarningToString(
                            fullTableName,
                            "Found duplicate value of output fraction: "
                                + std::to_string(frac)
                                + "; overwriting previous"
                        ));
                    }
                    copByOutFrac[frac] = cop;
                }
                if (input.contains("rate_unit"))
                {
                    std::string localRateUnitStr =
                        std::get<std::string>(input.at("rate_unit").Value);
                    auto maybeRateUnit = TagToPowerUnit(localRateUnitStr);
                    if (!maybeRateUnit.has_value())
                    {
                        errors.push_back(WriteErrorToString(
                            fullTableName,
                            "unhandled rate unit '" + localRateUnitStr + "'"
                        ));
                        return Result::Failure;
                    }
                    localRateUnit = maybeRateUnit.value();
                }
                double maxOutflow_W = Power_ToWatt(
                    std::get<double>(input.at("max_outflow").Value),
                    localRateUnit
                );
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
                auto const compIdAndConns = Model_AddVariableEfficiencyMover(
                    s.TheModel,
                    std::move(outflowsForCop_W),
                    std::move(copByOutflow),
                    inflowId,
                    outflowId,
                    tag,
                    report
                );
                id = compIdAndConns.Id;
                size_t moverIdx = s.TheModel.ComponentMap.Idx[id];
                s.TheModel.VarEffMovers[moverIdx].MaxOutflow_W =
                    static_cast<flow_t>(maxOutflow_W);
            }
            break;
            case ComponentType::SwitchType:
            {
                PowerUnit localRateUnit = rateUnit;
                id = Model_AddSwitch(s.TheModel, inflowId, tag);
                if (input.contains("max_outflow"))
                {
                    if (input.contains("rate_unit"))
                    {
                        std::string localRateUnitStr =
                            std::get<std::string>(input.at("rate_unit").Value);
                        auto maybeRateUnit = TagToPowerUnit(localRateUnitStr);
                        if (!maybeRateUnit.has_value())
                        {
                            errors.push_back(WriteErrorToString(
                                fullTableName,
                                "unhandled rate unit '" + localRateUnitStr + "'"
                            ));
                            return Result::Failure;
                        }
                        localRateUnit = maybeRateUnit.value();
                    }
                    double maxOutflow_W = Power_ToWatt(
                        std::get<double>(input.at("max_outflow").Value),
                        localRateUnit
                    );
                    size_t switchIdx = s.TheModel.ComponentMap.Idx[id];
                    s.TheModel.Switches[switchIdx].MaxOutflow_W =
                        static_cast<flow_t>(maxOutflow_W);
                }
            }
            break;
            default:
            {
                WriteErrorMessage(
                    fullTableName, "unhandled component type: " + ToString(ct)
                );
                std::exit(1);
            }
        }
        s.TheModel.ComponentMap.Report[id] = report;
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
        if (table.contains("group"))
        {
            auto maybeGroup =
                TOMLTable_ParseString(table, "group", fullTableName);
            if (!maybeGroup.has_value())
            {
                WriteErrorMessage(
                    fullTableName, "unable to parse 'group' as a string"
                );
                return Result::Failure;
            }
            std::string group = maybeGroup.value();
            AddComponentToGroup(s.TheModel, id, group);
        }
        return Result::Success;
    }

    Result
    ParseComponents(
        Simulation& s,
        toml::table const& table,
        ComponentValidationMap const& compValids,
        std::unordered_set<std::string> const& componentTagsInUse,
        Log const& log
    )
    {
        for (auto it = table.cbegin(); it != table.cend(); ++it)
        {
            std::string const& compTag = it->first;
            if (!componentTagsInUse.contains(compTag))
            {
                std::string tag = "components." + compTag;
                Log_Warning(
                    log,
                    tag,
                    "component is declared but does not appear in network "
                    "connections"
                );
                continue;
            }
            toml::table const& compTable = it->second.as_table();
            auto result =
                ParseSingleComponent(s, compTable, compTag, compValids, log);
            if (result == Result::Failure)
            {
                std::string tag = "components." + compTag;
                Log_Error(log, tag, "could not parse component");
                return Result::Failure;
            }
        }
        return Result::Success;
    }
} // namespace erin
