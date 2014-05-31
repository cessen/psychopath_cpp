bl_info = {
    "name": "PsychoBlend",
    "version": (0, 1),
    "author": "Nathan Vegdahl",
    "blender": (2, 70, 0),
    "description": "Psychopath renderer integration",
    "location": "",
    "wiki_url": "https://github.com/cessen/psychopath/wiki",
    "tracker_url": "https://github.com/cessen/psychopath/issues",
    "category": "Render"}


if "bpy" in locals():
    import imp
    imp.reload(ui)
    imp.reload(psy_export)
    imp.reload(render)
else:
    from . import ui, psy_export, render

import bpy
from bpy.types import (AddonPreferences,
                       PropertyGroup,
                       Operator,
                       )
from bpy.props import (StringProperty,
                       BoolProperty,
                       IntProperty,
                       FloatProperty,
                       FloatVectorProperty,
                       EnumProperty,
                       PointerProperty,
                       )


# Custom Scene settings
class RenderPsychopathSettingsScene(PropertyGroup):
    spp = IntProperty(
        name="Samples Per Pixel", description="Total number of samples to take per pixel",
        min=1, max=65536, default=16
        )

    dicing_rate = FloatProperty(
        name="Dicing Rate", description="The target microgeometry width in pixels",
        min=0.0001, max=100.0, soft_min=0.125, soft_max=1.0, default=0.25
        )

    motion_blur_segments = IntProperty(
        name="Motion Segments", description="The number of segments to use in motion blur.  Zero means no motion blur.  Will be rounded down to the nearest power of two.",
        min=0, max=256, default=0
        )

    shutter_start = FloatProperty(
        name="Shutter Open", description="The time during the frame that the shutter opens, for motion blur",
        min=-1.0, max=1.0, soft_min=0.0, soft_max=1.0, default=0.0
        )

    shutter_end = FloatProperty(
        name="Shutter Close", description="The time during the frame that the shutter closes, for motion blur",
        min=-1.0, max=1.0, soft_min=0.0, soft_max=1.0, default=0.5
        )

    export_path = StringProperty(
        name="Export Path", description="The path to where the .psy files should be exported when rendering.  If left blank, /tmp or the equivalent is used.",
        subtype='FILE_PATH'
        )

# Custom Camera properties
class PsychopathCamera(bpy.types.PropertyGroup):
    aperture_radius = FloatProperty(
        name="Aperture Radius", description="Size of the camera's aperture, for DoF",
        min=0.0, max=10000.0, soft_min=0.0, soft_max=2.0, default=0.0
        )


# Addon Preferences
class PsychopathPreferences(AddonPreferences):
    bl_idname = __name__

    filepath_psychopath = StringProperty(
                name="Psychopath Location",
                description="Path to renderer executable",
                subtype='DIR_PATH',
                )

    def draw(self, context):
        layout = self.layout
        layout.prop(self, "filepath_psychopath")


##### REGISTER #####
def register():
    bpy.utils.register_class(PsychopathPreferences)
    bpy.utils.register_class(RenderPsychopathSettingsScene)
    bpy.utils.register_class(PsychopathCamera)
    bpy.types.Scene.psychopath = PointerProperty(type=RenderPsychopathSettingsScene)
    bpy.types.Camera.psychopath = PointerProperty(type=PsychopathCamera)
    render.register()
    ui.register()


def unregister():
    bpy.utils.unregister_class(PsychopathPreferences)
    bpy.utils.unregister_class(RenderPsychopathSettingsScene)
    bpy.utils.unregister_class(PsychopathCamera)
    del bpy.types.Scene.psychopath
    del bpy.types.Camera.psychopath
    render.unregister()
    ui.unregister()
