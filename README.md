# DISCO (District Infrastructure System Component Optimization)


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

## Google Test




---------

This project serves as a template for consistent, cross-platform, open-source C++ library development.

One goal is to make it as simple as possible to go from cloning to a tested build with as few steps as possible. The general process should be as simple as:

```
cmake .
cmake --build . --config Release
ctest -C Release
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

## Licensing: BSD-3

Be sure to use your own copyright for your projects using this structure.