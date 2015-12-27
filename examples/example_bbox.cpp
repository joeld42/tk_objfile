
#include <stdio.h>
#include <stdlib.h>

#define TK_OBJFILE_IMPLEMENTATION
#include "tk_objfile.h"

void testErrorMessage( size_t lineNum, const char *message )
{
    printf("ERROR on line %zu: %s\n", lineNum, message );
}

void testSwitchMaterial( const char *materialName )
{
    printf(">>> Current material: %s\n", materialName );
}

void printVert( const char *msg, TK_TriangleVert v )
{
    printf("%10s : pos (%3.2f, %3.2f, %3.2f) nrm (%3.2f, %3.2f, %3.2f) uv (%3.2f, %3.2f )\n",
           msg,
           v.pos[0], v.pos[1], v.pos[2],
           v.nrm[0], v.nrm[1], v.nrm[2],
           v.st[0], v.st[1] );
           
}

void testProcessTriangle( TK_TriangleVert a, TK_TriangleVert b, TK_TriangleVert c )
{
    static int triangleCount = 0;
    printf("--- Tri %d -- \n", triangleCount++ );
    
    printVert( "A", a );
    printVert( "B", b );
    printVert( "C", c );
}


void testTriangleGroup( const char *mtlname, size_t numTriangles, TK_IndexedTriangle *triangles )
{
    printf("Triangles for material %s (%zu triangles)\n", mtlname, numTriangles );
    
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
    // test process triangle
//    TK_TriangleVert testA = {}, testB = {}, testC = {};
//    testSwitchMaterial( "fakeMaterial");
//    testProcessTriangle( testA, testB, testC );
    
    // Lower level, "callback" based API
    TK_ObjDelegate objDelegate = {};
    objDelegate.error = testErrorMessage;

    
    // Read the obj file
    if (argc < 2) {
        testErrorMessage(0, "No obj file specified.");
        return 1;
    }
    size_t objFileSize = 0;
    void *objFileData = readEntireFile( argv[1], &objFileSize );
    if (!objFileData) {
        printf("Could not open OBJ file '%s'\n", argv[1] );
    }
    
    printf("FILE SIZE %zu\n", objFileSize );
    printf("FILE DATA:----\n%s\n-----\n", (const char*)objFileData);
    
    // Prepass to determine memory reqs
    TK_ParseObj( objFileData, objFileSize, &objDelegate );
    printf("Scratch Mem: %zu\n", objDelegate.scratchMemSize );
    objDelegate.scratchMem = malloc( objDelegate.scratchMemSize );
    
    // Parse again with memory
    objDelegate.material = testSwitchMaterial;
    objDelegate.triangle = testProcessTriangle;
    
    TK_ParseObj( objFileData, objFileSize, &objDelegate );
    
    printf("Num Verts %zu\n", objDelegate.numVerts);
    printf("Num Norms %zu\n", objDelegate.numNorms );
    printf("Num STs %zu\n", objDelegate.numSts );
    
    
    
    return 0;
}
