# tk_objfile
Single file, header-only .OBJ mesh loader with zero dependancies

Features:
 - Single header implementation, zero dependancies 
 - No allocations, uses scratch memory passed in by caller
 - Reasonably fast
 - Handles multiple materials

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
differently than OpenGL/DX you'd probably have to repack it anyways, so
I removed it to keep things simple.

Examples:
---

example_bbox.cpp - Prints the bounding box for an obj file. 

objviewer - Object viewer using IMGUI/glfw. This is a pretty craptastic
viewer, it needs a lot of work, but it's a start. Handles multiple 
materials, will tint each material a different color. When loading objects, 
if there is a .png image with the same name as the material name, it will 
load that as a texture (see hugzilla.obj for an example)

TODO:
-------

improvements:
- handle negative indices (old style .LWO) 
- some large test files
- Add a flag to flip UVs automatically for opengl

future features:
- Recalculate normals if missing, or if requested
- Support tangents, calculate usable tangents
- Support subobjects ('o' lines) and groups ('g' lines)
