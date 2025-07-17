#!/usr/bin/env python3
import re
import sys
from datetime import datetime
from collections import defaultdict

def parse_nx_structure(content):
    """Parse NX structure file and extract hierarchy"""
    lines = content.strip().split('\n')
    
    # Skip header lines
    start_idx = 0
    for i, line in enumerate(lines):
        if '├─' in line or '└─' in line:
            start_idx = i
            break
    
    hierarchy = {}
    current_path = []
    indent_levels = []
    
    for line in lines[start_idx:]:
        # Skip empty lines
        if not line.strip():
            continue
            
        # Count leading spaces before tree characters
        spaces = 0
        for char in line:
            if char == ' ':
                spaces += 1
            else:
                break
                
        # Extract content after tree characters
        content = line.strip()
        if content.startswith('├─') or content.startswith('└─'):
            content = content[2:].strip()
        elif content.startswith('│'):
            continue  # Skip continuation lines
        else:
            continue
            
        # Parse the content
        match = re.match(r'(.+?)(?:\s*📁|\s*🖼️|\s*📍|\s*🔢|\s*📝)?(?:\s*\((\d+)\s*items?\))?$', content)
        if not match:
            continue
            
        name = match.group(1).strip()
        
        # Calculate depth based on spaces (each level is typically 2-4 spaces)
        depth = spaces // 2
        
        # Update current path based on depth
        while len(current_path) > depth:
            current_path.pop()
            
        current_path.append(name)
        
        # Store in hierarchy
        hierarchy['/'.join(current_path)] = True
    
    return hierarchy

def detect_patterns(hierarchy):
    """Detect button patterns and animations"""
    patterns = defaultdict(lambda: {'states': set(), 'frames': defaultdict(int)})
    
    for path in hierarchy:
        parts = path.split('/')
        
        # Look for button patterns (e.g., Basic.img/BtCancel/normal/0)
        if len(parts) >= 3:
            parent = parts[-3]
            state = parts[-2]
            frame = parts[-1]
            
            # Common UI states
            ui_states = {'normal', 'disabled', 'pressed', 'mouseOver', 'keyFocused', 'selected'}
            
            if state in ui_states and frame.isdigit():
                asset_path = '/'.join(parts[:-2])
                patterns[asset_path]['states'].add(state)
                patterns[asset_path]['frames'][state] = max(patterns[asset_path]['frames'][state], int(frame) + 1)
    
    return patterns

def compress_structure(content):
    """Convert NX structure to compressed format"""
    hierarchy = parse_nx_structure(content)
    patterns = detect_patterns(hierarchy)
    
    
    # Build compressed output
    output = []
    output.append(f"Compressed NX Structure")
    output.append(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    output.append(f"Note: Use states & frames to expand back to original structure")
    output.append("")
    
    # Process top-level images
    processed = set()
    img_files = set()
    
    # Find all .img files
    for path in hierarchy.keys():
        parts = path.split('/')
        for i, part in enumerate(parts):
            if part.endswith('.img'):
                img_files.add(part)
                break
    
    for img_file in sorted(img_files):
        output.append(img_file)
            
        # Find all assets under this image
        img_patterns = {}
        for pattern_path, pattern_data in patterns.items():
            if img_file in pattern_path:
                # Extract asset name after the img file
                parts = pattern_path.split('/')
                for i, part in enumerate(parts):
                    if part == img_file and i + 1 < len(parts):
                        asset_name = parts[i + 1]
                        if asset_name not in img_patterns:
                            img_patterns[asset_name] = pattern_data
                        break
        
        # Output compressed assets
        for asset_name in sorted(img_patterns.keys()):
            data = img_patterns[asset_name]
            if data['states']:
                states = sorted(data['states'])
                frames = data['frames']
                
                output.append(f" ├─ {asset_name}")
                output.append(f" │   states:{{{','.join(states)}}}")
                
                # Check if all states have same frame count
                frame_counts = [frames.get(s, 1) for s in states]
                if len(set(frame_counts)) == 1:
                    output.append(f" │   frames:{frame_counts[0]}")
                else:
                    output.append(f" │   anim:{{{','.join(str(frames.get(s, 1)) for s in states)}}}")
    
    return '\n'.join(output)

def main():
    if len(sys.argv) != 2:
        print("Usage: python compress_nx_structure.py <nx_structure_file>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    
    try:
        # Try UTF-8 first
        try:
            with open(input_file, 'r', encoding='utf-8') as f:
                content = f.read()
        except UnicodeDecodeError:
            # Fallback to cp1252 (Windows default)
            with open(input_file, 'r', encoding='cp1252') as f:
                content = f.read()
        
        compressed = compress_structure(content)
        
        # Generate output filename
        output_file = input_file.replace('_current.txt', '_compressed.txt')
        if output_file == input_file:
            output_file = input_file.replace('.txt', '_compressed.txt')
        
        # Write with UTF-8 encoding, handling any special characters
        with open(output_file, 'w', encoding='utf-8', errors='replace') as f:
            f.write(compressed)
        
        print(f"Compressed structure saved to: {output_file}")
        
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()