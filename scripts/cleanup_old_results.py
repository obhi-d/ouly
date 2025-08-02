#!/usr/bin/env python3
"""
Cleanup script to keep only the last 20 performance results.
Removes older results based on build number to maintain perfo branch size.
"""

import argparse
import json
import re
from pathlib import Path
from typing import Dict, List, Tuple


def parse_filename(filename: str) -> Dict[str, str]:
    """Parse benchmark filename to extract metadata."""
    # Pattern for files like: scheduler_comparison_gcc-4.2_20250729_000346.json
    # or extended-benchmark-results-ubuntu-latest-gcc-14-12345.json
    
    # Try GitHub Actions artifact pattern first: extended-benchmark-results-OS-COMPILER-BUILDNUM.ext
    github_pattern = r'extended-benchmark-results-([^-]+)-([^-]+)-([^-]+)-([^-]+)-(\d+)\.(json|txt)'
    match = re.match(github_pattern, filename)
    if match:
        return {
            'os': match.group(1),
            'os_version': match.group(2),
            'compiler': match.group(3),
            'compiler_version': match.group(4),
            'build_number': int(match.group(5)),
            'extension': match.group(6)
        }
    
    # Try scheduler comparison pattern: scheduler_comparison_COMPILER_YYYYMMDD_HHMMSS.ext
    scheduler_pattern = r'scheduler_comparison_([^_]+)_(\d{8})_(\d{6})\.(json|txt)'
    match = re.match(scheduler_pattern, filename)
    if match:
        timestamp = f"{match.group(2)}{match.group(3)}"  # YYYYMMDDHHMMSS
        return {
            'test_type': 'scheduler_comparison',
            'compiler': match.group(1),
            'date': match.group(2),
            'time': match.group(3),
            'build_number': int(timestamp),  # Use timestamp as sortable build number
            'extension': match.group(4)
        }
    
    # Fallback pattern for other potential formats: compiler-commit-buildnumber-testid.ext
    fallback_pattern = r'([^-]+)-([^-]+)-(\d+)-([^.]+)\.(json|txt)'
    match = re.match(fallback_pattern, filename)
    if match:
        return {
            'compiler': match.group(1),
            'commit_hash': match.group(2),
            'build_number': int(match.group(3)),
            'test_id': match.group(4),
            'extension': match.group(5)
        }
    
    # Even more fallback: try to extract any number sequence as a timestamp
    timestamp_pattern = r'.*?(\d{8,14}).*?\.(json|txt)'
    match = re.match(timestamp_pattern, filename)
    if match:
        timestamp = match.group(1)
        # If it looks like YYYYMMDD or YYYYMMDDHHMMSS, use it
        if len(timestamp) >= 8:
            return {
                'filename': filename,
                'build_number': int(timestamp),
                'extension': match.group(2)
            }
    
    return {}


def cleanup_old_results(results_dir: Path, keep_count: int = 20) -> Tuple[int, int]:
    """Clean up old performance results, keeping only the latest N builds.
    
    Returns:
        Tuple of (files_removed, files_kept)
    """
    
    if not results_dir.exists():
        print(f"Results directory {results_dir} does not exist")
        return 0, 0
    
    # Find all result files
    result_files = []
    for pattern in ['*.json', '*.txt']:
        result_files.extend(results_dir.rglob(pattern))
    
    # Parse and group by build number
    build_files = {}
    unparseable_files = []
    
    for file_path in result_files:
        try:
            file_info = parse_filename(file_path.name)
            if file_info and 'build_number' in file_info:
                build_num = file_info['build_number']
                if build_num not in build_files:
                    build_files[build_num] = []
                build_files[build_num].append(file_path)
            else:
                unparseable_files.append(file_path)
        except Exception as e:
            print(f"Error parsing {file_path.name}: {e}")
            unparseable_files.append(file_path)
    
    # Sort builds and determine which to keep
    if not build_files:
        print("No parseable build files found for cleanup")
        return 0, len(unparseable_files)
    
    sorted_builds = sorted(build_files.keys(), reverse=True)  # Newest first
    builds_to_keep = sorted_builds[:keep_count]
    builds_to_remove = sorted_builds[keep_count:]
    
    print(f"Found {len(sorted_builds)} builds: {sorted_builds[:5]}..." if len(sorted_builds) > 5 else f"Found builds: {sorted_builds}")
    print(f"Keeping {len(builds_to_keep)} builds, removing {len(builds_to_remove)} builds")
    
    # Remove old files
    files_removed = 0
    files_kept = 0
    
    for build_num in builds_to_remove:
        for file_path in build_files[build_num]:
            try:
                file_path.unlink()
                files_removed += 1
                print(f"Removed: {file_path.name}")
            except Exception as e:
                print(f"Error removing {file_path}: {e}")
    
    for build_num in builds_to_keep:
        files_kept += len(build_files[build_num])
    
    # Report unparseable files but don't remove them
    if unparseable_files:
        print(f"Found {len(unparseable_files)} files with unexpected naming format (keeping):")
        # Only show examples if there aren't too many
        show_count = min(5, len(unparseable_files))
        for file_path in unparseable_files[:show_count]:
            print(f"  {file_path.name}")
        if len(unparseable_files) > show_count:
            print(f"  ... and {len(unparseable_files) - show_count} more")
        files_kept += len(unparseable_files)
    
    return files_removed, files_kept


def main():
    parser = argparse.ArgumentParser(description='Clean up old performance results')
    parser.add_argument('results_dir', type=Path, help='Directory containing benchmark results')
    parser.add_argument('--keep', type=int, default=20, help='Number of latest builds to keep (default: 20)')
    parser.add_argument('--dry-run', action='store_true', help='Show what would be removed without actually removing')
    
    args = parser.parse_args()
    
    if args.dry_run:
        print("DRY RUN - No files will be actually removed")
    
    print(f"Cleaning up results in {args.results_dir}")
    print(f"Keeping latest {args.keep} builds")
    
    if args.dry_run:
        # For dry run, just count and report
        result_files = []
        for pattern in ['*.json', '*.txt']:
            result_files.extend(args.results_dir.rglob(pattern))
        
        build_files = {}
        unparseable_files = []
        for file_path in result_files:
            try:
                file_info = parse_filename(file_path.name)
                if file_info and 'build_number' in file_info:
                    build_num = file_info['build_number']
                    if build_num not in build_files:
                        build_files[build_num] = []
                    build_files[build_num].append(file_path)
                else:
                    unparseable_files.append(file_path)
            except Exception as e:
                print(f"Error parsing {file_path.name}: {e}")
                unparseable_files.append(file_path)
        
        sorted_builds = sorted(build_files.keys(), reverse=True) if build_files else []
        builds_to_remove = sorted_builds[args.keep:] if len(sorted_builds) > args.keep else []
        
        if not build_files:
            print(f"No parseable build files found. Found {len(unparseable_files)} unparseable files.")
            return
        
        files_to_remove = sum(len(build_files[build]) for build in builds_to_remove)
        files_to_keep = sum(len(build_files[build]) for build in sorted_builds[:args.keep])
        
        print(f"Would remove {files_to_remove} files from {len(builds_to_remove)} old builds")
        print(f"Would keep {files_to_keep} files from {min(len(sorted_builds), args.keep)} recent builds")
        print(f"Found {len(unparseable_files)} unparseable files (would be kept)")
        
        if builds_to_remove:
            print("Builds to remove:")
            for build in builds_to_remove:
                print(f"  Build {build}: {len(build_files[build])} files")
    else:
        removed, kept = cleanup_old_results(args.results_dir, args.keep)
        print(f"Cleanup complete: removed {removed} files, kept {kept} files")


if __name__ == '__main__':
    main()
