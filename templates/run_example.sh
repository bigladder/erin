rm output.toml
dotnet build template-engine
./template-engine/bin/Debug/net8.0/template-engine templates example-input.toml output.toml
