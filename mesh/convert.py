import sys
import os
import random

def convert_obj(input_file, output_file, name):
    vertices = []
    triangles = []

    with open(input_file, "r") as f:
        for line in f:
            line = line.strip()

            if line.startswith("v "):
                _, x, y, z = line.split()[:4]
                vertices.append((float(x), float(y), float(z)))

            elif line.startswith("f "):
                parts = line.split()[1:]

                # supports: f 1 2 3
                # also supports: f 1/1/1 2/2/2 3/3/3
                indices = []

                for p in parts[:3]:
                    index = p.split("/")[0]
                    indices.append(int(index) - 1)

                triangles.append(tuple(indices))


    with open(output_file, "w") as out:
        out.write("#ifndef " + name.upper() + "_H\n")
        out.write("#define " + name.upper() + "_H\n\n")

        out.write('#include "fox_mesh.h"\n\n')

        vertName = name.upper() + '_VERT '
        triName = name.upper() + '_TRI '

        out.write('#define ' + vertName + str(len(vertices)) + '\n')
        out.write('#define ' + triName + str(len(triangles)) + '\n\n')

        # vertices
        out.write(
            f"static const Vec3f {name}_verts[{vertName}] = {{\n"
        )

        for v in vertices:
            out.write(
                f"    {{{v[0]}f, {v[1]}f, {v[2]}f}},\n"
            )

        out.write("};\n\n")


        # triangles
        out.write(
            f"static const int {name}_tris[{triName}][3] = {{\n"
        )

        for t in triangles:
            out.write(
                f"    {{{t[0]}, {t[1]}, {t[2]}}},\n"
            )

        out.write("};\n\n")


        # colors
        out.write(
            f"static const Color_t {name}_colors[{triName}] = {{\n"
        )

        for _ in triangles:
            c = random.randint(180,255)
            out.write(
                f"    {{{c}, {c}, {c}, 255}},\n"
            )

        out.write("};\n\n")


        out.write("#endif\n")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(
            "Usage: python obj_to_h.py input.obj output.h"
        )
        sys.exit(1)

    obj = sys.argv[1]
    out = sys.argv[2]

    name = os.path.splitext(os.path.basename(out))[0]

    convert_obj(obj, out, name)

    print("Converted:", obj, "->", out)