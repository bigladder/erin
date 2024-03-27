/* Copyright (c) 2020-2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
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
		std::string Tag;
		bool IsDeprecated = false;
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
		Any,
		// TODO: rename below to String
		AnyString,
		EnumString,
		Number, // float or integer
		Integer, // only integer; will also parse 3.0 as 3, though
		ArrayOfDouble,
		ArrayOfString,
		ArrayOfTuple3OfString,
		ArrayOfTuple2OfNumber,
		MapFromStringToString,
	};

	struct PairsVector
	{
		std::vector<double> Firsts;
		std::vector<double> Seconds;
	};

	struct InputValue
	{
		InputType Type;
		std::variant<
			std::string,
			double,
			int64_t,
			std::vector<double>,
			std::vector<std::string>,
			// TODO: consider std::vector<std::array<std::string,2>> instead
			std::vector<std::vector<std::string>>,
			// TODO: consider std::vector<std::array<double,2>> instead
			std::vector<std::vector<double>>,
			std::unordered_map<std::string, std::string>> Value;
	};

	enum class InputSection
	{
		SimulationInfo,
		Loads_01Explicit,
		Loads_02FileBased,
		Components_ConstantLoad,
		Components_Load,
		Components_Source,
		Components_UncontrolledSource,
		Components_ConstEffConverter,
		Components_Mux,
		Components_Store,
		Components_PassThrough,
		Components_Mover,
		Dist_Fixed,
		Dist_Weibull,
		Dist_Uniform,
		Dist_Normal,
		Dist_01QuantileTableFromFile,
		Dist_02QuantileTableExplicit,
		Network,
		Scenarios,
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
		std::string FieldName;
		InputType Type;
		bool IsRequired;
		std::string Default;
		std::unordered_set<std::string> EnumValues;
		std::vector<TagWithDeprication> Aliases;
		std::unordered_set<InputSection> Sections;
	};

	struct ValidationInfo
	{
		std::unordered_map<std::string, InputType> TypeMap;
		std::unordered_set<std::string> RequiredFields;
		std::unordered_set<std::string> OptionalFields;
		std::unordered_map<std::string, std::unordered_set<std::string>> EnumMap;
		std::unordered_map<std::string, std::string> Defaults;
		std::unordered_map<std::string, std::vector<TagWithDeprication>> Aliases;
	};

	struct ComponentValidationMap
	{
		ValidationInfo ConstantLoad;
		ValidationInfo ScheduleBasedLoad;
		ValidationInfo ConstantSource;
		ValidationInfo ScheduleBasedSource;
		ValidationInfo ConstantEfficiencyConverter;
		ValidationInfo Mux;
		ValidationInfo Store;
		ValidationInfo PassThrough;
		ValidationInfo Mover;
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
  
}
#endif
