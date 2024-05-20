namespace tests;

public class UnitTest1
{
    [Fact]
    public void SimpleTest()
    {
        Assert.True(2 + 2 == 4);
    }

    [Fact]
    public void TestExampleWorks()
    {
        // Load the example file and run it to an output.toml
        string cwd = Directory.GetCurrentDirectory();
        string template_root_dir = Path.GetFullPath(Path.Join(cwd, "../../../.."));

        string example_input_toml = Path.Join(template_root_dir, "example-input.toml");
        string output_toml = Path.Join(template_root_dir, "out-from-test.toml");
        string example_output_toml = Path.Join(template_root_dir, "example-output.toml");
        string template_dir = Path.Join(template_root_dir, "templates");
        
        Assert.True(File.Exists(example_input_toml));
        Assert.True(File.Exists(example_output_toml));
        if (File.Exists(output_toml))
        {
            File.Delete(output_toml);
        }
        Assert.True(Directory.Exists(template_dir));

        TemplateEngine.Utils.Run(template_dir, example_input_toml, output_toml);

        Assert.True(File.Exists(output_toml));
        File.Delete(output_toml);
        // Load the example output
        // Ensure both TOML files are equivalent
    }
}
