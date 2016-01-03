
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#define TK_OBJFILE_IMPLEMENTATION
#include "tk_objfile.h"

// Note: while tk_objfile doesn't use any cstdlib functions itself, we use them in the example
// code to implement file loading, printing and allocation.

// Compute the bounding box for an obj, to show how to use the simple "triangle soup" API.
struct BoundingBox {
    float minPos[3];
    float maxPos[3];
};

inline float minFloat( float a, float b)
{
    if (a < b) return a;
    else return b;
}

inline float maxFloat( float a, float b)
{
    if (a >= b) return a;
    else return b;
}

void bboxErrorMessage( size_t lineNum, const char *message, void *userData )
{
    printf("ERROR on line %zu: %s\n", lineNum, message );
}

void bboxSwitchMaterial( const char *materialName, size_t numTriangles, void *userData )
{
    printf(">>> Current material: %s (%zu triangles)\n", materialName, numTriangles );
}

void bboxProcessTriangle( TK_TriangleVert a, TK_TriangleVert b, TK_TriangleVert c, void *userData )
{
    BoundingBox *bbox = (BoundingBox*)userData;
    for (int i=0; i < 3; i++) {
        bbox->minPos[i] = minFloat( a.pos[i], bbox->minPos[i] );
        bbox->maxPos[i] = maxFloat( a.pos[i], bbox->maxPos[i] );
    }
}

void *readEntireFile( const char *filename, size_t *out_filesz )
{
    FILE *fp = fopen( filename, "r" );
    if (!fp) return NULL;
    
    // Get file size
    fseek( fp, 0L, SEEK_END );
    size_t filesz = ftell(fp);
    fseek( fp, 0L, SEEK_SET );

    void *fileData = malloc( filesz );
    if (fileData)
    {
        size_t result = fread( fileData, filesz, 1, fp );
        
        // result is # of chunks read, we're asking for 1, fread
        // won't return partial reads, so it's all or nothing.
        if (!result)
        {
            free( fileData);
            fileData = NULL;
        }
        else
        {
            // read suceeded, set out filesize
            *out_filesz = filesz;
        }
    }
    
    return fileData;
    
}

int main(int argc, const char * argv[])
{
    // The bounding box that we will fill in
    BoundingBox bbox = { { FLT_MAX, FLT_MAX, FLT_MAX },
                         { FLT_MIN, FLT_MIN, FLT_MIN } };

    // Callbacks for API
    TK_ObjDelegate objDelegate = {};
    objDelegate.userData = (void*)&bbox;
    objDelegate.error = bboxErrorMessage;

    // Read the obj file
    if (argc < 2) {
        bboxErrorMessage(0, "No .OBJ file specified.", NULL );
        return 1;
    }
    size_t objFileSize = 0;
    void *objFileData = readEntireFile( argv[1], &objFileSize );
    if (!objFileData) {
        printf("Could not open .OBJ file '%s'\n", argv[1] );
    }
    
    // Prepass to determine memory reqs
    TK_ParseObj( objFileData, objFileSize, &objDelegate );
    printf("Scratch Mem: %zu\n", objDelegate.scratchMemSize );
    objDelegate.scratchMem = malloc( objDelegate.scratchMemSize );
    
    // Parse again with memory
    objDelegate.material = bboxSwitchMaterial;
    objDelegate.triangle = bboxProcessTriangle;
    
    TK_ParseObj( objFileData, objFileSize, &objDelegate );
    
    printf("Num Verts %zu\n", objDelegate.numVerts);
    printf("Num Norms %zu\n", objDelegate.numNorms );
    printf("Num STs %zu\n", objDelegate.numSts );
    
    printf("Bounding Box: min [%3.2f %3.2f %3.2f] max [%3.2f %3.2f %3.2f]\n",
           bbox.minPos[0], bbox.minPos[1], bbox.minPos[2],
           bbox.maxPos[0], bbox.maxPos[1], bbox.maxPos[2] );

    
    return 0;
}
