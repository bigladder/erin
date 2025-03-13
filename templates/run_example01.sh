rm output01.toml
dotnet build template-engine
./template-engine/bin/Debug/net8.0/template-engine templates example01-input.toml output01.toml true
