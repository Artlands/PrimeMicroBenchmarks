#!/usr/bin/env python3
import argparse
import os
import re
import shutil
from pathlib import Path


def normalize_freq(freq: str) -> str:
    match = re.match(r"^(\d+)\.(\d+)GHz$", freq)
    if not match:
        return freq
    left, right = match.groups()
    right = right.rstrip("0")
    if right == "":
        right = "0"
    return f"{left}.{right}GHz"


def collect_files(base_dir: Path, group: str, freq: str) -> dict:
    freq_dir = base_dir / group / freq
    out_dir = freq_dir / "out"
    prof_dir = freq_dir / "profiles"
    results = {"out": [], "profiles": []}
    if out_dir.is_dir():
        results["out"] = sorted(out_dir.glob("*.out"))
    if prof_dir.is_dir():
        results["profiles"] = sorted(prof_dir.glob("*.prof"))
    return results


def app_name_from_file(path: Path) -> str:
    return path.name.split(".", 1)[0]


def ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def copy_or_move(src: Path, dest: Path, move: bool) -> None:
    ensure_dir(dest.parent)
    if move:
        shutil.move(str(src), str(dest))
    else:
        shutil.copy2(src, dest)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Organize dvfs/energy data into apps/<app>/ folders."
    )
    parser.add_argument(
        "--freq",
        action="append",
        default=[],
        help="Frequency folder to include, e.g. 0.80GHz. Can be used multiple times.",
    )
    parser.add_argument(
        "--groups",
        nargs="+",
        default=["dvfs", "energy"],
        help="Experiment groups to include (default: dvfs energy).",
    )
    parser.add_argument(
        "--base",
        default="scripts",
        help="Base directory containing dvfs/energy folders (default: scripts).",
    )
    parser.add_argument(
        "--apps-dir",
        default="apps",
        help="Destination apps directory (default: apps).",
    )
    parser.add_argument(
        "--move",
        action="store_true",
        help="Move files instead of copying.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print actions without copying/moving.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    base_dir = Path(args.base)
    apps_dir = Path(args.apps_dir)

    if not args.freq:
        print("No --freq provided. Nothing to do.")
        return 1

    for group in args.groups:
        for freq in args.freq:
            files = collect_files(base_dir, group, freq)
            if not files["out"] and not files["profiles"]:
                print(f"Skipping {group}/{freq}: no files found.")
                continue

            label_freq = normalize_freq(freq)
            for out_file in files["out"]:
                app = app_name_from_file(out_file)
                dest = apps_dir / app / f"{app}_{group}_{label_freq}.out"
                action = "MOVE" if args.move else "COPY"
                print(f"{action} {out_file} -> {dest}")
                if not args.dry_run:
                    copy_or_move(out_file, dest, args.move)

            for prof_file in files["profiles"]:
                app = app_name_from_file(prof_file)
                dest = apps_dir / app / f"{app}_{group}_{label_freq}.prof"
                action = "MOVE" if args.move else "COPY"
                print(f"{action} {prof_file} -> {dest}")
                if not args.dry_run:
                    copy_or_move(prof_file, dest, args.move)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
