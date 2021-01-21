# Reliability

The reliability module is defined in the following files:

- `include/erin/reliability.h`
- `src/erin_reliability.cpp`

This module was designed based on the "entity/component/system" paradigm.
Roughly, the Entity/Component/System concepts correspond to the following:

- entity: an id.
  Usually, an index into a vector.
- component: plain raw data.
  Usually, a struct that collects data related to a concept.
- system: a function (or method on an object)
  Used to tackle one behavior or action.

Practically, data is arranged much like tables in a relational database management system.

The concepts covered are as follows:

## Dist - Distribution

Functions used to determine the time-advance for a given "dice roll" (value from the random number generator).
A distribution corresponds to either a fixed kind of statistical distribution or tabular data corresponding to a quantile or probability density function.

| `dist_id` (primary key) | `dist_type` | `dist_subtype_id` |
| ----------------------- | ----------- | ----------------- |
| `size_type`             | `DistType`  | `size_type`       |


### Dist Type: Fixed

A distribution function with a set/fixed value.
This is essentially a Dirac delta function as a PDF.
As a quantile, this is essentially a step-function where the value of the step is always fixed.

| `fixed_dist_id` (primary key) | `value` (in seconds) |
| ----------------------------- | -------------------- |
| `size_type`                   | `int64_t`            |


## Failure Mode

A single failure mode ties together a failure distribution and a corresponding repair distribution.

| `fm_id` (primary key) | `name`        | `failure_dist_id` | `repair_dist_id` |
| --------------------- | ------------- | ----------------- | ---------------- |
| `size_type`           | `std::string` | `size_type`       | `size_type`      |


## Component to Failure Mode

A single failure mode can be referenced by multiple components and a single component can have multiple failure modes.

| `id` (primary key) | `fm_id`     | `component_id` |
| ------------------ | ----------- | -------------- |
| `size_type`        | `size_type` | `size_type`    |


## Reliability Schedule

A mapping from `component_id` to a vector of `TimeState`:

```C++
struct TimeState {
  int64_t time{0};
  double state{0.0};
};
```

The reliability schedule is held in a mapping between component id and a vector of TimeState.
The vector of TimeState is pre-calculated from time zero to the maximum time of the simulation.
Whenever a scenario is activated, the relevant component TimeState vectors are clipped to the scenario's start and end times.

An `On_Off_Switch` element is used to turn the given component on/off per the reliability schedule.
