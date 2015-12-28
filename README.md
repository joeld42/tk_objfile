# tk_objfile
Single file, header-only .OBJ mesh loader with zero dependancies

Work in progress, check back soon...

TODO:

before release:
- clean up DBG/printfs
- finish/test simple bbox example
- other example (?)
- viewer example (?)
- docs in header

improvements:
- handle negative indices (old style .LWO) 
- some large test files

future features:
- Recalculate normals if missing, or if requested
- Support tangents, calculate reasonable tangents

Usage:
---
TODO

Examples:
---

example_bbox.cpp - Prints the bounding box for an obj file. Demonstrates the
"Triangle Soup" style API.

objviewer - Object viewer using IMGUI/glfw. Demonstrates the triangle group APIs.
I haven't really set up the objviewer to build easiy, so there are some hardcoded
paths, etc.
