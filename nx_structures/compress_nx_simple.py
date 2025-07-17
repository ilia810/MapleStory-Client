#!/usr/bin/env python3
"""
Simple but effective NX structure compression.
Preserves item counts and uses pattern recognition for ranges.
"""
import re
import sys
from datetime import datetime

def parse_line(line):
    """Parse a single line extracting all info"""
    # Count leading spaces
    indent = len(line) - len(line.lstrip())
    
    # Extract content
    content = line.strip()
    if content.startswith('├─') or content.startswith('└─'):
        content = content[2:].strip()
    elif content.startswith('│'):
        return None
    else:
        return None
    
    # Parse: name type (N items) = value
    match = re.match(r'(.+?)(?:\s*(📁|🖼️|📍|🔢|📝))?(?:\s*\((\d+)\s*items?\))?(?:\s*=\s*(.+))?$', content)
    if not match:
        return None
    
    return {
        'indent': indent,
        'name': match.group(1).strip(),
        'type': match.group(2) or '',
        'count': int(match.group(3)) if match.group(3) else 0,
        'value': match.group(4)
    }

def compress_structure(lines):
    """Compress the structure preserving all information"""
    output = []
    i = 0
    
    while i < len(lines):
        parsed = parse_line(lines[i])
        if not parsed:
            i += 1
            continue
        
        indent = parsed['indent']
        name = parsed['name']
        count = parsed['count']
        value = parsed['value']
        
        # Look ahead to see if children follow a pattern
        if count > 0:
            # Collect all immediate children
            children = []
            j = i + 1
            child_indent = None
            
            while j < len(lines):
                child = parse_line(lines[j])
                if not child:
                    j += 1
                    continue
                
                if child_indent is None:
                    child_indent = child['indent']
                
                if child['indent'] < child_indent:
                    break  # End of children
                elif child['indent'] == child_indent:
                    children.append(child)
                
                j += 1
            
            # Analyze children for patterns
            if all(c['name'].isdigit() for c in children):
                # Numeric sequence - check for ranges
                numbers = sorted([(int(c['name']), c['count']) for c in children])
                ranges = []
                
                k = 0
                while k < len(numbers):
                    start_num, start_count = numbers[k]
                    
                    # Find consecutive numbers with same item count
                    m = k + 1
                    while m < len(numbers) and numbers[m][0] == start_num + (m - k) and numbers[m][1] == start_count:
                        m += 1
                    
                    end_num = numbers[m-1][0]
                    
                    if m - k >= 3:  # Compress sequences of 3+
                        if start_count > 0:
                            ranges.append(f"{start_num}-{end_num}[{start_count}]")
                        else:
                            ranges.append(f"{start_num}-{end_num}")
                    else:
                        # Individual items
                        for n in range(k, m):
                            if numbers[n][1] > 0:
                                ranges.append(f"{numbers[n][0]}[{numbers[n][1]}]")
                            else:
                                ranges.append(str(numbers[n][0]))
                    
                    k = m
                
                # Output compressed format
                indent_str = " " * (indent // 2)
                if len(ranges) <= 8:
                    output.append(f"{indent_str}{name}[{count}]: {{{','.join(ranges)}}}")
                else:
                    # Break into multiple lines for readability
                    output.append(f"{indent_str}{name}[{count}]:")
                    for n in range(0, len(ranges), 10):
                        batch = ranges[n:n+10]
                        output.append(f"{indent_str}  {{{','.join(batch)}}}")
            else:
                # Non-numeric children or mixed
                indent_str = " " * (indent // 2)
                child_names = []
                
                for c in children:
                    if c['count'] > 0:
                        child_names.append(f"{c['name']}[{c['count']}]")
                    elif c['value']:
                        child_names.append(f"{c['name']}={c['value']}")
                    else:
                        child_names.append(c['name'])
                
                if len(child_names) <= 5:
                    output.append(f"{indent_str}{name}[{count}]: {{{','.join(child_names)}}}")
                else:
                    output.append(f"{indent_str}{name}[{count}]:")
                    for n in range(0, len(child_names), 8):
                        batch = child_names[n:n+8]
                        output.append(f"{indent_str}  {{{','.join(batch)}}}")
            
            # Skip the children we just processed
            i = j
        else:
            # Leaf node
            indent_str = " " * (indent // 2)
            if value:
                output.append(f"{indent_str}{name} = {value}")
            else:
                output.append(f"{indent_str}{name}")
            i += 1
    
    return output

def main():
    if len(sys.argv) != 2:
        print("Usage: python compress_nx_simple.py <nx_structure_file>")
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
        
        lines = content.strip().split('\n')
        
        # Skip header to find start of tree
        start_idx = 0
        for i, line in enumerate(lines):
            if '├─' in line or '└─' in line:
                start_idx = i
                break
        
        # Compress
        compressed_lines = compress_structure(lines[start_idx:])
        
        # Build output
        output = []
        output.append("NX Structure - Simple Compressed Format")
        output.append(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        output.append("Notation: name[N]=N items, {0-9}=range, {a,b}=list")
        output.append("")
        output.extend(compressed_lines)
        
        compressed = '\n'.join(output)
        
        # Generate output filename
        output_file = input_file.replace('_current.txt', '_simple.txt')
        if output_file == input_file:
            output_file = input_file.replace('.txt', '_simple.txt')
        
        # Write output
        with open(output_file, 'w', encoding='utf-8', errors='replace') as f:
            f.write(compressed)
        
        print(f"Simple compressed structure saved to: {output_file}")
        
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