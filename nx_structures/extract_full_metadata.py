#!/usr/bin/env python3
"""
NX Full Metadata Extractor
Extracts complete metadata from NX files including:
- Full asset paths
- Image dimensions and origins
- Animation frame counts and delays
- Z-indices and layering info
- All properties for AssetRegistry
"""

import os
import json
import struct
import zlib
from datetime import datetime
from collections import defaultdict, OrderedDict
from typing import Dict, List, Any, Optional, Tuple

class NXNode:
    """Represents a node in the NX file structure"""
    def __init__(self, name: str, node_type: str, data: Any = None):
        self.name = name
        self.type = node_type
        self.data = data
        self.children = OrderedDict()
        self.properties = {}
    
    def add_child(self, child: 'NXNode'):
        self.children[child.name] = child
    
    def to_dict(self) -> Dict[str, Any]:
        result = {
            'name': self.name,
            'type': self.type,
            'path': self.get_path()
        }
        
        if self.data is not None:
            result['data'] = self.data
            
        if self.properties:
            result['properties'] = self.properties
            
        if self.children:
            result['children'] = [child.to_dict() for child in self.children.values()]
            
        return result
    
    def get_path(self, path_parts: List[str] = None) -> str:
        if path_parts is None:
            path_parts = []
        path_parts.insert(0, self.name)
        return '/'.join(path_parts)

class NXMetadataExtractor:
    """Extracts metadata from NX file structures"""
    
    def __init__(self):
        self.asset_registry = defaultdict(dict)
        self.patterns = defaultdict(list)
        
    def extract_from_tree(self, root: NXNode, max_depth: int = 20) -> Dict[str, Any]:
        """Extract all metadata from an NX tree structure"""
        metadata = {
            'format': 'NX Complete Metadata v2.0',
            'generated': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
            'assets': {},
            'patterns': {},
            'statistics': {}
        }
        
        # Extract all assets recursively
        self._extract_node(root, "", metadata['assets'], 0, max_depth)
        
        # Analyze patterns
        metadata['patterns'] = self._analyze_patterns(metadata['assets'])
        
        # Generate statistics
        metadata['statistics'] = self._generate_statistics(metadata['assets'])
        
        return metadata
    
    def _extract_node(self, node: NXNode, path: str, assets: Dict, depth: int, max_depth: int):
        """Recursively extract metadata from a node"""
        if depth > max_depth:
            return
            
        current_path = f"{path}/{node.name}" if path else node.name
        
        # Create asset entry
        asset_info = {
            'type': node.type,
            'name': node.name,
            'path': current_path
        }
        
        # Add type-specific metadata
        if node.type == 'bitmap':
            asset_info.update(self._extract_bitmap_metadata(node))
        elif node.type == 'audio':
            asset_info.update(self._extract_audio_metadata(node))
        elif node.type == 'string':
            asset_info['value'] = node.data
        elif node.type in ['integer', 'real']:
            asset_info['value'] = node.data
        elif node.type == 'vector':
            asset_info['x'] = node.data[0] if isinstance(node.data, (list, tuple)) else node.data.get('x', 0)
            asset_info['y'] = node.data[1] if isinstance(node.data, (list, tuple)) else node.data.get('y', 0)
            
        # Extract properties from siblings (for animation frames)
        if node.name.isdigit() and node.parent:
            asset_info.update(self._extract_frame_properties(node))
            
        # Add to assets
        assets[current_path] = asset_info
        
        # Process children
        if node.children:
            asset_info['children'] = {}
            for child in node.children.values():
                self._extract_node(child, current_path, assets, depth + 1, max_depth)
                
    def _extract_bitmap_metadata(self, node: NXNode) -> Dict[str, Any]:
        """Extract bitmap-specific metadata"""
        metadata = {'asset_type': 'image'}
        
        # Get dimensions if available
        if hasattr(node.data, 'width'):
            metadata['width'] = node.data.width
            metadata['height'] = node.data.height
        elif isinstance(node.data, dict):
            metadata['width'] = node.data.get('width', 0)
            metadata['height'] = node.data.get('height', 0)
            
        # Check for origin property
        if 'origin' in node.children:
            origin = node.children['origin']
            if origin.type == 'vector':
                metadata['origin'] = {
                    'x': origin.data[0] if isinstance(origin.data, (list, tuple)) else origin.data.get('x', 0),
                    'y': origin.data[1] if isinstance(origin.data, (list, tuple)) else origin.data.get('y', 0)
                }
                
        # Check for other common properties
        for prop in ['delay', 'z', 'a0', 'a1']:
            if prop in node.children:
                metadata[prop] = node.children[prop].data
                
        return metadata
    
    def _extract_audio_metadata(self, node: NXNode) -> Dict[str, Any]:
        """Extract audio-specific metadata"""
        metadata = {'asset_type': 'audio'}
        
        if hasattr(node.data, 'length'):
            metadata['length'] = node.data.length
        elif isinstance(node.data, dict):
            metadata['length'] = node.data.get('length', 0)
            
        return metadata
    
    def _extract_frame_properties(self, node: NXNode) -> Dict[str, Any]:
        """Extract properties for animation frames"""
        properties = {}
        
        # Common frame properties
        property_names = ['origin', 'delay', 'z', 'a0', 'a1', 'move', 'rotate', 'action']
        
        for prop_name in property_names:
            if prop_name in node.children:
                prop_node = node.children[prop_name]
                if prop_node.type == 'vector':
                    properties[prop_name] = {
                        'x': prop_node.data[0] if isinstance(prop_node.data, (list, tuple)) else prop_node.data.get('x', 0),
                        'y': prop_node.data[1] if isinstance(prop_node.data, (list, tuple)) else prop_node.data.get('y', 0)
                    }
                else:
                    properties[prop_name] = prop_node.data
                    
        return properties
    
    def _analyze_patterns(self, assets: Dict[str, Any]) -> Dict[str, Any]:
        """Analyze assets for common patterns"""
        patterns = {
            'ui_elements': defaultdict(list),
            'animations': defaultdict(list),
            'sequences': []
        }
        
        for path, asset in assets.items():
            path_parts = path.split('/')
            
            # UI Button patterns
            if any('Basic.img' in part for part in path_parts):
                for part in path_parts:
                    if part.startswith('Bt'):
                        patterns['ui_elements']['buttons'].append({
                            'name': part,
                            'path': path,
                            'states': self._find_button_states(assets, path)
                        })
                        
            # Animation patterns
            if asset.get('type') == 'directory' and asset.get('children'):
                children = list(asset.get('children', {}).keys())
                if children and all(name.isdigit() for name in children[:5]):  # Check first 5
                    patterns['animations'][path] = {
                        'frame_count': len(children),
                        'frames': children
                    }
                    
        return dict(patterns)
    
    def _find_button_states(self, assets: Dict[str, Any], button_path: str) -> List[str]:
        """Find all states for a button"""
        states = []
        
        # Common button states
        common_states = ['normal', 'mouseOver', 'pressed', 'disabled', 'keyFocused', 'selected']
        
        for state in common_states:
            state_path = f"{button_path}/{state}"
            if state_path in assets:
                states.append(state)
                
        return states
    
    def _generate_statistics(self, assets: Dict[str, Any]) -> Dict[str, Any]:
        """Generate statistics about the assets"""
        stats = {
            'total_assets': len(assets),
            'by_type': defaultdict(int),
            'by_category': defaultdict(int),
            'image_assets': 0,
            'audio_assets': 0,
            'animations': 0
        }
        
        for path, asset in assets.items():
            asset_type = asset.get('type', 'unknown')
            stats['by_type'][asset_type] += 1
            
            if asset.get('asset_type') == 'image':
                stats['image_assets'] += 1
            elif asset.get('asset_type') == 'audio':
                stats['audio_assets'] += 1
                
            # Categorize by top-level path
            top_level = path.split('/')[0] if '/' in path else path
            stats['by_category'][top_level] += 1
            
        return dict(stats)
    
    def generate_asset_registry(self, metadata: Dict[str, Any]) -> Dict[str, Any]:
        """Generate AssetRegistry-compatible format"""
        registry = {
            'version': '1.0',
            'generated': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
            'assets': {},
            'lookup': defaultdict(list)
        }
        
        # Process all assets
        for path, asset in metadata.get('assets', {}).items():
            if asset.get('type') in ['bitmap', 'audio']:
                # Create registry entry
                entry = {
                    'path': path,
                    'type': asset.get('asset_type', asset.get('type')),
                    'properties': {}
                }
                
                # Add relevant properties
                for key in ['width', 'height', 'origin', 'delay', 'z', 'length']:
                    if key in asset:
                        entry['properties'][key] = asset[key]
                        
                registry['assets'][path] = entry
                
                # Create lookup entries
                parts = path.split('/')
                for i in range(len(parts)):
                    lookup_key = '/'.join(parts[:i+1])
                    registry['lookup'][lookup_key].append(path)
                    
        return dict(registry)

def parse_nx_dump(file_path: str) -> NXNode:
    """Parse an NX dump file and build tree structure"""
    root = NXNode('root', 'directory')
    
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()
        
    # Skip header
    start_idx = 0
    for i, line in enumerate(lines):
        if '├─' in line or '└─' in line:
            start_idx = i
            break
            
    # Build tree
    node_stack = [(root, -1)]
    
    for line in lines[start_idx:]:
        # Count indent
        indent = 0
        for char in line:
            if char == ' ':
                indent += 1
            else:
                break
                
        # Parse line
        content = line.strip()
        if not content or content.startswith('│'):
            continue
            
        if content.startswith('├─') or content.startswith('└─'):
            content = content[2:].strip()
            
        # Extract node info
        import re
        match = re.match(r'(.+?)(?:\s*(🖼️|🔊|📝|🔢|💱|📍|📁))?(?:\s*\(.*?\))?(?:\s*=\s*(.+))?$', content)
        if not match:
            continue
            
        name = match.group(1).strip()
        icon = match.group(2)
        value = match.group(3)
        
        # Determine type from icon
        type_map = {
            '🖼️': 'bitmap',
            '🔊': 'audio',
            '📝': 'string',
            '🔢': 'integer',
            '💱': 'real',
            '📍': 'vector',
            '📁': 'directory'
        }
        node_type = type_map.get(icon, 'unknown')
        
        # Create node
        node = NXNode(name, node_type, value)
        
        # Find parent
        while node_stack and node_stack[-1][1] >= indent:
            node_stack.pop()
            
        parent = node_stack[-1][0]
        parent.add_child(node)
        
        if node_type == 'directory':
            node_stack.append((node, indent))
            
    return root

def main():
    """Main extraction process"""
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python extract_full_metadata.py <nx_dump_file> [output_file]")
        print("Example: python extract_full_metadata.py UI_current.txt UI_full_metadata.json")
        sys.exit(1)
        
    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else input_file.replace('.txt', '_metadata.json')
    
    print(f"Extracting metadata from {input_file}...")
    
    # Parse the dump file
    tree = parse_nx_dump(input_file)
    
    # Extract metadata
    extractor = NXMetadataExtractor()
    metadata = extractor.extract_from_tree(tree)
    
    # Generate AssetRegistry
    registry = extractor.generate_asset_registry(metadata)
    
    # Save results
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(metadata, f, indent=2, ensure_ascii=False)
        
    registry_file = output_file.replace('_metadata.json', '_registry.json')
    with open(registry_file, 'w', encoding='utf-8') as f:
        json.dump(registry, f, indent=2, ensure_ascii=False)
        
    print(f"✓ Metadata saved to: {output_file}")
    print(f"✓ AssetRegistry saved to: {registry_file}")
    print(f"\nStatistics:")
    for key, value in metadata['statistics'].items():
        print(f"  {key}: {value}")

if __name__ == "__main__":
    main()