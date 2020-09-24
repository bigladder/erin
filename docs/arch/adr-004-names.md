# ADR 4: Names

## Context

The individual names of the modules.

ERIN was originally conceived as part of a bigger project called "DISCO".
DISCO was to be the run manager and do things such as:

- talk with an Excel-based component database
- investigate constraints and down-select
- load architecture templates and fill them
- run the simulation engine, ERIN, for sizing and design runs
- iterate and optimize over the design space
- provide a user interface
- return results

This view of DISCO and its need was later abandoned as too agressive for the time and funding available.
This ADR documents the original names for historic reasons.


## Decision

Here's what we have so far:

DISCO = District Infrastructure System Component Optimization: refers to the entire implementation.

Alternatives:

- DISCO: District Infrastructure Scenario Component Operation
- DISCO: District Infrastructure Scenario Component Optimization
- DENSS: District Energy Network Scenario Simulator

dancer: multiple case runner and results consolidator which may also include search and optimization in the future.
The "job runner".

E2RIN = Economics, Energy-use, and Resilience of Interacting Networks.
The engine that runs a single assessment of a network.

Alternatives:

- AREN = Assessment of Resilient Energy Networks. The engine written in C++ that assesses a single network.
- E2RIN = Economics, Energy Use, and Resilience of Infrastructure Networks
- EERIN = ... same as above, different writing
- CAREN = Cost Assessment of Resilient Energy Networks

E2RIN was later renamed to ERIN (Energy Resilience of Interacting Networks) and ERIN became the sole focus.
The "2" was dropped as it became apparent that it was not ideal to do the costing within the simulator; hence, there was no need to carry "economics" in the title (since the simulator didn't do economics).


## Status

Accepted.


## Consequences

These should still be considered working names but working names tend to become the actual names if not adjusted.
