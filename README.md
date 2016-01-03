# tk_objfile
Single file, header-only .OBJ mesh loader with zero dependancies

Joel Davis (joeld42@gmail.com) Twitter: @joeld42

Tapnik Software (www.tapnik.com)

Note: for backstory about this library read this blog post:
http://www.tapnik.com/blog/tk-objfile.html

Features:
 - Single header implementation, zero dependancies 
 - No allocations, uses scratch memory passed in by caller
 - Reasonably fast -- parses ajax_jotero_com.obj, (50MB, 544k triangles) in 700ms
 - Handles multiple materials, useful for OBJs with more than one texture
 - Will automatically triangulate convex faces (fan style).


Limitations:
 - Doesn't handle subobjects or groups (will parse, but groupings are lost)
 - Not very well tested
 - Crappy examples, no real build system 


Usage:
----------
This file parses an OBJ with a "triangle soup" style API. 

To use, simply include "tk_objfile.h". Exactly one of the including
C or CPP files needs to define TK_OBJFILE_IMPLEMENTATION before including
to generate the implementation. For example:

```C
#define TK_OBJFILE_IMPLEMENTATION
#include "tk_objfile.h"
```

Basic usage is to create a TK_ObjDelegate with the callbacks:

```C
// Called once for each material that has one or more triangles using it.
void (*material)( const char *mtlName, size_t numTriangles, void *userData );

// Called once for each triangle using the material.    
void (*triangle)( TK_TriangleVert a, TK_TriangleVert b, TK_TriangleVert c, void *userData );

// Will report errors from parsing.
void (*error)( size_t lineNumber, const char *message, void *userData );
```

All the callbacks are optional. All callbacks pass in a void *userData 
from the objDelegate for a convienent way to pass in some context.

MEMORY: The parser doesn't allocate any memory. Instead, you must pass in 
a "scratchMemory" buffer in the objDelegate that is large enough to hold 
the results from the parsing and a small amount of working memory. There 
are two approaches to this: 

First, you can call TK_ParseObj with 0 for scratchMemSize, and it will 
only do a pre-parse and fill in scratchMemSize with how much space it
needs. Then, allocate at least that much space in scratchMem and call
TK_ParseObj again and it will do the actual parse.

Alternatively, if you know how big the objects you'll be parsing is, 
or if you happen to have a large scratch buffer on hand, then you can
just pass that in in the first place. This saves one redundant pre-parse, 
but it doesn't really save much time, but it might be simpler.

Example:
----

Here's a example of how it might be called:
```C
	// Create delegate and assign callbacks
    TK_ObjDelegate objDelegate = {};
    objDelegate.error = myCallbackErrorMessage;
    objDelegate.material = myCallbackSwitchMaterial;
    objDelegate.triangle = myCallbackProcessTriangle;

    // Read the .OBJ file from disk
    size_t objFileSize = 0;
    void *objFileData = readEntireFile( "cube1.obj", &objFileSize );
    
    // Prepass to determine memory requirements
    objDelegate.scratchMemSize = 0;
    TK_ParseObj( objFileData, objFileSize, &objDelegate );

    // Allocate scratch memory
    objDelegate.scratchMem = malloc( objDelegate.scratchMemSize );

    // Parse again with memory. This will call material() and
    // triangle() callbacks
    TK_ParseObj( objFileData, objFileSize, &objDelegate );
```

Discussion:
------

The "triangle soup" style throws away the index vertex info. Originally I
included an API to preserve the indexed data, but since it's indexed
differently than OpenGL/DX you probably have to reindex it anyways, so
I removed it to keep things simple. In a real world pipeline you might want
to run it through a real triangle stripper or something.

I still want to add a simple wrapper API that uses cstdlib and just loads
the obj with a single call.

Examples:
---

example_bbox.cpp - Prints the bounding box for an obj file. This is a good
starting point to see how to use it.

objviewer - Object viewer using IMGUI/glfw. This is a pretty craptastic
viewer, it needs a lot of work, but it's a start. Handles multiple 
materials, will tint each material a different color. When loading objects, 
if there is a .png image with the same name as the material name, it will 
load that as a texture (see hugzilla.obj for an example)

Contributing:
------

If you would like to contribute, here's some things that would be helpful:
- If you find OBJ files that don't work, please send them to me or create an issue in github
- Improvements in the OBJ viewer are welcome


TODO:
-------

smaller improvements:
- handle negative indices (old style .LWO) 
- add an optional simple one-call wrapper API that uses stdlib
- Add a flag to flip UVs automatically for opengl
- Add a flag to preserve faceIDs, or even an alternate API that preserves faces
- Improve error handling for insuffient scratchmem

bigger future features:
- Recalculate normals if missing, or if requested
- Support tangents, calculate usable tangents
- Support subobjects ('o' lines) and groups ('g' lines)
