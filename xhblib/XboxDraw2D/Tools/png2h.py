import sys
import os
import math
from PIL import Image
import subprocess

def next_power_of_two(n):
    return 1 if n == 0 else 2**math.ceil(math.log2(n))

def process_image(input_path, output_header):
    # Load image and convert to RGBA
    img = Image.open(input_path).convert("RGBA")
    orig_width, orig_height = img.size
    
    # Calculate POT dimensions
    pot_width = next_power_of_two(orig_width)
    pot_height = next_power_of_two(orig_height)
    
    # Create new image with transparent background
    new_img = Image.new("RGBA", (pot_width, pot_height), (0, 0, 0, 0))
    new_img.paste(img, (0, 0))  # Paste original at top-left
    
    # Save as temporary PNG
    temp_png = "temp_pot.png"
    new_img.save(temp_png)
    
    # Convert to DDS using DirectX TexConv
    dds_output = "temp_pot.dds"
    subprocess.run([
        "texconv.exe", 
        "-f", "DXT5", 
        "-y", 
        "-o", os.path.dirname(dds_output),
        temp_png
    ])
    
    # Read DDS file
    with open(dds_output, "rb") as f:
        dds_data = f.read()
    
    # Generate C header
    var_name = os.path.splitext(os.path.basename(input_path))[0]
    with open(output_header, "w") as f:
        f.write(f"#pragma once\n")
        f.write(f"// Auto-generated from {input_path}\n")
        f.write(f"const int {var_name}_width = {orig_width};\n")
        f.write(f"const int {var_name}_height = {orig_height};\n")
        f.write(f"const int {var_name}_pot_width = {pot_width};\n")
        f.write(f"const int {var_name}_pot_height = {pot_height};\n")
        f.write(f"const unsigned char {var_name}_dds_data[] = {{\n")
        
        # Write DDS data as C array
        for i, byte in enumerate(dds_data):
            if i % 16 == 0:
                f.write("    ")
            f.write(f"0x{byte:02X},")
            if i % 16 == 15 or i == len(dds_data) - 1:
                f.write("\n")
        
        f.write("};\n")
        f.write(f"const unsigned int {var_name}_dds_size = sizeof({var_name}_dds_data);\n")
    
    # Cleanup temp files
    os.remove(temp_png)
    os.remove(dds_output)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python texture_converter.py <input.png> <output.h>")
        sys.exit(1)
    
    process_image(sys.argv[1], sys.argv[2])