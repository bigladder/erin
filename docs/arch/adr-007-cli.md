# ADR 7: Command-Line Interface

## Context

The command-line interface (CLI) for ERIN enables users and programmers to run simulations and execute utility functions
for development, testing, and interrogation. Some basic functionality had been enabled through separate executables tailored for 
principle tasks, but suffered from a lack of flexibility and standardization, along with textual feedback to the user. 
CLI syntax protocols are well-established, with `git` being a widely cited example. An ideal CLI executable can be 
relatively small, merely providing access to the larger, internal library, while enforcing command syntax. Fortunately, 
sophisticated, shared libraries are available, which largely remove any need to develop a CLI executable from scratch. 

## Decision

Two 3rd-party utilities were identified as potential candidates to provide CLI capability to ERIN:
- [CLI11](https://github.com/CLIUtils/CLI11)
- [argparse](https://github.com/p-ranav/argparse)

The search for a CLI tool to incorporate in ERIN was quickly truncated after making rapid progress with CLI11. CLI11 appears 
to provide simple methods for adding functionality, with a powerful and intuitive API. For example, "help" is automatically 
made available for the full CLI tool, as well as reporting of options and flags for each command that is defined.

## Status

Accepted.

## Consequences

The adoption of CLI11 appears to allow future expansion and modification of the CLI command set for ERIN with minimal effort. 
