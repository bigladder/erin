// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#ifndef ERIN_VALDATA_H
#define ERIN_VALDATA_H
#include <string>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <stdint.h>
#include <variant>

namespace erin
{

struct TagWithDeprication
{
    std::string tag;
    bool is_deprecated = false;
};

// TODO: add "types" from the user-manual:
// - real>0
// - frac
// - frac>0
// - str
// - [str]
// - [real]
// - etc.
enum class InputType
{
    any,
    string,
    enum_string,
    number,  // float or integer
    integer, // only integer; will also parse 3.0 as 3, though
    boolean,
    array_of_double,
    array_of_string,
    array_of_tuple3_of_string,
    array_of_tuple2_of_number,
    map_from_string_to_string,
};

struct PairsVector
{
    std::vector<double> firsts;
    std::vector<double> seconds;
};

struct InputValue
{
    InputType input_type;
    std::variant<bool,
                 std::string,
                 double,
                 int64_t,
                 std::vector<double>,
                 std::vector<std::string>,
                 // TODO: consider std::vector<std::array<std::string,2>> instead
                 std::vector<std::vector<std::string>>,
                 // TODO: consider std::vector<std::array<double,2>> instead
                 std::vector<std::vector<double>>,
                 std::unordered_map<std::string, std::string>>
        value;
};

enum class InputSection
{
    simulation_info,
    loads_01explicit,
    loads_02file_based,
    components_constant_load,
    components_load,
    components_source,
    components_uncontrolled_source,
    components_const_eff_converter,
    components_variable_eff_converter,
    components_mux,
    components_store,
    components_pass_through,
    components_variable_eff_mover,
    components_mover,
    components_switch,
    dist_fixed,
    dist_weibull,
    dist_uniform,
    dist_normal,
    dist_01quantile_table_from_file,
    dist_02quantile_table_explicit,
    network,
    scenarios,
};

// TODO: add ability to write out all of these to markdown
// for inclusion into the documentation. Consider adding a
// description tag to FieldInfo.
// TODO: add additional field (possibly replace others)
// - validatorFn :: std::function<InputValue, std::string>
//   - if the validator returns a non-empty string, that's a failure
//     the message returned is the failure message
// TODO: need also to have a "table validator" function somewhere...
struct FieldInfo
{
    std::string field_name;
    InputType input_type;
    bool is_required;
    bool inform_if_missing = false;
    std::string default_value;
    std::unordered_set<std::string> enum_values;
    std::vector<TagWithDeprication> aliases;
    std::unordered_set<InputSection> sections;
};

struct ValidationInfo
{
    // TODO: rename as FieldToType
    std::unordered_map<std::string, InputType> TypeMap;
    std::unordered_set<std::string> RequiredFields;
    std::unordered_set<std::string> OptionalFields;
    std::unordered_map<std::string, std::unordered_set<std::string>> EnumMap;
    std::unordered_map<std::string, std::string> Defaults;
    std::unordered_map<std::string, std::vector<TagWithDeprication>> Aliases;
    std::unordered_set<std::string> InformIfMissing;
};

struct ComponentValidationMap
{
    ValidationInfo ConstantLoad;
    ValidationInfo ScheduleBasedLoad;
    ValidationInfo ConstantSource;
    ValidationInfo ScheduleBasedSource;
    ValidationInfo ConstantEfficiencyConverter;
    ValidationInfo VariableEfficiencyConverter;
    ValidationInfo Mux;
    ValidationInfo Store;
    ValidationInfo PassThrough;
    ValidationInfo Mover;
    ValidationInfo VariableEfficiencyMover;
    ValidationInfo Switch;
};

struct DistributionValidationMap
{
    ValidationInfo Fixed;
    ValidationInfo Uniform;
    ValidationInfo Normal;
    ValidationInfo QuantileTableFromFile;
    ValidationInfo QuantileTableExplicit;
    ValidationInfo Weibull;
};

struct InputValidationMap
{
    ComponentValidationMap Comp;
    ValidationInfo Load_01Explicit;
    ValidationInfo Load_02FileBased;
    DistributionValidationMap Dist;
    ValidationInfo SimulationInfo;
    ValidationInfo Network;
    ValidationInfo Scenario;
};

} // namespace erin
#endif
