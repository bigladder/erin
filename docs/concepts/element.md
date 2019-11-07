# Element

Elements are used in the code (within E2RIN as C++ objects) to represent components.
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

# Comments

Is the Flow_meter a fundamental concept? Or should metering potentially be part of every Element?
If we eliminate explicit Flow_meters, it could significantly reduce the number of Elements required to represent any given Component...
