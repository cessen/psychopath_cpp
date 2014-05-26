import bpy

from math import degrees, pi, log
from mathutils import Vector, Matrix


def mat2str(m):
    """ Converts a matrix into a single-line string of values.
    """
    s = ""
    for j in range(4):
        for i in range(4):
            s += (" %f" % m[i][j])
    return s[1:]


class IndentedWriter:
    def __init__(self, file_handle):
        self.f = file_handle
        self.indent_level = 0
        self.indent_size = 4

    def indent(self):
        self.indent_level += self.indent_size

    def unindent(self):
        self.indent_level -= self.indent_size
        if self.indent_level < 0:
            self.indent_level = 0

    def write(self, text):
        self.f.write(' '*self.indent_level + text)


def set_frame(scene, frame, fraction):
    if fraction >= 0:
        scene.frame_set(frame, fraction)
    else:
        scene.frame_set(frame-1, 1.0+fraction)


def export_psy(scene, export_path, render_image_path):
    f = open(export_path, 'w')
    w = IndentedWriter(f)

    # Motion blur segments are rounded down to a power of two
    if scene.psychopath.motion_blur_segments > 0:
        time_samples = (2**int(log(scene.psychopath.motion_blur_segments, 2))) + 1
    else:
        time_samples = 1

    # pre-calculate useful values for exporting motion blur
    shutter_start = scene.psychopath.shutter_start
    shutter_diff = (scene.psychopath.shutter_end - scene.psychopath.shutter_start) / max(1, (time_samples-1))

    # Info
    w.write("# Exported from Blender 2.7x\n")

    fr = scene.frame_current

    # Scene begin
    w.write("\n\nScene $%s_fr%d {\n" % (scene.name, fr))
    w.indent()

    #######################
    # Output section begin
    w.write("Output {\n")
    w.indent()

    w.write('Path ["%s"]\n' % render_image_path)

    # Output section end
    w.unindent()
    w.write("}\n")

    ###############################
    # RenderSettings section begin
    w.write("RenderSettings {\n")
    w.indent()

    res_x = int(scene.render.resolution_x * (scene.render.resolution_percentage / 100))
    res_y = int(scene.render.resolution_y * (scene.render.resolution_percentage / 100))
    w.write('Resolution [%d %d]\n' % (res_x, res_y))
    w.write("SamplesPerPixel [%d]\n" % scene.psychopath.spp)
    w.write('Seed [%d]\n' % fr)

    # RenderSettings section end
    w.unindent()
    w.write("}\n")

    #######################
    # Camera section begin
    w.write("Camera {\n")
    w.indent()

    cam = scene.camera
    w.write("Fov [%f]\n" % degrees(cam.data.angle))
    w.write("FocalDistance [%f]\n" % cam.data.dof_distance)
    w.write("ApertureRadius [%f]\n" % (cam.data.psychopath.aperture_radius))

    matz = Matrix()
    matz[2][2] = -1
    for i in range(time_samples):
        set_frame(scene, fr, shutter_start + (shutter_diff*i))
        mat = cam.matrix_world.copy()
        mat = mat * matz
        w.write("Transform [%s]\n" % mat2str(mat))

    # Camera section end
    w.unindent()
    w.write("}\n")

    #######################
    # Export objects and materials
    export_scene_objects(scene, w)

    # Scene end
    w.unindent()
    w.write("}\n")

    # Cleanup
    f.close()
    scene.frame_set(fr)




def export_scene_objects(scene, w):
    # Motion blur segments are rounded down to a power of two
    if scene.psychopath.motion_blur_segments > 0:
        time_samples = (2**int(log(scene.psychopath.motion_blur_segments, 2))) + 1
    else:
        time_samples = 1

    # pre-calculate useful values for exporting motion blur
    shutter_start = scene.psychopath.shutter_start
    shutter_diff = (scene.psychopath.shutter_end - scene.psychopath.shutter_start) / max(1, (time_samples-1))

    #######################
    # Assembly section begin
    w.write("Assembly {\n")
    w.indent()

    fr = scene.frame_current

    for ob in scene.objects:
        if ob.type == 'MESH':
            w.write("# Mesh object: %s\n" % ob.name)

            # Collect time samples
            time_meshes = []
            time_mats = []
            for i in range(time_samples):
                set_frame(scene, fr, shutter_start + (shutter_diff*i))
                time_meshes += [ob.to_mesh(scene, True, 'RENDER')]
                time_mats += [ob.matrix_world.copy()]

            # Write patches
            polys = time_meshes[0].polygons
            face_count = 0
            for poly in polys:
                face_count += 1
                if len(poly.vertices) == 4:
                    # Object
                    w.write("BilinearPatch $%s.%d {\n" % (ob.name, face_count))
                    w.indent()
                    for i in range(time_samples):
                        mat = time_mats[i]
                        verts = time_meshes[i].vertices
                        vstr = ""
                        for vi in poly.vertices:
                            v = mat * verts[vi].co
                            vstr += ("%f %f %f " % (v[0], v[1], v[2]))
                        w.write("Vertices [%s]\n" % vstr[:-1])
                    w.unindent()
                    w.write("}\n")
                    # Instance
                    w.write("Instance {\n")
                    w.indent()
                    w.write("Data [$%s.%d]\n" % (ob.name, face_count))
                    w.unindent()
                    w.write("}\n")
            for m in time_meshes:
                bpy.data.meshes.remove(m)
            w.write("\n")
        elif ob.type == 'SURFACE':

            w.write("# Surface object: %s\n" % ob.name)

            # Collect time samples
            time_surfaces = []
            time_mats = []
            for i in range(time_samples):
                set_frame(scene, fr, shutter_start + (shutter_diff*i))
                time_surfaces += [ob.data.copy()]
                time_mats += [ob.matrix_world.copy()]

            # Write patch
            w.write("BicubicPatch $" + ob.name + " {\n")
            w.indent()
            for i in range(time_samples):
                mat = time_mats[i]
                verts = time_surfaces[i].splines[0].points
                vstr = ""
                for v in verts:
                    vt = mat * v.co
                    vstr += ("%f %f %f " % (vt[0], vt[1], vt[2]))
                w.write("Vertices [%s]\n" % vstr[:-1])
            for s in time_surfaces:
                bpy.data.curves.remove(s)
            w.unindent()
            w.write("}\n")

            # Write patch instance
            w.write("Instance {\n")
            w.indent()
            w.write("Data [$%s]\n" % ob.name)
            w.unindent()
            w.write("}\n")
        elif ob.type == 'LAMP' and ob.data.type == 'POINT':
            mat = ob.matrix_world
            loc = mat.to_translation()
            coldata = ob.data.color
            energy = ob.data.energy
            w.write("SphereLight $%s {\n" % ob.name)
            w.indent()
            w.write("Location [%f %f %f]\n" % (loc[0], loc[1], loc[2]))
            w.write("Radius [%f]\n" % ob.data.shadow_soft_size)
            w.write("Color [%f %f %f]\n" % (coldata[0], coldata[1], coldata[2]))
            w.write("Energy [%f]\n" % energy)
            w.unindent()
            w.write("}\n")

            # Write patch instance
            w.write("Instance {\n")
            w.indent()
            w.write("Data [$%s]\n" % ob.name)
            w.unindent()
            w.write("}\n")

    # Assembly section end
    w.unindent()
    w.write("}\n")
