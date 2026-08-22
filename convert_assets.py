import os
from PIL import Image

def rgba_to_rgb555(r, g, b, a=255):
    if a < 64 or (r > 240 and g > 240 and b > 240):
        return 0x0000  # Transparent pixel
    r5 = (r >> 3) & 0x1F
    g5 = (g >> 3) & 0x1F
    b5 = (b >> 3) & 0x1F
    return 0x8000 | r5 | (g5 << 5) | (b5 << 10)

def convert_image_to_cpp_array(img_path, target_w, target_h, is_popup=False):
    if not os.path.exists(img_path):
        print(f"Warning: {img_path} not found.")
        return None
    
    img = Image.open(img_path).convert('RGBA')
    bbox = img.getbbox()
    if bbox and not is_popup:
        img = img.crop(bbox)
        
    img = img.resize((target_w, target_h), Image.Resampling.LANCZOS)
    
    pixels = []
    for y in range(target_h):
        for x in range(target_w):
            r, g, b, a = img.getpixel((x, y))
            pixels.append(rgba_to_rgb555(r, g, b, a))
            
    return pixels

def generate_header():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    output_header = os.path.join(base_dir, "source", "gfx_assets.h")
    
    bcag_path = os.path.join(base_dir, "BCAG.png")
    wing_path = os.path.join(base_dir, "assets", "chicken", "wing.png")
    breast_path = os.path.join(base_dir, "assets", "chicken", "breast.png")
    nugget_path = os.path.join(base_dir, "assets", "chicken", "nugget.png")
    
    os.makedirs(os.path.dirname(output_header), exist_ok=True)
    
    # Convert chicken graphics (32x32)
    wing_pixels = convert_image_to_cpp_array(wing_path, 32, 32)
    breast_pixels = convert_image_to_cpp_array(breast_path, 32, 32)
    nugget_pixels = convert_image_to_cpp_array(nugget_path, 32, 32)
    
    # Convert popup banner graphic (100x40)
    popup_pixels = convert_image_to_cpp_array(bcag_path, 100, 40, is_popup=True)
    
    with open(output_header, "w") as f:
        f.write("// Auto-generated asset header for Barbeque Chicken Alert NDS\n")
        f.write("#ifndef GFX_ASSETS_H\n#define GFX_ASSETS_H\n\n")
        f.write("#include <nds.h>\n\n")
        f.write("#define SPRITE_SIZE 32\n")
        f.write("#define POPUP_W 100\n")
        f.write("#define POPUP_H 40\n\n")
        
        def write_array(name, pixels, w, h):
            if not pixels:
                f.write(f"static const u16 {name}[{w} * {h}] = {{\n")
                lines = ["0x83E0" for _ in range(w * h)]
                f.write("  " + ", ".join(lines) + "\n};\n\n")
                return
            
            f.write(f"static const u16 {name}[{w} * {h}] = {{\n")
            hex_str = [f"0x{p:04X}" for p in pixels]
            for i in range(0, len(hex_str), 16):
                f.write("  " + ", ".join(hex_str[i:i+16]) + ",\n")
            f.write("};\n\n")

        write_array("wing_gfx", wing_pixels, 32, 32)
        write_array("breast_gfx", breast_pixels, 32, 32)
        write_array("nugget_gfx", nugget_pixels, 32, 32)
        write_array("popup_gfx", popup_pixels, 100, 40)
        
        f.write("#endif // GFX_ASSETS_H\n")

    print(f"Generated {output_header} successfully.")

if __name__ == "__main__":
    generate_header()
