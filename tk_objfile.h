//
//  tk_objfile.h
//  tk_objfile
//
//  Created by Joel Davis on 12/5/15.
//  Copyright © 2015 Joel Davis. All rights reserved.
//

#ifndef TK_OBJFILE_H_INCLUDED
#define TK_OBJFILE_H_INCLUDED

// FIXME: (jbd) write an assert
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

// TKTriangleVert -- Simple triangle vert struct.
typedef struct
{
    float pos[3];
    float st[2];
    float nrm[3];
} TK_TriangleVert;
    
// TK_Triangle -- A triangle
typedef struct {
    TK_TriangleVert vertA;
    TK_TriangleVert vertB;
    TK_TriangleVert vertC;
} TK_Triangle;

typedef struct {
    size_t posIndex;
    size_t stIndex;
    size_t normIndex;
} TK_IndexedVert;
    
typedef struct {
    TK_IndexedVert vertA;
    TK_IndexedVert vertB;
    TK_IndexedVert vertC;
} TK_IndexedTriangle;

// TKObjDelegate -- Callbacks for the OBJ format parser. All callbacks are optional.
//
//  triangleGroup -- a bunch of triangles that share the same material
//
// Scratch Memory -- The parser needs some scratch memory to do it's work and to store the results.
// You must fill this in before you call the parser, as it doesn't do any allocations.
// If you call the parser with scratchMemory pointer of NULL, it will only do the pre-pass to
// determine how much memory it wants. It will fill in the scratchMemorySize, you may allocate
// it however you wish, and then call the parser again to do the actual parsing.
typedef struct
{
    void (*error)( size_t lineNumber, const char *message, void *userData );

    // "Triangle Soup" API -- calls triangles one at a time, grouped by material
    void (*material)( const char *mtlName, void *userData );
    void (*triangle)( TK_TriangleVert a, TK_TriangleVert b, TK_TriangleVert c, void *userData );
    
    // "Indexed" API -- batches of triangles, preserving the indexing of the obj file. Still
    // probably better to run it through a tri-stripper or something.
    void (*triangleGroup)( const char *mtlname, size_t numTriangles,
                           TK_IndexedTriangle *triangles, void *userData );
    
    // Scratch memory needed by parser.
    void *scratchMem;
    size_t scratchMemSize;
    
    // arbitrary user data passed through to callbacks
    void *userData;
    
    // bookkeeping during parsing
    size_t currentLineNumber;
    size_t numVerts;
    size_t numNorms;
    size_t numSts;
    size_t numFaces;
    
} TK_ObjDelegate;

    
// TK_ParseObj -- Parse an obj file into triangle soup.
//
// This is the "lower level" API, it will parse the obj formatted data and call
// delegate methods (such as processTriangle) for each triangle.
// TODO:(jbd) Add a SimpleDelegate that just packs the triangles into a list for convienance
void TK_ParseObj( void *objFileData, size_t objFileSize, TK_ObjDelegate objDelegate );
    
    
#ifdef __cplusplus
} // extern "C"
#endif

// =========================================================
//  IMPLEMENTATION
// =========================================================
#ifdef TK_OBJFILE_IMPLEMENTATION

// TKImpl_ParseType
typedef enum {
    TKimpl_ParseTypeCountOnly,
    TKimpl_ParseTypeFull,
} TKimpl_ParseType;

// TKImpl_MemArena
typedef struct {
    void *base;
    uint8_t *top;
    size_t remaining;
} TKImpl_MemArena;

void *TKImpl_PushSize( TKImpl_MemArena *arena, size_t structSize )
{
    if (structSize > arena->remaining) {
        return NULL;
    }
    
    void *result = (void*)arena->top;
    arena->top += structSize;
    arena->remaining -= structSize;
    
    return result;
}

#define TKImpl_PushStruct(arena,T) (T*)TKImpl_PushSize(arena,sizeof(T))
#define TKImpl_PushStructArray(arena,T,num) (T*)TKImpl_PushSize(arena,sizeof(T)*num)

// TKImpl_Material
typedef struct TKImpl_MaterialStruct {
    char  *mtlName; // not 0-delimited, be careful
    size_t numTriangles;
    TK_IndexedTriangle *triangles;
} TKImpl_Material;

#define TKIMPL_MAX_UNIQUE_MTLS (100)
#define TKIMPL_MAX_MATERIAL_NAME (256)

typedef struct {
    
    // vertex list from obj
    size_t numVertPos;
    size_t numVertSt;
    size_t numVertNrm;
    
    float *vertPos;
    float *vertSt;
    float *vertNrm;
    
    TKImpl_Material *materials;
    size_t numMaterials;
    
} TKImpl_ParseInfo;

int TKimpl_isIdentifier( char ch ) {
    if (ch=='\0'||ch==' '||ch=='\n'||ch=='\t') return 0;
    return 1;
}

// FIXME:(jbd) Remove this debug crap
char *TKimpl_printMtl( char *mtlName )
{
    static char buff[256];
    char *ch2 = buff;
    for (char *ch=mtlName; TKimpl_isIdentifier( *ch ); ch++) {
        *ch2++ = *ch;
    }
    *ch2 = '\0';
    return buff;
}

char *TKimpl_printToken( char *token, char *endtoken)
{
    static char buff[256];
    char *ch2 = buff;
    for (char *ch=token; ch < endtoken; ch++) {
        *ch2++ = *ch;
    }
    *ch2 = '\0';
    return buff;
    
}

char *TKimpl_stringDelimMtlName( char *dest, char *mtlName, size_t maxLen )
{
    char *ch2 = dest;
    for (char *ch=mtlName; TKimpl_isIdentifier( *ch ); ch++) {
        *ch2++ = *ch;
        if (ch2-dest >= (maxLen-1)) break;
    }
    *ch2 = '\0';
    return dest;
}


// This is kind of like strtok, which is the best function in C.
void TKimpl_nextToken( char **out_token, char **out_endtoken, char *endline )
{
    char *token = *out_token;
    char *endtoken = *out_endtoken;
    
    while (((*endtoken=='\n')||(*endtoken == ' ')) && (endtoken < endline)) {
        endtoken++;
    }
    token = endtoken;
    if (token >= endline) {
        *out_token = NULL;
        *out_endtoken = NULL;
        return;
    }
    
    while ((endtoken < endline) && (*endtoken != ' ')) {
        endtoken++;
    }
    
    *out_token = token;
    *out_endtoken = endtoken;
}

// NOTE:(jbd) Indices may be negative, old versions of Lightwave
// would produce negative indexes to index from the end of the list.
long TKimpl_parseIndex( char *token, char *endtoken )
{
    long result = 0;
    long sign = 1;
    for (char *ch = token; ch < endtoken; ch++) {
        if (*ch=='-') {
            sign = -1;
        } else if ((*ch>='0')&&(*ch<='9')) {
            int val = *ch - '0';
            result = (result * 10) + val;
        }
    }
    return sign * result;
}

void TKimpl_parseFaceIndices( char *token, char *endtoken,
                             size_t *pndx, size_t *stndx, size_t *nndx)
{
    // count slashes and find numeric tokens
    int numSlash = 0;
    char *numberDelim[4];
    long number[3];
    numberDelim[0] = token;
    for (char *ch = token; ch < endtoken; ch++) {
        if (*ch=='/') {
            numSlash++;
            numberDelim[numSlash] = ch+1;
        }
        if (numSlash>=2) break;
    }
    numberDelim[numSlash+1] = endtoken+1;
    
    // Parse the slash-delimted groups into indexes
    for (int i=0; i < numSlash+1; i++) {
        number[i] = TKimpl_parseIndex( numberDelim[i], numberDelim[i+1]-1 );
        if (number[i]>0) number[i]--; // OBJ file indices are 1-based
        printf("numToken %d: %s = %ld\n", i, TKimpl_printToken(numberDelim[i], numberDelim[i+1]-1), number[i]);
    }
    
    // decide which lists indexes represent based on number of slashes
    if (pndx) {
        // first number is always pos
        *pndx = number[0];
    }
    
    if (numSlash==1) {
        // one slash, pos+normal
        if (nndx) *nndx = number[1];
    } else if (numSlash==2) {
        // two slash, pos/st/nrm
        if (stndx) *stndx = number[1];
        if (nndx) *nndx = number[2];
    }
}

int TKimpl_compareToken( const char *target, char *token, char *endtoken )
{
    while (token < endtoken)
    {
        if (*target++ != *token++) return 0;
    }
    return 1;
}

void TKimpl_copyString( char *dest, const char *src )
{
    do {
        *dest++ = *src++;
    } while (*src);
    *dest='\0';
}


int TKimpl_compareMtlName( char *mtlA, char *mtlB )
{
    while (TKimpl_isIdentifier(*mtlA) && TKimpl_isIdentifier(*mtlB))
    {
        if (*mtlA != *mtlB) return 0;
        mtlA++;
        mtlB++;
    }
    
    return 1;
}


void TKimpl_memoryError( TK_ObjDelegate *objDelegate )
{
    if (objDelegate->error) {
        objDelegate->error( objDelegate->currentLineNumber, "Not enough scratch memory.",
                           objDelegate->userData );
    }
}

// CBB:(jbd) Handle exponent notation.
// Return 1 on success, 0 on failure
bool TKImpl_parseFloat( TK_ObjDelegate *objDelegate, char *token, char *endtoken, float *out_result )
{
    if ((!token) || (!endtoken))
    {
        // Token or endtoken is NULL
        if (objDelegate->error) {
            objDelegate->error( objDelegate->currentLineNumber, "Expected float.",
                               objDelegate->userData );
        }
        return 0;
    }
    else
    {
        char *ch = token;
        float value = 0.0;
        float mag= 0.1;
        float sign = 1.0;
        int inDecimal = 0;
        while (ch < endtoken) {
            if (*ch=='-') {
                sign = -1.0;
            } else if (*ch=='.') {
                inDecimal = 1;
            } else if ((*ch>='0') && (*ch<='9')) {
                float digitValue = (float)((*ch)-'0');
                if (inDecimal) {
                    value = (value + digitValue*mag);
                    mag /= 10.0;
                } else {
                    value = (value*10.0) + digitValue;
                }
            } else if (*ch=='+') {
                // Ignorable chars
            } else {
                if (objDelegate->error) {
                    char errBuff[35];
                    TKimpl_copyString( errBuff, "Unexpected character '_' in float." );
                    errBuff[22] = *ch;
                    objDelegate->error( objDelegate->currentLineNumber, errBuff,
                                       objDelegate->userData );
                }
                return 0;
            }
            ch++;
        }
        *out_result = sign * value;
    }
    return 1;
}


void TKimpl_ParseObjPass( void *objFileData, size_t objFileSize,
                         TKImpl_ParseInfo *info,
                         TKImpl_Material *uniqueMtls, size_t *numUniqueMtls,
                         TK_ObjDelegate *objDelegate, TKimpl_ParseType parseType )
{
    // Make default material
    TKImpl_Material *currMtl = NULL;
    if (parseType == TKimpl_ParseTypeCountOnly)
    {
        uniqueMtls[0].mtlName = (char *)"mtl.default ";
        uniqueMtls[0].numTriangles = 0;
        (*numUniqueMtls)++;
    }
    currMtl = &uniqueMtls[0];
    
    // Reset the delegate state
    objDelegate->currentLineNumber = 1;
    
    // Split file into lines
    char *start = (char*)objFileData;
    char *line = start;
    char *endline = line;
    
    while (line - start < objFileSize)
    {
        // Advance to the next endline
        do {
            endline++;
        } while ((*endline) && (*endline!='\n') && (endline - start < objFileSize));
        
        // skip leading whitespace
        while ( ((*line==' ') || (*line=='\t')) && (line != endline)) {
            line++;
        }

        printf("LINE: [%.*s]\n",  (int)(endline-line), line );
        
        // Skip Comments
        if (line[0]!='#')
        {
            char *token, *endtoken;
            token = line;
            endtoken = line-1;
            while (token)
            {
                TKimpl_nextToken( &token, &endtoken, endline);
                if (!token) break;
                
                // Handle tokens...
                if (TKimpl_compareToken("v", token, endtoken))
                {
                    if (parseType==TKimpl_ParseTypeCountOnly)
                    {
                        // Just count the vert
                        objDelegate->numVerts++;
                    }
                    else
                    {
                        // v X Y Z -- vertex position
                        float *vertPos = info->vertPos + (info->numVertPos*3);
                        
                        TKimpl_nextToken( &token, &endtoken, endline);
                        if (!TKImpl_parseFloat( objDelegate, token, endtoken, &(vertPos[0]) )) {
                            return;
                        }

                        TKimpl_nextToken( &token, &endtoken, endline);
                        if (!TKImpl_parseFloat( objDelegate, token, endtoken, &(vertPos[1]) )) {
                            return;
                        }
                        
                        TKimpl_nextToken( &token, &endtoken, endline);
                        if (!TKImpl_parseFloat( objDelegate, token, endtoken, &(vertPos[2]) )) {
                            return;
                        }
                        
                        // TODO: push vert onto vert list
                        printf("VERT: %f %f %f\n", vertPos[0], vertPos[1], vertPos[2] );
                        info->numVertPos++;
                    }
                    
                } else if (TKimpl_compareToken("vn", token, endtoken)) {

                    if (parseType==TKimpl_ParseTypeCountOnly)
                    {
                        objDelegate->numNorms++;
                    }
                    else
                    {
                        // vn X Y Z -- vertex normal
                        float *vertNrm = info->vertNrm + (info->numVertNrm*3);
                        
                        TKimpl_nextToken( &token, &endtoken, endline);
                        if (!TKImpl_parseFloat( objDelegate, token, endtoken, &(vertNrm[0]) )) {
                            return;
                        }
                        
                        TKimpl_nextToken( &token, &endtoken, endline);
                        if (!TKImpl_parseFloat( objDelegate, token, endtoken, &(vertNrm[1]) )) {
                            return;
                        }
                        
                        TKimpl_nextToken( &token, &endtoken, endline);
                        if (!TKImpl_parseFloat( objDelegate, token, endtoken, &(vertNrm[2]) )) {
                            return;
                        }
                        
                        // TODO: push normal onto nrm list
                        printf("NRM: %f %f %f\n", vertNrm[0], vertNrm[1], vertNrm[2] );
                        info->numVertNrm++;
                    }
                } else if (TKimpl_compareToken("vt", token, endtoken)) {
                    
                    if (parseType==TKimpl_ParseTypeCountOnly)
                    {
                        objDelegate->numSts++;
                    }
                    else
                    {
                        // vn S T -- vertex texture coord
                        float *vertSt = info->vertSt + (info->numVertSt*2);

                        TKimpl_nextToken( &token, &endtoken, endline);
                        if (!TKImpl_parseFloat( objDelegate, token, endtoken, &(vertSt[0]) )) {
                            return;
                        }
                        
                        TKimpl_nextToken( &token, &endtoken, endline);
                        if (!TKImpl_parseFloat( objDelegate, token, endtoken, &(vertSt[1]) )) {
                            return;
                        }
                        printf("ST: %f %f\n", vertSt[0], vertSt[1] );
                        info->numVertSt++;
                    }
                } else if (TKimpl_compareToken("usemtl", token, endtoken)) {
                    
                    // usemtl, is this an existing mtl group or a new one?
                    TKimpl_nextToken( &token, &endtoken, endline);
                    
                    TKImpl_Material *useMtl = NULL;
                    for (int i=1; i < *numUniqueMtls; i++) {
                        if (TKimpl_compareMtlName( uniqueMtls[i].mtlName, token )) {
                            printf("MATCH! (%s)\n", TKimpl_printMtl(uniqueMtls[i].mtlName) );

                            useMtl = &uniqueMtls[i];
                            break;
                        }
                    }
                    if ((!useMtl) && (parseType==TKimpl_ParseTypeCountOnly)) {
                        printf("Did not find a match for mtl..\n");
                        assert( *numUniqueMtls < TKIMPL_MAX_UNIQUE_MTLS );
                        useMtl = &uniqueMtls[(*numUniqueMtls)++];
                        useMtl->numTriangles = 0;
                        useMtl->mtlName = token;
                    }
                    currMtl = useMtl;
                    printf("Usemtl is %p\n", useMtl );
                } else if (TKimpl_compareToken("f", token, endtoken)) {
                    printf("---- face ---");
                    TK_IndexedTriangle tri;
                    TK_IndexedVert vert;
                    int count = 0;
                    do {
                        TKimpl_nextToken( &token, &endtoken, endline );
                        if (token) {
                            printf("TOKEN: %s\n", TKimpl_printToken(token, endtoken));

                            if (parseType==TKimpl_ParseTypeFull)
                            {
                                TKimpl_parseFaceIndices(token, endtoken,
                                                        &(vert.posIndex),
                                                        &(vert.stIndex),
                                                        &(vert.normIndex) );
                                if (count==0) {
                                    tri.vertA = vert;
                                } else if (count==1) {
                                    tri.vertB = vert;
                                } else if (count > 2) {
                                    tri.vertB = tri.vertC;
                                }
                                
                                if (count >= 2) {
                                    tri.vertC = vert;
                                    printf("adding triangle %zu\n", currMtl->numTriangles );
                                    currMtl->triangles[ currMtl->numTriangles++ ] = tri;
                                }
                            }
                            
                            count++;
                        }
                    } while (token);
                    
                    if ((count > 2) &&  (parseType==TKimpl_ParseTypeCountOnly)) {
                        currMtl->numTriangles += (count-2);
                    }
                    printf("... face done (%d verts).\n", count);
                }
            }
        }
        
        // next nonblank line
        do {
            line = ++endline;
            objDelegate->currentLineNumber++;
        } while (*endline=='\n');
    }
}

void TKimpl_GetIndexedTriangle( TK_Triangle *tri, TKImpl_ParseInfo *info, TK_IndexedTriangle ndxTri )
{
    tri->vertA.pos[0] = info->vertPos[ndxTri.vertA.posIndex*3 + 0];
    tri->vertA.pos[1] = info->vertPos[ndxTri.vertA.posIndex*3 + 1];
    tri->vertA.pos[2] = info->vertPos[ndxTri.vertA.posIndex*3 + 2];
    tri->vertA.nrm[0] = info->vertNrm[ndxTri.vertA.normIndex*3 + 0];
    tri->vertA.nrm[1] = info->vertNrm[ndxTri.vertA.normIndex*3 + 1];
    tri->vertA.nrm[2] = info->vertNrm[ndxTri.vertA.normIndex*3 + 2];
    tri->vertA.st[0] = info->vertSt[ndxTri.vertA.stIndex*3 + 0];
    tri->vertA.st[1] = info->vertSt[ndxTri.vertA.stIndex*3 + 1];

    tri->vertB.pos[0] = info->vertPos[ndxTri.vertB.posIndex*3 + 0];
    tri->vertB.pos[1] = info->vertPos[ndxTri.vertB.posIndex*3 + 1];
    tri->vertB.pos[2] = info->vertPos[ndxTri.vertB.posIndex*3 + 2];
    tri->vertB.nrm[0] = info->vertNrm[ndxTri.vertB.normIndex*3 + 0];
    tri->vertB.nrm[1] = info->vertNrm[ndxTri.vertB.normIndex*3 + 1];
    tri->vertB.nrm[2] = info->vertNrm[ndxTri.vertB.normIndex*3 + 2];
    tri->vertB.st[0] = info->vertSt[ndxTri.vertB.stIndex*3 + 0];
    tri->vertB.st[1] = info->vertSt[ndxTri.vertB.stIndex*3 + 1];

    tri->vertC.pos[0] = info->vertPos[ndxTri.vertC.posIndex*3 + 0];
    tri->vertC.pos[1] = info->vertPos[ndxTri.vertC.posIndex*3 + 1];
    tri->vertC.pos[2] = info->vertPos[ndxTri.vertC.posIndex*3 + 2];
    tri->vertC.nrm[0] = info->vertNrm[ndxTri.vertC.normIndex*3 + 0];
    tri->vertC.nrm[1] = info->vertNrm[ndxTri.vertC.normIndex*3 + 1];
    tri->vertC.nrm[2] = info->vertNrm[ndxTri.vertC.normIndex*3 + 2];
    tri->vertC.st[0] = info->vertSt[ndxTri.vertC.stIndex*3 + 0];
    tri->vertC.st[1] = info->vertSt[ndxTri.vertC.stIndex*3 + 1];

}

void TK_ParseObj( void *objFileData, size_t objFileSize, TK_ObjDelegate *objDelegate )
{
    TKImpl_Material uniqueMtls[TKIMPL_MAX_UNIQUE_MTLS];
    size_t numUniqueMtls = 0;
    
    // pre-pass, count how many verts, nrms and sts there are
    objDelegate->numVerts=0;
    objDelegate->numSts=0;
    objDelegate->numNorms=0;
    objDelegate->numFaces=0;
    
    // First pass, just count verts and unique materials...
    TKimpl_ParseObjPass( objFileData, objFileSize,  NULL,
                        uniqueMtls, &numUniqueMtls,
                        objDelegate, TKimpl_ParseTypeCountOnly );
    
    printf("Num Verts: %zu\n", objDelegate->numVerts );
    printf("Num Norms: %zu\n", objDelegate->numNorms );
    printf("Num Sts; %zu\n", objDelegate->numSts );
    printf("Num Faces: %zu\n", objDelegate->numFaces );

    
    printf(" Num Unique materials: %zu\n", numUniqueMtls );
    size_t totalTriangleCount = 0;
    for (int i=0; i < numUniqueMtls; i++) {
        printf("Mtl %d '%s' (%zu triangles)\n", i, TKimpl_printMtl(uniqueMtls[i].mtlName),
               uniqueMtls[i].numTriangles );
        totalTriangleCount += uniqueMtls[i].numTriangles;
    }
    
    // Calculate scratchMemSize
    size_t requiredScratchMem =
        sizeof(TKImpl_ParseInfo) +
        sizeof(float)*3*objDelegate->numVerts +
        sizeof(float)*3*objDelegate->numNorms +
        sizeof(float)*3*objDelegate->numSts +
        sizeof(TKImpl_Material) * numUniqueMtls +
        sizeof(TK_IndexedTriangle) * totalTriangleCount;


    // If no scratchMem, just stop now after the prepass
    if (!objDelegate->scratchMem) {
        objDelegate->scratchMemSize = requiredScratchMem;
        return;
    }
    else if (objDelegate->scratchMemSize < requiredScratchMem) {
        TKimpl_memoryError( objDelegate );
        return;
    }
    
    // Initialize our mem arena
    TKImpl_MemArena *arena = (TKImpl_MemArena *)objDelegate->scratchMem;
    arena->base = arena+1;
    arena->top = (uint8_t*)arena->base;
    arena->remaining = objDelegate->scratchMemSize - sizeof(TKImpl_MemArena);
    
    // Allocate our parseInfo struct
    TKImpl_ParseInfo *info = TKImpl_PushStruct(arena, TKImpl_ParseInfo);
    
    // Allocate vertex data lists
    info->numVertPos = 0;
    info->vertPos = (float*)TKImpl_PushSize(arena, sizeof(float)*3*objDelegate->numVerts);
    
    info->numVertNrm = 0;
    info->vertNrm = (float*)TKImpl_PushSize(arena, sizeof(float)*3*objDelegate->numNorms);
    
    info->numVertSt = 0;
    info->vertSt = (float*)TKImpl_PushSize(arena, sizeof(float)*2*objDelegate->numSts);
    
    info->materials = TKImpl_PushStructArray(arena, TKImpl_Material, numUniqueMtls );
    info->numMaterials = numUniqueMtls;
    
    for (int i = 0; i < numUniqueMtls; i++) {
        info->materials[i].mtlName = uniqueMtls[i].mtlName;
        info->materials[i].numTriangles = uniqueMtls[i].numTriangles;
        info->materials[i].triangles = TKImpl_PushStructArray( arena, TK_IndexedTriangle,
                                                              uniqueMtls[i].numTriangles );
    }
    
    // Now space is allocated for all the data, parse again and store
    TKimpl_ParseObjPass( objFileData, objFileSize,  info,
                        info->materials, &(info->numMaterials),
                        objDelegate, TKimpl_ParseTypeFull );
    
    // Now go through the results
    
    // First, call through the "triangle soup" api if requested
    if ((objDelegate->triangle) || (objDelegate->material)) {
        for (int mi=0; mi < info->numMaterials; mi++) {
            if (info->materials[mi].numTriangles > 0) {
                if (objDelegate->material) {
                    // Copy the mtlName into a nice 0-terminated string
                    char mtlName[TKIMPL_MAX_MATERIAL_NAME];
                    TKimpl_stringDelimMtlName(mtlName, info->materials[mi].mtlName,
                                              TKIMPL_MAX_MATERIAL_NAME );
                    
                    // emit the material name
                    objDelegate->material( mtlName, objDelegate->userData );
                }
                // Now emit all the triangles for the material
                if (objDelegate->triangle)
                {
                    for (size_t ti=0; ti < info->materials[mi].numTriangles; ti++) {
                        TK_Triangle tri;

                        TKimpl_GetIndexedTriangle( &tri, info, info->materials[mi].triangles[ti] );
                        objDelegate->triangle( tri.vertA, tri.vertB, tri.vertC, objDelegate->userData );
                    }
                }
            }
        }
    }
    
    // Next, call through the "index triangle group" api
    if (objDelegate->triangleGroup) {
        for (int mi=0; mi < info->numMaterials; mi++) {
            
            if (info->materials[mi].numTriangles > 0) {
                char mtlName[TKIMPL_MAX_MATERIAL_NAME];
                TKimpl_stringDelimMtlName(mtlName, info->materials[mi].mtlName,
                                          TKIMPL_MAX_MATERIAL_NAME );

                objDelegate->triangleGroup( mtlName, info->materials[mi].numTriangles,
                                           info->materials[mi].triangles, objDelegate->userData );
            }
        }
    }


    
}


#endif // TK_OBJFILE_IMPLEMENTATION

#endif 
