bl_info = {
    "name": "PsychoBlend",
    "version": (0, 1),
    "author": "Nathan Vegdahl",
    "blender": (2, 71, 0),
    "description": "Psychopath renderer integration",
    "location": "",
    "wiki_url": "https://github.com/cessen/psychopath/wiki",
    "tracker_url": "https://github.com/cessen/psychopath/issues",
    "category": "Rendering"}


if "bpy" in locals():
    import imp
    imp.reload(ui)
    imp.reload(psy_export)
else:
    from . import ui, psy_export

import bpy


##### REGISTER #####

def register():
    ui.register()


def unregister():
    ui.unregister()
