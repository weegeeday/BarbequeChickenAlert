import os
import struct

def convert_sounds():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    output_header = os.path.join(base_dir, "source", "sound_assets.h")
    
    raw1_path = os.path.join(base_dir, "sound1.raw")
    raw2_path = os.path.join(base_dir, "sound2.raw")
    raw3_path = os.path.join(base_dir, "sound3.raw")
    
    def read_raw_s16(path):
        if not os.path.exists(path):
            return []
        with open(path, "rb") as f:
            data = f.read()
        # Read as 16-bit signed shorts (s16le)
        count = len(data) // 2
        shorts = struct.unpack(f"<{count}h", data[:count*2])
        return shorts
        
    s1_shorts = read_raw_s16(raw1_path)
    s2_shorts = read_raw_s16(raw2_path)
    s3_shorts = read_raw_s16(raw3_path)
    
    with open(output_header, "w") as f:
        f.write("// Auto-generated original audio samples for Barbeque Chicken Alert NDS\n")
        f.write("#ifndef SOUND_ASSETS_H\n#define SOUND_ASSETS_H\n\n")
        f.write("#include <nds.h>\n\n")
        
        def write_s16_array(name, shorts):
            count = len(shorts)
            f.write(f"alignas(4) static const s16 {name}[{count}] = {{\n")
            hex_str = [f"{s}" for s in shorts]
            for i in range(0, len(hex_str), 16):
                f.write("  " + ", ".join(hex_str[i:i+16]) + ",\n")
            f.write("};\n\n")
            
        write_s16_array("sound_popup_pcm", s1_shorts)
        write_s16_array("sound_chicken_pcm", s2_shorts)
        write_s16_array("sound_rare_pcm", s3_shorts)
        
        f.write("#endif // SOUND_ASSETS_H\n")
        
    print(f"Generated {output_header} successfully with 4-byte aligned s16 PCM audio samples.")

if __name__ == "__main__":
    convert_sounds()
