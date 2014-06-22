Overview
========

Psychopath is a ray tracing renderer, aimed at rendering animations and VFX for
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
rendering geometry by first splitting and dicing it into sub-pixel sized
micro-geometry (similar to the Reyes rendering architecture) prior to doing
ray testing.  Even with geometry caching completely disabled, Psychopath is
able to render using this technique very efficiently.

Current Features
----------------
- Bilinear patches
- Bicubic bezier patches
- Spherical light sources
- Focal blur / DoF
- Camera motion blur
- Deformation motion blur
- Transforms and transform motion blur
- Full hierarchical instancing

Features Currently In-Progress
------------------------------
- Multiple importance sampling



PsychoBlend
===========

Included in the repository is an addon for [Blender](http://www.blender.org)
called "PsychoBlend" that allows you to do basic rendering of Blender scenes
with Psychopath.  Most Blender features are not yet supported.

Features Supported
------------------
- Quad mesh export (exported as bilinear patches, so only flat shading)
- Point lights exported as spherical lights (point lights have a "radius" setting)
- Focal blur / DoF
- Camera motion blur
- Transform motion blur
- Deformation motion blur
- Limited auto-detected instancing