bl_info = {
    "name": "Low Poly Reducer",
    "author": "PresentFox",
    "version": (1, 0, 0),
    "blender": (3, 0, 0),
    "location": "3D Viewport > Sidebar (N) > Low Poly",
    "description": "Reduce meshes to an exact triangle budget (quadric edge collapse) "
                   "and turn thin hair strips into closed pyramid blades",
    "category": "Mesh",
}

import heapq
from collections import defaultdict

import bpy
import bmesh
import numpy as np
from bpy.props import BoolProperty, FloatProperty, IntProperty


# =============================================================================
# Core algorithms (pure Python / numpy, no Blender calls)
# =============================================================================
def decimate(V, F, target, boundary_weight=50.0, flip_threshold=0.0):
    """
    Quadric-error edge collapse down to `target` triangles.

    Each vertex stores a 4x4 quadric (summed squared distance to its triangles'
    planes). Merging two vertices costs v^T (Q1+Q2) v. The merged vertex goes to one
    of the two endpoints or their midpoint (the mathematically optimal spot can
    overshoot at extreme reductions and creates lumps). Collapses that would flip a
    neighbouring triangle are rejected. Open borders get a heavy penalty so outlines
    don't shrink.
    """
    V = np.array(V, float).copy()
    F = [tuple(int(x) for x in f) for f in F]
    nv = len(V)
    alive_f = [True] * len(F)
    vf = [set() for _ in range(nv)]
    for i, f in enumerate(F):
        for v in f:
            vf[v].add(i)

    def plane(f):
        a, b, c = V[f[0]], V[f[1]], V[f[2]]
        n = np.cross(b - a, c - a)
        area2 = np.linalg.norm(n)
        if area2 < 1e-14:
            return None, 0.0
        n = n / area2
        return np.append(n, -n @ a), area2 / 2

    Q = np.zeros((nv, 4, 4))
    for f in F:
        p, area = plane(f)
        if p is None:
            continue
        K = np.outer(p, p) * area
        for v in f:
            Q[v] += K

    edges = defaultdict(list)
    for i, f in enumerate(F):
        for k in range(3):
            a, b = f[k], f[(k + 1) % 3]
            edges[(min(a, b), max(a, b))].append(i)
    for (a, b), faces in edges.items():
        if len(faces) != 1:
            continue
        p, _ = plane(F[faces[0]])
        if p is None:
            continue
        e = V[b] - V[a]
        el = np.linalg.norm(e)
        if el < 1e-12:
            continue
        n2 = np.cross(e / el, p[:3])
        n2 /= np.linalg.norm(n2)
        pp = np.append(n2, -n2 @ V[a])
        K = np.outer(pp, pp) * boundary_weight * el * el
        Q[a] += K
        Q[b] += K

    alive_v = [True] * nv
    version = [0] * nv

    def best_collapse(a, b):
        Qs = Q[a] + Q[b]
        best = None
        for p in (V[a], V[b], (V[a] + V[b]) / 2):
            h = np.append(p, 1.0)
            c = h @ Qs @ h
            if best is None or c < best[0]:
                best = (c, p)
        return best

    heap = []

    def push(a, b):
        c, p = best_collapse(a, b)
        heapq.heappush(heap, (c, a, b, version[a], version[b], tuple(p)))

    for a, b in edges:
        push(a, b)

    def normal(pts):
        n = np.cross(pts[1] - pts[0], pts[2] - pts[0])
        l = np.linalg.norm(n)
        return (n / l) if l > 1e-14 else None

    n_faces = len(F)
    while n_faces > target and heap:
        cost, a, b, va, vb, p = heapq.heappop(heap)
        if not (alive_v[a] and alive_v[b]) or version[a] != va or version[b] != vb:
            continue
        p = np.array(p)

        shared = vf[a] & vf[b]
        ok = True
        for fi in vf[a] | vf[b]:
            if fi in shared or not alive_f[fi]:
                continue
            f = F[fi]
            n_old = normal([V[i] for i in f])
            moved = [a if x == b else x for x in f]
            n_new = normal([p if x == a else V[x] for x in moved])
            if n_new is None or (n_old is not None and n_new @ n_old < flip_threshold):
                ok = False
                break
        if not ok:
            continue

        V[a] = p
        Q[a] = Q[a] + Q[b]
        alive_v[b] = False
        version[a] += 1
        for fi in list(vf[b]):
            if not alive_f[fi]:
                continue
            f = F[fi]
            if a in f:
                alive_f[fi] = False
                n_faces -= 1
                for v in f:
                    vf[v].discard(fi)
            else:
                F[fi] = tuple(a if x == b else x for x in f)
                vf[a].add(fi)
        vf[b] = set()

        neighbours = {v for fi in vf[a] for v in F[fi] if v != a}
        for n in neighbours:
            push(min(a, n), max(a, n))

    keep = [F[i] for i in range(len(F)) if alive_f[i]]
    used = sorted({v for f in keep for v in f})
    remap = {v: i for i, v in enumerate(used)}
    return V[used], np.array([[remap[v] for v in f] for f in keep])


def kmeans(points, K, iters=40, seed=3):
    """Small k-means++ so the add-on does not need scipy."""
    rng = np.random.default_rng(seed)
    n = len(points)
    K = max(1, min(K, n))
    centers = [points[rng.integers(n)]]
    for _ in range(1, K):
        d = np.min([((points - c) ** 2).sum(1) for c in centers], axis=0)
        p = d / d.sum() if d.sum() > 0 else None
        centers.append(points[rng.choice(n, p=p)])
    centers = np.array(centers)
    for _ in range(iters):
        lab = np.argmin(((points[:, None, :] - centers[None]) ** 2).sum(2), axis=1)
        new = np.array([points[lab == k].mean(0) if (lab == k).any() else centers[k]
                        for k in range(K)])
        if np.allclose(new, centers):
            break
        centers = new
    return lab


def make_blades(V, F, K, root, width_scale=1.4, thick_scale=1.2):
    """
    Cluster the triangles into K groups. For each, take its main direction (SVD),
    put a small triangular base at the end nearest `root` and a tip at the far end.
    Each blade is a closed tetrahedron (4 triangles): it can never show holes or get
    culled, which is what goes wrong when thin hair strips are decimated.
    """
    V = np.asarray(V, float)
    F = np.asarray(F, int)
    root = np.asarray(root, float)
    labels = kmeans(V[F].mean(1), K)
    Vo, Fo = [], []
    for k in range(labels.max() + 1):
        fi = np.where(labels == k)[0]
        if len(fi) == 0:
            continue
        idx = np.unique(F[fi].ravel())
        pts = V[idx]
        if len(pts) < 3:
            continue
        m = pts.mean(0)
        _, _, vt = np.linalg.svd(pts - m, full_matrices=False)
        e1, e2, e3 = vt[0], vt[1], vt[2]
        t = (pts - m) @ e1
        if np.linalg.norm(m + e1 * t.min() - root) > np.linalg.norm(m + e1 * t.max() - root):
            e1, t = -e1, -t
        length = t.max() - t.min()
        tip = pts[np.argmax(t)]
        base_centre = pts[t <= t.min() + 0.25 * length].mean(0)
        w = max(np.percentile(np.abs((pts - m) @ e2), 90) * width_scale, 0.09 * length)
        th = max(np.percentile(np.abs((pts - m) @ e3), 90) * thick_scale, 0.06 * length)
        base = [
            base_centre + e2 * w,
            base_centre - e2 * w * 0.7 + e3 * th,
            base_centre - e2 * w * 0.7 - e3 * th,
        ]
        start = len(Vo)
        Vo += base + [tip]
        centre = np.mean(Vo[start:start + 4], axis=0)
        for tri in ((0, 1, 2), (0, 2, 3), (2, 1, 3), (1, 0, 3)):
            a, b, c = [np.array(Vo[start + i]) for i in tri]
            if np.dot(np.cross(b - a, c - a), (a + b + c) / 3 - centre) < 0:
                tri = (tri[0], tri[2], tri[1])
            Fo.append(tuple(start + i for i in tri))
    return np.array(Vo), np.array(Fo)


def split_budget(tri_counts, total, minimum=4):
    """Share a total triangle budget between meshes (bigger meshes get more, but
    sub-linearly so small parts like a nose are not starved)."""
    w = np.sqrt(np.asarray(tri_counts, float))
    share = w / w.sum() * total
    out = np.maximum(minimum, np.floor(share)).astype(int)
    # trim or top up so the sum matches `total` as closely as possible
    while out.sum() > total and out.max() > minimum:
        out[np.argmax(out)] -= 1
    while out.sum() < total:
        out[np.argmax(share - out)] += 1
    return [int(x) for x in out]


# =============================================================================
# Blender glue
# =============================================================================
def mesh_to_arrays(obj):
    """Welded, triangulated copy of an object's mesh as numpy arrays (local space)."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    bm.verts.ensure_lookup_table()
    V = np.array([v.co[:] for v in bm.verts], float)
    F = np.array([[v.index for v in f.verts] for f in bm.faces], int)
    bm.free()
    return V, F


def new_object_from_arrays(name, V, F, src):
    me = bpy.data.meshes.new(name)
    me.from_pydata(np.asarray(V).tolist(), [], np.asarray(F).tolist())
    me.update()
    for poly in me.polygons:
        poly.use_smooth = False  # flat shading, like a low-poly model should be
    if src.data.materials:
        me.materials.append(src.data.materials[0])
    ob = bpy.data.objects.new(name, me)
    ob.matrix_world = src.matrix_world.copy()
    for col in src.users_collection:
        col.objects.link(ob)
    return ob


def tri_count(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def selected_meshes(context):
    return [o for o in context.selected_objects if o.type == 'MESH']


def finish_object(context, src, new_obj):
    if context.scene.lowpoly_keep_original:
        src.hide_set(True)
    else:
        bpy.data.objects.remove(src, do_unlink=True)


class LOWPOLY_OT_reduce(bpy.types.Operator):
    bl_idname = "lowpoly.reduce"
    bl_label = "Reduce Selected to Target"
    bl_description = "Reduce each selected mesh to the target triangle count"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        objs = selected_meshes(context)
        if not objs:
            self.report({'WARNING'}, "Select at least one mesh object")
            return {'CANCELLED'}
        sc = context.scene
        total = 0
        new_objs = []
        for src in objs:
            V, F = mesh_to_arrays(src)
            Vd, Fd = decimate(V, F, sc.lowpoly_target, boundary_weight=sc.lowpoly_border)
            new = new_object_from_arrays(src.name + "_low", Vd, Fd, src)
            new_objs.append((src, new))
            total += len(Fd)
        for src, new in new_objs:
            finish_object(context, src, new)
        self.report({'INFO'}, "Reduced %d object(s), %d triangles in total" % (len(objs), total))
        return {'FINISHED'}


class LOWPOLY_OT_budget(bpy.types.Operator):
    bl_idname = "lowpoly.budget"
    bl_label = "Reduce Selected to Total Budget"
    bl_description = "Split one total triangle budget across all selected meshes and reduce each"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        objs = selected_meshes(context)
        if not objs:
            self.report({'WARNING'}, "Select at least one mesh object")
            return {'CANCELLED'}
        sc = context.scene
        data = [mesh_to_arrays(o) for o in objs]
        targets = split_budget([len(F) for _, F in data], sc.lowpoly_budget)
        total = 0
        new_objs = []
        for src, (V, F), tgt in zip(objs, data, targets):
            Vd, Fd = decimate(V, F, tgt, boundary_weight=sc.lowpoly_border)
            new = new_object_from_arrays(src.name + "_low", Vd, Fd, src)
            new_objs.append((src, new))
            total += len(Fd)
        for src, new in new_objs:
            finish_object(context, src, new)
        self.report({'INFO'}, "%d triangles in total (budget %d)" % (total, sc.lowpoly_budget))
        return {'FINISHED'}


class LOWPOLY_OT_blades(bpy.types.Operator):
    bl_idname = "lowpoly.blades"
    bl_label = "Hair to Blades"
    bl_description = ("Rebuild each selected mesh as closed pyramid blades growing away "
                      "from the 3D cursor (put the cursor in the middle of the head)")
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        objs = selected_meshes(context)
        if not objs:
            self.report({'WARNING'}, "Select the hair mesh")
            return {'CANCELLED'}
        sc = context.scene
        total = 0
        new_objs = []
        for src in objs:
            V, F = mesh_to_arrays(src)
            root_local = np.array(src.matrix_world.inverted() @ context.scene.cursor.location)
            Vb, Fb = make_blades(V, F, sc.lowpoly_blades, root_local)
            new = new_object_from_arrays(src.name + "_blades", Vb, Fb, src)
            new_objs.append((src, new))
            total += len(Fb)
        for src, new in new_objs:
            finish_object(context, src, new)
        self.report({'INFO'}, "Built %d triangles of blades" % total)
        return {'FINISHED'}


class LOWPOLY_PT_panel(bpy.types.Panel):
    bl_label = "Low Poly Reducer"
    bl_idname = "LOWPOLY_PT_panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "Low Poly"

    def draw(self, context):
        layout = self.layout
        sc = context.scene
        objs = selected_meshes(context)
        box = layout.box()
        box.label(text="Selected: %d mesh(es), %d tris" % (len(objs), sum(tri_count(o) for o in objs)))
        all_tris = sum(tri_count(o) for o in context.scene.objects
                       if o.type == 'MESH' and o.visible_get())
        box.label(text="Visible in scene: %d tris" % all_tris)

        col = layout.column(align=True)
        col.label(text="Reduce each selected mesh:")
        col.prop(sc, "lowpoly_target")
        col.operator("lowpoly.reduce", icon='MOD_DECIM')

        col = layout.column(align=True)
        col.label(text="Or share one budget:")
        col.prop(sc, "lowpoly_budget")
        col.operator("lowpoly.budget", icon='MOD_DECIM')

        col = layout.column(align=True)
        col.label(text="Thin hair strips:")
        col.prop(sc, "lowpoly_blades")
        col.operator("lowpoly.blades", icon='STRANDS')
        col.label(text="Root = 3D cursor (Shift+S > Cursor to Selected on the head)")

        layout.separator()
        layout.prop(sc, "lowpoly_keep_original")
        layout.prop(sc, "lowpoly_border")


classes = (LOWPOLY_OT_reduce, LOWPOLY_OT_budget, LOWPOLY_OT_blades, LOWPOLY_PT_panel)


def register():
    S = bpy.types.Scene
    S.lowpoly_target = IntProperty(name="Target tris", default=90, min=4, max=100000)
    S.lowpoly_budget = IntProperty(name="Total budget", default=250, min=8, max=100000)
    S.lowpoly_blades = IntProperty(name="Blades", default=9, min=1, max=200)
    S.lowpoly_border = FloatProperty(
        name="Border protection", default=50.0, min=0.0, max=1000.0,
        description="How strongly open edges resist being pulled inward")
    S.lowpoly_keep_original = BoolProperty(
        name="Keep original (hidden)", default=True,
        description="Hide the original object instead of deleting it")
    for c in classes:
        bpy.utils.register_class(c)


def unregister():
    for c in reversed(classes):
        bpy.utils.unregister_class(c)
    S = bpy.types.Scene
    del S.lowpoly_target, S.lowpoly_budget, S.lowpoly_blades
    del S.lowpoly_border, S.lowpoly_keep_original


if __name__ == "__main__":
    register()