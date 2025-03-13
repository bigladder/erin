rm output02.toml
dotnet build template-engine
./template-engine/bin/Debug/net8.0/template-engine templates example02-input.toml output02.toml true

