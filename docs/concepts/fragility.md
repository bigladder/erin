# Fragility

Fragility indicates the likelihood of failure for a given component as a function of the intensity of various damage metrics associated with a scenario.
A component declares 0 or more fragilities.
When a scenario having damage metrics that a component is fragile to occurs, we look up the probability of failure using a fragility curve.
The fragility curve maps damage intensity on the horizontal axis to failure probability on the vertical axis.
If a component can be repaired due to a fragility failure, the component can specify a `fragility_repair_dist`.
Regardless of the number of fragilities declared on a component, only one `fragility_repair_dist` can be declared.
If a component is unlikely to be repaired due to a fragility failure, the `fragility_repair_dist` can be left blank and the component will be unavailable for the entire scenario.

The value of the `fragility_repair_dist` is a string that corresponds to a dist (i.e., distribution) id.
