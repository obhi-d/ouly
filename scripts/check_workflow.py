#!/usr/bin/env python3
"""
Workflow Status Checker for Ouly

This script helps validate GitHub workflow configuration and explains status badges.
"""

import yaml
import sys
import argparse
from pathlib import Path

def check_workflow_file(workflow_path: Path) -> dict:
    """Check a GitHub workflow file for configuration."""
    try:
        with open(workflow_path, 'r') as f:
            workflow = yaml.safe_load(f)
        
        results = {
            'valid': True,
            'jobs': list(workflow.get('jobs', {}).keys()),
            'issues': [],
            'recommendations': []
        }
        
        jobs = workflow.get('jobs', {})
        
        # Check for continue-on-error configuration
        for job_name, job_config in jobs.items():
            if 'macos' in str(job_config.get('runs-on', '')):
                if 'continue-on-error' not in job_config:
                    results['recommendations'].append(
                        f"Job '{job_name}' runs on macOS but lacks 'continue-on-error' configuration"
                    )
                    
        # Check for fail-fast configuration
        for job_name, job_config in jobs.items():
            strategy = job_config.get('strategy', {})
            if 'matrix' in strategy and strategy.get('fail-fast', True):
                results['recommendations'].append(
                    f"Job '{job_name}' has fail-fast=true, consider setting to false for better resilience"
                )
        
        return results
        
    except Exception as e:
        return {
            'valid': False,
            'error': str(e),
            'issues': [f"Failed to parse workflow file: {e}"],
            'recommendations': []
        }

def explain_badge_status():
    """Explain what different badge statuses mean."""
    print("🔍 GitHub Workflow Badge Status Guide")
    print("=" * 40)
    print()
    print("✅ GREEN Badge:")
    print("   - All primary jobs completed successfully")
    print("   - Performance benchmarks ran without issues")
    print("   - Core functionality is working correctly")
    print()
    print("⚠️  YELLOW/ORANGE Badge:")
    print("   - Some non-critical jobs failed (often macOS)")
    print("   - Performance benchmarks likely succeeded")
    print("   - Build verification had partial failures")
    print("   - Usually safe to ignore")
    print()
    print("❌ RED Badge:")
    print("   - Critical job failures occurred")
    print("   - Performance benchmarks may have failed")
    print("   - Requires investigation")
    print()
    print("💡 Tips:")
    print("   - Check individual job details in GitHub Actions")
    print("   - macOS failures are often expected due to compiler versions")
    print("   - Focus on Linux GCC/Clang benchmark results")

def main():
    parser = argparse.ArgumentParser(description="Check Ouly workflow configuration")
    parser.add_argument("--workflow", help="Path to workflow file", 
                       default=".github/workflows/performance.yml")
    parser.add_argument("--explain", action="store_true", 
                       help="Explain badge status meanings")
    
    args = parser.parse_args()
    
    if args.explain:
        explain_badge_status()
        return
    
    workflow_path = Path(args.workflow)
    if not workflow_path.exists():
        print(f"❌ Workflow file not found: {workflow_path}")
        sys.exit(1)
    
    print(f"🔍 Checking workflow: {workflow_path}")
    print()
    
    results = check_workflow_file(workflow_path)
    
    if results['valid']:
        print("✅ Workflow YAML is valid")
        print(f"📊 Jobs found: {', '.join(results['jobs'])}")
        print()
        
        if results['issues']:
            print("⚠️  Issues found:")
            for issue in results['issues']:
                print(f"   - {issue}")
            print()
        
        if results['recommendations']:
            print("💡 Recommendations:")
            for rec in results['recommendations']:
                print(f"   - {rec}")
            print()
        
        if not results['issues'] and not results['recommendations']:
            print("🎉 Workflow configuration looks good!")
        
    else:
        print(f"❌ Workflow validation failed: {results.get('error', 'Unknown error')}")
        for issue in results['issues']:
            print(f"   - {issue}")
        sys.exit(1)

if __name__ == "__main__":
    main()
