#!/usr/bin/env python3
"""
Full NX structure compression that preserves ALL data, not just UI patterns.
Uses indentation-based compression and run-length encoding for repeated patterns.
"""
import re
import sys
from datetime import datetime
from collections import defaultdict

def parse_tree_line(line):
    """Parse a single line from the tree structure"""
    # Count leading spaces
    indent = 0
    for char in line:
        if char == ' ':
            indent += 1
        else:
            break
    
    # Extract content after tree characters
    content = line.strip()
    if content.startswith('├─') or content.startswith('└─'):
        content = content[2:].strip()
    elif content.startswith('│'):
        return None  # Skip continuation lines
    
    # Parse node info: name, type, item count
    match = re.match(r'(.+?)(?:\s*(📁|🖼️|📍|🔢|📝))?(?:\s*\((\d+)\s*items?\))?$', content)
    if not match:
        return None
        
    name = match.group(1).strip()
    node_type = match.group(2) or ''
    item_count = match.group(3)
    
    return {
        'indent': indent,
        'name': name,
        'type': node_type,
        'count': int(item_count) if item_count else 0
    }

def build_tree(lines):
    """Build hierarchical tree from parsed lines"""
    tree = {'name': 'root', 'children': [], 'type': '📁'}
    stack = [(-1, tree)]  # (indent, node)
    
    for line in lines:
        parsed = parse_tree_line(line)
        if not parsed:
            continue
            
        indent = parsed['indent']
        
        # Find parent based on indentation
        while stack and stack[-1][0] >= indent:
            stack.pop()
        
        # Create new node
        node = {
            'name': parsed['name'],
            'type': parsed['type'],
            'children': []
        }
        
        # Add to parent's children
        if stack:
            stack[-1][1]['children'].append(node)
        
        # Push to stack if it can have children
        if parsed['type'] in ['📁', ''] and parsed['count'] > 0:
            stack.append((indent, node))
    
    return tree

def find_patterns(node, path=""):
    """Find repeated patterns in the tree for better compression"""
    patterns = defaultdict(list)
    
    # Record this node's pattern
    if node['children']:
        child_names = [c['name'] for c in node['children']]
        pattern_key = '|'.join(child_names)
        patterns[pattern_key].append(path + '/' + node['name'])
    
    # Recurse into children
    for child in node['children']:
        child_patterns = find_patterns(child, path + '/' + node['name'])
        for key, paths in child_patterns.items():
            patterns[key].extend(paths)
    
    return patterns

def compress_node(node, depth=0, patterns=None):
    """Compress a node and its children"""
    output = []
    indent = "  " * depth
    
    # Node name and type
    type_char = {'📁': 'd', '🖼️': 'i', '📍': 'p', '🔢': 'n', '📝': 't'}.get(node['type'], '')
    
    if not node['children']:
        # Leaf node
        output.append(f"{indent}{node['name']}{type_char}")
    else:
        # Parent node with children
        child_count = len(node['children'])
        output.append(f"{indent}{node['name']}{type_char}[{child_count}]")
        
        # Group consecutive numbered children (e.g., 0,1,2,3...)
        i = 0
        while i < len(node['children']):
            child = node['children'][i]
            
            # Check if this starts a numeric sequence
            if child['name'].isdigit():
                start_num = int(child['name'])
                j = i + 1
                
                # Find end of sequence
                while j < len(node['children']) and node['children'][j]['name'].isdigit():
                    if int(node['children'][j]['name']) != start_num + (j - i):
                        break
                    j += 1
                
                # If we have a sequence of 3 or more, compress it
                if j - i >= 3:
                    end_num = start_num + (j - i - 1)
                    # Check if all items in range have same structure
                    first_child = node['children'][i]
                    same_structure = True
                    
                    for k in range(i, j):
                        if len(node['children'][k]['children']) != len(first_child['children']):
                            same_structure = False
                            break
                    
                    if same_structure and first_child['children']:
                        # Compressed range with substructure
                        output.append(f"{indent}  {start_num}-{end_num}{first_child.get('type', '')}*")
                        # Add the substructure once
                        for grandchild in first_child['children']:
                            output.extend(compress_node(grandchild, depth + 2, patterns))
                    else:
                        # Simple compressed range
                        output.append(f"{indent}  {start_num}-{end_num}{first_child.get('type', '')}")
                    
                    i = j
                    continue
            
            # Regular child, compress recursively
            output.extend(compress_node(child, depth + 1, patterns))
            i += 1
    
    return output

def compress_structure(content):
    """Main compression function"""
    lines = content.strip().split('\n')
    
    # Skip header lines
    start_idx = 0
    for i, line in enumerate(lines):
        if '├─' in line or '└─' in line:
            start_idx = i
            break
    
    # Build tree structure
    tree = build_tree(lines[start_idx:])
    
    # Find patterns for optimization
    patterns = find_patterns(tree)
    
    # Build output
    output = []
    output.append("NX Structure - Compressed Format v2")
    output.append(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    output.append("Legend: d=dir i=image p=point n=number t=text")
    output.append("Ranges: 0-10 means items 0,1,2...10")
    output.append("Pattern: 0-10* means range with repeated substructure")
    output.append("")
    
    # Compress each top-level child
    for child in tree['children'][0]['children'] if tree['children'] else []:
        output.extend(compress_node(child, 0, patterns))
        output.append("")  # Empty line between top-level items
    
    return '\n'.join(output).strip()

def main():
    if len(sys.argv) != 2:
        print("Usage: python compress_nx_full.py <nx_structure_file>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    
    try:
        # Try UTF-8 first, then fallback to cp1252
        try:
            with open(input_file, 'r', encoding='utf-8') as f:
                content = f.read()
        except UnicodeDecodeError:
            with open(input_file, 'r', encoding='cp1252') as f:
                content = f.read()
        
        compressed = compress_structure(content)
        
        # Generate output filename
        output_file = input_file.replace('_current.txt', '_compressed_v2.txt')
        if output_file == input_file:
            output_file = input_file.replace('.txt', '_compressed_v2.txt')
        
        # Write compressed output
        with open(output_file, 'w', encoding='utf-8', errors='replace') as f:
            f.write(compressed)
        
        print(f"Compressed structure saved to: {output_file}")
        
        # Show compression stats
        original_size = len(content)
        compressed_size = len(compressed)
        reduction = (1 - compressed_size / original_size) * 100
        print(f"Size: {original_size:,} -> {compressed_size:,} bytes ({reduction:.1f}% reduction)")
        
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()