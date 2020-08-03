# Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
# See the LICENSE.txt file for additional terms and conditions.

if ARGV.length != 2
  puts "USAGE: ruby apply_header.rb <header_file.txt> <dir_root>"
  exit
end

header_file = ARGV[0]
dir_root = ARGV[1]

puts "header_file: #{header_file}"
puts "dir_root   : #{dir_root}"

header = File.read(header_file).strip.lines.map {|line| line.strip}.reject(&:empty?)

def process_file(path, header_lines, start_comment, line_comment, end_comment)
  puts "... processing #{path}"
  num_header_lines = header_lines.length
  raw = File.read(path)
  File.open(path, 'w') do |f|
    line_no = 0
    raw.each_line do |line|
      if line_no < num_header_lines
        pre = (line_no == 0) ? start_comment : line_comment
        post = (line_no == (num_header_lines - 1)) ? end_comment : ""
        f.write("#{pre}#{header_lines[line_no]}#{post}\n")
      else
        f.write(line.chomp + "\n")
      end
      line_no += 1
    end
  end
end

puts "processing *.cpp files"
Dir["#{dir_root}/**/*.cpp"].each do |path|
  process_file(path, header, "/* ", " * ", " */")
end

puts "processing *.h files"
Dir["#{dir_root}/**/*.h"].each do |path|
  process_file(path, header, "/* ", " * ", " */")
end

puts "processing *.rb files"
Dir["#{dir_root}/**/*.rb"].each do |path|
  process_file(path, header, "# ", "# ", "")
end

puts "processing CMakeLists.txt files"
Dir["#{dir_root}/**/CMakeLists.txt"].each do |path|
  process_file(path, header, "# ", "# ", "")
end

puts "Done!"
exit(0)
