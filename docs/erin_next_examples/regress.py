import sys
import subprocess
from pathlib import Path
import platform


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
    run_cli("10")
    run_cli("11")
    run_cli("12")
    run_cli("13")
    run_cli("26")
    run_cli("27")
    smoke_test("28")
    run_cli("29")
    run_cli("30")
    print("Passed all regression tests!")
