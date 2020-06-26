# ADR 6: Entity / Component / System

## Context

This explores a way to write C++ code that is different from the standard object hierarchy.
The main advantages are [1]:

- performance (the whole reason we're using C++ to begin with)
- extensibility (we can quickly adapt what attributes do and do not belong to any given entity)

[1]: https://medium.com/ingeniouslysimple/entities-components-and-systems-89c31464240d

For our use, the entity / component / system has the following definitions:

- entity: a globaly unique identifier; typically an index into a vector
- component: a collection of data; usually a struct of vectors
- system: a pure function or a class that operates on entities having attributes of one or more specific components

See also, [2]:

> [An] ECS [...] consists of:

> - Entity: a unique identifier (a number). Imagine it like an ID in a database.
> - Component: a data blob (a struct). This is the data that is associated with an entity. It is up to the user to create the data type, by creating a struct or a class in C++.
> - System: data transformation (a function). This is any function that operates on a number of components. The user will write simple functions that take components as inputs and produce components as outputs. It is just any C++ function, really.

[2]: https://wickedengine.net/2019/09/29/entity-component-system/

## Decision

This is mainly a decision based on how the code is growing and seeing the weaknesses of large object hierarchies.
That said, we will proceed with caution and add ECS in as needed and as time permits and as necessary.

## Status

Switching to use it on all new development going forward.

## Consequences

The main negative consequence is a differece in coding style across the project.
We hope this will open up new avenues for speed, extensibility, and performance, however.
If that bears fruit, we will eventually change the entire code-base to this style.
