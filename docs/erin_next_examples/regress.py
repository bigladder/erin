import sys
import subprocess
from pathlib import Path
import platform
import csv


if platform.system() == 'Windows':
    ROOT_DIR = Path('.') / '..' / '..'
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
    TEST_EXE = BIN_DIR / 'test_erin_next.exe'
    RAND_TEST_EXE = BIN_DIR / 'erin_next_random_test.exe'
    CLI_EXE = BIN_DIR / 'erin_next_cli.exe'
elif platform.system() == 'Darwin':
    BIN_DIR = Path('.') / '..' / '..' / 'build' / 'bin'
    TEST_EXE = BIN_DIR / 'test_erin_next'
    RAND_TEST_EXE = BIN_DIR / 'erin_next_random_test'
    CLI_EXE = BIN_DIR / 'erin_next_cli'
else:
    print(f"Unhandled platform, '{platform.system()}'")
    sys.exit(1)
print("Platform: " + platform.system())
print(f"BINARY DIR: {BIN_DIR}")


if not TEST_EXE.exists():
    print("Cannot find test executable...")
    sys.exit(1)


if not CLI_EXE.exists():
    print("Cannot find command-line interface executable...")
    sys.exit(1)


def run_tests():
    """
    Run the test suite
    """
    result = subprocess.run([TEST_EXE], capture_output=True)
    if result.returncode != 0:
        print("Tests did not pass!")
        print(f"stdout:\n{result.stdout}")
        print(f"stderr:\n{result.stderr}")
        sys.exit(1)
    result = subprocess.run([RAND_TEST_EXE], capture_output=True)
    if result.returncode != 0:
        print("Random tests did not pass!")
        print(f"stdout:\n{result.stdout}")
        print(f"stderr:\n{result.stderr}")
        sys.exit(1)


def smoke_test(example_name):
    """
    A smoke test just runs the example and confirms we get a 0 exit code
    """
    Path("out.csv").unlink(missing_ok=True)
    Path("stats.csv").unlink(missing_ok=True)
    toml_input = f"ex{example_name}.toml"
    result = subprocess.run([CLI_EXE, toml_input], capture_output=True)
    if result.returncode != 0:
        print(f"Error running CLI for example {example_name}")
        print(f"stdout:\n{result.stdout}")
        print(f"stderr:\n{result.stderr}")
        sys.exit(1)
    return result


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
                print(f"values differ {row_prefix}@{key}[{idx}]")
                print(f"-- original: {origx}")
                print(f"-- proposed: {propx}")


def run_cli(example_name):
    """
    Run the CLI for example name and check output diffs
    - example_name: string, "01" or "25" to call ex01.toml or ex25.toml
    """
    result = smoke_test(example_name)
    result = subprocess.run(
        ['diff', 'out.csv', f'ex{example_name}-out.csv'],
        capture_output=True)
    if result.returncode != 0:
        print(f"diff did not compare clean for out.csv for {example_name}")
        print(f"stdout:\n{result.stdout}")
        print(f"stderr:\n{result.stderr}")
        print(("=" * 20) + " DETAILED DIFF")
        compare_csv(f'ex{example_name}-out.csv', 'out.csv')
        sys.exit(1)
    result = subprocess.run(
        ['diff', 'stats.csv', f'ex{example_name}-stats.csv'],
        capture_output=True)
    if result.returncode != 0:
        print(f'diff did not compare clean for stats.csv for {example_name}')
        print(f"stdout:\n{result.stdout}")
        print(f"stderr:\n{result.stderr}")
        print(("=" * 20) + " DETAILED DIFF")
        compare_csv(f'ex{example_name}-stats.csv', 'stats.csv')
        sys.exit(1)


if __name__ == "__main__":
    run_tests()
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
    run_cli("26")
    run_cli("27")
    smoke_test("28")
    run_cli("29")
    run_cli("30")
    print("Passed all regression tests!")
