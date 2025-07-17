#!/usr/bin/env python3
"""
Quick Asset List Generator
Extracts asset paths from existing NX structure dumps
"""

import os
import re
from collections import defaultdict

def extract_assets_from_file(filename):
    """Extract all asset paths from a structure file"""
    assets = {
        'images': [],
        'audio': [],
        'animations': [],
        'ui_elements': [],
        'all_paths': []
    }
    
    current_path = []
    
    with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()
    
    # Track indentation to build paths
    indent_stack = []
    
    for line in lines:
        # Skip empty lines and headers
        if not line.strip() or '===' in line or 'Generated' in line:
            continue
            
        # Count leading spaces
        indent = 0
        for char in line:
            if char == ' ':
                indent += 1
            else:
                break
        
        # Parse the line
        content = line.strip()
        if not content.startswith('├─') and not content.startswith('└─'):
            continue
            
        content = content[2:].strip()  # Remove tree characters
        
        # Update path based on indentation
        while indent_stack and indent <= indent_stack[-1][0]:
            indent_stack.pop()
            current_path.pop()
        
        # Extract name and type
        match = re.match(r'(.+?)(?:\s*(🖼️|🔊|📁|📝|🔢|📍))?(?:\s*\(.*?\))?', content)
        if match:
            name = match.group(1).strip()
            icon = match.group(2)
            
            indent_stack.append((indent, name))
            current_path.append(name)
            
            full_path = '/'.join(current_path)
            assets['all_paths'].append(full_path)
            
            # Categorize by type
            if icon == '🖼️':
                assets['images'].append(full_path)
            elif icon == '🔊':
                assets['audio'].append(full_path)
            
            # Detect UI elements (buttons)
            if any(part.startswith('Bt') for part in current_path):
                if name in ['normal', 'mouseOver', 'pressed', 'disabled', 'keyFocused']:
                    assets['ui_elements'].append(full_path)
            
            # Detect animations (numbered sequences)
            if name.isdigit() and len(current_path) > 1:
                parent_path = '/'.join(current_path[:-1])
                if parent_path not in assets['animations']:
                    assets['animations'].append(parent_path)
    
    return assets

def main():
    """Generate complete asset lists from all structure files"""
    
    print("Generating Complete Asset Lists...")
    print("==================================\n")
    
    all_assets = defaultdict(list)
    
    # Process each structure file
    structure_files = [f for f in os.listdir('.') if f.endswith('_current.txt')]
    
    for filename in structure_files:
        print(f"Processing {filename}...")
        nx_name = filename.replace('_current.txt', '')
        
        try:
            assets = extract_assets_from_file(filename)
            
            # Add to global lists with NX prefix
            for img in assets['images']:
                all_assets['images'].append(f"{nx_name}/{img}")
            for audio in assets['audio']:
                all_assets['audio'].append(f"{nx_name}/{audio}")
            for anim in assets['animations']:
                all_assets['animations'].append(f"{nx_name}/{anim}")
            for ui in assets['ui_elements']:
                all_assets['ui_elements'].append(f"{nx_name}/{ui}")
                
        except Exception as e:
            print(f"  Error processing {filename}: {e}")
    
    # Write complete asset registry
    with open('ASSET_PATHS_COMPLETE.txt', 'w', encoding='utf-8') as f:
        f.write("HEAVENMS ASSET PATHS - Complete List\n")
        f.write("====================================\n")
        f.write("Generated from NX structure dumps\n\n")
        
        f.write(f"[IMAGE ASSETS] ({len(all_assets['images'])} found)\n")
        f.write("-" * 50 + "\n")
        for img in sorted(all_assets['images'])[:100]:  # First 100
            f.write(f"{img}\n")
        if len(all_assets['images']) > 100:
            f.write(f"... and {len(all_assets['images']) - 100} more\n")
        
        f.write(f"\n[UI ELEMENTS] ({len(all_assets['ui_elements'])} found)\n")
        f.write("-" * 50 + "\n")
        ui_buttons = defaultdict(list)
        for ui in all_assets['ui_elements']:
            if '/Bt' in ui:
                btn_name = ui.split('/Bt')[1].split('/')[0]
                ui_buttons[btn_name].append(ui)
        
        for btn_name, paths in sorted(ui_buttons.items())[:20]:
            f.write(f"\nButton: Bt{btn_name}\n")
            for path in sorted(paths):
                f.write(f"  {path}\n")
        
        f.write(f"\n[ANIMATIONS] ({len(all_assets['animations'])} found)\n")
        f.write("-" * 50 + "\n")
        for anim in sorted(all_assets['animations'])[:50]:
            f.write(f"{anim}\n")
        
        f.write(f"\n[AUDIO ASSETS] ({len(all_assets['audio'])} found)\n")
        f.write("-" * 50 + "\n")
        for audio in sorted(all_assets['audio'])[:20]:
            f.write(f"{audio}\n")
    
    # Write key login assets
    with open('LOGIN_ASSETS.txt', 'w', encoding='utf-8') as f:
        f.write("KEY LOGIN SCREEN ASSETS\n")
        f.write("======================\n\n")
        
        f.write("Login Buttons:\n")
        for ui in sorted(all_assets['ui_elements']):
            if 'Login.img/Title/Bt' in ui:
                f.write(f"  {ui}\n")
        
        f.write("\nLogin Backgrounds:\n")
        for img in all_assets['images']:
            if 'Login.img' in img and 'backgrnd' in img.lower():
                f.write(f"  {img}\n")
        
        f.write("\nCharacter Select:\n")
        for path in all_assets['images'] + all_assets['ui_elements']:
            if 'CharSelect' in path:
                f.write(f"  {path}\n")
                
    print("\n✓ Complete!")
    print("\nGenerated files:")
    print("  - ASSET_PATHS_COMPLETE.txt (all asset paths)")
    print("  - LOGIN_ASSETS.txt (login screen specific)")
    print("\nUse these paths directly in your AssetRegistry!")

if __name__ == "__main__":
    main()