---
title: "User's Guide"
subtitle: "Reliability Calculation Tool and Excel User Interface"
author: "Big Ladder Software"
date: "July 28, 2020"
documentclass: scrartcl
number-sections: true
figureTitle: "Figure"
tableTitle: "Table"
figPrefix: "figure"
tblPrefix: "table"
...
# Introduction

<!--

What's the purpose of the user guide.
What's the purpose of the tool.
What was it made to do.
What are the pieces.
Perhaps don't explain the acronym here.

-->

The purpose of this User's Guide is to give a working introduction to the command-line version of the resilience calculation tool (*E^2^RIN*[^1]) and a user interface for the tool written in Microsoft Excel.

The purpose of the tool itself is to simulate the energy flows through a district energy system composed of an interacting network of components.
The main contributions of this tool that we maintain are unique in aggregate are as follows:

- the tool accounts for both reliability (failure and repair) as well as resilience to various scenarios (design basis threats)
- while also accounting for topology and interaction between an open-ended number of energy networks
- while providing key energy usage, resilience, and reliability metrics for the modeler / planner.

Several command-line programs are included with the *E^2^RIN* distribution including 3 key executables along with a library written in the *C++* programming language.
The 3 executables will be given attention in this User's Guide as they are of particular interest to modelers.

The minimal user interface written in Microsoft Excel uses the command-line simulation tool behind the scenes as well as a *Modelkit/Params*[^2] template to make it easier to use.

[^1]: E^2^RIN originally stood for Energy, Economics, and Resilience of Interacting Networks.
However, the economics portion has been moved out of the engine itself.
The current name at the time of this writing is a working name subject to change in the future.

[^2]: *Modelkit/Params* is a separate open-source project [available](https://bigladdersoftware.com/projects/modelkit/) from Big Ladder Software.

# Simulation Overview

In this section, we describe the simulation process to assess the resilience of a district system network to various scenarios including Design Basis Threats.

District Energy Systems play a major role in enabling resilient communities.
However, resilience is contextual.
That is, one must specify what one is resilient to.
This is specified in the tool using various "scenarios" which represent normal operation and various Design Basis Threats.
Design Basis Threats are low-probability, high-impact events such as hurricanes, flooding, earthquakes, terrorist attacks, tornadoes, ice storms, viral pandemics, etc.
Taking into account relevant Design Basis Threats is necessary for enabling resilient public communities.

The tool operates over networks that supply energy to both individual buildings and districts.
These networks are comprised of components (loads, generation, distribution/routing, storage, and transmission assets) and connections.
These connections form the topology of the network -- what is connected to what.
Multiple flows of energy / material can be modeled: notably, both thermal (heating/cooling) and electrical flows and their interactions.

This network of components is subject to various scenarios which represent one or more ideal cases (i.e., "blue-sky") as well as Design Basis Threats (also known as "black-sky" events).
Each scenario has a probability of occurrence and zero or more "damage intensities" associated with it such as wind speed, vibration, water inundation level, etc.
Fragility curves are used to relate the scenario's damage intensities with the percentage chance that a given component will fail to work under the duress of the scenario.

Additionally, reliability statistics can be associated with components to model their routine failure and repair times and to take reliability into account in conjunction with various threats.
Note, however, that routine reliability statistics are most likely not applicable to an extreme event such as those represented from a design basis threat.
Fragility curves are more appropriate for that kind of assessment.

By looking at the performance of the network while taking into account the possibility of failure due to both typical reliability and failure due to threats, resilience metrics such as maximum downtime, energy availability, and load-not-served can be calculated.
This can, in turn, help planners to see whether a proposed system or change to an existing system will meet their threat-based resilience goals.

The workflow for using the tool is as follows:

* using a piece of paper, sketch out the network of locations and components and how they are connected
* using either the Excel user interface or a text editor, build an input file that describes:
  - the network of components
    - component physical characteristics
    - component failure modes
    - component fragility
    - how components are connected to each other
  - the scenarios to evaluate
    - the duration of the scenario
    - the occurrence distribution
    - damage intensities involved
  - load profiles associated with each load for each scenario
* simulate the given network over the given scenarios and examine the results

The simulation is specified using a discrete event simulator.
Events include:

- changes in a load
- changes in an uncontrollable source such as PV power generation
- routine failure of a working component under reliability
- routine repair of a failed component
- events due to physical limitations of devices (e.g., depleting the energy in a battery or diesel fuel tank)
- the initiation or ending of a scenario
- application of fragility curves at a scenario start

For every event that occurs, the simulation resolves and negotiates the conservation of energy throughout the network.
This results in resolving the flows through all connections in the network after each event.
Loads in particular are tracked to identify energy not served, time that a load's request is not fully supplied, and also to calculate the energy availability (energy served x 100% / energy requested).
These statistics are calculated *by load* and *by scenario*.

# Concept Overview

This section gives a quick overview of the key concepts used in the tool.
Understanding the concepts will help when authoring an input file as well as in interpreting the output results.

## Flows

A flow is any movement of a type of energy (usually) or material.
Examples include "electricity", "heated water for district heating", and "chilled water for cooling".
The flows specified are open-ended and not prescribed by the tool.
However, to aid new users, the Excel User Interface does limit the available flows to those typically used in an assessment.

A flow has a direction associated with it.
A flow can be zero (i.e., nothing is flowing) but cannot be negative.
Negative flows would imply a change in direction which would greatly increase the complexity of the simulation tool.
As such, we do not allow negative flows.
However, it is possible to simulate bi-directional flow by connecting components from both directions (more on this later).

## Components and Ports

A component is meant to represent any of a myriad of equipment used in a district energy system.
A component has zero or more inflow ports and zero or more outflow ports.
These ports take in zero or more flows, route and/or transform them, and output zero or more flows.
A component must have at least one port: inflow or outflow.

The fidelity of modeling is that of a 1-line diagram and accounts for energy flows only.
A component need only be taken into account if:

* it's function will significantly affect network flows
* and it's failure is statistically significant in the face of either reliability or fragility to a threat

## Component Types

Because we model components at a high-level of abstraction, a small number of component types can actually model a large number of real-world components.
In this section, we discuss the available component types and their characteristics.

### Component Type: Load

A load is essentially an exit point out of the network for "useful work".
A load typically represents an end use such as a building or cluster of building's electricity consumption or heating load consumption.

A load specifies its load versus time with a load profile which is specified per scenario.

### Component Type: Source

A source is an entry point into the simulation for providing energy flow into the network.
A source typically represents useful energy into the system such as electrical energy from the utility, natural gas into the district, or diesel fuel transported to a holding tank.

### Component Type: Uncontrolled Source

A normally source responds to a request up to the available max output power (which defaults to being unlimited).
In contrast, an uncontrolled source cannot be commanded to a given outflow because the source is uncontrollable.
Typical examples of uncontrolled sources are electricity generated from a photovoltaic array, heat generated from concentrating solar troughs, or electricity from a wind farm.
Another typical uncontrolled source is heat to be removed from a building as a "cooling load".

An uncontrolled source specifies its supply values versus time with a supply profile which is specified per scenario.
Note: functionally, a supply profile and load profile are the same thing.

### Component Type: Converter

A converter represents any component that takes in one kind of flow and converts it to another type of flow, usually with some loss.
Converters have an efficiency associated with them.
The current version of the tool only supports a constant-efficiency.
Typical examples of converter components are boilers, electric generators (fired by diesel fuel or natural gas), transformers, and line-losses.

The loss flow from one component can be chained into another converter component to simulate various loss-heat recovery mechanisms and equipment such as combined heat and power (CHP) equipment.

### Component Type: Storage

A storage component represents the ability to store flow.
The storage unit has both a charge (inflow) port and a discharge (outflow) port.
The storage component cannot accept more flow than it has capacity to store.
Similarly, a storage component cannot discharge more flow than it has stored.
Typical examples of a storage component include battery systems, pumped hydro, diesel fuel storage tanks, coal piles, and thermal energy storage tanks.

The current version of the storage tank does not have an efficiency or leakage component associated with it although these elements can be approximated with converter components.

### Component Type: Pass-Through

A pass-through component is a component that physically exists on the system but that only passes flow through itself without disruption.
This is useful for giving a location to associate failure modes and fragility curves (discussed below) as failure of the component may result in a loss of a flow.
Typical examples of pass-through components are above-ground and below-ground power lines, natural-gas pipe runs, district heating pipe runs, etc.

### Component Type: Muxer

A "muxer" or multiplexer component represents various components for splitting and joining flows.
Typical examples include manifolds, routers, electrical bus bars, and the like.

Muxers can have multiple inflow ports and multiple outflow ports.
Muxer's contain dispatch strategies to choose how requests are routed.
There are two dispatch strategies available in the current tool:

- in-order dispatch: all flow is requested to be satisfied from the first inflow port first.
  If that flow is insufficient, the second inflow port is requested for the remainder until all inflow ports are exhausted.
  For cases where outflow request is not met, the first outflow port is satisfied first.
  If flow remains, the next port is satisfied, etc.
- distribute dispatch: all flow is distributed between all ports.
  In this strategy, requests are distributed evenly between inflow ports.
  When flow is insufficient to meet all outflow request, available flow is distributed evenly to outflow ports.

These two flow strategies should be sufficient to mimic basic dispatch strategies.

### Component Type: Mover

Note: the mover component is currently only available from the command-line interface.
It has not yet been made available for the Excel User Interface.

A mover component is a component that moves energy from its inflow port to its outflow port with the assistance of a support flow.
Movers can be used to represent chillers and heat pumps (which move heat) as well as pumps and fans (which move fluids).

## Networks and Connections

Component connections via ports form a network.
Networks describe the interaction of various flows.

A connection describes:

- a source component and its outflow port
- a sink (i.e., receiving) component and its inflow port
- and the type of flow being delivered

## Scenarios

Scenarios represent both typical usage (i.e., blue sky events) as well as design basis threat events (a class 4 hurricane, an earthquake, a land-slide, etc.).

A scenario has:

- a duration (how long the scenario will last)
- an occurrence distribution which is a cumulative distribution function that expresses the likelihood of occurrence
- a maximum number of times the scenario can occur during the entire simulation (either *unlimited* or some finite number)
- and various damage intensities associated with the scenario

The damage intensities associated with a scenario are open-ended but are meant to represent numerical quantities that correspond with a fragility curve.
Some examples of damage intensities that could be associated with a scenario are "wind speed", "inundation depth", "vibration", etc.
Scenarios with no damage intensities are completely fine -- these would represent "blue-sky" scenarios (typical operation).

## Reliability: Failure Modes and Statistical Distributions

Reliability is handled strictly as a statistical matter using failure modes.
A failure mode is an associate between a failure cumulative distribution function and the corresponding repair cumulative distribution function.
Multiple failure modes can be specified for a single component.
For example, a diesel back-up generator may have one failure mode associated with its starter battery and another to represent more serious issues with the generator itself.

Every failure mode in the simulation is turned into an "availability schedule".
That is, for each failure mode, the dual cumulative distribution functions are alternatively sampled from time 0 to the end of the overall simulation time to derive a schedule of "available" and "failed".
When a scenario where reliability is calculated is scheduled to occur, the relevant portion of the availability schedules for any component are used to "schedule" the component as available and failed to simulate routine reliability events.

## Resilience: Intensities (Damage Metrics) and Fragility Curves

Resilience reflects how components react to the intense stresses of a design basis threat event.
Each scenario can specify an intensity or damage metric.
Any component having a fragility curve that responds to one or more of the scenario intensities is evaluated for failure due to the scenario's intensity.

For example, above-ground power lines may have a fragility to wind speed.
If a scenario specifies a wind speed of 150 mph, the above-ground power line component will use its fragility curve to look up its change of failure.
For fragility, a component is evaluated for failure at scenario start and either passes (staying up during the scenario) or fails (going down for the entire scenario).

# Input File Format

The simulation engine is a command-line program.
Even when it is accessed via the Excel User Interface, a text-based input file is written to describe the network of components and scenarios to simulate.

The input file format is written using the TOML[^3] input file language.
TOML is a plain-text input file format.

[^3]: TOML is described in detail here: https://toml.io/en/

The file consists of the following sections that describe the various concepts described above:

- `simulation_info`: general simulation information
- `loads`: load profiles (includes supply profiles for uncontrolled sources)
- `components`: all components in the network are described here
- `fragility`: all fragility curves are described here
- `cdf`: cumulative distribution functions
- `failure_mode`: failure modes are described here
- `networks`: networks are described here
- `scenarios`: scenarios are described here

Valid entries for each of the sections are described in [@tbl:sim-info]. <!-- ; @tbl:loads; @tbl:comps; @tbl:frags; @tbl:cdf; @tbl:fms; @tbl:nws; @tbl:scenarios]. -->

The types given are one of:

* string: a string of characters in "quotes"
* real: a real number (0.0, 1.5, 2e7, etc.)
* int: an integer
* array of *x*: an array of the given type

| key | type | required? | notes |
|----|--|--|--------|
| `time_unit` | string | no | The time unit. Default "years". One of {years, seconds, hours}. |
| `fixed_random` | real | no | Sets the random roll to a fixed value (0.0 $\ge$ r $\ge$ 1.0) |
| `fixed_random_series` | array of real | no | Sets random numbers to the given series. |
| `random_seed` | real | no | Sets the random number generator's seed. |
| `max_time` | int | no | Maximum simulation time. Default: 1000. |

: `simulation_info` specification key {#tbl:sim-info}

Note: [@tbl:sim-info] specifies various random values. At most, one of these values can be specified.

| key | type | required? | notes |
|----|--|--|--------|
| `csv_file` | string | no | path to CSV file with profile. |
| `time_rate_pairs` | array of array of real | no | array of (time, rate) pairs. |
| `time_unit` | string | no | one of {years, seconds, hours}. |
| `rate_unit` | string | no | one of {"kW"}. |

: `loads` specification key {#tbl:loads}

# Command-Line Tool


## e2rin_single

## e2rin_multi

## e2rin_graph

<!--



-->

# Microsoft Excel User Interface

<!--

Show the overview of the template for a given location and give an overview of what a location is.

-->

## Additional Concept: Location

## Modelkit/Params Template Engine


# Example Problem
