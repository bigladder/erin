# Build the User's Guide using Pandoc
# Author: Michael O'Keefe, 2020-07-24
require 'rake/clean'

def make_pdf(md_path, pdf_path)
  args = [
    "--pdf-engine xelatex",
    "--number-sections",
    "-V geometry:margin=1in",
    "--filter pandoc-crossref",
    "--out #{pdf_path}",
  ]
  `pandoc #{args.join(' ')} #{md_path}`
end

def make_doc(md_path, doc_path)
  `pandoc --from=markdown --to=docx --out #{doc_path} #{md_path}`
end

MARKDOWN_FILES = FileList['./*.md']
PDF_FILES = []
DOC_FILES = []
MARKDOWN_FILES.each do |path|
  pdf_name = path + ".pdf"
  file pdf_name => [path, "rakefile.rb"] do
    make_pdf(path, pdf_name)
  end
  PDF_FILES << pdf_name
  CLEAN << pdf_name
  doc_name = path + ".docx"
  file doc_name => [path, "rakefile.rb"] do
    make_doc(path, doc_name)
  end
  DOC_FILES << doc_name
  CLEAN << doc_name
end

desc "build the user's guide using Pandoc"
task :build => (PDF_FILES + DOC_FILES) do
  puts "building the User Guide ..."
end

task :default => [:build]
