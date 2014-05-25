import bpy

from math import degrees, pi
from mathutils import Vector, Matrix

TIME_SEG_SAMPS = 4
SHUTTER_TIME = 0.5

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


def export_psy(scene, filename):
    f = open(filename, 'w')
    w = IndentedWriter(f)

    # Info
    w.write("# Exported from Blender 2.7x\n")

    cur_fr = scene.frame_current

    for fr in range(scene.frame_start, scene.frame_end+1):
        scene.frame_set(fr)

        # Scene begin
        w.write("\n\nScene $%s_fr%d {\n" % (scene.name, fr))
        w.indent()

        #######################
        # Output section begin
        w.write("Output {\n")
        w.indent()

        w.write('Path ["' + scene.render.filepath + '%04d.png"]\n' % fr)

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
        w.write("ApertureRadius [%f]\n" % (cam.data.cycles.aperture_size * 2))

        matz = Matrix()
        matz[2][2] = -1
        for i in range(TIME_SEG_SAMPS):
            scene.frame_set(fr, (SHUTTER_TIME/max(1,TIME_SEG_SAMPS-1))*i)
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
        w.write("\n}\n")

        # Cleanup
        f.close()
        scene.frame_set(cur_fr)




def export_scene_objects(scene, w):
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
            for i in range(TIME_SEG_SAMPS):
                scene.frame_set(fr, (SHUTTER_TIME/max(1,TIME_SEG_SAMPS-1))*i)
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
                    for i in range(TIME_SEG_SAMPS):
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
            for i in range(TIME_SEG_SAMPS):
                scene.frame_set(fr, (SHUTTER_TIME/max(1,TIME_SEG_SAMPS-1))*i)
                time_surfaces += [ob.data.copy()]
                time_mats += [ob.matrix_world.copy()]

            # Write patch
            w.write("BicubicPatch $" + ob.name + " {\n")
            w.indent()
            for i in range(TIME_SEG_SAMPS):
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
        elif ob.type == 'LAMP':
            mat = ob.matrix_world
            loc = mat.to_translation()
            coldata = ob.data.node_tree.nodes['Emission'].inputs['Color'].default_value[:3]
            energy = ob.data.node_tree.nodes['Emission'].inputs['Strength'].default_value
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
    w.write("\n}\n")
