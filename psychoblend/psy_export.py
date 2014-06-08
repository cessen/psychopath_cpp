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


class PsychoExporter:
    def __init__(self, scene):
        self.scene = scene

        self.mesh_names = {}

        # Motion blur segments are rounded down to a power of two
        if scene.psychopath.motion_blur_segments > 0:
            self.time_samples = (2**int(log(scene.psychopath.motion_blur_segments, 2))) + 1
        else:
            self.time_samples = 1

        # pre-calculate useful values for exporting motion blur
        self.shutter_start = scene.psychopath.shutter_start
        self.shutter_diff = (scene.psychopath.shutter_end - scene.psychopath.shutter_start) / max(1, (self.time_samples-1))

        self.fr = scene.frame_current


    def set_frame(self, frame, fraction):
        if fraction >= 0:
            self.scene.frame_set(frame, fraction)
        else:
            self.scene.frame_set(frame-1, 1.0+fraction)


    def export_psy(self, export_path, render_image_path):
        f = open(export_path, 'w')
        self.w = IndentedWriter(f)

        # Info
        self.w.write("# Exported from Blender 2.7x\n")

        # Scene begin
        self.w.write("\n\nScene $%s_fr%d {\n" % (self.scene.name, self.fr))
        self.w.indent()

        #######################
        # Output section begin
        self.w.write("Output {\n")
        self.w.indent()

        self.w.write('Path ["%s"]\n' % render_image_path)

        # Output section end
        self.w.unindent()
        self.w.write("}\n")

        ###############################
        # RenderSettings section begin
        self.w.write("RenderSettings {\n")
        self.w.indent()

        res_x = int(self.scene.render.resolution_x * (self.scene.render.resolution_percentage / 100))
        res_y = int(self.scene.render.resolution_y * (self.scene.render.resolution_percentage / 100))
        self.w.write('Resolution [%d %d]\n' % (res_x, res_y))
        self.w.write("SamplesPerPixel [%d]\n" % self.scene.psychopath.spp)
        self.w.write("DicingRate [%f]\n" % self.scene.psychopath.dicing_rate)
        self.w.write('Seed [%d]\n' % self.fr)

        # RenderSettings section end
        self.w.unindent()
        self.w.write("}\n")

        #######################
        # Camera section begin
        self.w.write("Camera {\n")
        self.w.indent()

        cam = self.scene.camera

        if cam.data.dof_object == None:
            dof_distance = cam.data.dof_distance
        else:
            # TODO: implement DoF object tracking here
            dof_distance = 0.0
            print("WARNING: DoF object tracking not yet implemented.")

        self.w.write("Fov [%f]\n" % degrees(cam.data.angle))
        self.w.write("FocalDistance [%f]\n" % dof_distance)
        self.w.write("ApertureRadius [%f]\n" % (cam.data.psychopath.aperture_radius))

        matz = Matrix()
        matz[2][2] = -1
        for i in range(self.time_samples):
            self.set_frame(self.fr, self.shutter_start + (self.shutter_diff*i))
            mat = cam.matrix_world.copy()
            mat = mat * matz
            self.w.write("Transform [%s]\n" % mat2str(mat))

        # Camera section end
        self.w.unindent()
        self.w.write("}\n")

        #######################
        # Export objects and materials
        self.export_scene_objects()

        # Scene end
        self.w.unindent()
        self.w.write("}\n")

        # Cleanup
        f.close()
        self.scene.frame_set(self.fr)




    def export_scene_objects(self):
        #######################
        # Assembly section begin
        self.w.write("Assembly {\n")
        self.w.indent()

        for ob in self.scene.objects:
            if ob.type == 'MESH':
                self.export_mesh_object(ob)
            elif ob.type == 'SURFACE':

                self.w.write("# Surface object: %s\n" % ob.name)

                # Collect time samples
                time_surfaces = []
                time_mats = []
                for i in range(self.time_samples):
                    self.set_frame(self.fr, self.shutter_start + (self.shutter_diff*i))
                    time_surfaces += [ob.data.copy()]
                    time_mats += [ob.matrix_world.copy()]

                # Write patch
                self.w.write("BicubicPatch $" + ob.name + " {\n")
                self.w.indent()
                for i in range(self.time_samples):
                    mat = time_mats[i]
                    verts = time_surfaces[i].splines[0].points
                    vstr = ""
                    for v in verts:
                        vt = mat * v.co
                        vstr += ("%f %f %f " % (vt[0], vt[1], vt[2]))
                    self.w.write("Vertices [%s]\n" % vstr[:-1])
                for s in time_surfaces:
                    bpy.data.curves.remove(s)
                self.w.unindent()
                self.w.write("}\n")

                # Write patch instance
                self.w.write("Instance {\n")
                self.w.indent()
                self.w.write("Data [$%s]\n" % ob.name)
                self.w.unindent()
                self.w.write("}\n")
            elif ob.type == 'LAMP' and ob.data.type == 'POINT':
                mat = ob.matrix_world
                loc = mat.to_translation()
                coldata = ob.data.color
                energy = ob.data.energy
                self.w.write("SphereLight $%s {\n" % ob.name)
                self.w.indent()
                self.w.write("Location [%f %f %f]\n" % (loc[0], loc[1], loc[2]))
                self.w.write("Radius [%f]\n" % ob.data.shadow_soft_size)
                self.w.write("Color [%f %f %f]\n" % (coldata[0], coldata[1], coldata[2]))
                self.w.write("Energy [%f]\n" % energy)
                self.w.unindent()
                self.w.write("}\n")

                # Write patch instance
                self.w.write("Instance {\n")
                self.w.indent()
                self.w.write("Data [$%s]\n" % ob.name)
                self.w.unindent()
                self.w.write("}\n")

        # Assembly section end
        self.w.unindent()
        self.w.write("}\n")


    def export_mesh_object(self, ob):
        self.w.write("# Mesh object: %s\n" % ob.name)

        # Determine if and how to export the mesh data
        has_modifiers = len(ob.modifiers) > 0
        if has_modifiers:
            mesh_name = ob.name + "__" + ob.data.name
        else:
            mesh_name = ob.data.name
        export_mesh = (mesh_name not in self.mesh_names) or has_modifiers

        # Collect time samples
        time_meshes = []
        time_mats = []
        for i in range(self.time_samples):
            self.set_frame(self.fr, self.shutter_start + (self.shutter_diff*i))
            time_mats += [ob.matrix_world.copy()]
            if export_mesh and (has_modifiers or i == 0):
                time_meshes += [ob.to_mesh(self.scene, True, 'RENDER')]

        # Export mesh data if necessary
        if export_mesh:
            self.mesh_names[mesh_name] = True
            self.w.write("Assembly $%s {\n" % mesh_name)
            self.w.indent()

            # Write patches
            polys = time_meshes[0].polygons
            face_count = 0
            for poly in polys:
                face_count += 1
                if len(poly.vertices) == 4:
                    # Object
                    self.w.write("BilinearPatch $%s.%d {\n" % (mesh_name, face_count))
                    self.w.indent()
                    for i in range(len(time_meshes)):
                        verts = time_meshes[i].vertices
                        vstr = ""
                        for vi in poly.vertices:
                            v = verts[vi].co
                            vstr += ("%f %f %f " % (v[0], v[1], v[2]))
                        self.w.write("Vertices [%s]\n" % vstr[:-1])
                    self.w.unindent()
                    self.w.write("}\n")
                    # Instance
                    self.w.write("Instance {\n")
                    self.w.indent()
                    self.w.write("Data [$%s.%d]\n" % (mesh_name, face_count))
                    self.w.unindent()
                    self.w.write("}\n")
            for m in time_meshes:
                bpy.data.meshes.remove(m)

            # Assembly section end
            self.w.unindent()
            self.w.write("}\n")

        self.w.write("Instance {\n")
        self.w.indent()
        self.w.write("Data [$%s]\n" % mesh_name)
        for i in range(len(time_mats)):
            self.set_frame(self.fr, self.shutter_start + (self.shutter_diff*i))
            mat = time_mats[i].inverted()
            self.w.write("Transform [%s]\n" % mat2str(mat))
        self.w.unindent()
        self.w.write("}\n")
