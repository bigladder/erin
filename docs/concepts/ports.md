# Ports

A port can be either an inport or an outport although that comes up in their usage vs being an explicit class.
In other words, both inport and outport usages are still an instance of Port.
A connection between an inport and and outport forms a flow.
A flow has both a requested flow and an achieved flow.

The basic premise: don't send messages unless there is new information to propagate.
If a message has already been sent upstream or downstream, we assume it is still in place unless we hear different.
