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
    assert(!info.field_to_type.contains(f.field_name) &&
           "attempt to add same field definition more than once to one "
           "section");
    info.field_to_type.insert({f.field_name, f.input_type});
    if (f.input_type == InputType::enum_string)
    {
        assert(f.enum_values.size() > 0);
        info.enum_map.insert({f.field_name, f.enum_values});
    }
    if (f.is_required)
    {
        info.required_fields.insert(f.field_name);
    }
    else
    {
        info.optional_fields.insert(f.field_name);
    }
    if (!f.default_value.empty())
    {
        info.default_values.insert({f.field_name, f.default_value});
    }
    if (f.aliases.size() > 0)
    {
        info.aliases.insert({f.field_name, f.aliases});
    }
    if (f.inform_if_missing)
    {
        // NOTE: to inform if missing, we must have a default
        // ... otherwise it would be an error.
        assert(info.default_values.contains(f.field_name));
        info.inform_if_missing.insert(f.field_name);
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
            .field_name = "meta",
            .input_type = InputType::any,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections = all_sections,
        },
        // SIMULATION_INFO
        FieldInfo {
            .field_name = "input_format_version",
            .input_type = InputType::string,
            .is_required = false,
            .inform_if_missing = true,
            .default_value = current_input_version,
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::simulation_info,
                },
        },
        FieldInfo {
            .field_name = "rate_unit",
            .input_type = InputType::enum_string,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "W",
            .enum_values = ValidRateUnits,
            .aliases = {},
            .sections =
                {
                    InputSection::simulation_info,
                },
        },
        FieldInfo {
            .field_name = "quantity_unit",
            .input_type = InputType::enum_string,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "J",
            .enum_values = ValidQuantityUnits,
            .aliases = {},
            .sections =
                {
                    InputSection::simulation_info,
                },
        },
        FieldInfo {
            .field_name = "time_unit",
            .input_type = InputType::enum_string,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "yr",
            .enum_values = ValidTimeUnits,
            .aliases = {},
            .sections =
                {
                    InputSection::simulation_info,
                },
        },
        FieldInfo {
            .field_name = "max_time",
            .input_type = InputType::number,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "1000.0",
            .enum_values = ValidTimeUnits,
            .aliases = {},
            .sections =
                {
                    InputSection::simulation_info,
                },
        },
        FieldInfo {
            .field_name = "random_seed",
            .input_type = InputType::integer,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "17",
            .enum_values = ValidTimeUnits,
            .aliases = {},
            .sections =
                {
                    InputSection::simulation_info,
                },
        },
        FieldInfo {
            .field_name = "fixed_random",
            .input_type = InputType::number,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::simulation_info,
                },
        },
        FieldInfo {
            .field_name = "fixed_random_series",
            .input_type = InputType::array_of_double,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::simulation_info,
                },
        },
        // Loads -- File-Based
        FieldInfo {
            .field_name = "csv_file",
            .input_type = InputType::string,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::loads_02file_based,
                },
        },
        // TODO(mok): this should be a 3rd option, not part of
        // Loads_02FileBased
        FieldInfo {
            .field_name = "multi_part_csv",
            .input_type = InputType::string,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::loads_02file_based,
                },
        },
        // Loads -- Explicit
        FieldInfo {
            .field_name = "time_rate_pairs",
            .input_type = InputType::array_of_tuple2_of_number,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::loads_01explicit,
                },
        },
        FieldInfo {
            .field_name = "time_unit",
            .input_type = InputType::enum_string,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "s",
            .enum_values = ValidTimeUnits,
            .aliases = {},
            .sections =
                {
                    InputSection::loads_01explicit,
                },
        },
        FieldInfo {
            .field_name = "rate_unit",
            .input_type = InputType::enum_string,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "W",
            .enum_values = ValidRateUnits,
            .aliases = {},
            .sections =
                {
                    InputSection::loads_01explicit,
                },
        },
        // Components -- Global
        FieldInfo {
            .field_name = "type",
            .input_type = InputType::enum_string,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = comp_type_enums,
            .aliases = {},
            .sections = all_comp_sections,
        },
        FieldInfo {
            .field_name = "initial_age",
            .input_type = InputType::number,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "0.0",
            .enum_values = {},
            .aliases = {},
            .sections = all_comp_sections,
        },
        FieldInfo {
            .field_name = "time_unit",
            .input_type = InputType::enum_string,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "h",
            .enum_values = ValidTimeUnits,
            .aliases = {},
            .sections = all_comp_sections,
        },
        FieldInfo {
            .field_name = "group",
            .input_type = InputType::string,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "group",
            .enum_values = {},
            .aliases = {},
            .sections = all_comp_sections,
        },
        FieldInfo {
            .field_name = "report",
            .input_type = InputType::boolean,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "true",
            .enum_values = {},
            .aliases = {},
            .sections = all_comp_sections,
        },
        // Components -- All Except Loads
        FieldInfo {
            .field_name = "failure_modes",
            .input_type = InputType::array_of_string,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections = non_load_comp_sections,
        },
        FieldInfo {
            .field_name = "fragility_modes",
            .input_type = InputType::array_of_string,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections = non_load_comp_sections,
        },
        // Constant and Schedule-Based Load Component
        FieldInfo {
            .field_name = "inflow",
            .input_type = InputType::string,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
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
            .field_name = "loads_by_scenario",
            .input_type = InputType::map_from_string_to_string,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_load,
                },
        },
        FieldInfo {
            .field_name = "constant_request",
            .input_type = InputType::number,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_constant_load,
                },
        },
        // Constant Source and Uncontrolled Source
        FieldInfo {
            .field_name = "outflow",
            .input_type = InputType::string,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
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
            .field_name = "max_outflow",
            .input_type = InputType::number,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_source,
                    InputSection::components_uncontrolled_source,
                    InputSection::components_pass_through,
                    InputSection::components_mover,
                },
        },
        FieldInfo {
            .field_name = "rate_unit",
            .input_type = InputType::string,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
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
            .field_name = "supply_by_scenario",
            .input_type = InputType::map_from_string_to_string,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_uncontrolled_source,
                },
        },
        // Mux
        FieldInfo {
            .field_name = "flow",
            .input_type = InputType::string,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases =
                {
                    TagWithDeprication {
                        .tag = "stream",
                        .is_deprecated = true,
                    },
                },
            .sections =
                {
                    InputSection::components_mux,
                    InputSection::components_pass_through,
                    InputSection::components_store,
                    InputSection::components_switch,
                },
        },
        FieldInfo {
            .field_name = "num_outflows",
            .input_type = InputType::integer,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_mux,
                },
        },
        FieldInfo {
            .field_name = "num_inflows",
            .input_type = InputType::integer,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_mux,
                },
        },
        FieldInfo {
            .field_name = "max_outflows",
            .input_type = InputType::array_of_double,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_mux,
                },
        },
        // Constant and Variable Efficiency Converter
        FieldInfo {
            .field_name = "constant_efficiency",
            .input_type = InputType::number,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_const_eff_converter,
                },
        },
        FieldInfo {
            .field_name = "efficiency_by_fraction_out",
            .input_type = InputType::array_of_tuple2_of_number,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_variable_eff_converter,
                },
        },
        FieldInfo {
            .field_name = "lossflow",
            .input_type = InputType::string,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_const_eff_converter,
                    InputSection::components_variable_eff_converter,
                },
        },
        FieldInfo {
            .field_name = "max_outflow",
            .input_type = InputType::number,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_const_eff_converter,
                },
        },
        FieldInfo {
            .field_name = "max_outflow",
            .input_type = InputType::number,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_variable_eff_converter,
                    InputSection::components_variable_eff_mover,
                },
        },
        FieldInfo {
            .field_name = "max_lossflow",
            .input_type = InputType::number,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_const_eff_converter,
                    InputSection::components_variable_eff_converter,
                },
        },
        // Store
        FieldInfo {
            .field_name = "init_soc",
            .input_type = InputType::number,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "1.0",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_store,
                },
        },
        FieldInfo {
            .field_name = "capacity_unit",
            .input_type = InputType::enum_string,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "J",
            .enum_values = ValidQuantityUnits,
            .aliases = {},
            .sections =
                {
                    InputSection::components_store,
                },
        },
        FieldInfo {
            .field_name = "capacity",
            .input_type = InputType::number,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_store,
                },
        },
        // TODO[mok]: should max_charge still be required if now inflow?
        FieldInfo {
            .field_name = "max_charge",
            .input_type = InputType::number,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {{"max_inflow", true}},
            .sections =
                {
                    InputSection::components_store,
                },
        },
        FieldInfo {
            .field_name = "max_discharge",
            .input_type = InputType::number,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_store,
                },
        },
        FieldInfo {
            .field_name = "max_outflow",
            .input_type = InputType::number,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_store,
                },
        },
        FieldInfo {
            .field_name = "charge_at_soc",
            .input_type = InputType::number,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "0.8",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_store,
                },
        },
        FieldInfo {
            .field_name = "roundtrip_efficiency",
            .input_type = InputType::number,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "1.0",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_store,
                },
        },
        // COMP Mover
        FieldInfo {
            .field_name = "cop",
            .input_type = InputType::number,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_mover,
                },
        },
        FieldInfo {
            .field_name = "cop_by_fraction_out",
            .input_type = InputType::array_of_tuple2_of_number,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::components_variable_eff_mover,
                },
        },
        // DIST: Common
        FieldInfo {
            .field_name = "type",
            .input_type = InputType::enum_string,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = dist_type_enums,
            .aliases = {},
            .sections = dist_sections,
        },
        FieldInfo {
            .field_name = "time_unit",
            .input_type = InputType::enum_string,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = ValidTimeUnits,
            .aliases = {},
            .sections = dist_sections,
        },
        // DIST - FIXED
        FieldInfo {
            .field_name = "value",
            .input_type = InputType::number,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::dist_fixed,
                },
        },
        // DIST - UNIFORM
        FieldInfo {
            .field_name = "lower_bound",
            .input_type = InputType::number,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::dist_uniform,
                },
        },
        FieldInfo {
            .field_name = "upper_bound",
            .input_type = InputType::number,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::dist_uniform,
                },
        },
        // DIST - NORMAL
        FieldInfo {
            .field_name = "mean",
            .input_type = InputType::number,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::dist_normal,
                },
        },
        FieldInfo {
            .field_name = "standard_deviation",
            .input_type = InputType::number,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::dist_normal,
                },
        },
        // DIST - Quantile Table
        FieldInfo {
            .field_name = "csv_file",
            .input_type = InputType::string,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::dist_01quantile_table_from_file,
                },
        },
        FieldInfo {
            .field_name = "variate_time_pairs",
            .input_type = InputType::array_of_tuple2_of_number,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::dist_02quantile_table_explicit,
                },
        },
        // DIST - WEIBULL
        FieldInfo {
            .field_name = "shape",
            .input_type = InputType::number,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::dist_weibull,
                },
        },
        FieldInfo {
            .field_name = "scale",
            .input_type = InputType::number,
            .is_required = true,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::dist_weibull,
                },
        },
        FieldInfo {
            .field_name = "location",
            .input_type = InputType::number,
            .is_required = false,
            .inform_if_missing = false,
            .default_value = "",
            .enum_values = {},
            .aliases = {},
            .sections =
                {
                    InputSection::dist_weibull,
                },
        },
    };
    InputValidationMap v {};
    for (auto const& f : fields)
    {
        if (f.sections.size() == 0)
        {
            std::cerr << "Program Initialization Error: "
                      << "field '" << f.field_name << "' has no "
                      << "sections that it applies to" << std::endl;
            std::exit(1);
        }
        for (auto const sec : f.sections)
        {
            switch (sec)
            {
            case InputSection::simulation_info:
            {
                UpdateValidationInfoByField(v.simulation_info, f);
            }
            break;
            case InputSection::loads_01explicit:
            {
                UpdateValidationInfoByField(v.load_explicit, f);
            }
            break;
            case InputSection::loads_02file_based:
            {
                UpdateValidationInfoByField(v.load_file_based, f);
            }
            break;
            case InputSection::components_constant_load:
            {
                UpdateValidationInfoByField(v.component.constant_load, f);
            }
            break;
            case InputSection::components_load:
            {
                UpdateValidationInfoByField(v.component.schedule_based_load, f);
            }
            break;
            case InputSection::components_source:
            {
                UpdateValidationInfoByField(v.component.constant_source, f);
            }
            break;
            case InputSection::components_uncontrolled_source:
            {
                UpdateValidationInfoByField(v.component.schedule_based_source, f);
            }
            break;
            case InputSection::components_const_eff_converter:
            {
                UpdateValidationInfoByField(v.component.constant_efficiency_converter, f);
            }
            break;
            case InputSection::components_variable_eff_converter:
            {
                UpdateValidationInfoByField(v.component.variable_efficiency_converter, f);
            }
            break;
            case InputSection::components_mux:
            {
                UpdateValidationInfoByField(v.component.mux, f);
            }
            break;
            case InputSection::components_store:
            {
                UpdateValidationInfoByField(v.component.store, f);
            }
            break;
            case InputSection::components_pass_through:
            {
                UpdateValidationInfoByField(v.component.pass_through, f);
            }
            break;
            case InputSection::components_mover:
            {
                UpdateValidationInfoByField(v.component.mover, f);
            }
            break;
            case InputSection::components_variable_eff_mover:
            {
                UpdateValidationInfoByField(v.component.variable_efficiency_mover, f);
            }
            break;
            case InputSection::components_switch:
            {
                UpdateValidationInfoByField(v.component.transfer_switch, f);
            }
            break;
            case InputSection::dist_fixed:
            {
                UpdateValidationInfoByField(v.distribution.fixed, f);
            }
            break;
            case InputSection::dist_normal:
            {
                UpdateValidationInfoByField(v.distribution.normal, f);
            }
            break;
            case InputSection::dist_01quantile_table_from_file:
            {
                UpdateValidationInfoByField(v.distribution.quantile_table_from_file, f);
            }
            break;
            case InputSection::dist_02quantile_table_explicit:
            {
                UpdateValidationInfoByField(v.distribution.quantile_table_explicit, f);
            }
            break;
            case InputSection::dist_uniform:
            {
                UpdateValidationInfoByField(v.distribution.uniform, f);
            }
            break;
            case InputSection::dist_weibull:
            {
                UpdateValidationInfoByField(v.distribution.weibull, f);
            }
            break;
            // TODO: add in all the other distributions
            case InputSection::network:
            {
                UpdateValidationInfoByField(v.network, f);
            }
            break;
            case InputSection::scenarios:
            {
                UpdateValidationInfoByField(v.scenario, f);
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
