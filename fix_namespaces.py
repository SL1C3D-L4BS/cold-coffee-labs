#!/usr/bin/env python3
"""
Script to fix nested namespace syntax in C++ headers.
Converts 'namespace gw::core {' to traditional namespace declarations.
"""

import os
import re
from pathlib import Path

def fix_namespaces_in_file(file_path):
    """Fix nested namespace syntax in a single file."""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        original_content = content
        
        # Pattern to match nested namespace declarations like 'namespace gw::core {'
        # and 'namespace gw {'
        namespace_pattern = r'namespace\s+(gw(?:::\w+)*)\s*\{'
        
        def replace_namespace(match):
            namespace_parts = match.group(1).split('::')
            result = '\n'.join(f'namespace {part} {{' for part in namespace_parts)
            return result
        
        # Replace opening namespace declarations
        content = re.sub(namespace_pattern, replace_namespace, content)
        
        # Replace closing namespace declarations like '}  // namespace gw::core'
        # and '}  // namespace gw'
        closing_pattern = r'}\s*//\s*namespace\s+(gw(?:::\w+)*)'
        
        def replace_closing(match):
            namespace_parts = match.group(1).split('::')
            result = '\n'.join(f'}}  // namespace {part}' for part in reversed(namespace_parts))
            return result
        
        # Replace closing namespace declarations
        content = re.sub(closing_pattern, replace_closing, content)
        
        # Write back if changed
        if content != original_content:
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(content)
            print(f"Fixed namespaces in: {file_path}")
            return True
        return False
        
    except Exception as e:
        print(f"Error processing {file_path}: {e}")
        return False

def main():
    """Fix namespaces in all C++ files in the engine directory."""
    engine_dir = Path("engine")
    
    if not engine_dir.exists():
        print("Engine directory not found!")
        return
    
    fixed_count = 0
    
    # Find all .hpp and .cpp files in the engine directory
    for cpp_file in engine_dir.rglob("*.hpp"):
        if fix_namespaces_in_file(cpp_file):
            fixed_count += 1
    
    for cpp_file in engine_dir.rglob("*.cpp"):
        if fix_namespaces_in_file(cpp_file):
            fixed_count += 1
    
    print(f"\nFixed namespaces in {fixed_count} files.")

if __name__ == "__main__":
    main()
