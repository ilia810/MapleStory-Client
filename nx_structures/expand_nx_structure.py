#!/usr/bin/env python3
import re
import sys

def expand_compressed_structure(content):
    """Expand compressed NX structure back to full tree format"""
    lines = content.strip().split('\n')
    output = []
    
    # Skip header
    start_idx = 0
    for i, line in enumerate(lines):
        if line.strip() and not line.startswith('Compressed') and not line.startswith('Generated') and not line.startswith('Note'):
            start_idx = i
            break
    
    current_img = None
    
    for line in lines[start_idx:]:
        if not line.strip():
            continue
        
        # Check if it's a top-level .img file
        if line.strip().endswith('.img'):
            current_img = line.strip()
            output.append(f"├─ {current_img} 📁")
            continue
        
        # Parse asset entries
        if line.startswith(' ├─') or line.startswith(' └─'):
            asset_match = re.match(r'\s*[├└]─\s*(.+)', line)
            if asset_match:
                asset_name = asset_match.group(1).strip()
                current_asset = asset_name
                continue
        
        # Parse states
        if 'states:' in line:
            states_match = re.match(r'\s*\│?\s*states:\{(.+?)\}', line)
            if states_match:
                states = states_match.group(1).split(',')
                continue
        
        # Parse frames or animation
        if 'frames:' in line or 'anim:' in line:
            if 'frames:' in line:
                frames_match = re.match(r'\s*\│?\s*frames:(\d+)', line)
                if frames_match:
                    frame_count = int(frames_match.group(1))
                    frame_counts = {state: frame_count for state in states}
            else:  # anim:
                anim_match = re.match(r'\s*\│?\s*anim:\{(.+?)\}', line)
                if anim_match:
                    counts = [int(x) for x in anim_match.group(1).split(',')]
                    frame_counts = {state: count for state, count in zip(states, counts)}
            
            # Generate expanded structure
            if current_img and current_asset and states:
                output.append(f"  ├─ {current_asset} 📁 ({len(states)} items)")
                for i, state in enumerate(states):
                    state_prefix = "  │ └─" if i == len(states) - 1 else "  │ ├─"
                    frame_count = frame_counts.get(state, 1)
                    output.append(f"  {state_prefix} {state} 📁 ({frame_count} items)")
                    
                    for j in range(frame_count):
                        frame_prefix = "  │   └─" if j == frame_count - 1 else "  │   ├─"
                        if i == len(states) - 1:
                            frame_prefix = frame_prefix.replace("│", " ")
                        output.append(f"  {frame_prefix} {j} 🖼️ (1 items)")
                        output.append(f"  {'      └─' if i == len(states) - 1 else '│     └─'} origin 📍")
    
    return '\n'.join(output)

def main():
    if len(sys.argv) != 2:
        print("Usage: python expand_nx_structure.py <compressed_file>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    
    try:
        with open(input_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        expanded = expand_compressed_structure(content)
        
        # Generate output filename
        output_file = input_file.replace('_compressed.txt', '_expanded.txt')
        if output_file == input_file:
            output_file = input_file.replace('.txt', '_expanded.txt')
        
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(expanded)
        
        print(f"Expanded structure saved to: {output_file}")
        
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()