# Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
# See the LICENSE.txt file for additional terms and conditions.
# Runs a stress-test/benchmarking suite.
require 'open3'
require 'time'

THIS_DIR = File.expand_path(File.dirname(__FILE__))
IS_WIN = Gem.win_platform?
if IS_WIN
  str8760 = File.expand_path(File.join(THIS_DIR, '..', 'build', 'bin', 'Debug', 'str8760.exe'))
  str8760x5000 = File.expand_path(File.join(THIS_DIR, '..', 'build', 'bin', 'Debug', 'str8760x5000.exe'))
else
  str8760 = File.expand_path(File.join(THIS_DIR, '..', 'build', 'bin', 'str8760'))
  str8760x5000 = File.expand_path(File.join(THIS_DIR, '..', 'build', 'bin', 'str8760x5000'))
end

def run_command(cmd)
  puts("running: #{cmd}")
  start_time = Time.now
  stdout_str, stderr_str, status = Open3.capture3(cmd)
  finish_time = Time.now
  elapsed = finish_time - start_time
  puts("elapsed: #{elapsed} seconds")
  if !status.success?
    puts("NOT successful:")
    puts("stdout: #{stdout_str}")
    puts("stderr: #{stderr_str}")
    false
  else
    puts(stdout_str)
    puts(stderr_str)
    true
  end
end

flag = true
flag &&= run_command(str8760)
flag &&= run_command(str8760x5000)
if flag
  puts("overall success!")
else
  puts("issues found!")
end
puts("Done!")
