from PIL import Image, ImageFont, ImageDraw

def ttf_to_binary(ttf_path, output_bin, font_size=8):
    # ASCII range 33 (!) to 126 (~)
    start_char = 33
    end_char = 126
    
    try:
        # Load the font. You might need to tweak 'font_size' 
        # depending on the specific TTF design.
        font = ImageFont.truetype(ttf_path, font_size)
    except Exception as e:
        print(f"Error loading font: {e}")
        return

    with open(output_bin, 'wb') as bin_file:
        for i in range(start_char, end_char + 1):
            char = chr(i)
            
            # Create a 5x7 monochrome (1-bit) image
            img = Image.new('1', (5, 7), color=0)
            draw = ImageDraw.Draw(img)
            
            # Draw the character. 
            # (0, -1) offset helps align characters that have descenders
            draw.text((0, -1), char, font=font, fill=1)
            
            # Pack the image data into 7 bytes
            for y in range(7):
                byte = 0
                for x in range(5):
                    pixel = img.getpixel((x, y))
                    if pixel > 0:
                        # Pack MSB-first to match your draw_pixel function
                        byte |= (0x80 >> x)
                bin_file.write(bytes([byte]))

    print(f"Success: {output_bin} generated from {ttf_path}")

if __name__ == "__main__":
    # Change 'myfont.ttf' to your actual file name
    ttf_to_binary("Code_7x5.ttf", "font.bin", font_size=8)