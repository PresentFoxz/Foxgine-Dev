bl_info = {
    "name": "Custom Foxgine Mesh Exporter",
    "author": "PresentFox",
    "version": (1, 4, 0),
    "blender": (3, 6, 0),
    "location": "3D View > Sidebar > Foxgine; File > Export > Foxgine Mesh (.fox)",
    "description": "Color selected faces and export triangles with RGBA and culling",
    "category": "Import-Export",
}

import bpy
import bmesh
import os
from bpy.props import BoolProperty, FloatVectorProperty, StringProperty, IntProperty
from bpy_extras.io_utils import ExportHelper

COLOR_NAME = "FoxgineFaceColor"
CULL_NAME = "backface_culling"

def byte_color(value): return max(0, min(255, round(float(value) * 255)))

def edit_bmesh(context): return bmesh.from_edit_mesh(context.edit_object.data)

def selected_face_indices(bm):
    bm.faces.ensure_lookup_table()
    bm.faces.index_update()
    return [face.index for face in bm.faces if face.select and not face.hide]

def get_color_layer(bm, create=False):
    layer = bm.loops.layers.float_color.get(COLOR_NAME)
    if layer is None and create:
        layer = bm.loops.layers.float_color.new(COLOR_NAME)
        
        bm.faces.ensure_lookup_table()
        for face in bm.faces:
            for loop in face.loops:
                loop[layer] = (1.0, 1.0, 1.0, 1.0)
    
    return layer

def get_cull_layer(bm, create=False):
    layer = bm.faces.layers.int.get(CULL_NAME)
    if layer is None and create: layer = bm.faces.layers.int.new(CULL_NAME)

    return layer

class FOXGINE_OT_apply_face(bpy.types.Operator):
    bl_idname = "foxgine.apply_face"
    bl_label = "Apply to Selected Faces"
    bl_options = {'REGISTER', 'UNDO'}

    @classmethod
    def poll(cls, context): return context.edit_object is not None and context.edit_object.type == 'MESH'

    def execute(self, context):
        obj = context.edit_object
        bm = edit_bmesh(context)
        indices = selected_face_indices(bm)

        if not indices:
            self.report({'WARNING'}, "Select at least one face in Edit Mode")
            return {'CANCELLED'}
        
        get_color_layer(bm, create=True)
        get_cull_layer(bm, create=True)

        bm = edit_bmesh(context)
        bm.faces.ensure_lookup_table()

        color_layer = get_color_layer(bm)
        cull_layer = get_cull_layer(bm)
        rgba = tuple(context.scene.foxgine_face_color)
        cull = int(context.scene.foxgine_backface_culling)

        for index in indices:
            face = bm.faces[index]
            for loop in face.loops: loop[color_layer] = rgba

            face[cull_layer] = cull

        bmesh.update_edit_mesh(obj.data, loop_triangles=True, destructive=False)
        self.report({'INFO'}, f"Updated {len(indices)} face(s)")
        return {'FINISHED'}

class FOXGINE_OT_pick_face(bpy.types.Operator):
    bl_idname = "foxgine.pick_face"
    bl_label = "Pick Selected Face Settings"
    bl_options = {'REGISTER'}

    @classmethod
    def poll(cls, context): return context.edit_object is not None and context.edit_object.type == 'MESH'

    def execute(self, context):
        bm = edit_bmesh(context)
        selected = [f for f in bm.faces if f.select and not f.hide]

        if not selected:
            self.report({'WARNING'}, "Select a face in Edit Mode")
            return {'CANCELLED'}
        
        face = bm.faces.active
        if face is None or not face.select or face.hide: face = selected[0]

        color_layer = get_color_layer(bm)
        cull_layer = get_cull_layer(bm)
        if color_layer: rgba = tuple(face.loops[0][color_layer])
        else: rgba = (1.0, 1.0, 1.0, 1.0)

        context.scene.foxgine_face_color = rgba
        context.scene.foxgine_backface_culling = bool(face[cull_layer]) if cull_layer else False
        return {'FINISHED'}

class FOXGINE_OT_show_colors(bpy.types.Operator):
    bl_idname = "foxgine.show_colors"
    bl_label = "Show Foxgine Colors"
    bl_description = "Activate Foxgine face colors and show them across the model in Solid viewport shading"
    bl_options = {'REGISTER'}

    @classmethod
    def poll(cls, context): return (context.area is not None and context.area.type == 'VIEW_3D' and context.active_object is not None and context.active_object.type == 'MESH')

    def execute(self, context):
        obj = context.edit_object or context.active_object
        mesh = obj.data
        if obj.mode == 'EDIT': bmesh.update_edit_mesh(mesh, loop_triangles=False, destructive=False)

        attrs = mesh.color_attributes
        attr = attrs.get(COLOR_NAME)
        if attr is None:
            self.report({'WARNING'}, "No Foxgine face colors yet. Select faces and click Apply to Selected Faces first")
            return {'CANCELLED'}
        
        attrs.active_color = attr
        if hasattr(attrs, 'render_color_index'):
            index = next((i for i, item in enumerate(attrs) if item.name == COLOR_NAME), -1)
            if index >= 0: attrs.render_color_index = index

        shading = context.space_data.shading
        shading.type = 'SOLID'
        shading.color_type = 'VERTEX'
        shading.light = 'FLAT'

        context.area.tag_redraw()
        self.report({'INFO'}, "Foxgine face colors are now visible in Solid viewport shading")
        return {'FINISHED'}

class FOXGINE_PT_face_colors(bpy.types.Panel):
    bl_label = "Face Colors"
    bl_idname = "FOXGINE_PT_face_colors"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'Foxgine'

    def draw(self, context):
        layout = self.layout

        layout.label(text="Edit Mode > Face Select (3)")
        layout.prop(context.scene, "foxgine_face_color", text="RGBA")
        layout.prop(context.scene, "foxgine_backface_culling")

        col = layout.column(align=True)
        col.operator("foxgine.pick_face", icon='EYEDROPPER')
        col.operator("foxgine.apply_face", icon='BRUSH_DATA')

        layout.separator()
        layout.operator("foxgine.show_colors", text="Show Foxgine Colors", icon='SHADING_SOLID')
        layout.label(text="Faces with 4+ sides export as triangles")

class EXPORT_OT_custom_mesh(bpy.types.Operator, ExportHelper):
    bl_idname = "export_scene.custom_triangle_mesh"
    bl_label = "Export Foxgine Mesh"
    bl_options = {'PRESET'}

    filename_ext = ".fox"
    filter_glob: StringProperty(default="*.fox", options={'HIDDEN'})
    selected_only: BoolProperty(name="Selected Objects Only", default=True)
    apply_transforms: BoolProperty(
        name="Apply Object Transforms", default=False,
        description="Export vertices using world-space coordinates"
    )
    export_animation: BoolProperty(name="Export Animation Keyframes", default=False)
    export_all_animations: BoolProperty(
        name="Export All Animations", default=True,
        description="Export each animation Action separately"
    )

    def draw(self, context):
        layout = self.layout
        layout.prop(self, "selected_only")
        layout.prop(self, "apply_transforms")
        layout.separator()
        layout.prop(self, "export_animation")

        if self.export_animation:
            layout.prop(self, "export_all_animations")
            layout.label(text="Exports keyframes only")
            layout.label(text="Outputs to a separate _frames folder")

    def get_targets(self, objects):
        targets = {}
        visited = set()

        def add(obj):
            if not obj or obj.as_pointer() in visited: return
            visited.add(obj.as_pointer())
            targets[obj.as_pointer()] = obj

            if obj.data: targets[obj.data.as_pointer()] = obj.data
            if obj.type == 'MESH' and obj.data.shape_keys:
                keys = obj.data.shape_keys
                targets[keys.as_pointer()] = keys

            for modifier in obj.modifiers:
                if modifier.type == 'ARMATURE' and modifier.object: add(modifier.object)

            if obj.parent: add(obj.parent)

        for obj in objects: add(obj)
        return list(targets.values())

    def get_slots(self, action, target):
        slots = list(getattr(action, "slots", []))
        if not slots: return [None]
        ad = target.animation_data
        if ad and ad.action == action and getattr(ad, "action_slot", None) in slots: return [ad.action_slot]
        matching = [s for s in slots if s.target_id_type in {'UNSPECIFIED', target.id_type} and (s.name_display == target.name or s.identifier[2:] == target.name)]
        return matching

    def get_actions(self, targets, all_animations):
        candidates = {}
        owners = [t for t in targets if getattr(t, "animation_data", None)]

        def include(action, target, slot=None):
            if not action: return
            if slot is None:
                possible = self.get_slots(action, target)
                if not possible: return
                slot = possible[0]
            entry = candidates.setdefault(action.as_pointer(), [action, {}])
            entry[1][target.as_pointer()] = (target, slot)

        for target in owners:
            ad = target.animation_data
            include(ad.action, target, getattr(ad, "action_slot", None))
            if all_animations:
                for track in ad.nla_tracks:
                    for strip in track.strips:
                        if strip.action: include(strip.action, target, getattr(strip, "action_slot", None))

        if all_animations:
            for action in bpy.data.actions:
                if action.as_pointer() in candidates: continue
                for target in owners:
                    slots = self.get_slots(action, target)
                    if slots: include(action, target, slots[0])

        return {action.name: (action, list(bindings.values())) for action, bindings in candidates.values() if bindings}

    def get_keyframes(self, action, bindings):
        frames = set()
        slots = {s.as_pointer() for _, s in bindings if s is not None}

        def collect(curves, owner):
            for fc in curves:
                if fc.mute: continue
                found = sorted({round(key.co.x) for key in fc.keyframe_points})
                if found:
                    print(f"[Foxgine] {action.name} | {owner} | {fc.data_path}[{fc.array_index}] | {found}")
                    frames.update(found)

        if getattr(action, "layers", None):
            for layer in action.layers:
                for strip in layer.strips:
                    if not hasattr(strip, "channelbags"): continue
                    for bag in strip.channelbags:
                        if not slots or bag.slot.as_pointer() not in slots: continue
                        collect(bag.fcurves, bag.slot.identifier)
        else:
            collect(action.fcurves, "Legacy")

        return sorted(frames)

    def write_frame(self, context, objects, filepath):
        depsgraph = context.evaluated_depsgraph_get()
        vertices, faces = [], []

        for obj in objects:
            evaluated = obj.evaluated_get(depsgraph)
            mesh = evaluated.to_mesh()
            if mesh is None: continue

            try:
                if hasattr(mesh, 'corner_triangles'):
                    triangles = mesh.corner_triangles
                else:
                    mesh.calc_loop_triangles()
                    triangles = mesh.loop_triangles

                offset = len(vertices)

                for vert in mesh.vertices:
                    co = evaluated.matrix_world @ vert.co if self.apply_transforms else vert.co
                    vertices.append((co.x, co.z, -co.y))

                colors = mesh.color_attributes.get(COLOR_NAME)
                valid_colors = bool(colors and colors.domain == 'CORNER' and colors.data_type in {'FLOAT_COLOR', 'BYTE_COLOR'})
                cull_attr = mesh.attributes.get(CULL_NAME)

                for tri in triangles:
                    poly = mesh.polygons[tri.polygon_index]
                    material = obj.material_slots[poly.material_index].material if poly.material_index < len(obj.material_slots) else None
                    rgba = tuple(material.diffuse_color) if material else (1.0, 1.0, 1.0, 1.0)

                    if valid_colors:
                        corners = tri.loops if hasattr(tri, 'loops') else tri.corners
                        rgba = tuple(sum(colors.data[idx].color[channel] for idx in corners) / 3.0 for channel in range(4))

                    cull = 0
                    r, g, b, a = (byte_color(channel) for channel in rgba)

                    if cull_attr and cull_attr.domain == 'FACE' and cull_attr.data_type in {'INT', 'BOOLEAN'}:
                        cull = int(bool(cull_attr.data[tri.polygon_index].value))

                    i, j, k = (offset + idx for idx in tri.vertices)
                    faces.append((i, j, k, r, g, b, a, cull))
            finally:
                evaluated.to_mesh_clear()

        with open(filepath, 'w', encoding='utf-8', newline='\n') as handle:
            for x, y, z in vertices: handle.write(f"v {x:.6f} {y:.6f} {z:.6f}\n")
            for i, j, k, r, g, b, a, cull in faces: handle.write(f"f {i} {j} {k} {r} {g} {b} {a} {cull}\n")

        return len(vertices), len(faces)

    def execute(self, context):
        objects = context.selected_objects if self.selected_only else context.scene.objects
        objects = [obj for obj in objects if obj.type == 'MESH']

        if not objects:
            self.report({'ERROR'}, "No mesh objects to export")
            return {'CANCELLED'}

        for obj in objects:
            if obj.mode == 'EDIT': obj.update_from_editmode()

        scene = context.scene
        original_frame = scene.frame_current
        original_subframe = scene.frame_subframe
        states = []

        try:
            if self.export_animation:
                targets = self.get_targets(objects)
                actions = self.get_actions(targets, self.export_all_animations)

                if not actions:
                    self.report({'WARNING'}, "No animations found")
                    return {'CANCELLED'}

                base = os.path.splitext(os.path.basename(self.filepath))[0]
                folder = os.path.join(os.path.dirname(self.filepath), base + "_frames")
                os.makedirs(folder, exist_ok=True)

                for target in targets:
                    ad = target.animation_data
                    if not ad: continue
                    states.append((ad, ad.action, getattr(ad, "action_slot", None), ad.use_nla))

                total = 0
                index_entries = []

                for name, (action, bindings) in sorted(actions.items()):
                    frames = self.get_keyframes(action, bindings)
                    print(f"[Foxgine] EXPORT {name}: {frames}")
                    if not frames: continue

                    for ad, _, _, _ in states:
                        ad.use_nla = False
                        ad.action = None

                    for target, slot in bindings:
                        ad = target.animation_data_create()
                        ad.action = action
                        if slot is not None: ad.action_slot = slot

                    safe_name = "".join(c if c.isalnum() or c in "_-." else "_" for c in name)

                    previous = None
                    first_pose = None
                    last_pose = None
                    exported = []
                    closing_frame = None

                    for frame in frames:
                        scene.frame_set(frame)
                        filename = f"{base}_{safe_name}_{frame:04d}.fox"
                        path = os.path.join(folder, filename)
                        self.write_frame(context, objects, path)

                        with open(path, "rb") as file: current = file.read()
                        if first_pose is None: first_pose = current
                        last_pose = current

                        if current == previous:
                            os.remove(path)
                            continue

                        previous = current
                        exported.append((frame, filename))

                    if len(frames) > 1 and last_pose == first_pose:
                        closing_frame = frames[-1]
                        if exported and exported[-1][0] == closing_frame and len(exported) > 1:
                            os.remove(os.path.join(folder, exported[-1][1]))
                            exported.pop()

                    if exported:
                        index_entries.append((name, exported, closing_frame))
                        total += len(exported)

                vul_path = os.path.join(folder, f"{base}.vul")
                with open(vul_path, 'w', encoding='utf-8', newline='\n') as handle:
                    handle.write(f"# {base} Animation Index\n\nAnimations {len(index_entries)}\n")
                    for name, exported, closing_frame in index_entries:
                        handle.write(f"\n# {name}\nStartAnim {len(exported)}\n")
                        for index, (frame, filename) in enumerate(exported):
                            held = max(1, exported[index + 1][0] - frame) if index + 1 < len(exported) else max(1, closing_frame - frame) if closing_frame is not None else 1
                            location = f"{os.path.basename(folder)}/{filename}"
                            handle.write(f"Frame {location} {held}\n")
                        handle.write("EndAnim\n")

                self.report({'INFO'}, f"Exported {total} frames from {len(index_entries)} animations to {vul_path}")

            else:
                vertex_count, face_count = self.write_frame(context, objects, self.filepath)
                self.report({'INFO'}, f"Exported {vertex_count} vertices and {face_count} triangles")

        except (OSError, RuntimeError, ValueError, TypeError) as exc:
            self.report({'ERROR'}, str(exc))
            return {'CANCELLED'}

        finally:
            for ad, action, slot, use_nla in states:
                ad.action = action
                if action and slot is not None: ad.action_slot = slot
                ad.use_nla = use_nla

            if self.export_animation: scene.frame_set(original_frame, subframe=original_subframe)

        return {'FINISHED'}

def export_menu(self, context): self.layout.operator(EXPORT_OT_custom_mesh.bl_idname, text="Foxgine Mesh (.fox)")

CLASSES = (
    FOXGINE_OT_apply_face,
    FOXGINE_OT_pick_face,
    FOXGINE_OT_show_colors,
    FOXGINE_PT_face_colors,
    EXPORT_OT_custom_mesh,
)

def register():
    bpy.types.Scene.foxgine_face_color = FloatVectorProperty(
        name="Face RGBA", subtype='COLOR', size=4,
        min=0.0, max=1.0, default=(1.0, 1.0, 1.0, 1.0),
        description="Color assigned to all selected faces",
    )
    bpy.types.Scene.foxgine_backface_culling = BoolProperty(
        name="Backface Culling", default=False,
        description="0 = disabled, 1 = enabled in the Foxgine export",
    )

    for cls in CLASSES: bpy.utils.register_class(cls)
    bpy.types.TOPBAR_MT_file_export.append(export_menu)

def unregister():
    bpy.types.TOPBAR_MT_file_export.remove(export_menu)
    for cls in reversed(CLASSES): bpy.utils.unregister_class(cls)

    del bpy.types.Scene.foxgine_face_color
    del bpy.types.Scene.foxgine_backface_culling

if __name__ == '__main__': register()
