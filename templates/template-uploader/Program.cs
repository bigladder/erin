using System.Net;
using System.Net.Http.Json;
using System.Security;
using System.Text;

namespace template_uploader
{
	internal class Program
	{
		private static readonly bool VERBOSE = false;
		private static readonly int TIME_OUT_MS = 25000;
		public static void DisplayPrompt(string preamble, string prompt)
		{
			if (preamble != null && preamble.Length > 0)
			{
				Console.WriteLine(preamble);
			}
			if (prompt != null && prompt.Length > 0)
			{
				Console.Write(prompt);
			}
		}
		public static string ReadFromConsole(
			string preamble,
			string prompt,
			string defaultValue)
		{
			string result = defaultValue;
			DisplayPrompt(preamble, prompt);
			try
			{
				string? readValue = Console.ReadLine();
				if (readValue != null && readValue.Length > 0)
				{
					result = readValue;
				}
			}
			catch (Exception ex) when (
				ex is IOException
				|| ex is OutOfMemoryException
				|| ex is ArgumentOutOfRangeException
			)
			{
				Console.WriteLine($"Issue reading from console: {ex}");
				return defaultValue;
			}
			return result;
		}
		public static string ReadFromConsoleSecurely(
			string preamble,
			string prompt)
		{
			StringBuilder result = new();
			ConsoleKeyInfo key;
			DisplayPrompt(preamble, prompt);
			do
			{
				key = Console.ReadKey(true);
				char keyChar = key.KeyChar;
				if (!(Char.IsWhiteSpace(keyChar) || Char.IsControl(keyChar)))
				{
					result.Append(keyChar);
					Console.Write("*");
				}
			} while (key.Key != ConsoleKey.Enter);
			Console.WriteLine();
			return result.ToString();
		}
		public static string ReadToken(
			HttpClient client,
			string userName,
			string password,
			bool verbose = false)
		{
			Task<HttpResponseMessage> loginRequest =
				client.GetAsync(
					$"User/login?u={userName}&p={password}");
			if (!loginRequest.Wait(TIME_OUT_MS))
			{
				Console.WriteLine("Unable to login");
				Environment.Exit(1);
			}
			if (!loginRequest.IsCompletedSuccessfully)
			{
				Console.WriteLine("Unable to login");
				Environment.Exit(1);
			}
			HttpResponseMessage loginResponse = loginRequest.Result;
			if (!loginResponse.IsSuccessStatusCode)
			{
				Console.WriteLine("Login response unsuccessful");
				Environment.Exit(1);
			}
			Stream tokenStream = loginResponse.Content.ReadAsStream();
			string result = new StreamReader(tokenStream).ReadToEnd();
			if (result == null || result.Length == 0)
			{
				Console.WriteLine("Null token after login");
				Environment.Exit(1);
			}
			if (verbose)
			{
				Console.WriteLine($"Token: {result}");
			}
			return result;
		}
		public static void AddEquipment(
			HttpClient client,
			string token,
			string jsonContents
		)
		{
			string jsonData =
				$"{{\"t\":\"{token}\", \"tomlData\": {jsonContents}}}";
			HttpContent addEquipRequestBody = new StringContent(
				jsonData,
				System.Text.Encoding.UTF8,
				"application/json");
			Task<HttpResponseMessage> request =
				client.PostAsync(
					"ErinAdmin/addEquipmentType",
					addEquipRequestBody);
			if (!request.Wait(TIME_OUT_MS))
			{
				Console.WriteLine("Unable to addEquipment -- timeout");
				Environment.Exit(1);
			}
			if (!request.IsCompletedSuccessfully)
			{
				Console.WriteLine("Unable to add equipment");
				Environment.Exit(1);
			}
			HttpResponseMessage response = request.Result;
			if (!response.IsSuccessStatusCode)
			{
				Console.WriteLine("Add equipment response unsuccessful");
				Environment.Exit(1);
			}
			Stream addEquipStream = response.Content.ReadAsStream();
			string addEquipResultContent =
				new StreamReader(addEquipStream).ReadToEnd();
			Console.WriteLine($"Result:\n{addEquipResultContent}");
		}
		static string SerializeToJSON(string content)
		{
			string result = "";
			try
			{
				result =
					System.Text.Json.JsonSerializer.Serialize(content);
			}
			catch (Exception ex)
			{
				Console.WriteLine(
					$"Exception serializing content to JSON: {ex}");
				Environment.Exit(1);
			}
			return result;
		}
		static string ReadPathFromConsole()
		{
			string result = ReadFromConsole(
				preamble: "Type full path to template file to upload",
				prompt: "> ",
				defaultValue: "");
			if (result == null
				|| result.Length == 0
				|| !File.Exists(result))
			{
				Console.WriteLine($"Unable to find file at {result}");
				Environment.Exit(1);
			}
			return result;
		}
		static string ReadApiFromConsole()
		{
			string result = ReadFromConsole(
				preamble: "Please enter API Endpoint",
				prompt: "(default: https://localhost:44337/api/)> ",
				defaultValue: "https://localhost:44337/api/");
			if (!result.EndsWith('/'))
			{
				result += "/";
			}
			return result;
		}
		static string ReadFileContents(string pathToFile)
		{
			string fileContents = "";
			try
			{
				fileContents = File.ReadAllText(pathToFile);
			}
			catch (Exception ex)
			{
				Console.WriteLine($"Exception reading file {pathToFile}: {ex}");
				Environment.Exit(1);
			}
			return fileContents;
		}
		static void Main()
		{
			string api = ReadApiFromConsole();
			string userName = ReadFromConsole(
				preamble: "Please enter username",
				prompt: "> ",
				defaultValue: "");
			string password = ReadFromConsoleSecurely(
				preamble: "Please enter password",
				prompt: "> ");
			string pathToFile = ReadPathFromConsole();
			if (VERBOSE)
			{
				Console.WriteLine($"API       : {api}");
				Console.WriteLine($"UserName  : {userName}");
				string mutedPassword = new('*', password.Length);
				Console.WriteLine($"Password  : {mutedPassword}");
				Console.WriteLine($"PathToFile: {pathToFile}");
			}
			using HttpClient client = new() { BaseAddress = new Uri(api) };
			string token = ReadToken(client, userName, password);
			string fileContents = ReadFileContents(pathToFile);
			string jsonContents = SerializeToJSON(fileContents);
			AddEquipment(client, token, jsonContents);
		}
	}
}
