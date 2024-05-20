using Tomlyn;
namespace tests;

public class UnitTest1
{
    [Fact]
    public void TestExampleWorks()
    {
        // Load the example file and run it to an output.toml
        string cwd = Directory.GetCurrentDirectory();
        string templateRootDir = Path.GetFullPath(Path.Join(cwd, "../../../.."));

        string exampleInputToml = Path.Join(templateRootDir, "example-input.toml");
        string outputToml = Path.Join(templateRootDir, "out-from-test.toml");
        string exampleOutputToml = Path.Join(templateRootDir, "example-output.toml");
        string templateDir = Path.Join(templateRootDir, "templates");
        
        Assert.True(File.Exists(exampleInputToml));
        Assert.True(File.Exists(exampleOutputToml));
        if (File.Exists(outputToml))
        {
            File.Delete(outputToml);
        }
        Assert.True(Directory.Exists(templateDir));

        TemplateEngine.Utils.Run(templateDir, exampleInputToml, outputToml);

        Assert.True(File.Exists(outputToml));
        // Load the example output and the actual output
		string exOutStr = File.ReadAllText(exampleOutputToml);
		var exOutData = Toml.ToModel(exOutStr);
		string outStr = File.ReadAllText(outputToml);
		var outData = Toml.ToModel(outStr);
		List<string> diffs = TemplateEngine.Utils.ModelDifferences(exOutData, outData, path:"");
		Assert.True(diffs.Count == 0, userMessage: String.Join("\n- ", diffs));
        // Ensure both TOML files are equivalent
		// Cleanup
        File.Delete(outputToml);
    }
}
