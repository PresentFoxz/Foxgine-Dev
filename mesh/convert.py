import sys
import os
import random


def convert_obj(input_file, output_file):

    vertices = []
    triangles = []

    with open(input_file, "r") as f:
        for line in f:
            line = line.strip()

            if line.startswith("v "):
                _, x, y, z = line.split()[:4]

                vertices.append((
                    float(x),
                    float(y),
                    float(z)
                ))


            elif line.startswith("f "):
                parts = line.split()[1:]

                indices = []

                for p in parts[:3]:
                    index = p.split("/")[0]

                    # OBJ is 1-based
                    # FOX is 0-based
                    indices.append(int(index) - 1)


                color = random.randint(0, 255)

                triangles.append((
                    indices[0],
                    indices[1],
                    indices[2],
                    color,
                    0
                ))


    with open(output_file, "w") as out:

        # vertices
        for x, y, z in vertices:
            out.write(
                f"v {x} {y} {z}\n"
            )


        out.write("\n")


        # triangles
        for a, b, c, color, bfc in triangles:
            out.write(
                f"f {a} {b} {c} {color} {bfc}\n"
            )


if __name__ == "__main__":

    if len(sys.argv) < 3:
        print(
            "Usage: python obj_to_fox.py input.obj output.fox"
        )
        sys.exit(1)


    obj = sys.argv[1]
    out = sys.argv[2]

    convert_obj(obj, out)

    print(
        "Converted:",
        obj,
        "->",
        out
    )