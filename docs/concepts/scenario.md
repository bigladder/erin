# Scenario

A scenario is an event requiring a network simulation.
A scenario has:

- name :: string
- duration :: a Distribution
- occurrence :: a Distribution
- intensities :: Map String Distribution
- network id :: String

Whenever a scenario becomes active, the corresponding network should be run.

- Scenario(String name, Dist duration, Dist occurrence,
           (Map String Dist) intensities, Newtork nw,
           (Map String Component) components)
- Scenario::run() -> ScenarioResults
  - runs the given simulation starting at the current time.
- Scenario::get\_all\_results() -> (Vector ScenarioResults)
  - after the scenario simulation is finished, this allows us to retrieve all
    scenario results

ScenarioResults should contain:

- start\_time :: RealTimeType
- duration :: RealTimeType
- results :: (Map String (Vector Datum))
- streams :: (Map String StreamType)
- component\_types :: (Map String ComponentType)

Datum is:

- time :: RealTimeType
- requested\_value :: FlowValueType
- achieved\_value :: FlowValueType
