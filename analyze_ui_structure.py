import json
import os
from pathlib import Path

def analyze_ui_file(filepath):
    """Analyze a single UI JSON file and extract its main components."""
    with open(filepath, 'r', encoding='utf-8') as f:
        data = json.load(f)
    
    components = {}
    
    for key, value in data.items():
        if isinstance(value, dict) and '_dirType' in value:
            # This is a UI component
            component_type = value.get('_dirType', 'unknown')
            subcomponents = []
            
            # Look for meaningful subcomponents
            for subkey, subvalue in value.items():
                if isinstance(subvalue, dict) and not subkey.startswith('_'):
                    subcomponents.append(subkey)
            
            components[key] = {
                'type': component_type,
                'subcomponents': subcomponents[:10]  # Limit to first 10 for readability
            }
    
    return components

def create_task_list():
    """Create a comprehensive task list for all UI files."""
    ui_dir = Path("C:/HeavenClient/ui-json/UI.wz")
    tasks = []
    
    for ui_file in ui_dir.glob("*.json"):
        if ui_file.name == "UIWindow.img.json":
            # We already know this one has many components
            continue
            
        try:
            components = analyze_ui_file(ui_file)
            
            # Create a task for this UI file
            task = {
                'file': ui_file.name,
                'components': components,
                'task_description': f"Analyze and fix {ui_file.stem} UI components"
            }
            tasks.append(task)
            
        except Exception as e:
            print(f"Error processing {ui_file.name}: {e}")
    
    return tasks

# Analyze UIWindow.img.json separately since it's the most complex
def analyze_uiwindow():
    """Specifically analyze UIWindow.img.json components."""
    filepath = Path("C:/HeavenClient/ui-json/UI.wz/UIWindow.img.json")
    
    with open(filepath, 'r', encoding='utf-8') as f:
        data = json.load(f)
    
    # Group components by their purpose
    window_components = {
        'Common UI Elements': [],
        'Inventory Windows': [],
        'Character Windows': [],
        'Quest/NPC Windows': [],
        'Shop Windows': [],
        'Party/Guild Windows': [],
        'Other Windows': []
    }
    
    for key in data.keys():
        if key.startswith('_'):
            continue
            
        # Categorize based on key name
        key_lower = key.lower()
        
        if any(x in key_lower for x in ['item', 'equip', 'inventory', 'consume']):
            window_components['Inventory Windows'].append(key)
        elif any(x in key_lower for x in ['stat', 'skill', 'character', 'keyconfig']):
            window_components['Character Windows'].append(key)
        elif any(x in key_lower for x in ['quest', 'npc', 'talk']):
            window_components['Quest/NPC Windows'].append(key)
        elif any(x in key_lower for x in ['shop', 'store', 'merchant']):
            window_components['Shop Windows'].append(key)
        elif any(x in key_lower for x in ['party', 'guild', 'friend']):
            window_components['Party/Guild Windows'].append(key)
        elif any(x in key_lower for x in ['btui', 'cursor', 'basic']):
            window_components['Common UI Elements'].append(key)
        else:
            window_components['Other Windows'].append(key)
    
    return window_components

# Generate the analysis
print("=== UI File Analysis ===\n")

# First analyze UIWindow.img.json
print("UIWindow.img.json Components:")
print("-" * 50)
window_components = analyze_uiwindow()

for category, components in window_components.items():
    if components:
        print(f"\n{category}:")
        for comp in sorted(components)[:20]:  # Limit output
            print(f"  - {comp}")

print("\n\n=== Other UI Files ===")
print("-" * 50)

# Analyze other files
tasks = create_task_list()
for task in tasks:
    print(f"\n{task['file']}:")
    if task['components']:
        for comp_name, comp_info in list(task['components'].items())[:10]:
            print(f"  - {comp_name} ({comp_info['type']})")
            if comp_info['subcomponents']:
                print(f"    Subcomponents: {', '.join(comp_info['subcomponents'][:5])}")