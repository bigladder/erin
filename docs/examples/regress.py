import sys
import os
import subprocess
from pathlib import Path
import platform
import csv
import time
import datetime


print("Platform: " + platform.system())
if platform.system() == 'Windows':
    from shutil import which
    DIFF_PROG = 'fc' if which('fc') is not None else 'diff'
    ROOT_DIR = (Path('.') / '..' / '..').absolute()
    BIN_DIR = ROOT_DIR / 'build' / 'bin' / 'Release'
    if not BIN_DIR.exists():
        BIN_DIR = ROOT_DIR / 'build' / 'bin' / 'Debug'
    if not BIN_DIR.exists():
        BIN_DIR = ROOT_DIR / 'out' / 'build' / 'x64-Debug' / 'bin'
    if not BIN_DIR.exists():
        BIN_DIR = ROOT_DIR / 'out' / 'build' / 'x64-Release' / 'bin'
    if not BIN_DIR.exists():
        print("Could not find build directory!")
        sys.exit(1)
    TEST_EXE = BIN_DIR / 'erin_tests.exe'
    RAND_TEST_EXE = BIN_DIR / 'erin_next_random_tests.exe'
    LOOKUP_TABLE_TEST_EXE = BIN_DIR / 'erin_lookup_table_tests.exe'
    CLI_EXE = BIN_DIR / 'erin.exe'
    PERF01_EXE = BIN_DIR / 'erin_next_stress_test.exe'
elif platform.system() == 'Darwin' or platform.system() == 'Linux':
    DIFF_PROG = 'diff'
    BIN_DIR = (Path('.') / '..' / '..' / 'build' / 'bin').absolute()
    TEST_EXE = BIN_DIR / 'erin_tests'
    RAND_TEST_EXE = BIN_DIR / 'erin_next_random_tests'
    LOOKUP_TABLE_TEST_EXE = BIN_DIR / 'erin_lookup_table_tests'
    CLI_EXE = BIN_DIR / 'erin'
    PERF01_EXE = BIN_DIR / 'erin_next_stress_test'
else:
    print(f"Unhandled platform, '{platform.system()}'")
    sys.exit(1)
BIN_DIR = BIN_DIR.resolve()
print(f"BINARY DIR: {BIN_DIR}")
ALL_TESTS = [TEST_EXE, RAND_TEST_EXE, LOOKUP_TABLE_TEST_EXE]


if not TEST_EXE.exists():
    print("Cannot find test executable...")
    sys.exit(1)

if not RAND_TEST_EXE.exists():
    print("Cannot find random test executable...")
    sys.exit(1)

if not LOOKUP_TABLE_TEST_EXE.exists():
    print("Cannot find lookup-table test executable...")
    sys.exit(1)

if not CLI_EXE.exists():
    print("Cannot find command-line interface executable...")
    sys.exit(1)


def run_tests():
    """
    Run the test suite
    """
    for test in ALL_TESTS:
        result = subprocess.run([test], capture_output=True)
        if result.returncode != 0:
            stem = Path(test).stem
            print(f"Test '{stem}' did not pass!")
            print("stdout:\n")
            print(result.stderr.decode())
            print("stdout:\n")
            print(result.stderr.decode())
            sys.exit(1)
        else:
            print(".", end="", sep="", flush=True)
    print("", flush=True)


def smoke_test(example_name, dir=None, timeit=False, print_it=False):
    """
    A smoke test just runs the example and confirms we get a 0 exit code
    """
    cwd = str(Path.cwd().resolve())
    test_dir = cwd if dir is None else os.path.join(cwd, dir)
    out_filename = os.path.join(test_dir, 'out.csv');
    stats_filename = os.path.join(test_dir, 'stats.csv');

    Path(out_filename).unlink(missing_ok=True)
    Path(stats_filename).unlink(missing_ok=True)
    
    toml_input = f"ex{example_name}.toml"
    in_filename = os.path.join(test_dir, toml_input);
    start_time = time.perf_counter_ns()
    result = subprocess.run([CLI_EXE, "run", f"{in_filename}", "-e", f"{out_filename}", "-s", f"{stats_filename}"], capture_output=True, cwd=test_dir)
    end_time = time.perf_counter_ns()
    time_ns = end_time - start_time
    if result.returncode != 0:
        print(f"Error running CLI for example {example_name}")
        print("stdout:\n")
        print(result.stdout.decode())
        print("stderr:\n")
        print(result.stderr.decode())
        sys.exit(1)
    result.time_ns = time_ns
    if print_it:
        print(".", end="", sep="")
    return result


def run_bench(bench_file, example_name, dir=None):
    """
    Run a benchmark test.    
    """
    git_sha = "<no-git-sha-detected>"
    git_sha_result = subprocess.run(["git", "rev-parse", "HEAD"], capture_output=True)
    prog_info = subprocess.run([CLI_EXE, "version"], capture_output=True, text=True)
    is_debug = "Build Type: Debug" in prog_info.stdout
    if git_sha_result.returncode == 0:
        git_sha = git_sha_result.stdout.decode("utf-8").strip()
    now = datetime.datetime.now().isoformat()
    result = smoke_test(example_name, dir)
    with open(bench_file, 'a') as f:
        time_s = result.time_ns / 1_000_000_000.0
        debug_flag = "true" if is_debug else "false"
        print(f"{{\"time\": \"{now}\", \"commit\": \"{git_sha}\", \"debug\": {debug_flag}, \"name\": \"{example_name}\", \"time-s\": {time_s}}}", file=f)


def read_csv(path):
    header = None
    data = {}
    with open(path, newline='') as csvfile:
        rdr = csv.reader(csvfile)
        for row in rdr:
            if header is None:
                header = row
                for col in header:
                    data[col] = []
                continue
            for item, col in zip(row, header):
                data[col].append(item)
    return {"header": header, "data": data}


def compare_csv(original_csv_path, proposed_csv_path):
    """
    Compare CSV files    
    """
    print(f"original: {original_csv_path}")
    print(f"proposed: {proposed_csv_path}")
    orig = read_csv(original_csv_path)
    prop = read_csv(proposed_csv_path)
    # compare headers
    if len(orig["header"]) != len(prop["header"]):
        print("header lengths NOT equal")
        print(f"-- len(original['header']): {len(orig['header'])}")
        print(f"-- len(proposed['header']): {len(prop['header'])}")
    orig_set = set(orig['header'])
    prop_set = set(prop['header'])
    if orig_set != prop_set:
        in_orig_only = orig_set.difference(prop_set)
        in_prop_only = prop_set.difference(orig_set)
        print("in original but not proposed: ")
        print(f"- {','.join(sorted(in_orig_only))}")
        print("in proposed but not original: ")
        print(f"- {','.join(sorted(in_prop_only))}")
    col0_key = orig['header'][0] if len(orig['header']) > 0 else ""
    col1_key = orig['header'][1] if len(orig['header']) > 1 else ""
    def prefix_for(idx):
        items = []
        if col0_key in orig['data'] and col0_key in prop['data']:
            test_a = len(orig['data'][col0_key]) > idx
            test_b = len(prop['data'][col0_key]) > idx
            if test_a and test_b:
                vorig = orig['data'][col0_key][idx]
                vprop = prop['data'][col0_key][idx]
                if vorig == vprop:
                    items.append(vorig)
                else:
                    return ""
            else:
                return ""
        else:
            return ""
        if col1_key in orig['data'] and col1_key in prop['data']:
            test_a = len(orig['data'][col1_key]) > idx
            test_b = len(prop['data'][col1_key]) > idx
            if test_a and test_b:
                vorig = orig['data'][col1_key][idx]
                vprop = prop['data'][col1_key][idx]
                if vorig == vprop:
                    items.append(vorig)
                else:
                    return ""
            else:
                return ""
        else:
            return ""
        return ":".join(items)
    for key in orig["header"]:
        if key not in prop["data"]:
            continue
        orig_xs = orig["data"][key]
        prop_xs = prop["data"][key]
        if len(orig_xs) != len(prop_xs):
            print(f"Length of entries differs for {key}")
            print(f"-- original: {len(orig_xs)}")
            print(f"-- proposed: {len(prop_xs)}")
        for idx, items in enumerate(zip(orig_xs, prop_xs)):
            row_prefix = prefix_for(idx)
            origx, propx = items
            if origx != propx:
                print(f"values differ @ row={idx}; {row_prefix}@{key}")
                print(f"-- original: {origx}")
                print(f"-- proposed: {propx}")


def run_cli(example_name):
    """
    Run the CLI for example name and check output diffs
    - example_name: string, "01" or "25" to call ex01.toml or ex25.toml
    """
    _ = smoke_test(example_name)
    result = subprocess.run(
        [DIFF_PROG, 'out.csv', f'ex{example_name}-out.csv'],
        capture_output=True)
    if result.returncode != 0:
        print(f"diff did not compare clean for out.csv for {example_name}")
        print("stdout:\n")
        print(result.stdout.decode())
        print("stderr:\n")
        print(result.stderr.decode())
        print(("=" * 20) + " DETAILED DIFF")
        compare_csv(f'ex{example_name}-out.csv', 'out.csv')
        sys.exit(1)
    result = subprocess.run(
        ['diff', 'stats.csv', f'ex{example_name}-stats.csv'],
        capture_output=True)
    if result.returncode != 0:
        print(f'diff did not compare clean for stats.csv for {example_name}')
        print("stdout:\n")
        print(result.stdout.decode())
        print("stderr:\n")
        print(result.stderr.decode())
        print(("=" * 20) + " DETAILED DIFF")
        compare_csv(f'ex{example_name}-stats.csv', 'stats.csv')
        sys.exit(1)
    print(".", end="", sep="", flush=True)


def run_cli_test(example_name, dir=None, timeit=False, print_it=False):
    """
    Run the CLI for example name and check output diffs
    - example_name: string, "sim01" to test sim01.toml
    """

    _ = smoke_test(example_name, dir, timeit, print_it)

    cwd = str(Path.cwd().resolve())
    test_dir = cwd if dir is None else os.path.join(cwd, dir)
    
    ref_out = os.path.join(test_dir, f'ex{example_name}-out.csv');
    ref_stats = os.path.join(test_dir, f'ex{example_name}-stats.csv');
    out = os.path.join(test_dir, 'out.csv');
    stats = os.path.join(test_dir, 'stats.csv');
    result = subprocess.run(
        [DIFF_PROG, out, ref_out],
        capture_output=True)
    if result.returncode != 0:
        print(f"diff did not compare clean for out.csv for {example_name}")
        print("stdout:\n")
        print(result.stdout.decode())
        print("stderr:\n")
        print(result.stderr.decode())
        print(("=" * 20) + " DETAILED DIFF")
        compare_csv(ref_out, out)
        sys.exit(1)
    result = subprocess.run(
        ['diff', stats, ref_stats],
        capture_output=True)
    if result.returncode != 0:
        print(f'diff did not compare clean for stats.csv for {example_name}')
        print("stdout:\n")
        print(result.stdout.decode())
        print("stderr:\n")
        print(result.stderr.decode())
        print(("=" * 20) + " DETAILED DIFF")
        compare_csv(ref_stats, stats)
        sys.exit(1)
    print(".", end="", sep="", flush=True)

def run_perf():
    """
    Run performance tests and report timing
    """
    result = subprocess.run(
        [PERF01_EXE],
        capture_output=True)
    if result.returncode != 0:
        print(f"error running performance test {PERF01_EXE}")
        print("stdout:\n")
        print(result.stdout.decode())
        print("stderr:\n")
        print(result.stderr.decode())
    print(result.stdout.decode(), end='')


if __name__ == "__main__":
    run_tests()
    print("Passed all unit tests!")
    run_cli("01")
    run_cli("02")
    run_cli("03")
    run_cli("04")
    run_cli("05")
    run_cli("06")
    run_cli("07")
    run_cli("08")
    run_cli("09")
    run_cli("10")
    run_cli("11")
    run_cli("12")
    run_cli("13")
    run_cli("14")
    run_cli("15")
    run_cli("16")
    run_cli("17")
    run_cli("18")
    run_cli("19")
    run_cli("20")
    run_cli("21")
    run_cli("22")
    run_cli("23")
    run_cli("24")
    run_cli("25")
    run_cli("26")
    run_cli("27")
    smoke_test("28", print_it=True)
    run_cli("29")
    run_cli("30")
    run_cli("31")
    run_cli("32")
    #smoke_test("ft-illinois", dir="ft-illinois", print_it=True)
    run_cli_test("ft-illinois_packed", dir="ft-illinois_packed", print_it=True)
    print("\nPassed all regression tests!")
    run_perf()
    print("All performance tests run")
    bench_dir = (Path(".")/".."/".."/"benchmark").resolve()
    if not bench_dir.exists():
        bench_dir.mkdir(parents=True, exist_ok=True)
    benchfile = (bench_dir / "benchmark.txt").resolve()
    run_bench(benchfile, "ft-illinois", dir="ft-illinois")
    print("Ran all benchmarks")
