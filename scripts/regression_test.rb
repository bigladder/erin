# Run regression tests for the project.
# Assumes that `diff` is available from the command line.
# Author: Michael O'Keefe (2019-11-13)
require 'tmpdir'
THIS_DIR = File.expand_path(File.dirname(__FILE__))
exe_path = File.expand_path(
  File.join(THIS_DIR, "..", "build", "bin", "e2rin"))
if !File.exist?(exe_path)
  puts "Executable e2rin doesn't exist"
  exit(1)
end
examples = Dir[
  File.expand_path(File.join(THIS_DIR, "..", "docs", "examples", "*.toml"))]
# puts "Examples: #{examples}"
num_issues = 0
issues = {}

def run_diff(diff_target, expected_path)
  is_good = true
  issue = nil
  diff_path = 'temp.txt'
  if !File.exist?(diff_target)
    return [false, "#{diff_target} doesn't exist"]
  end
  if !File.exist?(expected_path)
    return [false, "#{expected_path} doesn't exist"]
  end
  status = system("diff #{diff_target} #{expected_path} > #{diff_path}")
  if status.nil? or !status
    is_good = false
    the_diff = ""
    if File.exist?(diff_path)
      the_diff = File.read(diff_path)
      File.delete(diff_path)
    end
    issue = "regression in #{diff_target}:\n#{the_diff}"
  end
  [is_good, issue]
end

examples.each do |ex|
  root = File.basename(ex, '.*')
  issues[root] = nil
  expected_out = File.expand_path(
    File.join(THIS_DIR, "..", "test", "reference", "#{root}-out.csv"))
  expected_stats = File.expand_path(
    File.join(THIS_DIR, "..", "test", "reference", "#{root}-stats.csv"))
  scenario_id = File.read(File.expand_path(
    File.join(THIS_DIR, "..", "test", "reference", "#{root}-scenario-id.txt"))).strip
  puts "Running #{root}..."
  Dir.mktmpdir do |dir|
    status = system("#{exe_path} #{ex} out.csv stats.csv \"#{scenario_id}\"")
    if status.nil? or !status
      issues[root] = "Bad status: #{status}"
      num_issues += 1
      next
    end
    is_good, issue = run_diff('out.csv', expected_out)
    if !is_good
      issues[root] = issue
      num_issues += 1
      next
    end
    is_good, issue = run_diff('stats.csv', expected_stats)
    if !is_good
      issues[root] = issue
      num_issues += 1
      next
    end
  end
end

puts("-"*60)
if num_issues > 0
  puts "#{num_issues} regressions found"
  issues.keys.sort.each do |k|
    puts "#{k}:"
    puts "  #{issues[k]}"
  end
  exit(1)
else
  puts "no regressions found"
  exit(0)
end
