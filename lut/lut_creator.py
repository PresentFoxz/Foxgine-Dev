import sys

def create_lut(output_file, lut_size):
    num = 0

    with open(output_file, "w") as out:
        out.write(f"#define LUT_SIZE {lut_size}\n")
        out.write(f"static const uint32_t Lut[LUT_SIZE] = {{\n    ")

        for i in range(lut_size):
            if i == 0: value = 0
            else: value = (1 << 16) // i

            out.write(f"{value}, ")

            num += 1
            if num >= 20:
                out.write("\n    ")
                num = 0

        out.write("\n};\n")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python lut_creator.py <lut_size>")
        sys.exit(1)

    lut = "Foxgine-Dev/lut/lutFile.h"
    size = int(sys.argv[1])

    create_lut(lut, size)

    print("Created:", lut, "->", size)