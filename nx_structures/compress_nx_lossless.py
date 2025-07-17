#!/usr/bin/env python3
"""
Lossless NX structure compression that preserves the COMPLETE hierarchy.
Uses JSON-like notation with pattern detection for repeated structures.
"""
import re
import sys
import json
from datetime import datetime
from collections import defaultdict, OrderedDict

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
    else:
        return None
    
    # Parse node info
    match = re.match(r'(.+?)(?:\s*(📁|🖼️|📍|🔢|📝))?(?:\s*\((\d+)\s*items?\))?(?:\s*=\s*(.+))?$', content)
    if not match:
        return None
        
    name = match.group(1).strip()
    node_type = match.group(2) or ''
    item_count = match.group(3)
    value = match.group(4)
    
    return {
        'indent': indent,
        'name': name,
        'type': node_type,
        'count': int(item_count) if item_count else 0,
        'value': value
    }

def build_tree(lines):
    """Build complete hierarchical tree from parsed lines"""
    tree = {'name': 'root', 'children': OrderedDict(), 'type': '📁'}
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
            'children': OrderedDict()
        }
        
        # Add value if present
        if parsed['value']:
            node['value'] = parsed['value']
        
        # Add to parent's children
        if stack:
            parent = stack[-1][1]
            parent['children'][parsed['name']] = node
        
        # Push to stack if it can have children
        if parsed['count'] > 0:
            stack.append((indent, node))
    
    return tree

def find_patterns(node, patterns=None):
    """Find repeated substructures for compression"""
    if patterns is None:
        patterns = defaultdict(list)
    
    # Create a signature for this node's structure
    if node['children']:
        # Sort children for consistent comparison
        child_sig = []
        for name, child in node['children'].items():
            if child['children']:
                child_sig.append(f"{name}:{len(child['children'])}")
            else:
                child_sig.append(name)
        
        signature = '|'.join(child_sig)
        patterns[signature].append(node)
    
    # Recurse into children
    for child in node['children'].values():
        find_patterns(child, patterns)
    
    return patterns

def compress_node(node, depth=0, pattern_refs=None):
    """Compress a node preserving full hierarchy"""
    if pattern_refs is None:
        pattern_refs = {}
    
    # Check if this is a numeric sequence
    if node['children']:
        child_names = list(node['children'].keys())
        
        # Check for pure numeric sequence
        if all(name.isdigit() for name in child_names):
            numbers = sorted([int(name) for name in child_names])
            
            # Group into ranges with same substructure
            ranges = []
            i = 0
            while i < len(numbers):
                start = numbers[i]
                
                # Get reference child structure
                ref_child = node['children'][str(start)]
                ref_structure = json.dumps(compress_node(ref_child, depth + 1, pattern_refs), sort_keys=True)
                
                # Find end of range with same structure
                j = i + 1
                while j < len(numbers):
                    if numbers[j] != start + (j - i):
                        break
                    
                    curr_child = node['children'][str(numbers[j])]
                    curr_structure = json.dumps(compress_node(curr_child, depth + 1, pattern_refs), sort_keys=True)
                    
                    if curr_structure != ref_structure:
                        break
                    j += 1
                
                end = numbers[j-1]
                
                if j - i >= 3 and ref_child['children']:  # Compress sequences of 3+ with children
                    # Store the substructure
                    sub_struct = compress_node(ref_child, depth + 1, pattern_refs)
                    ranges.append({
                        'range': f"{start}-{end}",
                        'struct': sub_struct
                    })
                elif j - i >= 5 and not ref_child['children']:  # Compress longer sequences without children
                    ranges.append({'range': f"{start}-{end}"})
                else:
                    # Add individual items
                    for k in range(i, j):
                        child = node['children'][str(numbers[k])]
                        if child['children']:
                            ranges.append({str(numbers[k]): compress_node(child, depth + 1, pattern_refs)})
                        else:
                            ranges.append(str(numbers[k]))
                
                i = j
            
            # Return compressed format
            if len(ranges) == 1 and isinstance(ranges[0], dict) and 'range' in ranges[0]:
                # Single range
                if 'struct' in ranges[0]:
                    return {node['name']: {'_range': ranges[0]['range'], '_struct': ranges[0]['struct']}}
                else:
                    return {node['name']: ranges[0]['range']}
            else:
                return {node['name']: ranges}
        else:
            # Non-numeric children
            compressed_children = OrderedDict()
            
            for child_name, child in node['children'].items():
                if child['children']:
                    compressed_children[child_name] = compress_node(child, depth + 1, pattern_refs)
                elif 'value' in child:
                    compressed_children[child_name] = child['value']
                else:
                    compressed_children[child_name] = None
            
            return compressed_children
    else:
        # Leaf node
        if 'value' in node:
            return node['value']
        else:
            return None

def compress_structure(content):
    """Main compression function"""
    lines = content.strip().split('\n')
    
    # Skip header
    start_idx = 0
    for i, line in enumerate(lines):
        if '├─' in line or '└─' in line:
            start_idx = i
            break
    
    # Build complete tree
    tree = build_tree(lines[start_idx:])
    
    # Find patterns
    patterns = find_patterns(tree)
    
    # Compress the tree
    compressed = OrderedDict()
    
    # Process root's children (skip the root container itself)
    if tree['children'] and len(tree['children']) == 1:
        # Skip the empty root container
        root_child = list(tree['children'].values())[0]
        for name, child in root_child['children'].items():
            if child['children']:
                compressed[name] = compress_node(child)
            else:
                compressed[name] = None
    
    # Build output
    output = {
        'format': 'NX Structure - Lossless Compressed v3',
        'generated': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
        'notation': {
            'range': '"0-10" = items 0,1,2...10',
            '_struct': 'substructure for range items',
            'null': 'empty leaf node'
        },
        'data': compressed
    }
    
    return json.dumps(output, indent=2, ensure_ascii=False)

def main():
    if len(sys.argv) != 2:
        print("Usage: python compress_nx_lossless.py <nx_structure_file>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    
    try:
        # Read input
        try:
            with open(input_file, 'r', encoding='utf-8') as f:
                content = f.read()
        except UnicodeDecodeError:
            with open(input_file, 'r', encoding='cp1252') as f:
                content = f.read()
        
        compressed = compress_structure(content)
        
        # Generate output filename
        output_file = input_file.replace('_current.txt', '_lossless.json')
        if output_file == input_file:
            output_file = input_file.replace('.txt', '_lossless.json')
        
        # Write compressed output
        with open(output_file, 'w', encoding='utf-8', errors='replace') as f:
            f.write(compressed)
        
        print(f"Lossless compressed structure saved to: {output_file}")
        
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