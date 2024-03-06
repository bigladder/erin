# ERIN (Energy Resilience of Interacting Networks)

ERIN is a district system energy-flow simulator.
It accounts for toplogy (what is connected to what) and disruptions.
Disruptions can be due to threat events and/or reliability-based failures.

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

## Clang Format

This project uses [Clang Format](https://clang.llvm.org/docs/ClangFormat.html).
Once installed, formatting is facilitated by using task: `task format`.
This makes the formatting step explicit.

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

One of the programs that builds with ERIN is `erin_graph`.
`erin_graph` can be used to generate a Graphviz input file from your input TOML file.
Although the graphviz input file can be created with no external dependencies, a Graphviz installation is required to actually create an output image.

Graphviz can be installed from [here](https://www.graphviz.org/).

The `erin_graph` program provides usage help including how to call Graphviz to generate your picture.

# Copyright and License

Copyright (c) 2020-2024 Big Ladder Software, LLC. All Rights Reserved.

See the LICENSE.txt file for the license for this project.

Original author of this source code is Michael O'Keefe, under employ of Big Ladder Software LLC.

## Checkout Line Tests

The test example in `test/checkout_line` is adapted from the ADEVS manual and source code Copyright James Nutaro and released under a BSD License.
The license file for that work is in `test/checkout_line/copyright.txt`.
The example has been slightly modified from the original to remove the need for file IO for specifying the problem and reading results.
In addition, port numbers have been eliminated in preference to using the adevs::SimpleDigraph network model.
Output to standard out from the Clerk model has also been turned off by default.
Additionally, minor changes have been made to upgrade to the latest adevs (i.e., bdevs) and to indicate internal variables using a leading underscore.

The vendor folder contains code from various 3rd party vendors as listed below:

## BDEVS

The license for BDEVS is copied below:

```
Copyright (c) 2013, James Nutaro
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met: 

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies, 
either expressed or implied, of the FreeBSD Project.

Bugs, comments, and questions can be sent to nutaro@gmail.com
```

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
