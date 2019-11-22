# Build the doit script such that it is up to date for all files in the repository.
# Author: Michael O'Keefe (2019-11-20)
require 'set'
require 'pathname'

CHECKS = "-*,cppcoreguidelines-*,clang-analyzer-*,modernize-*"
USE_BG = false
THIS_DIR = File.expand_path(File.dirname(__FILE__))
ROOT = File.expand_path(File.join(THIS_DIR, ".."))
ROOT_PN = Pathname.new(ROOT)
SRC_FILES = Dir.glob(File.join(ROOT, "**", "*.{h,cpp}")).reject do |f|
  f.include?("vendor") or f.include?("build")
end
puts "SRC_FILES (#{SRC_FILES.length}):\n"
lines = []
base_names_set = Set.new
bg_txt = if USE_BG then " &" else "" end
SRC_FILES.each do |f|
  puts "\t- #{f}"
  f_pn = Pathname.new(f)
  relative_path = f_pn.relative_path_from(ROOT_PN).to_path
  base_name = File.basename(f)
  if base_names_set.include?(base_name)
    n = 0
    base_stem = File.basename(f, '.*')
    base_ext = File.extname(f)
    while base_names_set.include?(base_name) do
      base_name = base_stem + "_#{n}" + base_ext
      n += 1
    end
  end
  base_names_set << base_name
  lines << "clang-tidy -p compile_commands.json --quiet --checks='#{CHECKS}' ../#{relative_path} > tidy/#{base_name}.txt#{bg_txt}"
end
prefix = File.read(File.join(THIS_DIR, "doit_prefix.txt"))
File.open(File.join(THIS_DIR, "doit"), 'w') do |f|
  f.write(prefix.strip)
  f.write("\n")
  lines.each do |line|
    f.write(line + "\n")
  end
  f.write("echo Waiting for clang-tidy background jobs to complete...\n")
  f.write("wait\n")
  f.write("let END_TIME=`date +%s`\n")
  f.write("let TIDY_DURATION=$(((END_TIME-START_TIME)/60))\n")
  f.write("echo Clang-Tidy: $((TIDY_DURATION)) minutes\n")
  f.write("echo Done!\n")
end
flag = false
Dir.chdir(THIS_DIR) do
  flag = system("chmod a+x doit")
end
puts "chmod didn't work" if flag.nil? or !flag
puts "Done!"
