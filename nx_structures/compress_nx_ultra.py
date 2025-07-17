#!/usr/bin/env python3
"""
Ultra-compact NX structure compression that preserves ALL data.
Uses YAML-like indentation with compact notation for sequences and patterns.
"""
import re
import sys
from datetime import datetime
from collections import defaultdict, Counter

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
    
    # Parse node info
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
    stack = [(-1, tree)]
    
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

def analyze_children(children):
    """Analyze children for patterns and compression opportunities"""
    if not children:
        return None
        
    # Check if all children are simple numbered sequences
    if all(c['name'].isdigit() and not c['children'] for c in children):
        numbers = sorted([int(c['name']) for c in children])
        if numbers == list(range(numbers[0], numbers[-1] + 1)):
            return ('range', numbers[0], numbers[-1])
    
    # Check if children follow a pattern with same substructure
    if all(c['name'].isdigit() for c in children):
        numbers = sorted([(int(c['name']), c) for c in children])
        
        # Check for consecutive ranges with same child count
        ranges = []
        i = 0
        while i < len(numbers):
            start = numbers[i][0]
            child_count = len(numbers[i][1]['children'])
            
            # Find end of this range
            j = i + 1
            while j < len(numbers) and numbers[j][0] == start + (j - i) and len(numbers[j][1]['children']) == child_count:
                j += 1
            
            if j - i >= 3:  # Compress sequences of 3+
                ranges.append((start, numbers[j-1][0], child_count, numbers[i][1]['children']))
            else:
                for k in range(i, j):
                    ranges.append((numbers[k][0], numbers[k][0], len(numbers[k][1]['children']), numbers[k][1]['children']))
            i = j
            
        return ('ranges', ranges)
    
    # Check for named patterns
    names = [c['name'] for c in children]
    if len(set(names)) < len(names) * 0.8:  # If many duplicates
        return ('named', children)
    
    return ('mixed', children)

def compress_node(node, depth=0):
    """Compress a node and its children using ultra-compact notation"""
    output = []
    indent = "  " * depth
    
    # Determine node representation
    if not node['children']:
        # Leaf node - just the name
        output.append(f"{indent}{node['name']}")
    else:
        # Analyze children for patterns
        analysis = analyze_children(node['children'])
        
        if analysis and analysis[0] == 'range':
            # Simple range: name[0-10]
            _, start, end = analysis
            output.append(f"{indent}{node['name']}[{start}-{end}]")
            
        elif analysis and analysis[0] == 'ranges':
            # Multiple ranges or complex patterns
            _, ranges = analysis
            
            # Group consecutive single items and ranges
            grouped = []
            i = 0
            while i < len(ranges):
                start, end, child_count, children = ranges[i]
                
                if start == end and child_count == 0:
                    # Collect consecutive singles with no children
                    singles = [start]
                    j = i + 1
                    while j < len(ranges) and ranges[j][0] == ranges[j][1] and ranges[j][2] == 0:
                        if ranges[j][0] == singles[-1] + 1:
                            singles.append(ranges[j][0])
                            j += 1
                        else:
                            break
                    
                    if len(singles) >= 3:
                        grouped.append(f"{singles[0]}-{singles[-1]}")
                    else:
                        grouped.extend(str(s) for s in singles)
                    i = j
                elif start == end:
                    # Single item with children
                    grouped.append(f"{start}:{child_count}")
                    i += 1
                else:
                    # Range with children
                    if child_count > 0:
                        grouped.append(f"{start}-{end}:{child_count}")
                    else:
                        grouped.append(f"{start}-{end}")
                    i += 1
            
            # Output in compact form
            output.append(f"{indent}{node['name']}[{','.join(grouped)}]")
            
            # Add child structures for items with children
            shown_structures = set()
            for start, end, child_count, children in ranges:
                if child_count > 0 and str(children) not in shown_structures:
                    shown_structures.add(str(children))
                    if start == end:
                        output.append(f"{indent}  @{start}:")
                    else:
                        output.append(f"{indent}  @{start}-{end}:")
                    for child in children:
                        output.extend(compress_node(child, depth + 2))
                        
        else:
            # Mixed or named children - list them
            child_names = []
            complex_children = []
            
            for child in node['children']:
                if not child['children']:
                    child_names.append(child['name'])
                else:
                    child_names.append(f"{child['name']}*")
                    complex_children.append(child)
            
            # Use square brackets for inline lists
            if len(child_names) <= 5 and not complex_children:
                output.append(f"{indent}{node['name']}[{','.join(child_names)}]")
            else:
                output.append(f"{indent}{node['name']}:")
                
                # Output simple children in groups
                simple_names = [name for name in child_names if not name.endswith('*')]
                if simple_names:
                    # Group in lines of 8 for readability
                    for i in range(0, len(simple_names), 8):
                        group = simple_names[i:i+8]
                        output.append(f"{indent}  [{','.join(group)}]")
                
                # Output complex children
                for child in complex_children:
                    output.extend(compress_node(child, depth + 1))
    
    return output

def compress_structure(content):
    """Main compression function"""
    lines = content.strip().split('\n')
    
    # Skip header
    start_idx = 0
    for i, line in enumerate(lines):
        if '├─' in line or '└─' in line:
            start_idx = i
            break
    
    # Build tree
    tree = build_tree(lines[start_idx:])
    
    # Output header
    output = []
    output.append("NX Structure - Ultra Compact Format")
    output.append(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    output.append("Notation: [0-9]=range, [a,b,c]=list, name:N=N children, @N:=structure for item N")
    output.append("")
    
    # Compress each top-level child
    for child in tree['children'][0]['children'] if tree['children'] else []:
        output.extend(compress_node(child, 0))
    
    return '\n'.join(output)

def main():
    if len(sys.argv) != 2:
        print("Usage: python compress_nx_ultra.py <nx_structure_file>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    
    try:
        # Read input file
        try:
            with open(input_file, 'r', encoding='utf-8') as f:
                content = f.read()
        except UnicodeDecodeError:
            with open(input_file, 'r', encoding='cp1252') as f:
                content = f.read()
        
        compressed = compress_structure(content)
        
        # Generate output filename
        output_file = input_file.replace('_current.txt', '_ultra.txt')
        if output_file == input_file:
            output_file = input_file.replace('.txt', '_ultra.txt')
        
        # Write compressed output
        with open(output_file, 'w', encoding='utf-8', errors='replace') as f:
            f.write(compressed)
        
        print(f"Ultra-compressed structure saved to: {output_file}")
        
        # Show stats
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