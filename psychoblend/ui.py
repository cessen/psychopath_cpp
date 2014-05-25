# Use some of the existing buttons.
from bl_ui import properties_render
properties_render.RENDER_PT_render.COMPAT_ENGINES.add('PSYCHOPATH_RENDER')
properties_render.RENDER_PT_dimensions.COMPAT_ENGINES.add('PSYCHOPATH_RENDER')
properties_render.RENDER_PT_output.COMPAT_ENGINES.add('PSYCHOPATH_RENDER')
del properties_render

def register():
	pass

def unregister():
	pass
