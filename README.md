Overview
========

Psychopath is a path tracer, aimed at rendering animations and VFX for
film.  It is currently still in an early prototyping stage of development.  You
can view a brief video of some animations rendered with it in
[this (now somewhat out-dated)
video](https://www.youtube.com/watch?v=rydLFAdhseo).

The long-term goals of Psychopath are to support efficient global illumination
rendering of scenes that are significantly larger than available RAM and/or
that contain procedural elements that need to be generated on-the-fly during
rendering.

The approach that Psychopath takes to enable this is to try to access the scene
data in as coherent a fashion as possible via breadth-first ray tracing,
allowing the cost of HDD access, expensive procedurals, etc. to be amortized
over large batches of rays.

In its current state this principle and its effectiveness are demonstrated by
by refining geometry to sub-pixel microgeometry on the fly during the rendering process, somewhat analogous to the Reyes rendering architecture.  Even with geometry caching completely disabled, Psychopath is able to render using this technique very efficiently.

Current Features
----------------
- Spheres
- Bilinear patches
- Bicubic bezier patches
- Spherical light sources
- Rectangular light sources
- Simple shader system (assign BSDF's to objects)
- Multiple importance sampling
- Spectral rendering (via monte carlo, not binning)
- Focal blur / DoF
- Camera motion blur
- Deformation motion blur
- Transforms and transform motion blur
- Full hierarchical instancing

Features Currently In-Progress
------------------------------
- A novel method for efficiently handling many (i.e. thousands or millions) of
  lights in a scene.  See [this thread](http://ompf2.com/viewtopic.php?f=3&t=1938) for an overview.



PsychoBlend
===========

Included in the repository is an addon for [Blender](http://www.blender.org)
called "PsychoBlend" that allows you to do basic rendering of Blender scenes
with Psychopath.  Most Blender features are not yet supported.

Features Supported
------------------
- Quad mesh export (exported as bilinear patches, so only flat shading)
- Point lights exported as spherical lights (point lights have a "radius" setting)
- Area lights, exported as rectangular area lights
- Simple material system for assigning different BSDF's to different objects
- Focal blur / DoF
- Camera motion blur
- Transform motion blur
- Deformation motion blur
- Exports dupligroups with full hierarchical instancing
- Limited auto-detected instancing of meshes