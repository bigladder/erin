using Tomlyn.Model;

using TemplateEngine;

const bool ECHO_ARGS = true;

if (args.Length != 3)
{
	Console.WriteLine("Usage: template-engine <templateDir> <inputFile> <outputFile>");
	return 1;
}

string templateDirectory = args[0];
string inputFile = args[1];
string outputFile = args[2];

if (ECHO_ARGS)
{
	Console.WriteLine($"template directory: {templateDirectory}");
	Console.WriteLine($"input file        : {inputFile}");
	Console.WriteLine($"output file       : {outputFile}");
}

Utils.Run(templateDirectory, inputFile, outputFile);
Console.WriteLine("Done!");
return 0;
