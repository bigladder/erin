# E2RIN (Economics, Energy-use, and Resilience of Interacting Networks)

Main engine for the DISCO (District Infrastructure System Component Optimization) tool.

## Running Google Test

```
mkdir build
cd build
cmake ..
cmake --build . --config Release
ctest -C Release
```

## Command-line Setup

If you'd like to develop or kick-off the compiler and tests from the command prompt (Mac OS X or Linux), then after you create your build directory, you can simlink a shell script to facilitate quick testing of commands:

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
4. `cmake -DERIN_TESTING=ON ..`
5. By operating system:
    - Windows: open the solution file
    - OS X or Linux: `make -j4`
6. `ctest --output-on-failure` (or run `ERIN_TESTS` in VisualStudio)


## Dependencies: Git submodules

Where possible, dependencies are added using git submodules. In a few exceptional cases, a dependency may be vendored directly in the root repository.

## Error handling (TODO)

Consistent approach to handling error callbacks.

## Testing: CTest and Googletest

Unit tests are created by googletest and automatically detected by CTest and added to the test suite

## Formatting: Clang format (TODO)

## Code Coverage: Codecov (TODO)

## CI: Travis-CI (TODO)

Automatic builds, testing, packaging, and deployment.

## Script Bindings (TODO)

## Documentation: Doxygen + ReadTheDocs(?) (TODO)

# LICENSE

See the LICENSE.txt file for the license for this project.

The test example in test/checkout_line is adapted from the ADEVS manual and source code Copyright James Nutaro and released under a BSD License.
The license file for that work is in test/checkout_line/copyright.txt.
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

## Licensing: MIT

Copyright 2019 Big Ladder Software LLC

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
