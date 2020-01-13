# Run regression tests for the project.
# Assumes that `diff` is available from the command line.
# Author: Michael O'Keefe (2019-11-13)
# To install dependencies, call `bundle install` from this project's top-level
# folder.
require 'tmpdir'
begin
  require 'edn_turbo'
rescue Exception
  require 'edn'
end
require 'rubygems'

IS_WIN = Gem.win_platform?

THIS_DIR = File.expand_path(File.dirname(__FILE__))
REFERENCE_PATH = File.expand_path(File.join(THIS_DIR, '..', 'test', 'reference'))
REGRESSION_RUNS_PATH = File.join(REFERENCE_PATH, 'runs.edn')
EXECUTABLES = {}
if IS_WIN
  EXECUTABLES[:e2rin_single] = File.expand_path(
    File.join(THIS_DIR, "..", "build", "bin", "Debug", "e2rin.exe"))
  EXECUTABLES[:e2rin_multi] = File.expand_path(
    File.join(THIS_DIR, "..", "build", "bin", "Debug", "e2rin_multi.exe"))
else
  EXECUTABLES[:e2rin_single] = File.expand_path(
    File.join(THIS_DIR, "..", "build", "bin", "e2rin"))
  EXECUTABLES[:e2rin_multi] = File.expand_path(
    File.join(THIS_DIR, "..", "build", "bin", "e2rin_multi"))
end
DIFF_PROGRAM = if IS_WIN then "FC" else "diff" end

# (Map String String) -> void
# check that the path part of the tag to path map exists. Fails
# and exits with code 1 if path doesn't exist.
def check_path_map(m)
  m.each do |tag, path|
    if !File.exist?(path)
      puts "Path for '#{tag}' not found at #{path}"
      exit(1)
    end
  end
end

check_path_map(EXECUTABLES)
check_path_map({"REGRESSION_RUNS_PATH": REGRESSION_RUNS_PATH})

examples = EDN.read(File.read(REGRESSION_RUNS_PATH))
puts "There are #{examples.length} regression examples..."

$num_issues = 0
$issues = {}

# String String -> (Tuple Bool (Or String Nil))
# - diff_target: String, path to the actual output of the recently run program
# - expected_path: String, path to expected output to test regressions against
# RETURN: 2-tuple of boolean and either string or nil
# first of the 2-tuple is a boolean; if true, no differences; else false
# if first element is false, the issue is reported in the string
def run_diff(diff_target, expected_path)
  is_good = true
  issue = nil
  diff_path = 'temp.txt'
  if !File.exist?(diff_target)
    return [false, "actual file: #{diff_target} doesn't exist"]
  end
  if !File.exist?(expected_path)
    return [false, "expected file: #{expected_path} doesn't exist"]
  end
  status = system("#{DIFF_PROGRAM} #{diff_target} #{expected_path} > #{diff_path}")
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

# Stringable Hash -> Nil
# - id: Stringable, anything that has a decent string representation for a test id
# - info: Hash table, with keys:
#   - :engine: symbol, symbol corresponding to key in EXECUTABLES for path to the simulation engine
#   - :args: string, the command-line arguments for the engine run
#   - :"compare-generated-to-ref": (Hash String String), the key is the
#     local output file, the corresponding value is the reference file to diff against
def run_regression(id, info)
  engine = info.fetch(:engine)
  exe_path = EXECUTABLES.fetch(engine)
  args = info.fetch(:args)
  comparisons = info.fetch(:"compare-generated-to-ref")
  puts "Running #{id}..."
  Dir.mktmpdir do |dir|
    cmd = "#{exe_path} #{args}"
    status = system(cmd)
    if status.nil? or !status
      $issues[id] = "issue running #{exe_path}\n\t"\
        "status: #{status}\n\t"\
        "id: #{id}\n\t"\
        "args: #{args}\n\t"\
        "command: \"#{cmd}\""
      $num_issues += 1
      return
    end
    comparisons.each do |local_path, reference_path|
      full_ref_path = File.join(REFERENCE_PATH, reference_path)
      is_good, issue = run_diff(local_path, full_ref_path)
      if !is_good
        $issues[id] = issue
        $num_issues += 1
        return
      end
    end
  end
end

examples.each_with_index do |ex, idx|
  run_regression("#{idx}", ex)
end

puts("-"*60)
if $num_issues > 0
  puts "#{$num_issues} regressions found"
  $issues.keys.sort.each do |k|
    next if $issues[k].nil?
    puts "#{k}:"
    puts "  #{$issues[k]}"
  end
  exit(1)
else
  puts "no regressions found"
  exit(0)
end
