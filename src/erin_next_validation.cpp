/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_validation.h"
#include "erin_next/erin_next_valdata.h"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <assert.h>

namespace erin
{

    std::string
    InputSection_toString(InputSection s)
    {
        switch (s)
        {
            case InputSection::SimulationInfo:
            {
                return "simulation_info";
            }
            break;
            case InputSection::Loads_01Explicit:
            {
                return "loads";
            }
            break;
            case InputSection::Loads_02FileBased:
            {
                return "loads (file_based)";
            }
            break;
            case InputSection::Components_ConstantLoad:
            {
                return "components (constant_load)";
            }
            break;
            case InputSection::Components_Load:
            {
                return "components (load)";
            }
            break;
            case InputSection::Components_Source:
            {
                return "components (source)";
            }
            break;
            case InputSection::Components_UncontrolledSource:
            {
                return "components (uncontrolled_source)";
            }
            break;
            case InputSection::Components_ConstEffConverter:
            {
                return "components (converter)";
            }
            break;
            case InputSection::Components_VariableEffConverter:
            {
                return "components (variable_efficiency_converter)";
            }
            break;
            case InputSection::Components_Mux:
            {
                return "components (mux)";
            }
            break;
            case InputSection::Components_Store:
            {
                return "components (store)";
            }
            break;
            case InputSection::Components_PassThrough:
            {
                return "components (pass_through)";
            }
            break;
            case InputSection::Components_Mover:
            {
                return "components (mover)";
            }
            break;
            case InputSection::Components_VariableEffMover:
            {
                return "components (variable_efficiency_mover)";
            }
            break;
            case InputSection::Dist_Fixed:
            {
                return "dist (fixed)";
            }
            break;
            case InputSection::Dist_Weibull:
            {
                return "dist (weibull)";
            }
            break;
            case InputSection::Dist_Uniform:
            {
                return "dist (uniform)";
            }
            break;
            case InputSection::Dist_Normal:
            {
                return "dist (normal)";
            }
            break;
            case InputSection::Dist_01QuantileTableFromFile:
            {
                return "dist (quantile_table, from file)";
            }
            break;
            case InputSection::Dist_02QuantileTableExplicit:
            {
                return "dist (quantile_table, explicit)";
            }
            break;
            case InputSection::Network:
            {
                return "network";
            }
            break;
            case InputSection::Scenarios:
            {
                return "scenarios";
            }
            break;
            default:
            {
            }
            break;
        }
        std::cerr << "Unhandled scenario" << std::endl;
        std::exit(1);
    }

    std::optional<InputSection>
    String_toInputSection(std::string tag)
    {
        if (tag == "simulation_info")
        {
            return InputSection::SimulationInfo;
        }
        if (tag == "loads")
        {
            return InputSection::Loads_01Explicit;
        }
        if (tag == "loads (file_based)")
        {
            return InputSection::Loads_02FileBased;
        }
        if (tag == "components (source)")
        {
            return InputSection::Components_Source;
        }
        if (tag == "components (constant_load)")
        {
            return InputSection::Components_ConstantLoad;
        }
        if (tag == "components (load)")
        {
            return InputSection::Components_Load;
        }
        if (tag == "components (converter)")
        {
            return InputSection::Components_ConstEffConverter;
        }
        if (tag == "components (variable_efficiency_converter)")
        {
            return InputSection::Components_VariableEffConverter;
        }
        if (tag == "components (mux)")
        {
            return InputSection::Components_Mux;
        }
        if (tag == "components (store)")
        {
            return InputSection::Components_Store;
        }
        if (tag == "components (pass_through)")
        {
            return InputSection::Components_PassThrough;
        }
        if (tag == "components (mover)")
        {
            return InputSection::Components_Mover;
        }
        if (tag == "components (variable_efficiency_mover)")
        {
            return InputSection::Components_VariableEffMover;
        }
        if (tag == "dist (fixed)")
        {
            return InputSection::Dist_Fixed;
        }
        if (tag == "network")
        {
            return InputSection::Network;
        }
        if (tag == "scenarios")
        {
            return InputSection::Scenarios;
        }
        return {};
    }

    void
    UpdateValidationInfoByField(ValidationInfo& info, FieldInfo const& f)
    {
        assert(
            !info.TypeMap.contains(f.FieldName)
            && "attempt to add same field definition more than once to one "
               "section"
        );
        info.TypeMap.insert({f.FieldName, f.Type});
        if (f.Type == InputType::EnumString)
        {
            assert(f.EnumValues.size() > 0);
            info.EnumMap.insert({f.FieldName, f.EnumValues});
        }
        if (f.IsRequired)
        {
            info.RequiredFields.insert(f.FieldName);
        }
        else
        {
            info.OptionalFields.insert(f.FieldName);
        }
        if (!f.Default.empty())
        {
            info.Defaults.insert({f.FieldName, f.Default});
        }
        if (f.Aliases.size() > 0)
        {
            info.Aliases.insert({f.FieldName, f.Aliases});
        }
        if (f.InformIfMissing)
        {
            // NOTE: to inform if missing, we must have a default
            // ... otherwise it would be an error.
            assert(info.Defaults.contains(f.FieldName));
            info.InformIfMissing.insert(f.FieldName);
        }
    }

    InputValidationMap
    SetupGlobalValidationInfo()
    {
        std::unordered_set<InputSection> allSections{
            InputSection::SimulationInfo,
            InputSection::Loads_01Explicit,
            InputSection::Loads_02FileBased,
            InputSection::Components_ConstantLoad,
            InputSection::Components_Load,
            InputSection::Components_Source,
            InputSection::Components_UncontrolledSource,
            InputSection::Components_ConstEffConverter,
            InputSection::Components_VariableEffConverter,
            InputSection::Components_Mux,
            InputSection::Components_Store,
            InputSection::Components_PassThrough,
            InputSection::Components_VariableEffMover,
            InputSection::Components_Mover,
            InputSection::Dist_Fixed,
            InputSection::Dist_Weibull,
            InputSection::Dist_Uniform,
            InputSection::Dist_Normal,
            InputSection::Dist_01QuantileTableFromFile,
            InputSection::Dist_02QuantileTableExplicit,
            InputSection::Network,
            InputSection::Scenarios,
        };
        std::unordered_set<InputSection> allCompSections{
            InputSection::Components_ConstantLoad,
            InputSection::Components_Load,
            InputSection::Components_Source,
            InputSection::Components_UncontrolledSource,
            InputSection::Components_ConstEffConverter,
            InputSection::Components_VariableEffConverter,
            InputSection::Components_Mux,
            InputSection::Components_Store,
            InputSection::Components_PassThrough,
            InputSection::Components_VariableEffMover,
            InputSection::Components_Mover,
        };
        std::unordered_set<InputSection> nonLoadCompSections{
            InputSection::Components_Source,
            InputSection::Components_UncontrolledSource,
            InputSection::Components_ConstEffConverter,
            InputSection::Components_VariableEffConverter,
            InputSection::Components_Mux,
            InputSection::Components_Store,
            InputSection::Components_PassThrough,
            InputSection::Components_VariableEffMover,
            InputSection::Components_Mover,
        };
        std::unordered_set<std::string> compTypeEnums{
            "constant_load",
            "converter",
            "variable_efficiency_converter",
            "load",
            "mover",
            "variable_efficiency_mover",
            "muxer",
            "mux",
            "pass_through",
            "source",
            "store",
            "uncontrolled_source",
        };
        std::unordered_set<std::string> distTypeEnums{
            "fixed",
            "uniform",
            "normal",
            "quantile_table",
            "weibull",
        };
        std::unordered_set<InputSection> distSections{
            InputSection::Dist_Fixed,
            InputSection::Dist_Uniform,
            InputSection::Dist_Normal,
            InputSection::Dist_01QuantileTableFromFile,
            InputSection::Dist_02QuantileTableExplicit,
            InputSection::Dist_Weibull,
        };
        std::vector<FieldInfo> fields{
            // GLOBAL
            FieldInfo{
                .FieldName = "meta",
                .Type = InputType::Any,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections = allSections,
            },
            // SIMULATION_INFO
            FieldInfo{
                .FieldName = "input_format_version",
                .Type = InputType::AnyString,
                .IsRequired = false,
                .InformIfMissing = true,
                .Default = current_input_version,
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::SimulationInfo,
                    },
            },
            FieldInfo{
                .FieldName = "rate_unit",
                .Type = InputType::EnumString,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "W",
                .EnumValues = ValidRateUnits,
                .Aliases = {},
                .Sections =
                    {
                        InputSection::SimulationInfo,
                    },
            },
            FieldInfo{
                .FieldName = "quantity_unit",
                .Type = InputType::EnumString,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "J",
                .EnumValues = ValidQuantityUnits,
                .Aliases = {},
                .Sections =
                    {
                        InputSection::SimulationInfo,
                    },
            },
            FieldInfo{
                .FieldName = "time_unit",
                .Type = InputType::EnumString,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "yr",
                .EnumValues = ValidTimeUnits,
                .Aliases = {},
                .Sections =
                    {
                        InputSection::SimulationInfo,
                    },
            },
            FieldInfo{
                .FieldName = "max_time",
                .Type = InputType::Number,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "1000.0",
                .EnumValues = ValidTimeUnits,
                .Aliases = {},
                .Sections =
                    {
                        InputSection::SimulationInfo,
                    },
            },
            FieldInfo{
                .FieldName = "random_seed",
                .Type = InputType::Integer,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "17",
                .EnumValues = ValidTimeUnits,
                .Aliases = {},
                .Sections =
                    {
                        InputSection::SimulationInfo,
                    },
            },
            FieldInfo{
                .FieldName = "fixed_random",
                .Type = InputType::Number,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::SimulationInfo,
                    },
            },
            FieldInfo{
                .FieldName = "fixed_random_series",
                .Type = InputType::ArrayOfDouble,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::SimulationInfo,
                    },
            },
            // Loads -- File-Based
            FieldInfo{
                .FieldName = "csv_file",
                .Type = InputType::AnyString,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Loads_02FileBased,
                    },
            },
            // Loads -- Explicit
            FieldInfo{
                .FieldName = "time_rate_pairs",
                .Type = InputType::ArrayOfTuple2OfNumber,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Loads_01Explicit,
                    },
            },
            FieldInfo{
                .FieldName = "time_unit",
                .Type = InputType::EnumString,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "s",
                .EnumValues = ValidTimeUnits,
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Loads_01Explicit,
                    },
            },
            FieldInfo{
                .FieldName = "rate_unit",
                .Type = InputType::EnumString,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "W",
                .EnumValues = ValidRateUnits,
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Loads_01Explicit,
                    },
            },
            // Components -- Global
            FieldInfo{
                .FieldName = "type",
                .Type = InputType::EnumString,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = compTypeEnums,
                .Aliases = {},
                .Sections = allCompSections,
            },
            FieldInfo{
                .FieldName = "initial_age",
                .Type = InputType::Number,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "0.0",
                .EnumValues = {},
                .Aliases = {},
                .Sections = allCompSections,
            },
            FieldInfo{
                .FieldName = "time_unit",
                .Type = InputType::EnumString,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "h",
                .EnumValues = ValidTimeUnits,
                .Aliases = {},
                .Sections = allCompSections,
            },
            // Components -- All Except Loads
            FieldInfo{
                .FieldName = "failure_modes",
                .Type = InputType::ArrayOfString,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections = nonLoadCompSections,
            },
            FieldInfo{
                .FieldName = "fragility_modes",
                .Type = InputType::ArrayOfString,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections = nonLoadCompSections,
            },
            // Constant and Schedule-Based Load Component
            FieldInfo{
                .FieldName = "inflow",
                .Type = InputType::AnyString,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_ConstantLoad,
                        InputSection::Components_Load,
                        InputSection::Components_ConstEffConverter,
                        InputSection::Components_VariableEffConverter,
                        InputSection::Components_Mover,
                        InputSection::Components_VariableEffMover,
                    },
            },
            FieldInfo{
                .FieldName = "loads_by_scenario",
                .Type = InputType::MapFromStringToString,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_Load,
                    },
            },
            // Constant Source and Uncontrolled Source
            FieldInfo{
                .FieldName = "outflow",
                .Type = InputType::AnyString,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_Source,
                        InputSection::Components_UncontrolledSource,
                        InputSection::Components_ConstEffConverter,
                        InputSection::Components_VariableEffConverter,
                        InputSection::Components_Mover,
                        InputSection::Components_VariableEffMover,
                    },
            },
            FieldInfo{
                .FieldName = "max_outflow",
                .Type = InputType::Number,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_Source,
                        InputSection::Components_UncontrolledSource,
                        InputSection::Components_PassThrough,
                        InputSection::Components_Mover,
                    },
            },
            FieldInfo{
                .FieldName = "rate_unit",
                .Type = InputType::AnyString,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_Source,
                        InputSection::Components_UncontrolledSource,
                        InputSection::Components_Store,
                        InputSection::Components_ConstEffConverter,
                        InputSection::Components_VariableEffConverter,
                        InputSection::Components_Mover,
                        InputSection::Components_VariableEffMover,
                    },
            },
            FieldInfo{
                .FieldName = "supply_by_scenario",
                .Type = InputType::MapFromStringToString,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_UncontrolledSource,
                    },
            },
            // Mux
            FieldInfo{
                .FieldName = "flow",
                .Type = InputType::AnyString,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases =
                    {
                        TagWithDeprication{
                            .Tag = "stream",
                            .IsDeprecated = true,
                        },
                    },
                .Sections =
                    {
                        InputSection::Components_Mux,
                        InputSection::Components_PassThrough,
                        InputSection::Components_Store,
                    },
            },
            FieldInfo{
                .FieldName = "num_outflows",
                .Type = InputType::Integer,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_Mux,
                    },
            },
            FieldInfo{
                .FieldName = "num_inflows",
                .Type = InputType::Integer,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_Mux,
                    },
            },
            FieldInfo{
                .FieldName = "max_outflows",
                .Type = InputType::ArrayOfDouble,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_Mux,
                    },
            },
            // Constant and Variable Efficiency Converter
            FieldInfo{
                .FieldName = "constant_efficiency",
                .Type = InputType::Number,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_ConstEffConverter,
                    },
            },
            FieldInfo{
                .FieldName = "efficiency_by_fraction_out",
                .Type = InputType::ArrayOfTuple2OfNumber,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_VariableEffConverter,
                    },
            },
            FieldInfo{
                .FieldName = "lossflow",
                .Type = InputType::AnyString,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_ConstEffConverter,
                        InputSection::Components_VariableEffConverter,
                    },
            },
            FieldInfo{
                .FieldName = "max_outflow",
                .Type = InputType::Number,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_ConstEffConverter,
                    },
            },
            FieldInfo{
                .FieldName = "max_outflow",
                .Type = InputType::Number,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_VariableEffConverter,
                        InputSection::Components_VariableEffMover,
                    },
            },
            FieldInfo{
                .FieldName = "max_lossflow",
                .Type = InputType::Number,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_ConstEffConverter,
                        InputSection::Components_VariableEffConverter,
                    },
            },
            // Store
            FieldInfo{
                .FieldName = "init_soc",
                .Type = InputType::Number,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "1.0",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_Store,
                    },
            },
            FieldInfo{
                .FieldName = "capacity_unit",
                .Type = InputType::EnumString,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "J",
                .EnumValues = ValidQuantityUnits,
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_Store,
                    },
            },
            FieldInfo{
                .FieldName = "capacity",
                .Type = InputType::Number,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_Store,
                    },
            },
            // TODO[mok]: should max_charge still be required if now inflow?
            FieldInfo{
                .FieldName = "max_charge",
                .Type = InputType::Number,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {{"max_inflow", true}},
                .Sections =
                    {
                        InputSection::Components_Store,
                    },
            },
            FieldInfo{
                .FieldName = "max_discharge",
                .Type = InputType::Number,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_Store,
                    },
            },
            FieldInfo{
                .FieldName = "max_outflow",
                .Type = InputType::Number,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_Store,
                    },
            },
            FieldInfo{
                .FieldName = "charge_at_soc",
                .Type = InputType::Number,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "0.8",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_Store,
                    },
            },
            FieldInfo{
                .FieldName = "roundtrip_efficiency",
                .Type = InputType::Number,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "1.0",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_Store,
                    },
            },
            // COMP Mover
            FieldInfo{
                .FieldName = "cop",
                .Type = InputType::Number,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_Mover,
                    },
            },
            FieldInfo{
                .FieldName = "cop_by_fraction_out",
                .Type = InputType::ArrayOfTuple2OfNumber,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Components_VariableEffMover,
                    },
            },
            // DIST: Common
            FieldInfo{
                .FieldName = "type",
                .Type = InputType::EnumString,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = distTypeEnums,
                .Aliases = {},
                .Sections = distSections,
            },
            FieldInfo{
                .FieldName = "time_unit",
                .Type = InputType::EnumString,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = ValidTimeUnits,
                .Aliases = {},
                .Sections = distSections,
            },
            // DIST - FIXED
            FieldInfo{
                .FieldName = "value",
                .Type = InputType::Number,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Dist_Fixed,
                    },
            },
            // DIST - UNIFORM
            FieldInfo{
                .FieldName = "lower_bound",
                .Type = InputType::Number,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Dist_Uniform,
                    },
            },
            FieldInfo{
                .FieldName = "upper_bound",
                .Type = InputType::Number,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Dist_Uniform,
                    },
            },
            // DIST - NORMAL
            FieldInfo{
                .FieldName = "mean",
                .Type = InputType::Number,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Dist_Normal,
                    },
            },
            FieldInfo{
                .FieldName = "standard_deviation",
                .Type = InputType::Number,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Dist_Normal,
                    },
            },
            // DIST - Quantile Table
            FieldInfo{
                .FieldName = "csv_file",
                .Type = InputType::AnyString,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Dist_01QuantileTableFromFile,
                    },
            },
            FieldInfo{
                .FieldName = "variate_time_pairs",
                .Type = InputType::ArrayOfTuple2OfNumber,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Dist_02QuantileTableExplicit,
                    },
            },
            // DIST - WEIBULL
            FieldInfo{
                .FieldName = "shape",
                .Type = InputType::Number,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Dist_Weibull,
                    },
            },
            FieldInfo{
                .FieldName = "scale",
                .Type = InputType::Number,
                .IsRequired = true,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Dist_Weibull,
                    },
            },
            FieldInfo{
                .FieldName = "location",
                .Type = InputType::Number,
                .IsRequired = false,
                .InformIfMissing = false,
                .Default = "",
                .EnumValues = {},
                .Aliases = {},
                .Sections =
                    {
                        InputSection::Dist_Weibull,
                    },
            },
        };
        InputValidationMap v{};
        for (auto const& f : fields)
        {
            if (f.Sections.size() == 0)
            {
                std::cerr << "Program Initialization Error: " << "field '"
                          << f.FieldName << "' has no "
                          << "sections that it applies to" << std::endl;
                std::exit(1);
            }
            for (auto const sec : f.Sections)
            {
                switch (sec)
                {
                    case InputSection::SimulationInfo:
                    {
                        UpdateValidationInfoByField(v.SimulationInfo, f);
                    }
                    break;
                    case InputSection::Loads_01Explicit:
                    {
                        UpdateValidationInfoByField(v.Load_01Explicit, f);
                    }
                    break;
                    case InputSection::Loads_02FileBased:
                    {
                        UpdateValidationInfoByField(v.Load_02FileBased, f);
                    }
                    break;
                    case InputSection::Components_ConstantLoad:
                    {
                        UpdateValidationInfoByField(v.Comp.ConstantLoad, f);
                    }
                    break;
                    case InputSection::Components_Load:
                    {
                        UpdateValidationInfoByField(
                            v.Comp.ScheduleBasedLoad, f
                        );
                    }
                    break;
                    case InputSection::Components_Source:
                    {
                        UpdateValidationInfoByField(v.Comp.ConstantSource, f);
                    }
                    break;
                    case InputSection::Components_UncontrolledSource:
                    {
                        UpdateValidationInfoByField(
                            v.Comp.ScheduleBasedSource, f
                        );
                    }
                    break;
                    case InputSection::Components_ConstEffConverter:
                    {
                        UpdateValidationInfoByField(
                            v.Comp.ConstantEfficiencyConverter, f
                        );
                    }
                    break;
                    case InputSection::Components_VariableEffConverter:
                    {
                        UpdateValidationInfoByField(
                            v.Comp.VariableEfficiencyConverter, f
                        );
                    }
                    break;
                    case InputSection::Components_Mux:
                    {
                        UpdateValidationInfoByField(v.Comp.Mux, f);
                    }
                    break;
                    case InputSection::Components_Store:
                    {
                        UpdateValidationInfoByField(v.Comp.Store, f);
                    }
                    break;
                    case InputSection::Components_PassThrough:
                    {
                        UpdateValidationInfoByField(v.Comp.PassThrough, f);
                    }
                    break;
                    case InputSection::Components_Mover:
                    {
                        UpdateValidationInfoByField(v.Comp.Mover, f);
                    }
                    break;
                    case InputSection::Components_VariableEffMover:
                    {
                        UpdateValidationInfoByField(
                            v.Comp.VariableEfficiencyMover, f
                        );
                    }
                    break;
                    case InputSection::Dist_Fixed:
                    {
                        UpdateValidationInfoByField(v.Dist.Fixed, f);
                    }
                    break;
                    case InputSection::Dist_Normal:
                    {
                        UpdateValidationInfoByField(v.Dist.Normal, f);
                    }
                    break;
                    case InputSection::Dist_01QuantileTableFromFile:
                    {
                        UpdateValidationInfoByField(
                            v.Dist.QuantileTableFromFile, f
                        );
                    }
                    break;
                    case InputSection::Dist_02QuantileTableExplicit:
                    {
                        UpdateValidationInfoByField(
                            v.Dist.QuantileTableExplicit, f
                        );
                    }
                    break;
                    case InputSection::Dist_Uniform:
                    {
                        UpdateValidationInfoByField(v.Dist.Uniform, f);
                    }
                    break;
                    case InputSection::Dist_Weibull:
                    {
                        UpdateValidationInfoByField(v.Dist.Weibull, f);
                    }
                    break;
                    // TODO: add in all the other distributions
                    case InputSection::Network:
                    {
                        UpdateValidationInfoByField(v.Network, f);
                    }
                    break;
                    case InputSection::Scenarios:
                    {
                        UpdateValidationInfoByField(v.Scenario, f);
                    }
                    break;
                    default:
                    {
                        std::cerr << "Program Initialization Error: "
                                  << "unhandled section '"
                                  << InputSection_toString(sec) << "'"
                                  << std::endl;
                        std::exit(1);
                    }
                    break;
                }
            }
        }
        return v;
    }

} // namespace erin
