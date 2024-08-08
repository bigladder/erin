rm output03.toml
dotnet build template-engine
./template-engine/bin/Debug/net8.0/template-engine templates example03-input.toml output03.toml true

