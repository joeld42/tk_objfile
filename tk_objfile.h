//
//  tk_objfile.h
//  tk_objfile
//
//  Created by Joel Davis on 12/5/15.
//  Copyright © 2015 Joel Davis. All rights reserved.
//

#ifndef TK_OBJFILE_H_INCLUDED
#define TK_OBJFILE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

// TK_TriangleVert -- Simple triangle vert struct.
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
    
// TKObjDelegate -- Callbacks for the OBJ format parser. All callbacks are optional.
//
// Scratch Memory -- The parser needs some scratch memory to do its work and to store the results.
// You must fill this in before you call the parser, as it doesn't do any allocations.
// If you call the parser with scratchMemory pointer of NULL, it will only do the pre-pass to
// determine how much memory it wants. It will fill in the scratchMemorySize, you may allocate
// it however you wish, and then call the parser again to do the actual parsing.
typedef struct
{
    void (*error)( size_t lineNumber, const char *message, void *userData );

    // "Triangle Soup" API -- calls triangles one at a time, grouped by material
    void (*material)( const char *mtlName, size_t numTriangles, void *userData );
    void (*triangle)( TK_TriangleVert a, TK_TriangleVert b, TK_TriangleVert c, void *userData );
    
    // Scratch memory needed by parser.
    // If scratchMemSize is 0, results will not be returned but it will be
    // filled in with the required scratchMemSize
    void *scratchMem;
    size_t scratchMemSize;
    
    // arbitrary user data passed through to callbacks
    void *userData;
    
    // stats used during parsing.
    size_t currentLineNumber;
    size_t numVerts;
    size_t numNorms;
    size_t numSts;
    size_t numFaces;
    size_t numTriangles;
    
} TK_ObjDelegate;

    
// TK_ParseObj -- Parse an obj file into triangle soup.
//
// Parse the obj formatted data and call delegate methods for each triangle.
// TODO:(jbd) Add a SimpleParse that just packs the triangles into a list for convienance
void TK_ParseObj( void *objFileData, size_t objFileSize, TK_ObjDelegate *objDelegate );
    
    
#ifdef __cplusplus
} // extern "C"
#endif

// =========================================================
//  IMPLEMENTATION
// =========================================================
#ifdef TK_OBJFILE_IMPLEMENTATION

// NOTE: This uses a custom version of strtof (which is probably not as good). A few
// people have told me that this is silly, there's no reason to avoid strtof or atof
// from the cstdlib. They're probably right, there's no real compelling reason to avoid
// the C stdlib, but since I'm doing this mostly for my own exercise I want to keep to
// the "zero dependancies, from scratch" philosophy.
//
// However, if you prefer to use the stdlib strtof, you can simply add:
//    #define TK_STRTOF strtof
// before you include tk_objfile and it will happily use that instead (or define
// it to be your own implementation).
#ifndef TK_STRTOF
#define TK_STRTOF TKimpl_stringToFloat
#endif

// Implementation types (TKimpl_*) are internal, and may 
// change without warning between versions. 

typedef struct {
    ssize_t posIndex;
    ssize_t stIndex;
    ssize_t normIndex;
} TKimpl_IndexedVert;

typedef struct {
    TKimpl_IndexedVert vertA;
    TKimpl_IndexedVert vertB;
    TKimpl_IndexedVert vertC;
} TKimpl_IndexedTriangle;

// TKimpl_Material
typedef struct {
    char  *mtlName; // not 0-delimited, be careful
    size_t numTriangles;
    TKimpl_IndexedTriangle *triangles;
} TKimpl_Material;

// Maximum number of unique materials in an obj file
#define TKIMPL_MAX_UNIQUE_MTLS (100)

// Maximum length of a material name
#define TKIMPL_MAX_MATERIAL_NAME (256)

typedef struct {
    
    // vertex list from obj
    size_t numVertPos;
    size_t numVertSt;
    size_t numVertNrm;
    
    float *vertPos;
    float *vertSt;
    float *vertNrm;
    
    TKimpl_Material *materials;
    size_t numMaterials;
    
} TKimpl_Geometry;

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


int TKimpl_isIdentifier( char ch ) {
    if (ch=='\0'||ch==' '||ch=='\n'||ch=='\t'||ch=='\r') return 0;
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
                             ssize_t *pndx, ssize_t *stndx, ssize_t *nndx)
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
    }
    
    // decide which lists indexes represent based on number of slashes
    if (pndx) {
        // first number is always pos
        *pndx = number[0];
    }
    
    if (numSlash==0) {
        // No slashes, pos only
        if (stndx) *stndx = 0;
        if (nndx) *nndx = 0;
    } else if (numSlash==1) {
        // one slash, pos/st
        if (nndx) *nndx = 0;
        if (stndx) *stndx = number[1];
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


int TKimpl_isFloatChar( char ch )
{
    if ( ((ch>='0')&&(ch<='9')) || (ch=='-') || (ch=='.')) return 1;
    else return 0;
}

float TKimpl_stringToFloat( char *str, char **str_end )
{
    char *ch = str;
    float value = 0.0;
    float mag= 0.1;
    float sign = 1.0;
    int inDecimal = 0;
    while (TKimpl_isFloatChar(*ch)) {
        
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
        }
        ch++;
    }
    if (str_end) *str_end = ch;
    return sign * value;
}

// Return 1 on success, 0 on failure
int TKimpl_parseFloat( TK_ObjDelegate *objDelegate, char *token, char *endtoken, float *out_result )
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
        char *endt = NULL;
        float value = 0.0;
        value = TK_STRTOF(token, &endt);
        if (endt != endtoken) {
             if (objDelegate->error) {
                 objDelegate->error( objDelegate->currentLineNumber, "Could not parse float.",
                                    objDelegate->userData );
             }
             return 0;
        }
        *out_result = value;
    }
    return 1;
}


void TKimpl_ParseObjPass( void *objFileData, size_t objFileSize,
                         TKimpl_Geometry *geom,
                         TKimpl_Material *uniqueMtls, size_t *numUniqueMtls,
                         TK_ObjDelegate *objDelegate, TKimpl_ParseType parseType )
{
    // Make default material
    TKimpl_Material *currMtl = NULL;
    if (parseType == TKimpl_ParseTypeCountOnly)
    {
        uniqueMtls[0].mtlName = (char *)"mtl.default "; // trailing space is intentional
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
        
        // Skip Comments
        if (line[0]!='#')
        {
            char *token, *endtoken;
            token = line;
            endtoken = line;
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
                        float *vertPos = geom->vertPos + (geom->numVertPos*3);
                        
                        TKimpl_nextToken( &token, &endtoken, endline);
                        if (!TKimpl_parseFloat( objDelegate, token, endtoken, &(vertPos[0]) )) {
                            return;
                        }

                        TKimpl_nextToken( &token, &endtoken, endline);
                        if (!TKimpl_parseFloat( objDelegate, token, endtoken, &(vertPos[1]) )) {
                            return;
                        }
                        
                        TKimpl_nextToken( &token, &endtoken, endline);
                        if (!TKimpl_parseFloat( objDelegate, token, endtoken, &(vertPos[2]) )) {
                            return;
                        }
                        
                        geom->numVertPos++;
                    }
                    
                } else if (TKimpl_compareToken("vn", token, endtoken)) {

                    if (parseType==TKimpl_ParseTypeCountOnly)
                    {
                        objDelegate->numNorms++;
                    }
                    else
                    {
                        // vn X Y Z -- vertex normal
                        float *vertNrm = geom->vertNrm + (geom->numVertNrm*3);
                        
                        TKimpl_nextToken( &token, &endtoken, endline);
                        if (!TKimpl_parseFloat( objDelegate, token, endtoken, &(vertNrm[0]) )) {
                            return;
                        }
                        
                        TKimpl_nextToken( &token, &endtoken, endline);
                        if (!TKimpl_parseFloat( objDelegate, token, endtoken, &(vertNrm[1]) )) {
                            return;
                        }
                        
                        TKimpl_nextToken( &token, &endtoken, endline);
                        if (!TKimpl_parseFloat( objDelegate, token, endtoken, &(vertNrm[2]) )) {
                            return;
                        }
                        
                        geom->numVertNrm++;
                    }
                } else if (TKimpl_compareToken("vt", token, endtoken)) {
                    
                    if (parseType==TKimpl_ParseTypeCountOnly)
                    {
                        objDelegate->numSts++;
                    }
                    else
                    {
                        // vn S T -- vertex texture coord
                        float *vertSt = geom->vertSt + (geom->numVertSt*2);

                        TKimpl_nextToken( &token, &endtoken, endline);
                        if (!TKimpl_parseFloat( objDelegate, token, endtoken, &(vertSt[0]) )) {
                            return;
                        }
                        
                        TKimpl_nextToken( &token, &endtoken, endline);
                        if (!TKimpl_parseFloat( objDelegate, token, endtoken, &(vertSt[1]) )) {
                            return;
                        }

                        geom->numVertSt++;
                    }
                } else if (TKimpl_compareToken("usemtl", token, endtoken)) {
                    
                    // usemtl, is this an existing mtl group or a new one?
                    TKimpl_nextToken( &token, &endtoken, endline);
                    
                    TKimpl_Material *useMtl = NULL;
                    for (int i=1; i < *numUniqueMtls; i++) {
                        if (TKimpl_compareMtlName( uniqueMtls[i].mtlName, token )) {
                            useMtl = &uniqueMtls[i];
                            break;
                        }
                    }
                    if ((!useMtl) && (parseType==TKimpl_ParseTypeCountOnly)) {
                        //assert( *numUniqueMtls < TKIMPL_MAX_UNIQUE_MTLS );
                        useMtl = &uniqueMtls[(*numUniqueMtls)++];
                        useMtl->numTriangles = 0;
                        useMtl->mtlName = token;
                    }
                    currMtl = useMtl;

                } else if (TKimpl_compareToken("f", token, endtoken)) {
                    TKimpl_IndexedTriangle tri;
                    TKimpl_IndexedVert vert;
                    int count = 0;
                    do {
                        TKimpl_nextToken( &token, &endtoken, endline );
                        if (token) {
                            if (parseType==TKimpl_ParseTypeFull)
                            {
                                TKimpl_parseFaceIndices(token, endtoken,
                                                        &(vert.posIndex),
                                                        &(vert.stIndex),
                                                        &(vert.normIndex) );

                                if (vert.posIndex < 0) {
                                   vert.posIndex = geom->numVertPos + vert.posIndex;
                                }

                                if (vert.stIndex < 0) {
                                   vert.stIndex = geom->numVertSt + vert.stIndex;
                                }

                                if (vert.normIndex < 0) {
                                   vert.normIndex = geom->numVertNrm + vert.normIndex;
                                }

                                if (count==0) {
                                    tri.vertA = vert;
                                } else if (count==1) {
                                    tri.vertB = vert;
                                } else if (count >= 3) {
                                    tri.vertB = tri.vertC;
                                }
                                
                                if (count >= 2) {
                                    tri.vertC = vert;
                                    currMtl->triangles[ currMtl->numTriangles++ ] = tri;
                                }
                            }
                            
                            count++;
                        }
                    } while (token);
                    
                    if ((count > 2) &&  (parseType==TKimpl_ParseTypeCountOnly)) {
                        int triCount = count-2;
                        currMtl->numTriangles += triCount;
                        objDelegate->numFaces += 1;
                        objDelegate->numTriangles += triCount;
                    }
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

void TKimpl_GetIndexedTriangle( TK_Triangle *tri, TKimpl_Geometry *geom, TKimpl_IndexedTriangle ndxTri )
{
    tri->vertA.pos[0] = geom->vertPos[ndxTri.vertA.posIndex*3 + 0];
    tri->vertA.pos[1] = geom->vertPos[ndxTri.vertA.posIndex*3 + 1];
    tri->vertA.pos[2] = geom->vertPos[ndxTri.vertA.posIndex*3 + 2];
    tri->vertA.nrm[0] = geom->vertNrm[ndxTri.vertA.normIndex*3 + 0];
    tri->vertA.nrm[1] = geom->vertNrm[ndxTri.vertA.normIndex*3 + 1];
    tri->vertA.nrm[2] = geom->vertNrm[ndxTri.vertA.normIndex*3 + 2];
    tri->vertA.st[0] = geom->vertSt[ndxTri.vertA.stIndex*2 + 0];
    tri->vertA.st[1] = geom->vertSt[ndxTri.vertA.stIndex*2 + 1];

    tri->vertB.pos[0] = geom->vertPos[ndxTri.vertB.posIndex*3 + 0];
    tri->vertB.pos[1] = geom->vertPos[ndxTri.vertB.posIndex*3 + 1];
    tri->vertB.pos[2] = geom->vertPos[ndxTri.vertB.posIndex*3 + 2];
    tri->vertB.nrm[0] = geom->vertNrm[ndxTri.vertB.normIndex*3 + 0];
    tri->vertB.nrm[1] = geom->vertNrm[ndxTri.vertB.normIndex*3 + 1];
    tri->vertB.nrm[2] = geom->vertNrm[ndxTri.vertB.normIndex*3 + 2];
    tri->vertB.st[0] = geom->vertSt[ndxTri.vertB.stIndex*2 + 0];
    tri->vertB.st[1] = geom->vertSt[ndxTri.vertB.stIndex*2 + 1];

    tri->vertC.pos[0] = geom->vertPos[ndxTri.vertC.posIndex*3 + 0];
    tri->vertC.pos[1] = geom->vertPos[ndxTri.vertC.posIndex*3 + 1];
    tri->vertC.pos[2] = geom->vertPos[ndxTri.vertC.posIndex*3 + 2];
    tri->vertC.nrm[0] = geom->vertNrm[ndxTri.vertC.normIndex*3 + 0];
    tri->vertC.nrm[1] = geom->vertNrm[ndxTri.vertC.normIndex*3 + 1];
    tri->vertC.nrm[2] = geom->vertNrm[ndxTri.vertC.normIndex*3 + 2];
    tri->vertC.st[0] = geom->vertSt[ndxTri.vertC.stIndex*2 + 0];
    tri->vertC.st[1] = geom->vertSt[ndxTri.vertC.stIndex*2 + 1];
}

void TK_ParseObj( void *objFileData, size_t objFileSize, TK_ObjDelegate *objDelegate )
{
    TKimpl_Material uniqueMtls[TKIMPL_MAX_UNIQUE_MTLS];
    size_t numUniqueMtls = 0;
    
    // pre-pass, count how many verts, nrms and sts there are
    objDelegate->numVerts=0;
    objDelegate->numSts=0;
    objDelegate->numNorms=0;
    objDelegate->numFaces=0;
    objDelegate->numTriangles=0;
    
    // First pass, just count verts and unique materials...
    TKimpl_ParseObjPass( objFileData, objFileSize,  NULL,
                        uniqueMtls, &numUniqueMtls,
                        objDelegate, TKimpl_ParseTypeCountOnly );
    
    // Make sure we reserve space for at least a single
    // st and normal, if they are not present in the obj
    if (!objDelegate->numSts) objDelegate->numSts = 1;
    if (!objDelegate->numNorms) objDelegate->numNorms = 1;
    
    size_t totalTriangleCount = 0;
    for (int i=0; i < numUniqueMtls; i++) {
        totalTriangleCount += uniqueMtls[i].numTriangles;
    }
    
    // Calculate scratchMemSize
    size_t requiredScratchMem =
        sizeof(TKImpl_MemArena) +
        sizeof(TKimpl_Geometry) +
        sizeof(float)*3*objDelegate->numVerts +
        sizeof(float)*3*objDelegate->numNorms +
        sizeof(float)*2*objDelegate->numSts +
        sizeof(TKimpl_Material) * numUniqueMtls +
        sizeof(TKimpl_IndexedTriangle) * totalTriangleCount;
    
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
    
    // Allocate our geom
    TKimpl_Geometry *geom = TKImpl_PushStruct(arena, TKimpl_Geometry);
    
    // Allocate vertex data lists
    geom->numVertPos = 0;
    geom->vertPos = (float*)TKImpl_PushSize(arena, sizeof(float)*3*objDelegate->numVerts);
    
    geom->numVertNrm = 0;
    geom->vertNrm = (float*)TKImpl_PushSize(arena, sizeof(float)*3*objDelegate->numNorms);
    
    geom->numVertSt = 0;
    geom->vertSt = (float*)TKImpl_PushSize(arena, sizeof(float)*2*objDelegate->numSts);
    
    geom->materials = TKImpl_PushStructArray(arena, TKimpl_Material, numUniqueMtls );
    geom->numMaterials = numUniqueMtls;
    
    for (int i = 0; i < numUniqueMtls; i++) {
        geom->materials[i].mtlName = uniqueMtls[i].mtlName;
        geom->materials[i].triangles = TKImpl_PushStructArray( arena, TKimpl_IndexedTriangle,
                                                              uniqueMtls[i].numTriangles );
        geom->materials[i].numTriangles = 0;
    }
    
    // Now space is allocated for all the data, parse again and store
    TKimpl_ParseObjPass( objFileData, objFileSize,  geom,
                        geom->materials, &(geom->numMaterials),
                        objDelegate, TKimpl_ParseTypeFull );
    
    // If we have no STs or Norms, push a default one
    if (geom->numVertSt==0) {
        geom->vertSt[0] = 0.0;
        geom->vertSt[1] = 0.0;
    }
    
    if (geom->numVertNrm==0) {
        geom->vertNrm[0] = 0.0;
        geom->vertNrm[1] = 1.0;
        geom->vertNrm[2] = 0.0;
    }

    
    // Now go through the results with the "triangle soup" API
    if ((objDelegate->triangle) || (objDelegate->material)) {
        for (int mi=0; mi < geom->numMaterials; mi++) {
            if (geom->materials[mi].numTriangles > 0) {
                if (objDelegate->material) {
                    // Copy the mtlName into a nice 0-terminated string
                    char mtlName[TKIMPL_MAX_MATERIAL_NAME];
                    TKimpl_stringDelimMtlName(mtlName, geom->materials[mi].mtlName,
                                              TKIMPL_MAX_MATERIAL_NAME );
                    
                    // emit the material name
                    objDelegate->material( mtlName,
                                          geom->materials[mi].numTriangles,
                                          objDelegate->userData );
                }
                // Now emit all the triangles for the material
                if (objDelegate->triangle)
                {
                    for (size_t ti=0; ti < geom->materials[mi].numTriangles; ti++) {
                        TK_Triangle tri = {};

                        TKimpl_GetIndexedTriangle( &tri, geom, geom->materials[mi].triangles[ti] );
                        objDelegate->triangle( tri.vertA, tri.vertB, tri.vertC, objDelegate->userData );
                    }
                }
            }
        }
    }
}


#endif // TK_OBJFILE_IMPLEMENTATION

#endif 
