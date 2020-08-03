# Building the User's Guide

This directory contains files for building the User Guide.

## Dependencies

### Ruby Interpreter

Any modern ruby interpreter should be fine.
At the time of this writing, we are using Ruby 2.7.0p0.
To install Ruby, either check your operating system's package manager or check install instructions from:

- https://www.ruby-lang.org/en/
- https://rubyinstaller.org/

### Pandoc

[Pandoc](https://pandoc.org/) is a universal document converter.
It is capable of generating HTML, LaTeX, PDF, and Microsoft Word Documents from a common source [Markdown](https://pandoc.org/MANUAL.html#pandocs-markdown) format.

Pandoc can be installed from your operating system's package manager or from here:

- https://pandoc.org/installing.html

Note: you may also need to intall a minimal LaTeX environment per the instructions above.

At the time of this writing, we are using Pandoc version 2.10.1.
You will also need a copy of [Pandoc Crossref](https://github.com/lierdakil/pandoc-crossref).

## Building the Docs

Building the docs is facilitated using Ruby's Make-clone called [Rake](https://github.com/ruby/rake) which ships with ruby.

From a command-prompt in this directory, type:

    > rake -T

To get a list of available tasks.

To build the documents, type:

    > rake build

or simply:

    > rake

which selects `build` as the default task.

Building will create a `*.docx`, `*.html`, `*.pdf`, and `*.tex` file in the current directory.
