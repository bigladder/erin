# ERIN (Energy Resilience of Interacting Networks)

ERIN is a district system energy-flow simulator.
It accounts for toplogy (what is connected to what) and disruptions.
Disruptions can be due to threat events and/or reliability-based failures.


## Status: Beta

ERIN is currently under active development.
As such, we offer no guarantees for stability at this time.
ERIN should currently be considered as beta software.


## Running Google Test

```
mkdir build
cmake -S . -B build -DERIN_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=1
cmake --build build --config Release
```

## Using Task

[Task](https://taskfile.dev/) is a cross-platform task runner and build tool.
It can be used to assist with varius development tasks.

You can install the tool from the website above.
Once installed, call `task --list` to list available tasks. 

NOTE: to install Taskfile, please follow the instructions on the website above.

## Clang Format

This project uses [Clang Format](https://clang.llvm.org/docs/ClangFormat.html).
Once installed, formatting is facilitated by using task: `task format`.
This makes the formatting step explicit.

Note: we have added a Dockerfile to enable developers to run clang format within Docker.

To run clang-format from Docker, do the following:


0. Install Taskfile if you haven't already
1. Install Docker Desktop
2. Start Docker Desktop
3. Run task docker-format

NOTE: if you prefer to not use taskfile, you can run the commands manually.
First, start Docker Desktop.
Next, open the Taskfile which is a plain text file written in YAML format.
Copy and run the docker-build task's command in your shell at the repo top directory.
When those commands have completed, run docker-format tasks's command.


## Running Tests

Task contains a convenient test task.
However, running this task requires you to have a working version of Python installed.
If you would prefer to run the tests manually, the procedure is:

- `build/bin/test_erin_next` -- call this to run the unit test suite
- regression tests:
    - run each example file in `docs/erin_next_examples` using `build/bin/erin_next_cli`
    - diff each `out.csv` and `stats.csv` against that example's expected output

The python file `docs/erin_next_examples/regress.py` does all of the above for you.

## Architectural Design Strategy

This C++ project uses a programming style inspired by:

- [Data-oriented Design](https://en.wikipedia.org/wiki/Data-oriented_design)
- [Entity-component System](https://en.wikipedia.org/wiki/Entity_component_system)

The above links will give a quick overview of the concepts.
However, they are not the end of the story.
A fairly comprehensive treatment of data-oriented design can be found here:

[Data-Oriented Design by Richard Fabien](https://www.amazon.com/dp/1916478700)

A good quote that summarizes the "spin" of data-oriented design is:

> If you don’t understand the data, you don’t understand the problem.
> Understand the problem by understanding the data.
>
> -- Mike Acton, Data Oriented Design and C++, CppCon 2014

Linus Torvalds, the creator of Linux and Git, said this once in a mailing list post:

> I'm a huge proponent of designing your code around the data, rather than the other way around.
> 
> -- Linux Torvalds

This is, in essence, what we're trying to do in ERIN.

## Developer Notes

This section is for developers of the code.
A couple of things to note:

- ERIN uses unsigned integers for flows
    - this makes determining when the network has reached quiescence easy
    - HOWEVER, care is needed in adding uint\* types to avoid wrap-around
- when developing:
    - ALWAYS run tests before committing (it is very easy to introduce a defect)
    - do write tests that exercise all new features

## Command-line Setup

A script is available to run the compiler and tests from the command prompt (Mac OS X or Linux).
To use it, you can simlink a shell script after creating your build directory:

```
mkdir build
cd build
ln -s ../scripts/doit .
./doit
```

## Directory structure

- root
    - cmake
    - docs
    - examples
    - include
    - src
    - test
    - vendor

## Version control: Git

This setup relies on the projects being managed in a Git repository.

## Build system: CMake

Manages the overall build with specific scripts for handling:

- Defaulting build type (i.e., Release)
- Uniform compiler flags (TODO)
- Identifying build target architecture (i.e., operating system, compiler, and CPU instruction architecture) (TODO)
- Automatic versioning based on Git repository tags (TODO)
- Optional static or dynamic libraries
- Handling of MSVC and Windows specific requirements (e.g., MT/MD flags, export headers) (TODO)

### Building the Project

To build the project, follow these steps:

1. check out the source code using git
2. `mkdir build`
3. `cd build`
4. All systems: `cmake -DERIN_TESTING=ON ..`
    - *OR* for Xcode on OS X: `cmake -G Xcode -DERIN_TESTING=ON ..`
5. By operating system:
    - Windows or OS X Xcode: open the solution file
    - OS X or Linux: `make -j4`
6. `ctest --output-on-failure` (or run `ERIN_TESTS` in VisualStudio or Xcode)


## Dependencies: Git submodules

Where possible, dependencies are added using git submodules. In a few exceptional cases, a dependency may be vendored directly in the root repository.

## Error handling (TODO)

Consistent approach to handling error callbacks.

## Debug Environment: Visual Studio Code

- install CodeLLDB
- TODO: copy debug configuration here

## Testing: CTest and Googletest

Unit tests are created by googletest and automatically detected by CTest and added to the test suite

## Graphviz Visualization of the Network Topology

TODO: update after erin_graph is updated.

One of the programs that builds with ERIN is `erin_graph`.
`erin_graph` can be used to generate a Graphviz input file from your input TOML file.
Although the graphviz input file can be created with no external dependencies, a Graphviz installation is required to actually create an output image.

Graphviz can be installed from [here](https://www.graphviz.org/).

The `erin_graph` program provides usage help including how to call Graphviz to generate your picture.

# Copyright and License

Copyright (c) 2020-2024 Big Ladder Software, LLC. All Rights Reserved.

See the LICENSE.txt file for the license for this project.

Original author of this source code is Michael O'Keefe, under employ of Big Ladder Software LLC.

# 3rd Party Copyright and License

The vendor folder contains code from various 3rd party vendors as listed below:

## toml11

The license for toml11 is copied below:

```
The MIT License (MIT)

Copyright (c) 2017 Toru Niina

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

## Google Test

The license for Google Test is copied below:

```
Copyright 2008, Google Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
    * Neither the name of Google Inc. nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

## CLI11

The license for CLI11 is copied below:

```
CLI11 2.2 Copyright (c) 2017-2024 University of Cincinnati, developed by Henry
Schreiner under NSF AWARD 1414736. All rights reserved.

Redistribution and use in source and binary forms of CLI11, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

## fmt

The license for fmt is copied below:

```
Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

--- Optional exception to the license ---

As an exception, if, as a result of your compiling your source code, portions
of this Software are embedded into a machine-executable object form of such
source code, you may redistribute such embedded portions in such object form
without including the above copyright and permission notices.
```

