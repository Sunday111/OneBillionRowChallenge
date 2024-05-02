from pathlib import Path
import subprocess
import time

SCRIPT_DIR = Path(__file__).parent.resolve()
BUILD_DIR = SCRIPT_DIR / "build"
BIN_DIR = BUILD_DIR / "bin"
DATA_DIR = SCRIPT_DIR / "data"


def read_file(path: Path) -> str:
    with open(file=path, mode="r", encoding="utf-8") as file:
        return file.read()


def run_and_measure_use_time(suffix: str) -> float:
    obrc_path = (BIN_DIR / "obrc").as_posix()
    file_path = (DATA_DIR / f"measurements_{suffix}.txt").as_posix()

    result = subprocess.run(
        check=True,
        args=f"/usr/bin/time -f '%e' {obrc_path} {file_path} > /dev/null",
        shell=True,
        capture_output=True,
    )

    return float(result.stderr.decode("utf-8"))


def run_and_measure(suffix: str) -> float:
    obrc_path = (BIN_DIR / "obrc").as_posix()
    file_path = (DATA_DIR / f"measurements_{suffix}.txt").as_posix()

    start = time.time()
    result = subprocess.run(
        check=True,
        args=[obrc_path, file_path],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    return time.time() - start


def run_and_compare(suffix: str):
    start_time = time.time()
    finished_process = subprocess.run(
        check=True,
        args=[
            BIN_DIR / "obrc",
            DATA_DIR / f"measurements_{suffix}.txt",
        ],
        capture_output=True,
    )
    end_time = time.time()

    result_str = finished_process.stdout.decode("utf-8")
    expected_str = read_file(DATA_DIR / f"baseline_{suffix}.txt")

    if expected_str == result_str:
        print(f"Test ran in {end_time - start_time}s, result is valid")
    else:
        raise RuntimeError("Wrong results!")

    durations: list[float] = []
    for _ in range(5):
        durations.append(run_and_measure(suffix))

    print(f"Avg time: {sum(durations) / len(durations)}")
    print(f"Min time: {min(durations)}")
    print(f"Max time: {max(durations)}")


def main():
    run_and_compare("1bil")


if __name__ == "__main__":
    main()
