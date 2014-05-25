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
            min=1, max=65536, default=16)

    dicing_rate = FloatProperty(
            name="Dicing Rate", description="The target microgeometry width in pixels",
            min=0.0001, max=100.0, soft_min=0.125, soft_max=1.0, default=0.5)


# Addon Preferences
class PsychopathPreferences(AddonPreferences):
    bl_idname = __name__

    filepath_psychopath = StringProperty(
                name="Psychopath Location",
                description="Path to renderer executable",
                subtype='FILE_PATH',
                )

    def draw(self, context):
        layout = self.layout
        layout.prop(self, "filepath_psychopath")


##### REGISTER #####
def register():
    bpy.utils.register_class(PsychopathPreferences)
    bpy.utils.register_class(RenderPsychopathSettingsScene)
    bpy.types.Scene.psychopath = PointerProperty(type=RenderPsychopathSettingsScene)
    render.register()
    ui.register()


def unregister():
    bpy.utils.unregister_class(PsychopathPreferences)
    bpy.utils.unregister_class(RenderPsychopathSettingsScene)
    del bpy.types.Scene.psychopath
    render.unregister()
    ui.unregister()
