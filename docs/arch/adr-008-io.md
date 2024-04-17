# ADR 8: Writing and Logging Results

## Context

After initial benchmarking, a typical simulation is appearing to take:

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
- switch to a better input file format for time-series data
- switch to a better output file format for time-series data
- write output data in a threaded fashion
- do not perform processing on output

The last bullet above is worth mentioning a few more words on.
Currently, within the simulator, all flows are in Watts.
Similarly, time is in seconds and energy in Joules.
However, when writing to output, we convert units and round numbers.
This does help when comparing input file versions and is nice for the user.
However, is it ERIN's job to format those entries?
Or would we be better serving the user to just print them out faster?

Potential candidates for input and/or output data include:

- [Apache Parquet](https://parquet.apache.org/): a columndar storage format
- Protocol buffers

NOTE: as an important point, this is a proposal to ADD additional input/output file options.
In particular, CSV is very good for casual usage and debugging.
However, when running ERIN in a parformance-critical context, another input/output format may serve us better.

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
