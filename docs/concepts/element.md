# Element

Elements are used in the code (within ERIN as C++ objects) to represent components.
Typically, many elements are used to represent one component.
Elements are meant to be "simple" (i.e., as opposed to complex); that is, Elements are concerned with doing one conceptual thing in an unencombered, untwisted, uninterleaved, and unfettered way.

Elements represent fundamental network actions:

- flow source
- flow sink
- flow conversion (inflow to an outflow, typically with some loss)
- flow storage
- flow routing (manifolds, routers, etc.)
- flow limiting
- flow metering (measuring flow)
- flow switching (on/off behavior; a gate, a switch)
- ... others?

# Invariants

- There can be at most one unique (element/port, element/port) connection.
  This implies that during an external transition, we receive input from an upstream or downstream element at most once.
- Element ports are inflow or outflow (with respect to the element).
  That is, an inflow port is flow into the element.
  An outflow port is flow out of an element.
  We never receive negative values on inflow or outflow ports. 
  That is, flow never reverses direction.
  To model flow changing direction requires hooking up explicit inflow and outflow ports for that purpose.
  For example, the inflow port on an energy store would represent "charging".
  Similarly, the outflow prot on the energy store would represent "discharge".
- Flow requests propagate from sinks back to sources.
  If nothing is heard from upstream, the request is assumed to be fulfilled. 
  In response to a request, an upstream component may deliver less than requested, but never more.
- Requests are stored and never "destoyed".
  If conditions change such that a request can later be satisfied, that new information will be propagated.
  As requests change, that information is propagated.
  As achieved conditions change, that information is propagated.
  We are ultimately interested in the achieved conditions but the request information is never destroyed.
- For a given ((source-comp,outflow-port-j), (sink-comp,inflow-port-l)) combination:
  - at a given time:
    - sink-comp would send at most 1 inflow-request message to source-comp
    - source-comp would be silent if it (and all other upstream components) can meet the request
    - source-comp would send at most 1 outflow-achieved message to sink-comp if
      it cannot meet request OR if situations change for a pending request at a
      later time (e.g., energy store runs out of stored energy)

# Comments

Is the Flow_meter a fundamental concept? Or should metering potentially be part of every Element?
If we eliminate explicit Flow_meters, it could significantly reduce the number of Elements required to represent any given Component...

Currently, Elements are the devs models and Components are conceptual.
However, it may actually make more sense for Elements to be minimal C++ classes with input_request, output_request, etc. and let Components be the devs models.
The main reason for doing that would be to cut down on the number of entities that go into the devs simulator since the overhead of devs grows with the number of entities.
Note: this suggestion would involve a fairly major conceptual overhall so we just note it in passing for the moment.
