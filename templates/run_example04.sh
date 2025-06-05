rm output04.toml
dotnet build template-engine
./template-engine/bin/Debug/net8.0/template-engine templates example04-input.toml output04.toml true

