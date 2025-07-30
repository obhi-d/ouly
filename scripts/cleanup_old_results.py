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
    pattern = r'([^-]+)-([^-]+)-([^-]+)-([^.]+)\.(json|txt)'
    match = re.match(pattern, filename)
    
    if match:
        return {
            'compiler': match.group(1),
            'commit_hash': match.group(2),
            'build_number': int(match.group(3)),
            'test_id': match.group(4),
            'extension': match.group(5)
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
        file_info = parse_filename(file_path.name)
        if file_info and 'build_number' in file_info:
            build_num = file_info['build_number']
            if build_num not in build_files:
                build_files[build_num] = []
            build_files[build_num].append(file_path)
        else:
            unparseable_files.append(file_path)
    
    # Sort builds and determine which to keep
    sorted_builds = sorted(build_files.keys(), reverse=True)  # Newest first
    builds_to_keep = sorted_builds[:keep_count]
    builds_to_remove = sorted_builds[keep_count:]
    
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
        for file_path in unparseable_files[:10]:  # Show first 10
            print(f"  {file_path.name}")
        if len(unparseable_files) > 10:
            print(f"  ... and {len(unparseable_files) - 10} more")
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
        for file_path in result_files:
            file_info = parse_filename(file_path.name)
            if file_info and 'build_number' in file_info:
                build_num = file_info['build_number']
                if build_num not in build_files:
                    build_files[build_num] = []
                build_files[build_num].append(file_path)
        
        sorted_builds = sorted(build_files.keys(), reverse=True)
        builds_to_remove = sorted_builds[args.keep:]
        
        files_to_remove = sum(len(build_files[build]) for build in builds_to_remove)
        files_to_keep = sum(len(build_files[build]) for build in sorted_builds[:args.keep])
        
        print(f"Would remove {files_to_remove} files from {len(builds_to_remove)} old builds")
        print(f"Would keep {files_to_keep} files from {min(len(sorted_builds), args.keep)} recent builds")
        
        if builds_to_remove:
            print("Builds to remove:")
            for build in builds_to_remove:
                print(f"  Build {build}: {len(build_files[build])} files")
    else:
        removed, kept = cleanup_old_results(args.results_dir, args.keep)
        print(f"Cleanup complete: removed {removed} files, kept {kept} files")


if __name__ == '__main__':
    main()
