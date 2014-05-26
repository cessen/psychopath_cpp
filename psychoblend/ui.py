import bpy

# Use some of the existing buttons.
from bl_ui import properties_render
properties_render.RENDER_PT_render.COMPAT_ENGINES.add('PSYCHOPATH_RENDER')
properties_render.RENDER_PT_dimensions.COMPAT_ENGINES.add('PSYCHOPATH_RENDER')
properties_render.RENDER_PT_output.COMPAT_ENGINES.add('PSYCHOPATH_RENDER')
del properties_render

from bl_ui import properties_data_camera
properties_data_camera.DATA_PT_lens.COMPAT_ENGINES.add('PSYCHOPATH_RENDER')
properties_data_camera.DATA_PT_camera.COMPAT_ENGINES.add('PSYCHOPATH_RENDER')
properties_data_camera.DATA_PT_camera_display.COMPAT_ENGINES.add('PSYCHOPATH_RENDER')
properties_data_camera.DATA_PT_custom_props_camera.COMPAT_ENGINES.add('PSYCHOPATH_RENDER')
del properties_data_camera

class PsychopathPanel():
    COMPAT_ENGINES = {'PSYCHOPATH_RENDER'}

    @classmethod
    def poll(cls, context):
        rd = context.scene.render
        return (rd.use_game_engine is False) and (rd.engine in cls.COMPAT_ENGINES)


class RENDER_PT_psychopath_render_settings(PsychopathPanel, bpy.types.Panel):
    bl_label = "Render Settings"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "render"

    def draw(self, context):
        scene = context.scene
        layout = self.layout

        col = layout.column()

        col.label(text="Sampling")
        col.prop(scene.psychopath, "spp")

        col.label(text="Dicing")
        col.prop(scene.psychopath, "dicing_rate")

        col.label(text="Motion Blur")
        col.prop(scene.psychopath, "motion_blur_segments")
        col.prop(scene.psychopath, "shutter_start")
        col.prop(scene.psychopath, "shutter_end")


class RENDER_PT_psychopath_export_settings(PsychopathPanel, bpy.types.Panel):
    bl_label = "Export Settings"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "render"

    def draw(self, context):
        scene = context.scene
        layout = self.layout

        col = layout.column()
        col.prop(scene.psychopath, "export_path")


class DATA_PT_psychopath_camera_dof(PsychopathPanel, bpy.types.Panel):
    bl_label = "Depth of Field"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "data"

    @classmethod
    def poll(cls, context):
        engine = context.scene.render.engine
        return context.camera and (engine in cls.COMPAT_ENGINES)

    def draw(self, context):
        ob = context.active_object
        layout = self.layout

        col = layout.column()

        col.prop(ob.data, "dof_object")
        col.prop(ob.data, "dof_distance")
        col.prop(ob.data.psychopath, "aperture_radius")


def register():
    bpy.utils.register_class(RENDER_PT_psychopath_render_settings)
    bpy.utils.register_class(RENDER_PT_psychopath_export_settings)
    bpy.utils.register_class(DATA_PT_psychopath_camera_dof)

def unregister():
    bpy.utils.unregister_class(RENDER_PT_psychopath_render_settings)
    bpy.utils.unregister_class(RENDER_PT_psychopath_export_settings)
    bpy.utils.unregister_class(DATA_PT_psychopath_camera_dof)
