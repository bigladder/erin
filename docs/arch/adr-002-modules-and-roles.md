# ADR 2: Choice of Modules for the Final Tool

## Context

It may be simpler and better to separate the pieces of the software tool into 3 pieces:

- a user interface
- database (equipment)
- optimization, search, multiple runs, and results consolidation
- evaluate a single case


## Decision

- user interface
    - unsure yet, leaning towards MS Excel for IEA Annex 73
    - for Citadel, this would be implemented as JavaScript in the browser
- database
    - current database is in an MS Excel spreadsheet
    - use Python scripts to populate a SQLite database for IEA Annex 73
    - for Citadel, port to MySQL
- "dancer"
    - currently using Ruby despite no 0.5 TOML support
- "ERIN"
    - evaluate a single case
    - written in C++ (see ADR 1)


## Status

No longer relevant.
It became apparent that the vision to build DISCO, dancer, *and* ERIN was too ambitions for the available time and funding.
Focus has come to just ERIN.


## Consequences

Although the entirety of DISCO could be written in C++, by breaking pieces out, we believe this will enable us to work faster and leverage the strengths of different languages.
It's also possible that some of the pieces could be repurposed for other things.
