import os

# Get all current and compressed files
current_files = [f for f in os.listdir('.') if f.endswith('_current.txt') and f != '📋_NX_CURRENT_SUMMARY.txt']
total_original = 0
total_compressed = 0

print("File compression results:")
print("-" * 60)

for current_file in sorted(current_files):
    if os.path.exists(current_file):
        current_size = os.path.getsize(current_file)
        total_original += current_size
        
        compressed_file = current_file.replace('_current.txt', '_compressed.txt')
        if os.path.exists(compressed_file):
            compressed_size = os.path.getsize(compressed_file)
            total_compressed += compressed_size
            reduction = (1 - compressed_size / current_size) * 100
            print(f"{current_file.replace('_current.txt', ''):15} {current_size/1024:7.1f} KB -> {compressed_size/1024:7.1f} KB ({reduction:5.1f}% reduction)")
        else:
            print(f"{current_file.replace('_current.txt', ''):15} {current_size/1024:7.1f} KB -> [compression failed]")

print("-" * 60)
total_reduction = (1 - total_compressed / total_original) * 100 if total_original > 0 else 0
print(f"{'Total':15} {total_original/1024:7.1f} KB -> {total_compressed/1024:7.1f} KB ({total_reduction:5.1f}% reduction)")