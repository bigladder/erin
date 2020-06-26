# Discrete Event System Specification (DEVS)

An atomic DEVS model contains the following functions:

- internal transition function ($\delta_int$): $S \rightarrow S$ the internal transition function
- external transition function ($\delta_ext$): $Q \times X \rightarrow S$, the external transition function
- confluent transition function ($\delta_conf$): $S \times X \rightarrow S$, the confluent transition function
- output function ($\lambda$): $S \rightarrow Y$, the output function
- time advance function ($ta$): $S \rightarrow (R \cup \infty)$, the time advance function

Flow for an undisturbed element:

1. `s0 = ...`, the initial state
2. `dt = time_advance(s0)`, calculating the time advance in seconds
3. clock moves dt seconds forward
4. `ys = output_function(s0)`, note that we calculate outputs on the previous state
5. `s1 = internal_transition(s0)`, we transition to the next state and go back to 2

Flow for an element with external transition:

1. `s0 = ...`, the initial state, initial time is `t0`
2. `dt = time_advance(s0)`, calculating the time advance in seconds
3. clock moves e seconds forward
4. `s1 = external_transition(s0, e, xs)` where `0 <= e <= dt`; time is `t0 + e`
5. `dt = time_advance(s1)`, time advance is recalculated
6. clock moves dt seconds forward
7. `ys = output_function(s1)`, outputs are calculated
8. `s2 = internal_transition(s1)`, now we return to step 2

Flow for an element with confluent transition:

1. `s0 = ...`, the initial state, initial time is `t0`
2. `dt = time_advance(s0)`, calculating the time advance in seconds
3. clock moves dt seconds forward
4. `ys = output_function(s0)`, outputs are calculated
5. `s1 = confluent_transition(s0, xs)` where `e = dt`, so no need to specify it
