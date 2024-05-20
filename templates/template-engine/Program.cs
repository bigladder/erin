using TemplateEngine;

const bool ECHO_ARGS = true;

if (args.Length != 3 && args.Length != 4)
{
	Console.WriteLine("Usage: template-engine <templateDir> <inputFile> <outputFile> <verbose:optional>");
	return 1;
}

string templateDirectory = args[0];
string inputFile = args[1];
string outputFile = args[2];
bool verbose = false;
if (args.Length == 4)
{
	if (!bool.TryParse(args[3], out verbose))
	{
		Console.WriteLine($"Problem parsing verbosity flag; assuming false");
		verbose = false;
	}
}

if (ECHO_ARGS)
{
	Console.WriteLine($"template directory: {templateDirectory}");
	Console.WriteLine($"input file        : {inputFile}");
	Console.WriteLine($"output file       : {outputFile}");
	Console.WriteLine($"verbosity         : {verbose}");
}

Utils.Run(templateDirectory, inputFile, outputFile, verbose);
if (verbose)
{
	Console.WriteLine("Done!");
}
return 0;
