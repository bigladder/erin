import sys
import subprocess
from pathlib import Path


BIN_DIR = Path('.') / '..' / '..' / 'out' / 'build' / 'x64-Debug' / 'bin'
TEST_EXE = BIN_DIR / 'test_erin_next.exe'
RAND_TEST_EXE = BIN_DIR / 'erin_next_random_test.exe'
CLI_EXE = BIN_DIR / 'erin_next_cli.exe'

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


def run_cli(example_name):
    """
    Run the CLI for example name and check output diffs
    - example_name: string, "01" or "25" to call ex01.toml or ex25.toml
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
    result = subprocess.run(
        ['diff', 'out.csv', f'ex{example_name}-out.csv'],
        capture_output=True)
    if result.returncode != 0:
        print(f"diff did not compare clean for out.csv for {example_name}")
        print(f"stdout:\n{result.stdout}")
        print(f"stderr:\n{result.stderr}")
        sys.exit(1)
    result = subprocess.run(
        ['diff', 'stats.csv', f'ex{example_name}-stats.csv'],
        capture_output=True)
    if result.returncode != 0:
        print(f'diff did not compare clean for stats.csv for {example_name}')
        print(f"stdout:\n{result.stdout}")
        print(f"stderr:\n{result.stderr}")
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
    print("Passed all regression tests!")
