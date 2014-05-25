import bpy
from . import psy_export

class PsychopathRender(bpy.types.RenderEngine):
    bl_idname = 'PSYCHOPATH_RENDER'
    bl_label = "Psychopath"
    DELAY = 0.5

    @staticmethod
    def _locate_binary():
        addon_prefs = bpy.context.user_preferences.addons[__package__].preferences

        # Use the system preference if its set.
        psy_binary = addon_prefs.filepath_psychopath
        if psy_binary:
            if os.path.exists(psy_binary):
                return psy_binary
            else:
                print("User Preference to psychopath %r NOT FOUND, checking $PATH" % psy_binary)

        # search the path all os's
        psy_binary_default = "psychopath"

        os_path_ls = os.getenv("PATH").split(':') + [""]

        for dir_name in os_path_ls:
            psy_binary = os.path.join(dir_name, psy_binary_default)
            if os.path.exists(psy_binary):
                return psy_binary
        return ""

    def _export(self, scene, povPath, renderImagePath):
        # TODO
        pass

    def _render(self, scene):
        try:
            os.remove(self._temp_file_out)  # so as not to load the old file
        except OSError:
            pass

        psy_binary = PsychopathRender._locate_binary()
        if not psy_binary:
            print("Psychopath: could not execute psychopath, possibly Psychopath isn't installed")
            return False

        # TODO: figure out command line options
        args = []

        # Start Rendering!
        try:
            self._process = subprocess.Popen([psy_binary] + args,
                                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        except OSError:
            # TODO, report api
            print("Psychopath: could not execute '%s'" % psy_binary)
            import traceback
            traceback.print_exc()
            print ("***-DONE-***")
            return False

        else:
            print("Psychopath found")
            print("Command line arguments passed: " + str(args))
            return True

        # Now that we have a valid process

    def _cleanup(self):
        for f in (self._temp_file_in, self._temp_file_ini, self._temp_file_out):
            for i in range(5):
                try:
                    os.unlink(f)
                    break
                except OSError:
                    # Wait a bit before retrying file might be still in use by Blender,
                    # and Windows does not know how to delete a file in use!
                    time.sleep(self.DELAY)
        for i in unpacked_images:
            for c in range(5):
                try:
                    os.unlink(i)
                    break
                except OSError:
                    # Wait a bit before retrying file might be still in use by Blender,
                    # and Windows does not know how to delete a file in use!
                    time.sleep(self.DELAY)

    def render(self, scene):
        # has to be called to update the frame on exporting animations
        scene.frame_set(scene.frame_current)

        # start export
        self.update_stats("", "Psychopath: Exporting data from Blender")
        self._export(scene, psyPath, renderImagePath)
        self.update_stats("", "Psychopath: Parsing File")

        # Start rendering
        if not self._render(scene):
            self.update_stats("", "Psychopath: Not found")
            return

        r = scene.render
        # compute resolution
        x = int(r.resolution_x * r.resolution_percentage * 0.01)
        y = int(r.resolution_y * r.resolution_percentage * 0.01)

        if os.path.exists(self._temp_file_out):
            xmin = int(r.border_min_x * x)
            ymin = int(r.border_min_y * y)
            xmax = int(r.border_max_x * x)
            ymax = int(r.border_max_y * y)

            result = self.begin_result(0, 0, x, y)
            lay = result.layers[0]

            # This assumes the file has been fully written We wait a bit, just in case!
            time.sleep(self.DELAY)
            try:
                lay.load_from_file(self._temp_file_out)
            except RuntimeError:
                print("***PSYCHOPATH ERROR WHILE READING OUTPUT FILE***")

def register():
    bpy.utils.register_class(PsychopathRender)

def unregister():
    bpy.utils.unregister_class(PsychopathRender)
