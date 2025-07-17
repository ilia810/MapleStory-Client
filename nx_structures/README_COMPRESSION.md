# NX Structure Compression System

This directory contains multiple compression approaches for NX structure files, each optimized for different use cases.

## Overview

The NX structure files (`*_current.txt`) are **already summarized** versions of the full NX data, showing structure with item counts like `(3 items)` but not all deep leaf nodes. Our compression systems work with these summarized structures.

## Available Compression Tools

### 1. UI Pattern Compression (`compress_nx_structure.py`)
**Best for**: UI.nx files with button states
- Detects UI patterns (normal, pressed, disabled, etc.)
- Achieves 97%+ compression for UI elements
- **Limitation**: Only captures UI button patterns, loses other structural data

### 2. Full Hierarchy Compression (`compress_nx_full.py`)
**Best for**: Preserving more structure depth
- Maintains parent-child relationships
- Uses range notation for numeric sequences
- Achieves 68% compression
- **Limitation**: Still loses some deeper levels

### 3. Ultra Compact Format (`compress_nx_ultra.py`)
**Best for**: Maximum compression with pattern detection
- Uses advanced pattern detection
- Achieves 79% compression
- Groups similar structures

### 4. Lossless JSON Format (`compress_nx_lossless.py`)
**Best for**: Data interchange and processing
- Outputs structured JSON
- Preserves complete hierarchy as shown in input
- Only 9% compression (preserves everything)

### 5. Simple Format (`compress_nx_simple.py`)
**Best for**: Human-readable summaries
- Shows item counts inline: `name[N]`
- Uses range notation: `{0-10}`
- Achieves 99%+ compression
- **Limitation**: Too compressed for reconstruction

## Understanding the Data

The `_current.txt` files show structure like:
```
├─ airstrike 📁 (15 items)
  ├─ 0 📁 (3 items)
  ├─ 1 📁 (3 items)
  ├─ 10 📁 (4 items)
```

The `(N items)` notation means there are N children, but they're not shown in the summary. This is by design to keep files readable.

## Recommendations

1. **For UI files**: Use `compress_nx_structure.py` - designed specifically for UI patterns
2. **For general use**: Use `compress_nx_ultra.py` - good balance of compression and information
3. **For data processing**: Use `compress_nx_lossless.py` - JSON format preserves everything
4. **For documentation**: Use the original `_current.txt` files - they're already optimized

## Usage Examples

```bash
# UI-specific compression (best for UI.nx)
python compress_nx_structure.py UI_current.txt

# General compression with pattern detection
python compress_nx_ultra.py Character_current.txt

# Lossless JSON output
python compress_nx_lossless.py Item_current.txt

# Check compression results
python check_sizes.py
```

## Important Notes

- The `_current.txt` files are already summaries, not complete data dumps
- Item counts `(N items)` indicate deeper structure exists but isn't shown
- Choose compression based on your use case, not just size reduction
- For AssetRegistry development, the original files are often most useful