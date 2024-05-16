namespace TemplateEngine
{
    public enum ParamType
    {
        Unhandled,
        Enumeration,
        Fraction,
        Integer,
        Number,
        String,
        TableFromStringToString,
    }

    public class TemplateParameter
    {
        public string Name { get; set; } = "";
        public ParamType Type { get; set; }
        public bool IsOptional { get; set; }
        public Func<object, bool> Validate { get; set; }

        public TemplateParameter()
        {
            Validate = x => true;
        }
    }
}
