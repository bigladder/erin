# ADR 1: Choice of C++ As the Implementation Language

## Context

ADR is an "architecture decision record" to record why certain choices were made.
See [this blog post](http://thinkrelevance.com/blog/2011/11/15/documenting-architecture-decisions) for more detail.
Note: this document has also been added to the repository under `docs/reference/Documenting Architecture Decisions by Michael Nygard.pdf` for convenience.

The context of this ADR is to discuss the choice of programming language for E2RIN, the code name for the engine part of DISCO.
The requirements as I understand them are:

- Requirement for the engine-part to run fast
- Existence of ADEVS (and BDEVS) as a C++ library we can build on
- In-house Knowledge of C++

## Decision

Use C++ for the engine language.


## Status

Accepted


## Consequences

C++ may run fast but it is:

- slow to develop with (versus Ruby, Python, Java, Clojure, etc.)
    - is difficult to integrate additional libraries (no package managers)
    - although I know C++, it is not my most efficient development language
- can have subtle errors such as memory leaks

Our current approach is to have a fast engine that reads in TOML and writes out TOML and does simulations.
See also the discussion in ADR 3.
