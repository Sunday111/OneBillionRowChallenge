from pathlib import Path
import subprocess
import time
from typing import Tuple

SCRIPT_DIR = Path(__file__).parent.resolve()
ROOT_DIR = SCRIPT_DIR.parent.resolve()
BUILD_DIR = ROOT_DIR / "build"
BIN_DIR = BUILD_DIR / "bin"
DATA_DIR = ROOT_DIR / "data"
JAVA_DIR = ROOT_DIR / "java"
PROGRAM_PATH = BIN_DIR / "obrc"


def read_file(path: Path) -> bytes:
    with open(file=path, mode="rb") as file:
        return file.read()


def run_and_measure_use_time(suffix: str) -> float:
    obrc_path = PROGRAM_PATH.as_posix()
    file_path = (DATA_DIR / f"measurements_{suffix}.txt").as_posix()

    result = subprocess.run(
        check=True,
        args=f"/usr/bin/time -f '%e' {obrc_path} {file_path} > /dev/null",
        shell=True,
        capture_output=True,
    )

    return float(result.stderr.decode("utf-8"))


def run_and_measure(suffix: str) -> float:
    obrc_path = PROGRAM_PATH.as_posix()
    file_path = (DATA_DIR / f"measurements_{suffix}.txt").as_posix()

    start = time.time()
    result = subprocess.run(
        check=True,
        args=[obrc_path, file_path],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    return time.time() - start


def create_files_for_testing(num_lines: int, suffix: str) -> Tuple[Path, Path]:
    DATA_DIR.mkdir(exist_ok=True)

    measurements_file_path = DATA_DIR / f"measurements_{suffix}.txt"
    if measurements_file_path.exists():
        assert measurements_file_path.is_file()
    else:
        print(f"{measurements_file_path} does not exist. Generating")

        subprocess.run(
            check=True,
            args=[
                "java",
                JAVA_DIR / "CreateMeasurements.java",
                str(num_lines),
                measurements_file_path,
            ],
        )

    expected_result_file_path = DATA_DIR / f"expected_{suffix}.txt"
    if expected_result_file_path.exists():
        assert expected_result_file_path.is_file()
    else:
        print(f"{expected_result_file_path} does not exist. Generating")

        completed_process = subprocess.run(
            check=True,
            capture_output=True,
            args=[
                "java",
                JAVA_DIR / "CalculateAverage.java",
                measurements_file_path,
            ],
        )

        with open(file=expected_result_file_path, mode="wb") as expected_result_file:
            expected_result_file.write(completed_process.stdout)

    return measurements_file_path, expected_result_file_path


def run_and_compare():
    lines_and_suffixes = [
        (10000, "10k"),
        (100000, "100k"),
        (1000000, "1mil"),
        (10000000, "10mil"),
        (100000000, "100mil"),
        (1000000000, "1bil"),
    ]

    all_results_correct = True
    for num_lines, suffix in lines_and_suffixes:
        measurements_file_path, expected_result_file_path = create_files_for_testing(num_lines, suffix)

        start_time = time.time()
        finished_process = subprocess.run(
            check=True,
            args=[
                PROGRAM_PATH,
                measurements_file_path,
            ],
            capture_output=True,
        )
        end_time = time.time()

        actual_result_file_path = DATA_DIR / f"actual_{suffix}.txt"

        with open(file=actual_result_file_path, mode="wb") as actual_result_file:
            actual_result_file.write(finished_process.stdout)

        expected_str = read_file(expected_result_file_path)
        if expected_str == finished_process.stdout:
            print(f"Test for {num_lines} lines ({suffix}) ran in {end_time - start_time}s, result is valid")
        else:
            all_results_correct = False
            print(f"Test for {num_lines} lines ({suffix}) has wrong result.")
            print(f"    code --diff {expected_result_file_path} {actual_result_file_path}")

    if all_results_correct:
        print("Passed all the tests. Benchmarking")

        durations: list[float] = []
        for _ in range(50):
            durations.append(run_and_measure(suffix))

        print(f"Avg time: {sum(durations) / len(durations)}")
        print(f"Min time: {min(durations)}")
        print(f"Max time: {max(durations)}")


def main():
    run_and_compare()


if __name__ == "__main__":
    main()
