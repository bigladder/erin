# Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
# See the LICENSE.txt file for additional terms and conditions.
# Build the User's Guide using Pandoc
# Author: Michael O'Keefe, 2020-07-24
require 'rake/clean'

COMMON_ARGS = [
  "--number-sections",
  "--filter pandoc-crossref",
  "--syntax-definition=toml.xml",
  "--table-of-contents",
]

def make_pdf(md_path, pdf_path)
  args = COMMON_ARGS + [
    "--pdf-engine xelatex",
    "-V geometry:margin=1in",
  ]
  `pandoc #{args.join(' ')} --out #{pdf_path} #{md_path}`
end

def make_doc(md_path, doc_path)
  args = COMMON_ARGS + [
    "--from=markdown",
    "--to=docx",
  ]
  `pandoc #{args.join(' ')} --out #{doc_path} #{md_path}`
end

def make_tex(md_path, tex_path)
  args = COMMON_ARGS + [
    "--from=markdown",
    "--to=latex",
  ]
  `pandoc #{args.join(' ')} --out #{tex_path} #{md_path}`
end

def make_html(md_path, html_path)
  args = COMMON_ARGS + [
    "--from=markdown",
    "--to=html",
    "--standalone",
    "--mathjax",
    "--css=github-pandoc.css",
  ]
  `pandoc #{args.join(' ')} --out #{html_path} #{md_path}`
end

def add_task(path, ext, fn, files)
  name = path + ext
  file name => [path, "rakefile.rb"] do
    fn.(path, name)
  end
  files << name
  CLEAN << name
end

MARKDOWN_FILES = FileList['./*.md'].exclude("./README.md")
PDF_FILES = []
DOC_FILES = []
TEX_FILES = []
HTML_FILES = []
MARKDOWN_FILES.each do |path|
  add_task(path, ".pdf", ->(path, name) { make_pdf(path, name) }, PDF_FILES)
  add_task(path, ".docx", ->(path, name) { make_doc(path, name) }, DOC_FILES)
  add_task(path, ".tex", ->(path, name) { make_tex(path, name) }, TEX_FILES)
  add_task(path, ".html", ->(path, name) { make_html(path, name) }, HTML_FILES)
end

desc "build the user's guide using Pandoc"
task :build => (PDF_FILES + DOC_FILES + TEX_FILES + HTML_FILES) do
  puts "building the User Guide ..."
end

task :default => [:build]
