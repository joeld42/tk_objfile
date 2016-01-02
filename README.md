# tk_objfile
Single file, header-only .OBJ mesh loader with zero dependancies

Joel Davis (joeld42@gmail.com)
Tapnik Software

Features:
 - Single header implementation, zero dependancies 
 - No allocations, uses scratch memory passed in by caller
 - Reasonably fast -- parses Ajax_Jotero.obj, (50MB, 544k triangles) in 2s
 - Handles multiple materials, useful for OBJs with more than one texture

 Limitations:
 - Doesn't handle subobjects or groups (will parse, but groupings are lost)
 - Not very well tested
 - Crappy examples, no real build system


Usage:
----------
This file parses an OBJ with a "triangle soup" style API. Basic usage 
is to create a TK_ObjDelegate with two callbacks:

material(...) - Called once for each material that has one or more
triangles using it.

triangle(...) - Called once for each triangle using the material.

Both callbacks pass in a userData from the objDelegate. 

Discussion:
------
The "triangle soup" style throws away the index vertex info. Originally I
included an API to preserve the indexed data, but since it's indexed
differently than OpenGL/DX you probably have to reindex it anyways, so
I removed it to keep things simple. In a real world pipeline you might want
to run it through a 

I still want to add a simple wrapper API that uses cstdlib and just loads
the obj with a single call.

Examples:
---

example_bbox.cpp - Prints the bounding box for an obj file. 

objviewer - Object viewer using IMGUI/glfw. This is a pretty craptastic
viewer, it needs a lot of work, but it's a start. Handles multiple 
materials, will tint each material a different color. When loading objects, 
if there is a .png image with the same name as the material name, it will 
load that as a texture (see hugzilla.obj for an example)

Contributing:
------

If you would like to contribute, here's some things that would be helpful:
- If you find OBJ files that don't work, please send them to me or create an issue in github
- Improvements in the OBJ viewer


TODO:
-------

smaller improvements:
- handle negative indices (old style .LWO) 
- add an optional simple one-call wrapper API that uses stdlib
- some large test files
- Add a flag to flip UVs automatically for opengl
- Add a flag to preserve faceIDs, or even an alternate API that preserves faces

bigger future features:
- Recalculate normals if missing, or if requested
- Support tangents, calculate usable tangents
- Support subobjects ('o' lines) and groups ('g' lines)
