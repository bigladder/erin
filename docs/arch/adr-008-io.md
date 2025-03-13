# ADR 8: Reading, Writing, and Logging

## Context

This ADR is heavily performance oriented although there are other considerations as well.

After initial profiling, a typical simulation is appearing to take:

- 1/3 of the time in actual simulation
- 1/3 of the time reading in data
- 1/3 of the time writing out data

These are results as of April 17, 2024.

We have committed to exploring the integration of a custom Logger.
This will enable writing during simulation to be configured.
For example, we can turn all logging off or only show warnings/errors.

For writing files, we are currently using a CSV format.
We use a custom CSV writer.
Writing is being conducted using std::cout or ofilestream.

Note: we investigated our benchmark toml file in more detail.
We discovered that there are many CSVs being read by a single TOML file.
Each of those reads happens one-at-a-time in series while processing the TOML inputs.
Each file has a single time/flow trace.
One idea that may provide for a huge speedup is to place all traces into a single CSV file.
For example, the existing header could be changed from this:

```
<filename: 12_ae_hall__steam.csv>
hours,kW
0,272.3
1,274.3
2,274.3
...
```

to

```
12_ae_hall__steam,8760,21_field_house__electricity,8760
hours,kW,hours,kW
0,272.3,0,204.6
1,274.3,1,204.6
2,274.3,2,227.4
...
```

That is, the header could be extended to store a tag and the size.
The size would be nice to allow us to reserve the correct amount of space when reading in.
The corresponding hours and flow amounts could then appear below.
ERIN could read in this entire file and then just pull out loads as necessary.
This would require engine rework to add a multi_profile_csv option but could be a quick way to gain some speedups.

We would like to explore reducing this overhead for reading / writing.

For reading, 5.06 s out of 5.85 s (86%) of input processing is CSV reading.
For writing, the vast majority of time is spent in `double->string` processing.
This is 6.93 s out of 9.01 s (77%) of the time.
For simulation, it appears that the majority of time is spent doing simulation.

In this current configuration, logging of debug messages is turned off.
We do need the ability to control the logging of messages better.

The message categories of interest are:

- debug: useful for debugging
  - would like to allow logger settings from the command line for this
  - however, extreme detail could be behind a compiler flag
- info: messages such as percent complete; and what stage of writing
- warning: warning messages; could be turned off
- error messages: must be shown

Note that logging is separate from output file writing.
However, they may use similar techniques.

Possible avenues to explore:

- enhance csv reading/writing and/or switch to a better library for it
- add support for additional input file format for time-series data that is more performant
- add support for an additional output file format for time-series data that is more performant
- read input (input CSVs and/or a new format) in a threaded fashion
- write output data (output CSVs and/or a new format) in a threaded fashion
- do not perform processing on output (i.e., no rounding or dropping of unnecessary zeros)
- revisit the verbosity of the output file; for example, write "x" vs "available" in outputs

The last bullet above is worth mentioning a few more words on.
It seems that the rounding of results and dropping of zeros (i.e., 3.400 => 3.4 and 15.000 => 15) is a performance bottleneck.
This is nice for the casual user to have these formatting procedures.
Howerver, adding a flag to elide these formatting niceties for "performance writing" may be in order.

Potential candidates for input and/or output data include:

- [Apache Parquet](https://parquet.apache.org/): a columndar storage format
- Protocol buffers

NOTE: as an important point, this is a proposal to ADD additional input/output file options.
In particular, CSV is very good for casual usage and debugging.
However, when running ERIN in a performance-critical context, another input/output format may serve us better.

The Apache Parquet format is particularly interesting because it has:

- a complementary in-memory format called [Apache Arrow](https://arrow.apache.org/)
- has implementations (readers/writers) in Python, C++, Java, and C#
  - [Parquet for C++](https://github.com/apache/arrow/tree/main/cpp/tools/parquet)
  - [ParquetSharp for C#](https://github.com/G-Research/ParquetSharp)
  - [Parquet for Java](https://github.com/apache/parquet-mr/)
  - [PyArrow for Python](https://arrow.apache.org/docs/python/index.html)
- is small and fast to read and should be faster to write than CSV
- parquet does have mechanisms to interact with protobuf
  - see [this link](https://github.com/rdblue/parquet-avro-protobuf/blob/master/README.md)

The Arrow in-memory format could allow input to be directly read into memory.
That would greatly reduce the processing time.

## Decision

We will:

- discuss with team to get broader input
  - investigate protocol buffers vs parquet
- enable the use of an (optional) threaded writer for output files
- evaluate csv read/write performance
  - by updating our code
  - and/or looking at other libraries
- evaluate and try parquet as an optional format for read/write
- investigate use of the Arrow in-memory format
- investigate the possibility of using native internal units for output
  - also investigate output without rounding

## Status

Proposed

## Consequences

The biggest consequences are:

- (+) being able to reduce simulation time by addressing reading/writing/logging
- (+) being able to reduce data transfer by reducing payload sizes for input/output
- (+) potentially lack of full knowledge of outcomesallocation of resources.
- (-) some avenues could lead to a dead end 
- (-) some avenues may be staff-resource intensive
