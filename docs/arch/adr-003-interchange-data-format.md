# ADR 3: Use TOML as the Data Interchange Format

## Context

Need a format to run "ERIN" for early problem specification.

Note: a stretch goal is to implement a programmatic interface to erin to allow skipping of the reading/writing to the file system.
If erin is able to run cases fast and there are a lot of cases, a programmatic interface would allow us to skip reading/writing to the file system which could greatly enhance the speed.

However, we are starting with a data interchange format first because:

1. it enables us to move forward and it does work for even the final solution (provided it is fast enough)
2. we wouldn't want to build programmatic bindings until erin's API is fairly stable

The input format should be:

- easy for users to type if needed since a robust UI is yet to come
- easy for programming languages to read/write


## Decision

TOML was chosen initially for its nice user-friendly syntax.
Specifically, TOML 0.5 was chosen due to its relative stability (only backward
compatible changes from 0.5 to 1.0 and inclusion of some very nice features).

However, XML, JSON, and CSV could also be options.

"Dancer" was initially thought to be in Ruby.
Ruby does not currently have a TOML 0.5 library but does have TOML 0.4 support.
Thankfully, the differences between TOML 0.5 and 0.4 are not large.
We considered Python due to TOML 0.5 support but the libraries turned out to be buggy.
See ADR 2 for more discussion.

C++ support for TOML 0.5 does seem robust so far.
Ruby's TOML 0.4 (toml-rb 1.1.2) support seems sufficient at the moment.

Although TOML support seems to be working, as a stretch goal (nice-to-have), we
will also support JSON because all languages have robust support for JSON.


## Status

Tentative


## Consequences

TOML 0.5 seems like a great choice if all of the languages we use in-house support it:

- C++ (0.5 seems very robust)
- Python (0.5 support is buggy but there)
  - unable to parse some valid forms `a = {\n\tb = 1,\nc = 2}`
  - possible potential to make bug reports and pull requests
- C# (0.5 support reported)
- Ruby (no 0.5 support at this time but 0.4 support appears robust)
- JavaScript (0.5 support reported)

We have effectively downgrade to some 0.4/0.5 hybrid to enable more language implementation support.
In practice, this seems to be working thus far.

Another possibility is to switch to a format that has rock-solid support (in
order of preference):

- [JSON](http://www.json.org/)
- [XML](https://www.w3.org/TR/2008/REC-xml-20081126/)
- CSV

[YAML](https://yaml.org/) is another possibility.
However, although I find it to be easily read by humans, it is not as easily typed.

[EDN](https://github.com/edn-format/edn) would also be an excellent format but
I would be concerned that it's support among other languages is similar to TOML
(i.e., spotty support with errors).

Note: for JSON, [this library](https://github.com/nlohmann/json) looks
excellent and Python and Ruby support is no problem.
