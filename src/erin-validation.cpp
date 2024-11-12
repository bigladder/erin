// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#include "erin/validation.h"
#include "erin/valdata.h"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <assert.h>

namespace erin
{

std::string InputSection_toString(InputSection s)
{
    switch (s)
    {
    case InputSection::simulation_info:
    {
        return "simulation_info";
    }
    break;
    case InputSection::loads_01explicit:
    {
        return "loads";
    }
    break;
    case InputSection::loads_02file_based:
    {
        return "loads (file_based)";
    }
    break;
    case InputSection::components_constant_load:
    {
        return "components (constant_load)";
    }
    break;
    case InputSection::components_load:
    {
        return "components (load)";
    }
    break;
    case InputSection::components_source:
    {
        return "components (source)";
    }
    break;
    case InputSection::components_uncontrolled_source:
    {
        return "components (uncontrolled_source)";
    }
    break;
    case InputSection::components_const_eff_converter:
    {
        return "components (converter)";
    }
    break;
    case InputSection::components_variable_eff_converter:
    {
        return "components (variable_efficiency_converter)";
    }
    break;
    case InputSection::components_mux:
    {
        return "components (mux)";
    }
    break;
    case InputSection::components_store:
    {
        return "components (store)";
    }
    break;
    case InputSection::components_pass_through:
    {
        return "components (pass_through)";
    }
    break;
    case InputSection::components_mover:
    {
        return "components (mover)";
    }
    break;
    case InputSection::components_variable_eff_mover:
    {
        return "components (variable_efficiency_mover)";
    }
    break;
    case InputSection::dist_fixed:
    {
        return "dist (fixed)";
    }
    break;
    case InputSection::dist_weibull:
    {
        return "dist (weibull)";
    }
    break;
    case InputSection::dist_uniform:
    {
        return "dist (uniform)";
    }
    break;
    case InputSection::dist_normal:
    {
        return "dist (normal)";
    }
    break;
    case InputSection::dist_01quantile_table_from_file:
    {
        return "dist (quantile_table, from file)";
    }
    break;
    case InputSection::dist_02quantile_table_explicit:
    {
        return "dist (quantile_table, explicit)";
    }
    break;
    case InputSection::network:
    {
        return "network";
    }
    break;
    case InputSection::scenarios:
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

std::optional<InputSection> String_toInputSection(std::string tag)
{
    if (tag == "simulation_info")
    {
        return InputSection::simulation_info;
    }
    if (tag == "loads")
    {
        return InputSection::loads_01explicit;
    }
    if (tag == "loads (file_based)")
    {
        return InputSection::loads_02file_based;
    }
    if (tag == "components (source)")
    {
        return InputSection::components_source;
    }
    if (tag == "components (constant_load)")
    {
        return InputSection::components_constant_load;
    }
    if (tag == "components (load)")
    {
        return InputSection::components_load;
    }
    if (tag == "components (converter)")
    {
        return InputSection::components_const_eff_converter;
    }
    if (tag == "components (variable_efficiency_converter)")
    {
        return InputSection::components_variable_eff_converter;
    }
    if (tag == "components (mux)")
    {
        return InputSection::components_mux;
    }
    if (tag == "components (store)")
    {
        return InputSection::components_store;
    }
    if (tag == "components (pass_through)")
    {
        return InputSection::components_pass_through;
    }
    if (tag == "components (mover)")
    {
        return InputSection::components_mover;
    }
    if (tag == "components (variable_efficiency_mover)")
    {
        return InputSection::components_variable_eff_mover;
    }
    if (tag == "dist (fixed)")
    {
        return InputSection::dist_fixed;
    }
    if (tag == "network")
    {
        return InputSection::network;
    }
    if (tag == "scenarios")
    {
        return InputSection::scenarios;
    }
    return {};
}

void UpdateValidationInfoByField(ValidationInfo& info, FieldInfo const& f)
{
    assert(!info.TypeMap.contains(f.FieldName) &&
           "attempt to add same field definition more than once to one "
           "section");
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

InputValidationMap setup_global_validation_info()
{
    std::unordered_set<InputSection> all_sections {
        InputSection::simulation_info,
        InputSection::loads_01explicit,
        InputSection::loads_02file_based,
        InputSection::components_constant_load,
        InputSection::components_load,
        InputSection::components_source,
        InputSection::components_uncontrolled_source,
        InputSection::components_const_eff_converter,
        InputSection::components_variable_eff_converter,
        InputSection::components_mux,
        InputSection::components_store,
        InputSection::components_pass_through,
        InputSection::components_variable_eff_mover,
        InputSection::components_mover,
        InputSection::components_switch,
        InputSection::dist_fixed,
        InputSection::dist_weibull,
        InputSection::dist_uniform,
        InputSection::dist_normal,
        InputSection::dist_01quantile_table_from_file,
        InputSection::dist_02quantile_table_explicit,
        InputSection::network,
        InputSection::scenarios,
    };
    std::unordered_set<InputSection> all_comp_sections {
        InputSection::components_constant_load,
        InputSection::components_load,
        InputSection::components_source,
        InputSection::components_uncontrolled_source,
        InputSection::components_const_eff_converter,
        InputSection::components_variable_eff_converter,
        InputSection::components_mux,
        InputSection::components_store,
        InputSection::components_pass_through,
        InputSection::components_variable_eff_mover,
        InputSection::components_mover,
        InputSection::components_switch,
    };
    std::unordered_set<InputSection> non_load_comp_sections {
        InputSection::components_source,
        InputSection::components_uncontrolled_source,
        InputSection::components_const_eff_converter,
        InputSection::components_variable_eff_converter,
        InputSection::components_mux,
        InputSection::components_store,
        InputSection::components_pass_through,
        InputSection::components_variable_eff_mover,
        InputSection::components_mover,
        InputSection::components_switch,
    };
    std::unordered_set<std::string> comp_type_enums {
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
        "switch",
    };
    std::unordered_set<std::string> dist_type_enums {
        "fixed",
        "uniform",
        "normal",
        "quantile_table",
        "weibull",
    };
    std::unordered_set<InputSection> dist_sections {
        InputSection::dist_fixed,
        InputSection::dist_uniform,
        InputSection::dist_normal,
        InputSection::dist_01quantile_table_from_file,
        InputSection::dist_02quantile_table_explicit,
        InputSection::dist_weibull,
    };
    std::vector<FieldInfo> fields {
        // GLOBAL
        FieldInfo {
            .FieldName = "meta",
            .Type = InputType::Any,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections = all_sections,
        },
        // SIMULATION_INFO
        FieldInfo {
            .FieldName = "input_format_version",
            .Type = InputType::AnyString,
            .IsRequired = false,
            .InformIfMissing = true,
            .Default = current_input_version,
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::simulation_info,
                },
        },
        FieldInfo {
            .FieldName = "rate_unit",
            .Type = InputType::EnumString,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "W",
            .EnumValues = ValidRateUnits,
            .Aliases = {},
            .Sections =
                {
                    InputSection::simulation_info,
                },
        },
        FieldInfo {
            .FieldName = "quantity_unit",
            .Type = InputType::EnumString,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "J",
            .EnumValues = ValidQuantityUnits,
            .Aliases = {},
            .Sections =
                {
                    InputSection::simulation_info,
                },
        },
        FieldInfo {
            .FieldName = "time_unit",
            .Type = InputType::EnumString,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "yr",
            .EnumValues = ValidTimeUnits,
            .Aliases = {},
            .Sections =
                {
                    InputSection::simulation_info,
                },
        },
        FieldInfo {
            .FieldName = "max_time",
            .Type = InputType::Number,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "1000.0",
            .EnumValues = ValidTimeUnits,
            .Aliases = {},
            .Sections =
                {
                    InputSection::simulation_info,
                },
        },
        FieldInfo {
            .FieldName = "random_seed",
            .Type = InputType::Integer,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "17",
            .EnumValues = ValidTimeUnits,
            .Aliases = {},
            .Sections =
                {
                    InputSection::simulation_info,
                },
        },
        FieldInfo {
            .FieldName = "fixed_random",
            .Type = InputType::Number,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::simulation_info,
                },
        },
        FieldInfo {
            .FieldName = "fixed_random_series",
            .Type = InputType::ArrayOfDouble,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::simulation_info,
                },
        },
        // Loads -- File-Based
        FieldInfo {
            .FieldName = "csv_file",
            .Type = InputType::AnyString,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::loads_02file_based,
                },
        },
        // TODO(mok): this should be a 3rd option, not part of
        // Loads_02FileBased
        FieldInfo {
            .FieldName = "multi_part_csv",
            .Type = InputType::AnyString,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::loads_02file_based,
                },
        },
        // Loads -- Explicit
        FieldInfo {
            .FieldName = "time_rate_pairs",
            .Type = InputType::ArrayOfTuple2OfNumber,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::loads_01explicit,
                },
        },
        FieldInfo {
            .FieldName = "time_unit",
            .Type = InputType::EnumString,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "s",
            .EnumValues = ValidTimeUnits,
            .Aliases = {},
            .Sections =
                {
                    InputSection::loads_01explicit,
                },
        },
        FieldInfo {
            .FieldName = "rate_unit",
            .Type = InputType::EnumString,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "W",
            .EnumValues = ValidRateUnits,
            .Aliases = {},
            .Sections =
                {
                    InputSection::loads_01explicit,
                },
        },
        // Components -- Global
        FieldInfo {
            .FieldName = "type",
            .Type = InputType::EnumString,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = comp_type_enums,
            .Aliases = {},
            .Sections = all_comp_sections,
        },
        FieldInfo {
            .FieldName = "initial_age",
            .Type = InputType::Number,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "0.0",
            .EnumValues = {},
            .Aliases = {},
            .Sections = all_comp_sections,
        },
        FieldInfo {
            .FieldName = "time_unit",
            .Type = InputType::EnumString,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "h",
            .EnumValues = ValidTimeUnits,
            .Aliases = {},
            .Sections = all_comp_sections,
        },
        FieldInfo {
            .FieldName = "group",
            .Type = InputType::AnyString,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "group",
            .EnumValues = {},
            .Aliases = {},
            .Sections = all_comp_sections,
        },
        FieldInfo {
            .FieldName = "report",
            .Type = InputType::Bool,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "true",
            .EnumValues = {},
            .Aliases = {},
            .Sections = all_comp_sections,
        },
        // Components -- All Except Loads
        FieldInfo {
            .FieldName = "failure_modes",
            .Type = InputType::ArrayOfString,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections = non_load_comp_sections,
        },
        FieldInfo {
            .FieldName = "fragility_modes",
            .Type = InputType::ArrayOfString,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections = non_load_comp_sections,
        },
        // Constant and Schedule-Based Load Component
        FieldInfo {
            .FieldName = "inflow",
            .Type = InputType::AnyString,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_constant_load,
                    InputSection::components_load,
                    InputSection::components_const_eff_converter,
                    InputSection::components_variable_eff_converter,
                    InputSection::components_mover,
                    InputSection::components_variable_eff_mover,
                },
        },
        FieldInfo {
            .FieldName = "loads_by_scenario",
            .Type = InputType::MapFromStringToString,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_load,
                },
        },
        FieldInfo {
            .FieldName = "constant_request",
            .Type = InputType::Number,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_constant_load,
                },
        },
        // Constant Source and Uncontrolled Source
        FieldInfo {
            .FieldName = "outflow",
            .Type = InputType::AnyString,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_source,
                    InputSection::components_uncontrolled_source,
                    InputSection::components_const_eff_converter,
                    InputSection::components_variable_eff_converter,
                    InputSection::components_mover,
                    InputSection::components_variable_eff_mover,
                },
        },
        FieldInfo {
            .FieldName = "max_outflow",
            .Type = InputType::Number,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_source,
                    InputSection::components_uncontrolled_source,
                    InputSection::components_pass_through,
                    InputSection::components_mover,
                },
        },
        FieldInfo {
            .FieldName = "rate_unit",
            .Type = InputType::AnyString,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_constant_load,
                    InputSection::components_source,
                    InputSection::components_uncontrolled_source,
                    InputSection::components_store,
                    InputSection::components_const_eff_converter,
                    InputSection::components_variable_eff_converter,
                    InputSection::components_mover,
                    InputSection::components_variable_eff_mover,
                },
        },
        FieldInfo {
            .FieldName = "supply_by_scenario",
            .Type = InputType::MapFromStringToString,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_uncontrolled_source,
                },
        },
        // Mux
        FieldInfo {
            .FieldName = "flow",
            .Type = InputType::AnyString,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases =
                {
                    TagWithDeprication {
                        .tag = "stream",
                        .is_deprecated = true,
                    },
                },
            .Sections =
                {
                    InputSection::components_mux,
                    InputSection::components_pass_through,
                    InputSection::components_store,
                    InputSection::components_switch,
                },
        },
        FieldInfo {
            .FieldName = "num_outflows",
            .Type = InputType::Integer,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_mux,
                },
        },
        FieldInfo {
            .FieldName = "num_inflows",
            .Type = InputType::Integer,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_mux,
                },
        },
        FieldInfo {
            .FieldName = "max_outflows",
            .Type = InputType::ArrayOfDouble,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_mux,
                },
        },
        // Constant and Variable Efficiency Converter
        FieldInfo {
            .FieldName = "constant_efficiency",
            .Type = InputType::Number,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_const_eff_converter,
                },
        },
        FieldInfo {
            .FieldName = "efficiency_by_fraction_out",
            .Type = InputType::ArrayOfTuple2OfNumber,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_variable_eff_converter,
                },
        },
        FieldInfo {
            .FieldName = "lossflow",
            .Type = InputType::AnyString,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_const_eff_converter,
                    InputSection::components_variable_eff_converter,
                },
        },
        FieldInfo {
            .FieldName = "max_outflow",
            .Type = InputType::Number,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_const_eff_converter,
                },
        },
        FieldInfo {
            .FieldName = "max_outflow",
            .Type = InputType::Number,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_variable_eff_converter,
                    InputSection::components_variable_eff_mover,
                },
        },
        FieldInfo {
            .FieldName = "max_lossflow",
            .Type = InputType::Number,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_const_eff_converter,
                    InputSection::components_variable_eff_converter,
                },
        },
        // Store
        FieldInfo {
            .FieldName = "init_soc",
            .Type = InputType::Number,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "1.0",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_store,
                },
        },
        FieldInfo {
            .FieldName = "capacity_unit",
            .Type = InputType::EnumString,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "J",
            .EnumValues = ValidQuantityUnits,
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_store,
                },
        },
        FieldInfo {
            .FieldName = "capacity",
            .Type = InputType::Number,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_store,
                },
        },
        // TODO[mok]: should max_charge still be required if now inflow?
        FieldInfo {
            .FieldName = "max_charge",
            .Type = InputType::Number,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {{"max_inflow", true}},
            .Sections =
                {
                    InputSection::components_store,
                },
        },
        FieldInfo {
            .FieldName = "max_discharge",
            .Type = InputType::Number,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_store,
                },
        },
        FieldInfo {
            .FieldName = "max_outflow",
            .Type = InputType::Number,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_store,
                },
        },
        FieldInfo {
            .FieldName = "charge_at_soc",
            .Type = InputType::Number,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "0.8",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_store,
                },
        },
        FieldInfo {
            .FieldName = "roundtrip_efficiency",
            .Type = InputType::Number,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "1.0",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_store,
                },
        },
        // COMP Mover
        FieldInfo {
            .FieldName = "cop",
            .Type = InputType::Number,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_mover,
                },
        },
        FieldInfo {
            .FieldName = "cop_by_fraction_out",
            .Type = InputType::ArrayOfTuple2OfNumber,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::components_variable_eff_mover,
                },
        },
        // DIST: Common
        FieldInfo {
            .FieldName = "type",
            .Type = InputType::EnumString,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = dist_type_enums,
            .Aliases = {},
            .Sections = dist_sections,
        },
        FieldInfo {
            .FieldName = "time_unit",
            .Type = InputType::EnumString,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = ValidTimeUnits,
            .Aliases = {},
            .Sections = dist_sections,
        },
        // DIST - FIXED
        FieldInfo {
            .FieldName = "value",
            .Type = InputType::Number,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::dist_fixed,
                },
        },
        // DIST - UNIFORM
        FieldInfo {
            .FieldName = "lower_bound",
            .Type = InputType::Number,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::dist_uniform,
                },
        },
        FieldInfo {
            .FieldName = "upper_bound",
            .Type = InputType::Number,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::dist_uniform,
                },
        },
        // DIST - NORMAL
        FieldInfo {
            .FieldName = "mean",
            .Type = InputType::Number,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::dist_normal,
                },
        },
        FieldInfo {
            .FieldName = "standard_deviation",
            .Type = InputType::Number,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::dist_normal,
                },
        },
        // DIST - Quantile Table
        FieldInfo {
            .FieldName = "csv_file",
            .Type = InputType::AnyString,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::dist_01quantile_table_from_file,
                },
        },
        FieldInfo {
            .FieldName = "variate_time_pairs",
            .Type = InputType::ArrayOfTuple2OfNumber,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::dist_02quantile_table_explicit,
                },
        },
        // DIST - WEIBULL
        FieldInfo {
            .FieldName = "shape",
            .Type = InputType::Number,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::dist_weibull,
                },
        },
        FieldInfo {
            .FieldName = "scale",
            .Type = InputType::Number,
            .IsRequired = true,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::dist_weibull,
                },
        },
        FieldInfo {
            .FieldName = "location",
            .Type = InputType::Number,
            .IsRequired = false,
            .InformIfMissing = false,
            .Default = "",
            .EnumValues = {},
            .Aliases = {},
            .Sections =
                {
                    InputSection::dist_weibull,
                },
        },
    };
    InputValidationMap v {};
    for (auto const& f : fields)
    {
        if (f.Sections.size() == 0)
        {
            std::cerr << "Program Initialization Error: "
                      << "field '" << f.FieldName << "' has no "
                      << "sections that it applies to" << std::endl;
            std::exit(1);
        }
        for (auto const sec : f.Sections)
        {
            switch (sec)
            {
            case InputSection::simulation_info:
            {
                UpdateValidationInfoByField(v.SimulationInfo, f);
            }
            break;
            case InputSection::loads_01explicit:
            {
                UpdateValidationInfoByField(v.Load_01Explicit, f);
            }
            break;
            case InputSection::loads_02file_based:
            {
                UpdateValidationInfoByField(v.Load_02FileBased, f);
            }
            break;
            case InputSection::components_constant_load:
            {
                UpdateValidationInfoByField(v.Comp.ConstantLoad, f);
            }
            break;
            case InputSection::components_load:
            {
                UpdateValidationInfoByField(v.Comp.ScheduleBasedLoad, f);
            }
            break;
            case InputSection::components_source:
            {
                UpdateValidationInfoByField(v.Comp.ConstantSource, f);
            }
            break;
            case InputSection::components_uncontrolled_source:
            {
                UpdateValidationInfoByField(v.Comp.ScheduleBasedSource, f);
            }
            break;
            case InputSection::components_const_eff_converter:
            {
                UpdateValidationInfoByField(v.Comp.ConstantEfficiencyConverter, f);
            }
            break;
            case InputSection::components_variable_eff_converter:
            {
                UpdateValidationInfoByField(v.Comp.VariableEfficiencyConverter, f);
            }
            break;
            case InputSection::components_mux:
            {
                UpdateValidationInfoByField(v.Comp.Mux, f);
            }
            break;
            case InputSection::components_store:
            {
                UpdateValidationInfoByField(v.Comp.Store, f);
            }
            break;
            case InputSection::components_pass_through:
            {
                UpdateValidationInfoByField(v.Comp.PassThrough, f);
            }
            break;
            case InputSection::components_mover:
            {
                UpdateValidationInfoByField(v.Comp.Mover, f);
            }
            break;
            case InputSection::components_variable_eff_mover:
            {
                UpdateValidationInfoByField(v.Comp.VariableEfficiencyMover, f);
            }
            break;
            case InputSection::components_switch:
            {
                UpdateValidationInfoByField(v.Comp.Switch, f);
            }
            break;
            case InputSection::dist_fixed:
            {
                UpdateValidationInfoByField(v.Dist.Fixed, f);
            }
            break;
            case InputSection::dist_normal:
            {
                UpdateValidationInfoByField(v.Dist.Normal, f);
            }
            break;
            case InputSection::dist_01quantile_table_from_file:
            {
                UpdateValidationInfoByField(v.Dist.QuantileTableFromFile, f);
            }
            break;
            case InputSection::dist_02quantile_table_explicit:
            {
                UpdateValidationInfoByField(v.Dist.QuantileTableExplicit, f);
            }
            break;
            case InputSection::dist_uniform:
            {
                UpdateValidationInfoByField(v.Dist.Uniform, f);
            }
            break;
            case InputSection::dist_weibull:
            {
                UpdateValidationInfoByField(v.Dist.Weibull, f);
            }
            break;
            // TODO: add in all the other distributions
            case InputSection::network:
            {
                UpdateValidationInfoByField(v.Network, f);
            }
            break;
            case InputSection::scenarios:
            {
                UpdateValidationInfoByField(v.Scenario, f);
            }
            break;
            default:
            {
                std::cerr << "Program Initialization Error: "
                          << "unhandled section '" << InputSection_toString(sec) << "'"
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
