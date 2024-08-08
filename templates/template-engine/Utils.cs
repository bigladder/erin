using Tomlyn;
using Tomlyn.Model;

namespace TemplateEngine
{
	public class Utils
	{

		public static List<string>
		ModelDifferences(object? a, object? b, string path=".")
		{
			List<string> differences = new();
			if (a == null && b == null)
			{
			}
			else if (a is TomlTable aTable)
			{
				if (b is TomlTable bTable)
				{
					List<string> subDiffs = ModelDifferences(aTable, bTable, path);
					if (subDiffs.Count > 0)
					{
						differences.AddRange(subDiffs);
					}
				}
				else
				{
					differences.Add(
						$"[{path}] A is a TomlTable but B is not"
					);
				}
			}
			else if (a is TomlArray aArray)
			{
				if (b is TomlArray bArray)
				{
					List<string> subDiffs = ModelDifferences(aArray, bArray, path);
					if (subDiffs.Count > 0)
					{
						differences.AddRange(subDiffs);
					}
				}
				else
				{
					differences.Add(
						$"[{path}] A is a TomlArray but B is not"
					);
				}
			}
			else if (a is TomlTableArray aTableArray)
			{
				if (b is TomlTableArray bTableArray)
				{
					List<string> subDiffs =
						ModelDifferences(aTableArray, bTableArray);
					if (subDiffs.Count > 0)
					{
						differences.AddRange(subDiffs);
					}
				}
				else
				{
					differences.Add(
						$"[{path}] A is TomlTableArray but B is not"
					);
				}
			}
			else if (a is long aLong)
			{
				if (b is long bLong)
				{
					if (aLong != bLong)
					{
						differences.Add(
							$"[{path}] A ({aLong}) != B ({bLong})"
						);
					}
				}
				else if (b is double bDouble)
				{
					double aDouble = (double)aLong;
					if (aDouble != bDouble)
					{
						differences.Add(
							$"[{path}] A ({aDouble}) != B ({bDouble})"
						);
					}
				}
				else
				{
					differences.Add(
						$"[{path}] A is long but B is not"
					);
				}
			}
			else if (a is string aString)
			{
				if (b is string bString)
				{
					if (aString != bString)
					{
						differences.Add(
							$"[{path}] A is '{aString}' but B is '{bString}'"
						);
					}
				}
				else
				{
					differences.Add(
						$"[{path}] A is string but B is not"
					);
				}
			}
			else if (a is double aDouble)
			{
				if (b is double bDouble)
				{
					if (aDouble != bDouble)
					{
						differences.Add(
							$"[{path}] A ({aDouble}) != B ({bDouble})"
						);
					}
				}
				else if (b is long bLong)
				{
					double bDouble2 = (double)bLong;
					if (aDouble != bDouble2)
					{
						differences.Add(
							$"[{path}] A ({aDouble}) != B ({bDouble2})"
						);
					}
				}
				else
				{
					differences.Add(
						$"[{path}] A is double but B is not"
					);
				}
			}
			else
			{
				differences.Add(
					$"[{path}] Unhandled type for A {a}"
				);
			}
			return differences;
		}

		public static List<string>
		ModelDifferences(TomlArray a, TomlArray b, string path=".")
		{
			List<string> differences = [];
			if (a.Count != b.Count)
			{
				differences.Add($"[{path}] Array count for A ({a.Count}) not the same count as B ({b.Count})");
				return differences;
			}
			for (int idx=0; idx < a.Count; ++idx)
			{
				object? aItem = a[idx];
				object? bItem = b[idx];
				List<string> subDiffs = ModelDifferences(aItem, bItem, path + $".{idx}");
				if (subDiffs.Count > 0)
				{
					differences.AddRange(subDiffs);
					return differences;
				}
			}
			return differences;
		}

		public static List<string>
		ModelDifferences(TomlTableArray a, TomlTableArray b, string path=".")
		{
			List<string> differences = [];
			if (a.Count != b.Count)
			{
				differences.Add(
					$"[{path}] A count ({a.Count}) != B count ({b.Count})"
				);
				return differences;
			}
			for (int idx=0; idx < a.Count; ++idx)
			{
				TomlTable aItem = a[idx];
				TomlTable bItem = b[idx];
				List<string> subDiffs =
					ModelDifferences(aItem, bItem, path: path + $".{idx}");
				if (subDiffs.Count > 0)
				{
					differences.AddRange(subDiffs);
					return differences;
				}
			}
			return differences;
		}

		public static List<string>
		ModelDifferences(TomlTable a, TomlTable b, string path=".")
		{
			List<string> differences = new();
			List<string> aKeys = new(a.Keys);
			List<string> bKeys = new(b.Keys);
			if (aKeys.Count != bKeys.Count)
			{
				differences.Add($"[{path}] Key counts differ: a.Keys.Count = {aKeys.Count}; b.Keys.Count = {bKeys.Count}");
				HashSet<string> aKeySet = new(aKeys);
				HashSet<string> bKeySet = new(bKeys);
				HashSet<string> aNotB = new(aKeySet.Except(bKeySet));
				HashSet<string> bNotA = new(bKeySet.Except(aKeySet));
				differences.Add($"[{path}] In A, not B: {String.Join(", ", aNotB.ToList())}");
				differences.Add($"[{path}] In B, not A: {String.Join(", ", bNotA.ToList())}");
				return differences;
			}
			foreach (string key in aKeys)
			{
				object aValue = a[key];
				object bValue = b[key];
				List<string> subDiffs = ModelDifferences(aValue, bValue, path + $".{key}");
				if (subDiffs.Count > 0)
				{
					differences.AddRange(subDiffs);
					return differences;
				}
			}
			return differences;
		}

		public static TomlTable
		ReadTemplate(string templatePath)
		{
			string rawData = File.ReadAllText(templatePath);
			return Toml.ToModel(rawData);
		}

		public static Dictionary<string, TomlTable>
		ReadTemplateLibrary(string libraryPath)
		{
			Dictionary<string, TomlTable> templates = [];
			string[] templateFileNames = Directory.GetFiles(libraryPath, "*.toml");
			foreach (string templateFileName in templateFileNames)
			{
				TomlTable t = ReadTemplate(templateFileName);
				string templateName = Get<string>(t, "name");
				templates[templateName] = t;
			}
			return templates;
		}

		public static TomlTable
		ReadInputModel(string inputFilePath)
		{
			string rawData = File.ReadAllText(inputFilePath);
			return Toml.ToModel(rawData);
		}

		public static TomlTable
		ReadInputModelFromStream(MemoryStream stream)
		{
			var reader = new StreamReader(stream);
			string rawData = reader.ReadToEnd();
			return Toml.ToModel(rawData);
		}

		public static T 
		Get<T>(TomlTable table, string key)
		{	
			if (!table.ContainsKey(key))
			{
				throw new Exception($"[ERROR] Missing required key {key}");
			}
			try
			{
				return (T)table[key];
			}
			catch
			{
				throw new Exception($"[ERROR] Could not convert {table[key]} to desired type");
			}
		}

		public static void
		PrintConnections(List<(string, string, string)> connections)
		{
			Console.WriteLine("Connections:");
			foreach ((string fromStr, string toStr, string flowTypeStr) in connections)
			{
				Console.WriteLine($"- {fromStr} => {toStr} [{flowTypeStr}]");
			}
		}

		public static (string, string, string)
		ReadConnectionTriple(object? triple)
		{
			if (triple == null)
			{
				throw new Exception("[ERROR] connection triple is null");
			}
			var tripleArray = (TomlArray)triple;
			if (tripleArray == null || tripleArray.Count < 3)
			{
				throw new Exception("[ERROR] connection triple is null or doesn't contain at least 3 items");
			}
			object? fromItem = tripleArray[0];
			object? toItem = tripleArray[1];
			object? flowType = tripleArray[2];
			if (fromItem == null || toItem == null || flowType == null)
			{
				throw new Exception("[ERROR] connection triple could not be reified to strings");
			}
			string fromStr = (string)fromItem;
			string toStr = (string)toItem;
			string flowTypeStr = (string)flowType;
			return (fromStr, toStr, flowTypeStr);
		}

		public static List<(string, string, string)>
		ReadConnectionsFromModel(TomlTable model)
		{
			List<(string, string, string)> connections = [];
			if (model.ContainsKey("network"))
			{
				var networkTable = (TomlTable)model["network"];
				if (networkTable.ContainsKey("connections"))
				{
					var connElements = (TomlArray)networkTable["connections"];
					foreach (var triple in connElements)
					{
						connections.Add(ReadConnectionTriple(triple));
					}
				}
				else
				{
					throw new Exception("[ERROR] input file must contain network.connections");
				}
			}
			else
			{
				throw new Exception("[ERROR] input file must contain network");
			}
			return connections;
		}

		public static List<(string, string, string)>
		ReadConnectionsFromTemplate(TomlTable template)
		{
			List<(string, string, string)> templateConnections = [];
			TomlArray rawTemplateConnections = Get<TomlArray>(template, "connections");
			if (rawTemplateConnections != null)
			{
				foreach (object? item in rawTemplateConnections)
				{
					templateConnections.Add(ReadConnectionTriple(item));
				}
			}
			return templateConnections;
		}

		public static ParamType
		StringToParamType(string tag)
		{
			if (tag == "enum" || tag == "enumeration")
			{
				return ParamType.Enumeration;
			}
			if (tag == "frac" || tag == "fraction")
			{
				return ParamType.Fraction;
			}
			if (tag == "int" || tag == "integer")
			{
				return ParamType.Integer;
			}
			if (tag == "num" || tag == "number")
			{
				return ParamType.Number;
			}
			if (tag == "str" || tag == "string")
			{
				return ParamType.String;
			}
			if (tag == "(table string string)" || tag == "(table str str)")
			{
				return ParamType.TableFromStringToString;
			}
			return ParamType.Unhandled;
		}

		public static void
		PrintParameters(Dictionary<string, TemplateParameter> parameters)
		{
			Console.WriteLine("Parameters:");
			foreach (KeyValuePair<string, TemplateParameter> kvp in parameters)
			{
				TemplateParameter tp = kvp.Value;
				string optText = tp.IsOptional ? " (optional)" : "";
				Console.WriteLine($"- {kvp.Key}{optText}: {tp.Type}");
			}
		}

		public static HashSet<string>
		ExtractEnumOptions(TomlTable singleParam)
		{
			HashSet<string> options = [];
			if (!singleParam.ContainsKey("enum_options"))
			{
				return options;
			}
			TomlArray rawEnumOptions = Get<TomlArray>(singleParam, "enum_options");											
			foreach (object? opt in rawEnumOptions)
			{
				if (opt == null)
				{
					return [];
				}
				options.Add((string)opt);
			}
			return options;
		}

		public static void
		AddToParametersDictFromTable(
			TomlTable paramTable,
			Dictionary<string, TemplateParameter> parameters,
			string templateName,
			bool isOptional
		)
		{
			foreach (KeyValuePair<string, object> paramKvp in paramTable)
			{
				string paramName = paramKvp.Key;
				TomlTable singleParam = (TomlTable)paramKvp.Value;
				string paramTypeAsStr = Get<string>(singleParam, "type");
				ParamType paramType = StringToParamType(paramTypeAsStr);
				if (paramType == ParamType.Unhandled)
				{
					throw new Exception($"[ERROR] Unhandled parameter type for '{paramName}' ({paramTypeAsStr})");
				}
				switch (paramType)
				{
					case ParamType.Enumeration:
					{
						HashSet<string> enumOptions = ExtractEnumOptions(singleParam);
						if (enumOptions.Count == 0)
						{
							throw new Exception($"[ERROR] Template {templateName} does not contain required 'enum_options' for parameter {paramName}");
						}
						TemplateParameter tp = new()
						{
							Name = paramName,
							Type = paramType,
							IsOptional = isOptional,
							Validate = x => {
								try
								{
									string value = (string)x;
									return enumOptions.Contains(value);
								}
								catch {}
								return false;
							},
						};
						parameters.Add(paramName, tp);
					}
					break;
					case ParamType.Fraction:
					{
						TemplateParameter tp = new()
						{
							Name = paramName,
							Type = paramType,
							IsOptional = isOptional,
							Validate = x => {
								try
								{
									double value = (double)x;
									return value >= 0.0 && value <= 1.0;
								}
								catch {}
								return false;
							},
						};
						parameters.Add(paramName, tp);
					}
					break;
					case ParamType.Integer:
					{
						TemplateParameter tp = new()
						{
							Name = paramName,
							Type = paramType,
							IsOptional = isOptional,
							Validate = x => {
								try
								{
									long value = (long)x;
									return true;
								}
								catch {}
								return false;
							},
						};
						parameters.Add(paramName, tp);
					}
					break;
					case ParamType.Number:
					{
						TemplateParameter tp = new()
						{
							Name = paramName,
							Type = paramType,
							IsOptional = isOptional,
							Validate = x => {
								try
								{
									double value = (double)x;
									return true;
								}
								catch {}
								return false;
							},
						};
						parameters.Add(paramName, tp);
					}
					break;
					case ParamType.String:
					{
						TemplateParameter tp = new()
						{
							Name = paramName,
							Type = paramType,
							IsOptional = isOptional,
							Validate = x => {
								try
								{
									string value = (string)x;
									return true;
								}
								catch {}
								return false;
							},
						};
						parameters.Add(paramName, tp);
					}
					break;
					case ParamType.TableFromStringToString:
					{
						TemplateParameter tp = new()
						{
							Name = paramName,
							Type = paramType,
							IsOptional = isOptional,
							Validate = x => {
								try
								{
									TomlTable value = (TomlTable)x;
									foreach (KeyValuePair<string, object> kvp in value)
									{
										string subValue = (string)kvp.Value;
									}
									return true;
								}
								catch {}
								return false;
							},
						};
						parameters.Add(paramName, tp);
					}
					break;
					default:
					{
						throw new Exception($"[ERROR] Unhandled param type: {paramType}");
					}
				}
			}
		}

		public static TomlArray
		ConnectionsToTomlArray(List<(string, string, string)> connections)
		{
			List<List<string>> conns = [];
			foreach ((string fromStr, string toStr, string flowTypeStr) in connections)
			{
				conns.Add([fromStr, toStr, flowTypeStr]);
			}
			conns.Sort((List<string> a, List<string> b) => {
				string aHash = String.Join(":", a);
				string bHash = String.Join(":", b);
				return aHash.CompareTo(bHash);
			});
			TomlArray cs = [];
			foreach (List<string> c in conns)
			{
				TomlArray conn = [];
				conn.Add(c[0]);
				conn.Add(c[1]);
				conn.Add(c[2]);
				cs.Add(conn);
			}
			return cs;
		}

		public static void
		AddTemplateConnections(
			List<(string, string, string)> connections,
			List<(string, string, string)> templateConnections,
			string templateComponentName,
			bool verbose=false
		)
		{
			HashSet<(string, string, string)> toRemove = [];
			List<(string,string,string)> toAdd = [];
			foreach ((string fromStr, string toStr, string flowTypeStr) in connections)
			{
				string fromCompName = fromStr.Split(":")[0].Trim();
				bool fromParsed = int.TryParse(
					fromStr.Split(":")[1].Split("(")[1].Split(")")[0].Trim(),
					out int fromCompPort);
				string toCompName = toStr.Split(":")[0].Trim();
				bool toParsed = int.TryParse(
					toStr.Split(":")[1].Split("(")[1].Split(")")[0].Trim(),
					out int toCompPort);
				if (!fromParsed)
				{
					throw new Exception($"[ERROR] Unable to parse port for {fromStr}");
				}
				if (!toParsed)
				{
					throw new Exception($"[ERROR] Unable to parse port for {toStr}");
				}
				if (verbose)
				{
					Console.WriteLine($"[INFO] fromCompName = {fromCompName}; fromCompPort = {fromCompPort}");
					Console.WriteLine($"[INFO] toCompName = {toCompName}; toCompPort = {toCompPort}");
				}
				if (fromCompName == templateComponentName)
				{
					if (verbose)
					{
						Console.WriteLine($"[INFO] template match for '{templateComponentName}'");
					}
					string toKey = $"<{fromCompPort}>";
					bool foundMatch = false;
					foreach ((string tFrom, string tTo, string tFlowType) in templateConnections)
					{
						if (tTo == toKey && flowTypeStr == tFlowType)
						{
							foundMatch = true;
							toRemove.Add((fromStr, toStr, flowTypeStr));
							toAdd.Add((tFrom, toStr, flowTypeStr));
							break;
						}
					}
					if (!foundMatch)
					{
						throw new Exception(
							$"[ERROR] can't find template match for "
							+ $"({fromStr}, {toStr}, {flowTypeStr})"
						);
					}
				}
				else if (toCompName == templateComponentName)
				{
					if (verbose)
					{
						Console.WriteLine($"[INFO] template match for '{templateComponentName}'");
					}
					string fromKey = $"<{toCompPort}>";
					bool foundMatch = false;
					foreach ((string tFrom, string tTo, string tFlowType) in templateConnections)
					{
						if (tFrom == fromKey && flowTypeStr == tFlowType)
						{
							foundMatch = true;
							toRemove.Add((fromStr, toStr, flowTypeStr));
							toAdd.Add((fromStr, tTo, flowTypeStr));
							break;
						}
					}
					if (!foundMatch)
					{
						throw new Exception(
							$"[ERROR] can't find template match for "
							+ $"({fromStr}, {toStr}, {flowTypeStr})"
						);
					}
				}
			}
			foreach (var item in toRemove)
			{
				connections.Remove(item);
			}
			foreach (var item in toAdd)
			{
				connections.Add(item);
			}
			foreach ((string tFrom, string tTo, string tFlowType) in templateConnections)
			{
				if (!tFrom.StartsWith("<") && !tTo.StartsWith("<"))
				{
					// TODO: need to sanitize names to ensure they're unique
					connections.Add((tFrom, tTo, tFlowType));
				}
			}
		}

		public static TomlTable
		FillComponentParameters(
			TomlTable templateComponent,
			Dictionary<string, TemplateParameter> parameters,
			TomlTable component,
			string templateInfo,
			bool verbose=false
		)
		{
			TomlTable result = [];
			foreach (KeyValuePair<string, object> kvp in templateComponent)
			{
				if (kvp.Key == "id")
				{
					continue;
				}
				result[kvp.Key] = kvp.Value;
			}
			HashSet<string> toRemove = [];
			Dictionary<string, object> toAdd = [];
			foreach (KeyValuePair<string, object> kvp in templateComponent)
			{
				string fieldName = kvp.Key;
				string paramName = "";
				bool isParam = false;
				object? defaultValue = null;
				try
				{
					TomlTable fieldValue = (TomlTable)kvp.Value;
					isParam = fieldValue.ContainsKey("param");
					if (isParam)
					{
						paramName = (string)fieldValue["param"];
						if (fieldValue.ContainsKey("default"))
						{
							defaultValue = fieldValue["default"];
						}
						if (verbose)
						{
							Console.WriteLine(
								$"[INFO] Found field '{fieldName}' parameterized on '{paramName}' {templateInfo}"
							);
						}
					}
				}
				catch
				{}
				if (isParam)
				{
					if (!component.ContainsKey(paramName))
					{
						throw new Exception($"[ERROR] Component missing param {paramName} {templateInfo}");
					}
					if (!parameters.ContainsKey(paramName))
					{
						throw new Exception($"[ERROR] Reference to undefined parameter {paramName} {templateInfo}");
					}
					TemplateParameter tp = parameters[paramName];
					if (!tp.IsOptional && !component.ContainsKey(paramName))
					{
						throw new Exception($"[ERROR] Missing required param '{paramName}' {templateInfo}");
					}
					object? paramValue = null;
					if (component.ContainsKey(paramName))
					{
						paramValue = component[paramName];
					}
					if (paramValue != null)
					{
						if (!tp.Validate(paramValue))
						{
							throw new Exception($"[ERROR] Invalid value for parameter {paramName} {templateInfo}");
						}
						toAdd.Add(fieldName, paramValue);
					}
					else if (defaultValue != null)
					{
						if (!tp.Validate(defaultValue))
						{
							throw new Exception($"[ERROR] Invalid default value for parameter {paramName} {templateInfo}");
						}
						toAdd.Add(fieldName, defaultValue);
					}
					toRemove.Add(fieldName);
				}
			}
			foreach (string key in toRemove)
			{
				result.Remove(key);
			}
			foreach (KeyValuePair<string, object> kvp in toAdd)
			{
				result.Add(kvp.Key, kvp.Value);
			}
			return result;
		}

		public static string
		EnsureIdIsUnique(
			string rawId,
			HashSet<string> existingNames)
		{
			string result = rawId;
			int nextNumber = 0;
			while (existingNames.Contains(result))
			{
				result = $"{rawId}_{nextNumber}";
				nextNumber++;
			}
			existingNames.Add(result);
			return result;
		}

		public static HashSet<string>
		ExtractComponentNamesFromConnections(
			List<(string, string, string)> connections)
		{
			HashSet<string> result =
				new();
			foreach ((string fromComp, string toComp, string _) in connections)
			{
				string[] fromParts = fromComp.Split(":");
				if (fromParts.Length > 0)
				{
					result.Add(fromParts[0]);
				}
				string[] toParts = toComp.Split(":");
				if (toParts.Length > 0)
				{
					result.Add(toParts[0]);
				}
			}
			return result;
		}

		public static Dictionary<string, string>
		GenerateUniqueNamesForTemplateNames(
			HashSet<string> templateNames,
			HashSet<string> existingNames)
		{
			Dictionary<string, string> result = new();
			foreach (string name in templateNames)
			{
				string uniqueName = name;
				if (existingNames.Contains(name))
				{
					uniqueName = EnsureIdIsUnique(name, existingNames);
				}
				result[name] = uniqueName;
			}
			return result;
		}

		public static List<(string, string, string)>
		ReplaceComponentNamesInConnections(
			List<(string, string, string)> connections,
			Dictionary<string, string> existingToNewNames)
		{
			string updateName(string name)
			{
				string result = name;
				List<string> parts = name.Split(":").ToList();
				if (parts.Count > 1)
				{
					if (existingToNewNames.ContainsKey(parts[0]))
					{
						List<string> newParts = new();
						newParts.Add(existingToNewNames[parts[0]]);
						for (int i = 1; i < parts.Count; ++i)
						{
							newParts.Add(parts[i]);
						}
						result = String.Join(":", newParts);
					}
				}
				return result;
			};
			List<(string, string, string)> result = new();
			foreach ((string fromComp, string toComp, string flow) in connections)
			{
				string newFromName = updateName(fromComp);
				string newToName = updateName(toComp);
				result.Add((newFromName, newToName, flow));
			}
			return result;
		}

		public static string
		ReplaceName(string name, Dictionary<string, string> existingToNewNames)
		{
			if (existingToNewNames.ContainsKey(name))
			{
				return existingToNewNames[name];
			}
			return name;
		}

		public static bool
		ModelContainsTemplates(TomlTable model)
		{
			foreach (KeyValuePair<string, object> kvp in model)
			{
				if (kvp.Key == "components")
				{
					if (kvp.Value is Dictionary<string, object> compTable)
					{
						foreach (KeyValuePair<string, object> compKvp in compTable)
						{
							string compType = "";
							if (compKvp.Value is TomlTable compData)
							{
								compType = Get<string>(compData, "type");
							}
							else if (compKvp.Value is Dictionary<string, object> compData2)
							{
								if (compData2.ContainsKey("type"))
								{
									if (compData2["type"] is string compTypeStr)
									{
										compType = compTypeStr;
									}
								}
							}
							if (compType == "template")
							{
								return true;
							}
						}
					}
				}
			}
			return false;
		}

		public static TomlTable
		ApplyTemplatesToModel(
			Dictionary<string, TomlTable> templates,
			TomlTable model,
			bool verbose=false
		)
		{
			TomlTable newModel = [];
			List<(string, string, string)> connections = ReadConnectionsFromModel(model);
			if (verbose)
			{
				PrintConnections(connections);
			}
			foreach (KeyValuePair<string, object> kvp in model)
			{
				if (kvp.Key != "components" && kvp.Key != "network")
				{
					newModel[kvp.Key] = kvp.Value;
					continue;
				}
				if (kvp.Key == "components")	
				{
					Dictionary<string, object> newComponents = [];
					var compTable = (TomlTable)kvp.Value;
					HashSet<string> existingComponentNames = new(compTable.Keys);
					HashSet<string> generatedComponentNames = new();
					foreach (KeyValuePair<string, object> compKvp in compTable)
					{
						string compName = compKvp.Key;
						var compData = (TomlTable)compKvp.Value;
						string compType = Get<string>(compData, "type");
						if (compType != "template")
						{
							newComponents[compName] = compData;
							continue;
						}
						else
						{
							// TODO: need to modify this code to only load/process a template once
							// ... and then apply that template to the component (without mutating it).
							// ... A good use case for Immutable Containers?
							string templateName = Get<string>(compData, "template_name");
							string parentGroup = "";
							if (compData.ContainsKey("group"))
							{
								parentGroup = Get<string>(compData, "group");
							}
							if (templates.TryGetValue(templateName, out TomlTable? value))
							{
								if (verbose)
								{
									Console.WriteLine(
										$"[INFO] Expanding template {templateName} for component {compName}"
									);
								}
								var template = value;
								long numInflows = Get<long>(template, "num_inflows");
								long numOutflows = Get<long>(template, "num_outflows");
								if (verbose)
								{
									Console.WriteLine(
										$"[INFO] {templateName} has {numInflows} inflow(s)"
									);
									Console.WriteLine(
										$"[INFO] {templateName} has {numOutflows} outflow(s)"
									);
								}
								List<(string, string, string)> templateConnections =
									ReadConnectionsFromTemplate(template);
								if (verbose)
								{
									PrintConnections(templateConnections);
								}
								HashSet<string> namesUsedInConnections =
									ExtractComponentNamesFromConnections(templateConnections);
								Dictionary<string, string> uniqueNamesByTemplateNames =
									GenerateUniqueNamesForTemplateNames(namesUsedInConnections, existingComponentNames);
								foreach (string newName in uniqueNamesByTemplateNames.Values)
								{
									existingComponentNames.Add(newName);
								}
								List<(string, string, string)> uniqueTemplateConnections =
									ReplaceComponentNamesInConnections(templateConnections, uniqueNamesByTemplateNames);
								if (verbose)
								{
									Console.WriteLine("[INFO] ensuring template connections are unique");
									foreach (var conn in uniqueTemplateConnections)
									{
										Console.WriteLine($"- {conn.Item1} => {conn.Item2} [{conn.Item3}]");
									}
								}
								AddTemplateConnections(
									connections,
									uniqueTemplateConnections,
									compName
								);
								Dictionary<string, TemplateParameter> parameters = [];
								if (template.ContainsKey("parameters"))
								{
									TomlTable paramTable = Get<TomlTable>(template, "parameters");
									if (paramTable.ContainsKey("required"))
									{
										TomlTable requiredParams = Get<TomlTable>(paramTable, "required");
										AddToParametersDictFromTable(
											requiredParams,
											parameters,
											templateName,
											isOptional: false
										);
									}
									if (paramTable.ContainsKey("optional"))
									{
										TomlTable optionalParams = Get<TomlTable>(paramTable, "optional");
										AddToParametersDictFromTable(
											optionalParams,
											parameters,
											templateName,
											isOptional: true
										);
									}
								}
								if (verbose)
								{
									PrintParameters(parameters);
								}
								Dictionary<string, TomlTable> templateComponents = [];
								var tempCompArray =
									Get<TomlTableArray>(template, "components");
								foreach (TomlTable tempCompTable in tempCompArray)
								{
									if (!tempCompTable.ContainsKey("id"))
									{
										Console.WriteLine($"[ERROR] with tempCompTable for {compName}:");
										foreach (string key in tempCompTable.Keys)
										{
											Console.WriteLine($"- {key}: {tempCompTable[key]}");
										}
										throw new Exception($"[ERROR] missing required field 'id' in template component");
									}
									string rawId = Get<string>(tempCompTable, "id");
									string id = ReplaceName(rawId, uniqueNamesByTemplateNames);
									if (verbose)
									{
										Console.WriteLine($"[INFO] id replacement of {rawId} => {id}");
									}
									string templateInfo =
										$"(template: {templateName}; template component id: {id}; "
										+ $"component name: {compName})";
									TomlTable reifiedComponent = FillComponentParameters(
										tempCompTable, parameters, compData, templateInfo, verbose);
									reifiedComponent["group"] = parentGroup == "" ? compName : parentGroup;
									templateComponents[id] = reifiedComponent;
								}
								foreach (KeyValuePair<string, TomlTable> ttKvp in templateComponents)
								{
									newComponents.Add(ttKvp.Key, ttKvp.Value);
								}
							}
							else
							{
								Console.WriteLine(
									$"[ERROR] Could not find template {templateName} for component {compName}"
								);
							}
						}
					}
					newModel["components"] = newComponents;
				}
			}
			TomlTable newNetwork = new()
			{
				{ "connections", ConnectionsToTomlArray(connections) },
			};
			newModel["network"] = newNetwork;
			return newModel;
		}

		public static void
		WriteModel(TomlTable model, string outputFilePath) {
			TomlTable m = model;
			File.WriteAllText(outputFilePath, Toml.FromModel(m));
		}

		public static void
		WriteModelToStream(TomlTable model, MemoryStream stream)
		{
			var sw = new StreamWriter(stream);
			sw.Write(Toml.FromModel(model));
			sw.Flush();
		}

		public static void
		Run(
			string templateDirectory,
			string inputFile,
			string outputFile,
			bool verbose=false
		)
		{
			Dictionary<string, TomlTable> templates =
				ReadTemplateLibrary(templateDirectory);
			MemoryStream ms = new();
			var model = ReadInputModel(inputFile);
			var modifiedModel = ApplyTemplatesToModel(templates, model, verbose);
			int numPasses = 0;
			using (var stream = new MemoryStream())
			{
				while (ModelContainsTemplates(modifiedModel))
				{
					if (verbose)
					{
						Console.WriteLine($"[INFO] Nested templates detected. Recursive expansion pass {++numPasses}");
					}
					WriteModelToStream(modifiedModel, stream);
					stream.Seek(0, SeekOrigin.Begin);
					model = ReadInputModelFromStream(stream);
					stream.Seek(0, SeekOrigin.Begin);
					modifiedModel = ApplyTemplatesToModel(templates, model, verbose);
				}
			}
			WriteModel(modifiedModel, outputFile);
		}
	}
}
