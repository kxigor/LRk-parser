#!/usr/bin/env python3
import argparse
import concurrent.futures
import os
import shutil
import subprocess
import sys

def run_tidy(file_path, tidy_cmd, build_dir, config_file):
    cmd = [
        tidy_cmd,
        f"-p={build_dir}",
        f"--config-file={config_file}",
        "--quiet",
        file_path,
        "--extra-arg=-Wno-unknown-warning-option",
    ]
    result = subprocess.run(cmd, capture_output=True, text=True)
    return (result.returncode == 0, file_path, result.stdout + result.stderr)

def main():
    parser = argparse.ArgumentParser(description="Run Clang-Tidy checks")
    parser.add_argument("--tidy-exec", default="clang-tidy", help="Path to clang-tidy executable")
    parser.add_argument("--build-dir", required=True, help="Path to compile_commands.json directory")
    parser.add_argument("--config", required=True, help="Path to .clang-tidy config")
    parser.add_argument("files", nargs="+", help="List of files to check")
    args = parser.parse_args()

    if not shutil.which(args.tidy_exec):
        print(f"Error: {args.tidy_exec} not found.")
        sys.exit(1)

    compile_db = os.path.join(args.build_dir, "compile_commands.json")
    if not os.path.exists(compile_db):
        print(f"Error: compile_commands.json not found in {args.build_dir}")
        print("Did you run cmake with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON?")
        sys.exit(1)

    workers = min(4, os.cpu_count() or 1)
    print(f"Running {args.tidy_exec} on {len(args.files)} files using {workers} workers...")

    failed_files = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as executor:
        future_to_file = {
            executor.submit(run_tidy, f, args.tidy_exec, args.build_dir, args.config): f
            for f in args.files
        }
        for future in concurrent.futures.as_completed(future_to_file):
            success, filename, output = future.result()
            if not success:
                print(f"\nError in {filename}:")
                print(output.strip())
                failed_files.append(filename)

    if failed_files:
        print(f"\nFAILED: {len(failed_files)} files failed static analysis.")
        sys.exit(1)
    print("\nSUCCESS: All files passed static analysis.")

if __name__ == "__main__":
    main()
