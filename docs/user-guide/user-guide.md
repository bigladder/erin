---
title: "User's Guide"
subtitle: "Reliability Calculation Tool and Excel User Interface"
author: "Big Ladder Software"
date: "July 28, 2020"
documentclass: scrartcl
number-sections: true
figureTitle: "Figure"
tableTitle: "Table"
figPrefix: "Figure"
tblPrefix: "Table"
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
- while providing key energy usage, resilience, and reliability metrics for the modeler / planner
- and the tool is available as open-source software.

Several command-line programs are included with the *E^2^RIN* distribution including 3 key executables along with a library written in the *C++* programming language.
Documentation of the library itself is beyond the scope of this document.
However, the 3 executables will be given attention in this User's Guide as they are of particular interest to modelers.

The minimal user interface written in Microsoft Excel uses the command-line simulation tool behind the scenes as well as a *Modelkit/Params*[^2] template to make it easier to use.
We will cover usage of the Microsoft Excel interface in addition to the command-line programs.

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

Valid entries for each of the sections are described in [@tbl:sim-info; @tbl:loads; @tbl:comps-common; @tbl:source; @tbl:load; @tbl:converter; @tbl:pass-through; @tbl:uncontrolled-src; @tbl:mover; @tbl:frags; @tbl:cdf; @tbl:fm; @tbl:nw; @tbl:scenarios].

The types given are one of:

* str: a string of characters in "quotes"
* bool: true or false
* real: a real number (0.0, 1.5, 2e7, etc.)
* real>0: a real number greater than 0.0. $0.0 <$ real>0
* int: an integer
* int>0: an integer > 0
* $[X]$: an array of the given type, $X$
* $\left[\left[X\right]\right]$: an array of arrays of $X$
* time: time unit. One of {"years", "days", "hours", "minutes", "seconds"}
* cap: capacity unit. One of {"kJ", "kWh"}
* disp: dispatch strategy. One of {"distribute", "in_order"}.
* frac: real fraction. $0.0 \le$ frac $\le 1.0$
* frac>0: real fraction greater than 0.0. $0.0 <$ frac $\le 1.0$
* rate: the rate unit. Currently, only "kW" is accepted.
* $X \rightarrow Y$: designates a map data structure (a.k.a., dictionary, hash table, table, etc.).
  Associates $Y$ with $X$.

In the TOML input file, all constructs except `simulation_info` have an id.
The id is used when one construct references another.

This looks as follows:

```toml
[loads.load_id_1]
...
[loads.load_id_2]
...
[components.comp_id_1]
...
[components.comp_id_2]
...
[fragility.fragility_id_1]
...
[cdf.cdf_id_1]
...
[failure_mode.fm_id_1]
...
[networks.nw_id_1]
...
[scenarios.scen_id_1]
...
```

An id must follow the rules of TOML "bare keys"[^bare-keys] with the exception that dashes (`-`) are not allowed and the key must start with an ASCII letter:

> Bare keys may only contain ASCII letters, ASCII digits, underscores, ... .

[^bare-keys]: see https://toml.io/en/v1.0.0-rc.1#keys

| key                   | type   | required? | notes                                   |
| ----                  | --     | --        | --------                                |
| `time_unit`           | time   | no        | The time unit. Default "years"          |
| `fixed_random`        | frac   | no        | Sets the random roll to a fixed value   |
| `fixed_random_series` | [real] | no        | Sets random numbers to the given series |
| `random_seed`         | real   | no        | Sets the random number generator's seed |
| `max_time`            | int    | no        | Maximum simulation time. Default: 1000  |

: `simulation_info` specification {#tbl:sim-info}

Note: [@tbl:sim-info] specifies various random values. At most, one of these values can be specified.

| key               | type         | required? | notes                           |
| ----              | --           | --        | --------                        |
| `csv_file`        | str          | no        | path to CSV file with profile   |
| `time_rate_pairs` | \[\[real\]\] | no        | array of (time, rate) pairs     |
| `time_unit`       | time         | no        | time unit for `time_rate_pairs` |
| `rate_unit`       | rate         | no        | rate unit for `time_rate_pairs` |

: `loads` specification {#tbl:loads}

For [@tbl:loads], one must specify either a `csv_file` *or* `time_rate_pairs`, `time_unit`, and `rate_unit`.
Unfortunately, only "kW" is available for `rate_unit` at the moment although `time_unit` accepts "years", "seconds", or "hours".
Practically speaking, you will almost always use a `csv_file` unless you just want to test a simple load.

For the `csv_file`, the header must be "hours,kW" with data filled into the rows below.
The "hours" column is the elapsed time in hours.
The "kW" column is the flow in kW.
The first column header can be set to values beside "hours"; any time unit is valid.
However, the rate unit is currently locked in as "kW".

| key             | type    | required? | notes                             |
| ----            | --      | --        | --------                          |
| `failure_modes` | \[str\] | no        | failure mode ids for component    |
| `fragilities`   | \[str\] | no        | fragility curve ids for component |

: `components`: common attributes {#tbl:comps-common}

[@tbl:comps-common] lists the attributes common to all components.
These relate to reliability and resilience: failure modes and fragility curves.

| key           | type | required? | notes                     |
| ----          | --   | --        | --------                  |
| `type`        | str  | yes       | must be "source"          |
| `outflow`     | str  | yes       | type of outflow           |
| `max_outflow` | real | no        | maximum allowable outflow |

: `components`: Source Component {#tbl:source}

| key                 | type                  | required? | notes                         |
| ----                | --                    | --        | --------                      |
| `type`              | str                   | yes       | must be "load"                |
| `inflow`            | str                   | yes       | type of outflow               |
| `loads_by_scenario` | str $\rightarrow$ str | yes       | map of scenario id to load id |

: `components`: Load Component {#tbl:load}

In [@tbl:load], the `loads_by_scenario` structure is specified as follows:

```toml
loads_by_scenario.scenario_id_1 = "load_id_1"
loads_by_scenario.scenario_id_2 = "load_id_2"
```

| key                   | type   | required? | notes                             |
| ----                  | --     | --        | --------                          |
| `type`                | str    | yes       | must be "converter"               |
| `inflow`              | str    | yes       | type of inflow                    |
| `outflow`             | str    | yes       | type of outflow                   |
| `lossflow`            | str    | no        | type of lossflow. Default: inflow |
| `constant_efficiency` | frac>0 | yes       | constant efficiency               |

: `components`: Converter Component {#tbl:converter}

| key             | type | required? | notes                                  |
| ----            | --   | --        | --------                               |
| `type`          | str  | yes       | must be "store"                        |
| `flow`          | str  | yes       | type of flow (inflow, outflow, stored) |
| `capacity_unit` | cap  | no        | capacity unit. Default: "kJ"           |
| `capacity`      | real | yes       | capacity of the store                  |
| `max_inflow`    | real | yes       | maximum inflow (charge rate)           |

: `components`: Storage Component {#tbl:store}

During simulation, the `max_inflow` sets the requested charging rate for a storage unit (see [@tbl:store]).
By default, a storage unit will always request to charge itself to its maximum capacity.
However, it will always honor its discharge request above its charge request.
That is, if discharge is requested, it will discharge rather than charge.
If charging and discharging at the same time, charge flow will "short circuit" to meet the discharge request first.
Any flow left over will charge the store.

| key                 | type | required? | notes                                  |
| ----                | --   | --        | --------                               |
| `type`              | str  | yes       | must be "muxer"                        |
| `flow`              | str  | yes       | type of flow (inflow, outflow)         |
| `num_inflows`       | int  | yes       | the number of inflow ports             |
| `num_outflows`      | int  | yes       | the number of outflow ports            |
| `dispatch_strategy` | disp | no        | dispatch strategy. Default: "in_order" |

: `components`: Storage Component {#tbl:muxer}

| key    | type | required? | notes                          |
| ----   | --   | --        | --------                       |
| `type` | str  | yes       | must be "pass_through"         |
| `flow` | str  | yes       | type of flow (inflow, outflow) |

: `components`: Pass-Through Component {#tbl:pass-through}

| key                  | type                  | required? | notes                          |
| ----                 | --                    | --        | --------                       |
| `type`               | str                   | yes       | must be "uncontrolled_source"  |
| `outflow`            | str                   | yes       | type of outflow                |
| `supply_by_scenario` | str $\rightarrow$ str | yes       | scenario id to load profile id |

: `components`: Uncontrolled Source Component {#tbl:uncontrolled-src}

Similar to the load component, the uncontrolled source's `supply_by_scenario` specifies supply profiles by scenario.
These look like the following:

```toml
supply_by_scenario.scenario_id_1 = "load_id_1"
supply_by_scenario.scenario_id_2 = "load_id_2"
```

Note that the uncontrolled source supply profiles are also drawn from the same section of the input file specified as `loads`.

| key       | type   | required? | notes                                               |
| ----      | --     | --        | --------                                            |
| `type`    | str    | yes       | must be "mover"                                     |
| `inflow0` | str    | yes       | the inflow being "moved"                            |
| `inflow1` | str    | yes       | the "support" inflow that enables "moving" to occur |
| `outflow` | str    | yes       | the outflow                                         |
| `cop`     | real>0 | yes       | the coefficient of performance                      |

: `components`: Mover Component {#tbl:mover}

In [@tbl:mover], the `cop` field ties together the three flows `inflow0`, `inflow1`, and `outflow` using the following relations:

$$cop = \frac{inflow_0}{inflow_1}$$

$$inflow_0 = cop \times inflow_1$$

$$inflow_1 = inflow_0 \times \frac{1}{cop}$$

$$outflow = (1 + cop) \times inflow_1 = (1 + \frac{1}{cop}) \times inflow_0 = inflow_0 + inflow_1$$

| key             | type | required? | notes                                                      |
| ----            | --   | --        | --------                                                   |
| `vulnerable_to` | str  | yes       | the scenario intensity (i.e., damage metric) vulnerable to |
| `type`          | str  | yes       | must be "linear"                                           |
| `lower_bound`   | real | yes       | the value below which we are impervious to damage          |
| `upper_bound`   | real | yes       | the value above which we face certain destruction          |

: `fragility` specification {#tbl:frags}

Fragility curves are specified using the attributes listed in [@tbl:frags].
[@fig:fragility-curve] shows a graphical representation of the data specification.

![Fragility Curve](images/fragility-curve.png){#fig:fragility-curve}

A fragility curve maps a scenario's intensity (i.e., damage metric) to a probability of failure.
We must specify which damage metric is of interest and also the curve relationship.
Currently, the only available fragility curve type is linear.
For the linear curve, we specify the `lower_bound`, the bound below which we are impervious to destruction.
We also specify the `upper_bound`, the bound above which we face certain destruction.

| key         | type | required? | notes                                         |
| ----        | --   | --        | --------                                      |
| `type`      | str  | yes       | must be "linear"                              |
| `value`     | real | yes       | the value of the fixed CDF                    |
| `time_unit` | time | yes       | the time unit used to specify the fixed value |

: `cdf` specification {#tbl:cdf}

[@tbl:cdf] specifies a cumulative distribution function.
At this time, the only distribution type available is "fixed".
A fixed distribution is a degenerate distribution that always samples a single point -- the `value`.

| key           | type | required? | notes              |
| ----          | --   | --        | --------           |
| `failure_cdf` | str  | yes       | the failure CDF id |
| `repair_cdf`  | str  | yes       | the repair CDF id  |

: `failure_mode` specification {#tbl:fm}

| key           | type        | required? | notes           |
| ----          | --          | --        | --------        |
| `connections` | \[\[str\]\] | yes       | the connections |

: `networks` specification {#tbl:nw}

The `networks` data definition involves a "mini-language" to specify connections.
The language is as follows:

```toml
connections = [
  [ "src_comp_id:OUT(outflow_port)", "sink_comp_id:IN(inflow_port)", "flow"],
  ...
  ]
```

The `connections` key is an array of 3-tuples.
The first element of the 3-tuple is the source component id separated by a ":" and then the word "OUT(.)".
You will type the outflow port in place of the ".".
Note that numbering starts from 0.

The second element of the 3-tuple is the sink component id.
That is, the component that *receives* the flow.
The sink component id is written, then a ":", and finally the word "IN(.)".
You will type the inflow port id in place of the ".".
Numbering of inflow ports starts from 0.

The final element of the 3-tuple is the flow id.
You are requested to write the flow id as a check that ports are not being wired incorrectly.

| key                       | type                   | required? | notes                                                 |
| ----                      | --                     | --        | --------                                              |
| `time_unit`               | time                   | no        | time units for scenario. Default: "hours"             |
| `occurrence_distribution` | table                  | yes       | see notes in text                                     |
| `duration`                | int>0                  | yes       | the duration of the scenario                          |
| `max_occurrences`         | int                    | yes       | the maximum number of occurrences. -1 means unlimited |
| `calculate_reliability`   | bool                   | no        | whether to calculate reliability. Default: false      |
| `network`                 | str                    | yes       | the id of the network to use                          |
| `intensity`               | str $\rightarrow$ real | no        | specify intensity (damage metric) values              |

: `scenarios` specification {#tbl:scenarios}

In [@tbl:scenarios], the `occurrence_distribution` is currently implemented as a literal table:

```toml
  occurrence_distribution = { type = "linear", value = 8, time_unit = "hours" }
```

The possible values for the `occurrence_distribution` table are given in [@tbl:cdf].

# Command-Line Tool

Three command-line programs are available for simulation and assistance.
They will be given an overview here.

## `e2rin`

Simulates a single scenario and generates results.

**USAGE**:

`e2rin` `<input_file_path>` `<output_file_path>` `<stats_file_path>` `<scenario_id>`

- `input_file_path`: path to TOML input file
- `output_file_path`: path to CSV output file for time-series data
- `stats_file_path`: path to CSV output file for statistics
- `scenario_id`: the id of the scenario to run

## `e2rin_multi`

Simulates all scenarios in the input file over the simulation time and generates results.

**USAGE**:

`e2rin_multi` `<input_file_path>` `<output_file_path>` `<stats_file_path>`

- `input_file_path`: path to TOML input file
- `output_file_path`: path to CSV output file for time-series data
- `stats_file_path`: path to CSV output file for statistics

## `e2rin_graph`


# Microsoft Excel User Interface

<!--

Show the overview of the template for a given location and give an overview of what a location is.

-->

## Additional Concept: Location

## Modelkit/Params Template Engine


# Example Problem
