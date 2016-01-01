
#include <string.h>
#include <math.h>

#include <imgui.h>

#include "imgui_impl_glfw_gl3.h"

#include <stdio.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#define TK_OBJFILE_IMPLEMENTATION
#include "tk_objfile.h"

#define offset_d(i,f)    (long(&(i)->f) - long(i))
#define offset_s(t,f)    offset_d((t*)1000, f)

#define DEG2RAD(x) (x*M_PI/180.0)

enum
{
    VertexAttrib_POSITION,
    VertexAttrib_TEXCOORD,
    VertexAttrib_NORMAL,
//    VertexAttrib_TANGENT,
//    VertexAttrib_BITANGENT,
//    VertexAttrib_COLOR,
    VertexAttrib_COUNT
};

#define deg_to_rad(x) (x * (M_PI/180.0f))

void glCheckError(const char* file, unsigned int line )
{
    // Get the last error
    GLenum errorCode = glGetError();
    
    if (errorCode != GL_NO_ERROR)
    {
        const char *error = "Unknown error";
        const char *description  = "No description";
        
        // Decode the error code
        switch (errorCode)
        {
            case GL_INVALID_ENUM:
            {
                error = "GL_INVALID_ENUM";
                description = "An unacceptable value has been specified for an enumerated argument.";
                break;
            }
                
            case GL_INVALID_VALUE:
            {
                error = "GL_INVALID_VALUE";
                description = "A numeric argument is out of range.";
                break;
            }
                
            case GL_INVALID_OPERATION:
            {
                error = "GL_INVALID_OPERATION";
                description = "The specified operation is not allowed in the current state.";
                break;
            }
                
            case GL_STACK_OVERFLOW:
            {
                error = "GL_STACK_OVERFLOW";
                description = "This command would cause a stack overflow.";
                break;
            }
                
            case GL_STACK_UNDERFLOW:
            {
                error = "GL_STACK_UNDERFLOW";
                description = "This command would cause a stack underflow.";
                break;
            }
                
            case GL_OUT_OF_MEMORY:
            {
                error = "GL_OUT_OF_MEMORY";
                description = "There is not enough memory left to execute the command.";
                break;
            }
                
            case GL_INVALID_FRAMEBUFFER_OPERATION:
            {
                error = "GL_INVALID_FRAMEBUFFER_OPERATION";
                description = "The object bound to FRAMEBUFFER_BINDING is not \"framebuffer complete\".";
                break;
            }
        }
        
        // Log the error
        printf(" GL ERROR %s:%d -- %s (%s)\n", file, line, error, description );
        printf("...\n");
    }
}

#define CHECKGL glCheckError( __FILE__, __LINE__)

void matrixFrustumf2(float *mat, GLfloat left, GLfloat right, GLfloat bottom,
                  GLfloat top, GLfloat znear, GLfloat zfar)
{
    float temp, temp2, temp3, temp4;
    temp = 2.0f * znear;
    temp2 = right - left;
    temp3 = top - bottom;
    temp4 = zfar - znear;
    mat[0] = temp / temp2;
    mat[1] = 0.0f;
    mat[2] = 0.0f;
    mat[3] = 0.0f;
    
    mat[4] = 0.0f;
    mat[5] = temp / temp3;
    mat[6] = 0.0f;
    mat[7] = 0.0f;
    
    mat[8] = (right + left) / temp2;
    mat[9] = (top + bottom) / temp3;
    mat[10] = ((-zfar - znear) / temp4);
    mat[11] = -1.0f;
    
    mat[12] = 0.0f;
    mat[13] = 0.0f;
    mat[14] = ((-temp * zfar) / temp4);
    mat[15] = 0.0f;
}

void matrixPerspectivef2(float *mat, GLfloat fovyInDegrees,
                      GLfloat aspectRatio, GLfloat znear, GLfloat zfar)
{
    float ymax, xmax;
    ymax = znear * tanf(fovyInDegrees * 3.14f / 360.0f);
    xmax = ymax * aspectRatio;
    matrixFrustumf2(mat, -xmax, xmax, -ymax, ymax, znear, zfar);
}

void matrixDbgPrint( char *message, float *m )
{
    printf("%10s: %3.2f %3.2f %3.2f %3.2f\n"
           "            %3.2f %3.2f %3.2f %3.2f\n"
           "            %3.2f %3.2f %3.2f %3.2f\n"
           "            %3.2f %3.2f %3.2f %3.2f\n",
           message,
           m[0], m[1], m[2], m[3],
           m[4], m[5], m[6], m[7],
           m[8], m[9], m[10], m[11],
           m[12], m[13], m[14], m[15] );
           
}

void matrixMultiply(float *mOut, float *mA, float *mB)
{
    mOut[ 0] = mA[ 0]*mB[ 0] + mA[ 1]*mB[ 4] + mA[ 2]*mB[ 8] + mA[ 3]*mB[12];
    mOut[ 1] = mA[ 0]*mB[ 1] + mA[ 1]*mB[ 5] + mA[ 2]*mB[ 9] + mA[ 3]*mB[13];
    mOut[ 2] = mA[ 0]*mB[ 2] + mA[ 1]*mB[ 6] + mA[ 2]*mB[10] + mA[ 3]*mB[14];
    mOut[ 3] = mA[ 0]*mB[ 3] + mA[ 1]*mB[ 7] + mA[ 2]*mB[11] + mA[ 3]*mB[15];
    
    mOut[ 4] = mA[ 4]*mB[ 0] + mA[ 5]*mB[ 4] + mA[ 6]*mB[ 8] + mA[ 7]*mB[12];
    mOut[ 5] = mA[ 4]*mB[ 1] + mA[ 5]*mB[ 5] + mA[ 6]*mB[ 9] + mA[ 7]*mB[13];
    mOut[ 6] = mA[ 4]*mB[ 2] + mA[ 5]*mB[ 6] + mA[ 6]*mB[10] + mA[ 7]*mB[14];
    mOut[ 7] = mA[ 4]*mB[ 3] + mA[ 5]*mB[ 7] + mA[ 6]*mB[11] + mA[ 7]*mB[15];
    
    mOut[ 8] = mA[ 8]*mB[ 0] + mA[ 9]*mB[ 4] + mA[10]*mB[ 8] + mA[11]*mB[12];
    mOut[ 9] = mA[ 8]*mB[ 1] + mA[ 9]*mB[ 5] + mA[10]*mB[ 9] + mA[11]*mB[13];
    mOut[10] = mA[ 8]*mB[ 2] + mA[ 9]*mB[ 6] + mA[10]*mB[10] + mA[11]*mB[14];
    mOut[11] = mA[ 8]*mB[ 3] + mA[ 9]*mB[ 7] + mA[10]*mB[11] + mA[11]*mB[15];
    
    mOut[12] = mA[12]*mB[ 0] + mA[13]*mB[ 4] + mA[14]*mB[ 8] + mA[15]*mB[12];
    mOut[13] = mA[12]*mB[ 1] + mA[13]*mB[ 5] + mA[14]*mB[ 9] + mA[15]*mB[13];
    mOut[14] = mA[12]*mB[ 2] + mA[13]*mB[ 6] + mA[14]*mB[10] + mA[15]*mB[14];
    mOut[15] = mA[12]*mB[ 3] + mA[13]*mB[ 7] + mA[14]*mB[11] + mA[15]*mB[15];
}

void matrixTranslation(float *mOut,
                       float fX,
                       float fY,
                       float fZ)
{
    mOut[ 0]=1.0f;	mOut[ 4]=0.0f;	mOut[ 8]=0.0f;	mOut[12]=fX;
    mOut[ 1]=0.0f;	mOut[ 5]=1.0f;	mOut[ 9]=0.0f;	mOut[13]=fY;
    mOut[ 2]=0.0f;	mOut[ 6]=0.0f;	mOut[10]=1.0f;	mOut[14]=fZ;
    mOut[ 3]=0.0f;	mOut[ 7]=0.0f;	mOut[11]=0.0f;	mOut[15]=1.0f;
}

inline void vec3SetXYZ( float *vec, float x, float y, float z)
{
    vec[0] = x;
    vec[1] = y;
    vec[2] = z;
}

void vec3CrossProduct( float *vOut, float *v1, float *v2)
{
    
    /* Perform calculation on a dummy VECTOR (result) */
    vOut[0] = v1[1] * v2[2] - v1[2] * v2[1];
    vOut[1] = v1[2] * v2[0] - v1[0] * v2[2];
    vOut[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

void vec3Normalize(float *vOut, float *vIn)
{
    float	f;
    double temp;
    
    temp = (double)(vIn[0] * vIn[0] + vIn[1] * vIn[1] + vIn[2] * vIn[2]);
    temp = 1.0 / sqrt(temp);
    f = (float)temp;
    
    vOut[0] = vIn[0] * f;
    vOut[1] = vIn[1] * f;
    vOut[2] = vIn[2] * f;
}

void matrixLookAtLH(float *mOut,
                    float *vEye,
                    float *vAt,
                    float *vUp)
{
    float f[3], f2[3], vUpActual[3], s[3], u[3];
    float t[16];
    float mLookDir[16];
    
    f2[0] = vEye[0] - vAt[0];
    f2[1] = vEye[1] - vAt[1];
    f2[2] = vEye[2] - vAt[2];
    
    vec3Normalize(f, f2);
    vec3Normalize(vUpActual, vUp);
    vec3CrossProduct(s, f, vUpActual);
    vec3CrossProduct(u, s, f);
    
    mLookDir[ 0] = s[0];
    mLookDir[ 1] = u[0];
    mLookDir[ 2] = -f[0];
    mLookDir[ 3] = 0;
    
    mLookDir[ 4] = s[1];
    mLookDir[ 5] = u[1];
    mLookDir[ 6] = -f[1];
    mLookDir[ 7] = 0;
    
    mLookDir[ 8] = s[2];
    mLookDir[ 9] = u[2];
    mLookDir[10] = -f[2];
    mLookDir[11] = 0;
    
    mLookDir[12] = 0;
    mLookDir[13] = 0;
    mLookDir[14] = 0;
    mLookDir[15] = 1;
    
    matrixTranslation(t, -vEye[0], -vEye[1], -vEye[2]);
    matrixMultiply(mOut, t, mLookDir);
}

void matrixLookAtRH(float *mOut,
                    float *vEye,
                    float *vAt,
                    float *vUp)
{
    float f2[3], f[3], vUpActual[3], s[3], u[3];
    float t[16];
    float mLookDir[16];
    
    f2[0] = vAt[0] - vEye[0];
    f2[1] = vAt[1] - vEye[1];
    f2[2] = vAt[2] - vEye[2];
    
    vec3Normalize(f, f2);
    vec3Normalize(vUpActual, vUp);
    vec3CrossProduct(s, f, vUpActual);
    vec3CrossProduct(u, s, f);
    
    mLookDir[ 0] = s[0];
    mLookDir[ 1] = u[0];
    mLookDir[ 2] = -f[0];
    mLookDir[ 3] = 0;
    
    mLookDir[ 4] = s[1];
    mLookDir[ 5] = u[1];
    mLookDir[ 6] = -f[1];
    mLookDir[ 7] = 0;
    
    mLookDir[ 8] = s[2];
    mLookDir[ 9] = u[2];
    mLookDir[10] = -f[2];
    mLookDir[11] = 0;
    
    mLookDir[12] = 0;
    mLookDir[13] = 0;
    mLookDir[14] = 0;
    mLookDir[15] = 1;
    
    matrixTranslation(t, -vEye[0], -vEye[1], -vEye[2]);
    matrixMultiply(mOut, t, mLookDir);
}

enum
{
    Uniform_PROJMATRIX,
    Uniform_TINTCOLOR,
    
    Uniform_COUNT
};

struct ObjDrawBuffer
{
    TK_TriangleVert *buffer;
    
    size_t vertCapacity;
    size_t vertUsed;
    
    GLuint vbo;
    GLuint vao;
};


typedef struct ObjMeshGroupStruct
{
    char *mtlName;
    ObjDrawBuffer drawbuffer;
    ObjMeshGroupStruct *next;
    
} ObjMeshGroup;

struct ObjDisplayOptions
{
    bool wireFrame;
};

// ================================
//   ObjMesh
// ================================
struct ObjMesh
{
    // General info
    TK_ObjDelegate *objDelegate;
    char *objFilename;
    
    // Obj geometry
    ObjMeshGroup *rootGroup;
    ObjMeshGroup *currentGroup;
    size_t groupCount;
    
    // Camera
    float objCenter[3];
    float camPos[3];
    float camAngle, camTilt;
    size_t totalNumTriangles;
    
    // Obj shader
    int shaderHandle, vsHandle, fsHandle;
    int attrib[VertexAttrib_COUNT];
    int uniform[Uniform_COUNT];
    
    // GUI stuff
    ObjDisplayOptions displayOpts;
    bool showInspector;
};

size_t ObjDrawBuffer_PushVert( ObjDrawBuffer *buff, TK_TriangleVert vert )
{
    // Can't add to a buffer anymore once you draw it
    assert( buff->vbo == 0);

    // Make sure we have enough space for the new vert
    assert( buff->buffer );
    assert( buff->vertUsed < buff->vertCapacity );
    
    if (!buff->buffer)
    {
        // Start with space for 100 verts
        buff->vertUsed = 0;
        buff->vertCapacity = 100;
        buff->buffer = (TK_TriangleVert*)malloc( sizeof(TK_TriangleVert)*buff->vertCapacity );
    }
    else if (buff->vertUsed == buff->vertCapacity)
    {
        // Need to grow buffer
        size_t newCapacity = buff->vertCapacity * 2;
        buff->buffer = (TK_TriangleVert*)realloc( buff->buffer, newCapacity*sizeof(TK_TriangleVert) );
        buff->vertCapacity = newCapacity;
    }
    
    // Now we have space, add the vert
    size_t vertIndex = buff->vertUsed++;
    memcpy( buff->buffer + vertIndex, &vert, sizeof(TK_TriangleVert));
    
    return vertIndex;
}

void ObjMesh_addGroup( ObjMesh *mesh, ObjMeshGroup *group)
{
    group->next = mesh->rootGroup;
    mesh->rootGroup = group;
    mesh->groupCount++;
}

void ObjMesh_renderGroup( ObjMesh *mesh, ObjMeshGroup *group )
{
    if (group->drawbuffer.vbo == 0) {
        
        glGenBuffers( 1, &(group->drawbuffer.vbo) );
        CHECKGL;
        
        glBindBuffer( GL_ARRAY_BUFFER, group->drawbuffer.vbo );
        CHECKGL;
        
        glBufferData( GL_ARRAY_BUFFER, group->drawbuffer.vertUsed * sizeof( TK_TriangleVert ),
                     group->drawbuffer.buffer, GL_STATIC_DRAW );
        CHECKGL;
        
        glGenVertexArrays(1, &(group->drawbuffer.vao));
        CHECKGL;
        
        glBindVertexArray(group->drawbuffer.vao);
        CHECKGL;
        
    } else {
//        glBindBuffer( GL_ARRAY_BUFFER, group->drawbuffer.vbo );
//        CHECKGL;
        glBindVertexArray(group->drawbuffer.vao);
        CHECKGL;
        
        glBindBuffer( GL_ARRAY_BUFFER, group->drawbuffer.vbo );
        CHECKGL;
        
    }
    
    // TODO: bind texture for this group
    
    
    // Bind vertex attributes
    glEnableVertexAttribArray( mesh->attrib[VertexAttrib_POSITION] );
    CHECKGL;
    
    glVertexAttribPointer( mesh->attrib[VertexAttrib_POSITION], 3, GL_FLOAT, GL_FALSE,
                          sizeof(TK_TriangleVert), (void*)offset_s( TK_TriangleVert, pos) );
    CHECKGL;
    
    glEnableVertexAttribArray( mesh->attrib[VertexAttrib_TEXCOORD] );
    glVertexAttribPointer( mesh->attrib[VertexAttrib_TEXCOORD], 2, GL_FLOAT, GL_FALSE,
                          sizeof(TK_TriangleVert), (void*)offset_s( TK_TriangleVert, st ) );
    CHECKGL;
    
    glEnableVertexAttribArray( mesh->attrib[VertexAttrib_NORMAL] );
    glVertexAttribPointer( mesh->attrib[VertexAttrib_NORMAL], 3, GL_FLOAT, GL_FALSE,
                          sizeof(TK_TriangleVert), (void*)offset_s( TK_TriangleVert, nrm ) );
    CHECKGL;
    
    // Draw it!
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)group->drawbuffer.vertUsed );
//    glPointSize( 15.0 );
//    glDrawArrays( GL_POINTS, 0, (GLsizei)group->drawbuffer.vertUsed );
   CHECKGL;
}

void checkShaderLog( int shader, int shaderType, const GLchar *shaderText )
{
    GLint logLength=0;
    glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &logLength );
    if (logLength > 0)
    {
        char *log = (char *)malloc(logLength);
        glGetShaderInfoLog( shader, logLength, NULL, log );
        printf("--------------------------\n%s\n\n", shaderText );
        printf("Error compiling %s shader:\n%s\n-------\n",
               (shaderType==GL_VERTEX_SHADER)?"Vertex":"Fragment",
               log );
        free(log);
    } else {
        printf( "%s shader compiled successfully...\n",
               (shaderType==GL_VERTEX_SHADER)?"Vertex":"Fragment" );
    }
}

void ObjMesh_setupShader( ObjMesh *mesh )
{
    const GLchar *vertex_shader =
    "#version 330\n"
    "uniform mat4 ProjMtx;\n"
    "in vec3 Position;\n"
    "in vec2 TexCoord;\n"
    "in vec3 Normal;\n"
    "out vec2 Frag_UV;\n"
    "out vec4 Frag_Color;\n"
    "void main()\n"
    "{\n"
    "	Frag_UV = TexCoord;\n"
    "	Frag_Color = vec4(Normal, 1.0);\n"
    "	gl_Position = ProjMtx * vec4(Position,1);\n"
//    "gl_PointSize = 3.0;"
    "}\n";
    
    const GLchar* fragment_shader =
    "#version 330\n"
    "uniform sampler2D Texture;\n"
    "in vec2 Frag_UV;\n"
    "in vec4 Frag_Color;\n"
    "out vec4 Out_Color;\n"
    "void main()\n"
    "{\n"
//    "	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
    "  Out_Color = Frag_Color;\n"
    "}\n";
    
    mesh->shaderHandle = glCreateProgram();
    mesh->vsHandle = glCreateShader(GL_VERTEX_SHADER);
    mesh->fsHandle = glCreateShader(GL_FRAGMENT_SHADER);
    CHECKGL;
    
    glShaderSource(mesh->vsHandle, 1, &vertex_shader, 0);
    CHECKGL;
    
    glShaderSource(mesh->fsHandle, 1, &fragment_shader, 0);
    CHECKGL;
    
    glCompileShader(mesh->vsHandle);
    checkShaderLog( mesh->vsHandle, GL_VERTEX_SHADER, vertex_shader );
    CHECKGL;
    
    glCompileShader(mesh->fsHandle);
    checkShaderLog( mesh->fsHandle, GL_FRAGMENT_SHADER, fragment_shader );
    CHECKGL;
    
    glAttachShader(mesh->shaderHandle, mesh->vsHandle);
    CHECKGL;
    
    glAttachShader(mesh->shaderHandle, mesh->fsHandle);
    CHECKGL;
    
    glLinkProgram(mesh->shaderHandle);
    CHECKGL;
    
    mesh->uniform[Uniform_PROJMATRIX] = glGetUniformLocation( mesh->shaderHandle, "ProjMtx");
    
    mesh->attrib[VertexAttrib_POSITION] = glGetAttribLocation( mesh->shaderHandle, "Position");
    printf("Position Attrib %d\n",mesh->attrib[VertexAttrib_POSITION] );
    
    mesh->attrib[VertexAttrib_TEXCOORD] = glGetAttribLocation( mesh->shaderHandle, "TexCoord" );
    printf("TexCoord Attrib %d\n",mesh->attrib[VertexAttrib_TEXCOORD] );
    
    mesh->attrib[VertexAttrib_NORMAL] = glGetAttribLocation( mesh->shaderHandle, "Normal" );
    printf("Normal Attrib %d\n",mesh->attrib[VertexAttrib_NORMAL] );
    
//    g_AttribLocationTex = glGetUniformLocation(g_ShaderHandle, "Texture");
//    g_AttribLocationProjMtx = glGetUniformLocation(g_ShaderHandle, "ProjMtx");
//    g_AttribLocationPosition = glGetAttribLocation(g_ShaderHandle, "Position");
//    g_AttribLocationUV = glGetAttribLocation(g_ShaderHandle, "UV");
//    g_AttribLocationColor = glGetAttribLocation(g_ShaderHandle, "Color");
    
    glEnable( GL_DEPTH_TEST );
    
}

void ObjMesh_update( ObjMesh *mesh )
{
    static bool mousePressed[3];
    static bool lastMousePressed[3];
    double mouseX, mouseY;
    static bool dragging;
    static float startAngle;
    static double startX;
    
    ImGui_ImplGlfwGL3_GetMousePos(&mouseX, &mouseY, mousePressed );
    
    //printf("Mouse pos %f %f\n", mouseX, mouseY );
    if ((!dragging) && (mousePressed[0])) {
        dragging = true;
        startAngle = mesh->camAngle;
        startX = mouseX;
    }
    
    if (dragging) {
        printf("DRAGGING: %f\n", mouseX - startX );
//        float camAngle, camTilt;
        mesh->camAngle = startAngle + (mouseX - startX) * 0.5;

    }
    
    if ((dragging) && (!mousePressed[0])) {
        dragging = false;
        
    }
    
    // update camPos based on camAngle
    float radius = 5.0; // FIXME: get from OBJ bounding radius
    float angRad = DEG2RAD( mesh->camAngle );
    vec3SetXYZ( mesh->camPos,
               mesh->objCenter[0] + cos( angRad ) * radius,
               mesh->objCenter[1],
               mesh->objCenter[2] + sin(angRad) * radius );
//    printf("camPos %f %f %f\n", mesh->camPos[0], mesh->camPos[1], mesh->camPos[2] );
    
    for (int i=0; i < 3; i++) {
        lastMousePressed[i] = mousePressed[i];
    }
}

void ObjMesh_renderAll( ObjMesh *mesh )
{
    
    if (!mesh->shaderHandle) {
        ObjMesh_setupShader( mesh );
        CHECKGL;
    }


    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    
    // Setup viewport,  projection matrix
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    
    float proj[16];
    float modelview[16];
    
    float modelViewProj[16];
    
    matrixPerspectivef2(proj, 50.0, (float)fb_width/(float)fb_height, 0.01, 1000.0 );
//    matrixTranslation( modelview, 0.0, 0.0, -1.5 );
    float vUp[3] = { 0.0, 1.0, 0.0 };
//    vec3SetXYZ( mesh->camPos, 0, 0, 1.5 );
    
//    static float ang = 0.0;
//    float radius = 2.5;
//    vec3SetXYZ( mesh->camPos, cos(ang)*radius, 0.0, sin(ang)*radius );
//    ang += 0.01;
    
//    vec3SetXYZ( mesh->objCenter, 0, 0, 0 );
    matrixLookAtRH( modelview, mesh->camPos, mesh->objCenter, vUp );
    
    matrixMultiply( modelViewProj, modelview, proj );
//    matrixDbgPrint( "modelview", modelview );
    
    glUseProgram( mesh->shaderHandle );
    glUniformMatrix4fv( mesh->uniform[Uniform_PROJMATRIX], 1, GL_FALSE, modelViewProj );
    
    CHECKGL;
    
    for (ObjMeshGroup *group = mesh->rootGroup;
         group; group = group->next ) {
        ObjMesh_renderGroup( mesh, group );
        CHECKGL;
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

// FIXME: have a single error callback
static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

void objviewerErrorMessage( size_t lineNum, const char *message, void *userData )
{
    printf("ERROR on line %zu: %s\n", lineNum, message );
}

void objviewerMaterial( const char *materialName, size_t numTriangles, void *userData )
{
    ObjMesh *mesh = (ObjMesh*)userData;

    ObjMeshGroup *group = (ObjMeshGroup*)malloc(sizeof(ObjMeshGroup));
    memset( group, 0, sizeof(ObjMeshGroup));
    
    // Set up the material
    group->mtlName = strdup( materialName );
    group->drawbuffer.buffer = (TK_TriangleVert*)malloc( sizeof(TK_TriangleVert)*numTriangles*3 );
    group->drawbuffer.vertCapacity = numTriangles*3;
    group->drawbuffer.vertUsed = 0;

    ObjMesh_addGroup( mesh, group );
    
    mesh->currentGroup = group;
    mesh->totalNumTriangles += numTriangles;
}

void objviewerTriangle( TK_TriangleVert a, TK_TriangleVert b, TK_TriangleVert c, void *userData )
{
    ObjMesh *mesh = (ObjMesh*)userData;
    ObjMeshGroup *group = mesh->currentGroup;
    assert(group);
    
    // Add the triangle to the current material group
    ObjDrawBuffer_PushVert( &group->drawbuffer, a );
    ObjDrawBuffer_PushVert( &group->drawbuffer, b );
    ObjDrawBuffer_PushVert( &group->drawbuffer, c );
    
    // Add to the centeroid
    for (int i=0; i < 3; i++)
    {
        mesh->objCenter[i] += a.pos[i];
        mesh->objCenter[i] += b.pos[i];
        mesh->objCenter[i] += c.pos[i];
    }
}

void objviewerFinished( void *userData )
{
    ObjMesh *mesh = (ObjMesh*)userData;
    double centeroidDivisor = mesh->totalNumTriangles * 3;
    mesh->objCenter[0] /= centeroidDivisor;
    mesh->objCenter[1] /= centeroidDivisor;
    mesh->objCenter[2] /= centeroidDivisor;
    
    printf("CENTEROID: %f %f %f\n",
           mesh->objCenter[0],
           mesh->objCenter[1],
           mesh->objCenter[2] );
}

#if 0
void objviewerGeometry( TK_Geometry *geom, void *userData)
{
    ObjMesh *mesh = (ObjMesh*)userData;
    mesh->geom = geom;
    
    printf("OBJVIEWER: Geometry %zu pos verts\n", geom->numVertPos );
}

void objviewerTriangleGroup( const char *mtlname, size_t numTriangles,
                            TK_IndexedTriangle *triangles, void *userData )
{
    ObjMesh *mesh = (ObjMesh*)userData;
    ObjMeshGroup *group = (ObjMeshGroup*)malloc(sizeof(ObjMeshGroup));
    memset( group, 0, sizeof(ObjMeshGroup));
    
    group->mtlName = strdup( mtlname );
    // FIXME: make dynamic
    group->drawbuffer.numTriangleIndexes = numTriangles;
    
    // Go through and re-index the triangles for opengl
    for (size_t i=0; i < numTriangles; i++)
    {
        ObjDrawVert vert = ObjDrawVert_FromIndexTriangleVert( draw)
        group->drawbuffer.triangleIndexes[i*3+0] = triangles[
    }
    
    
    ObjMesh_addGroup( mesh, group );
}
#endif



int main(int argc, char *argv[])
{
    // Load OBJ file
    if (argc < 2) {
        printf("USAGE: objviewer <objfile.obj>\n");
        exit(1);
    }
    
    size_t objFileSize = 0;
    void *objFileData = readEntireFile( argv[1], &objFileSize );
    if (!objFileData) {
        printf("Could not open OBJ file '%s'\n", argv[1] );
    }
    
    
    // Set up our mesh data
    ObjMesh theMesh = {};
    TK_ObjDelegate objDelegate = {};
    objDelegate.userData = (void*)&theMesh;
    objDelegate.error = objviewerErrorMessage;
    objDelegate.material = objviewerMaterial;
    objDelegate.triangle = objviewerTriangle;
    objDelegate.finished = objviewerFinished;
    
    // Extract filename from OBJ path
    char *objFilename = strrchr( argv[1], '/');
    if (objFilename!=NULL) {
        objFilename = objFilename+1;
    } else {
        objFilename = argv[1];
    }
    theMesh.objFilename = strdup( objFilename );
    theMesh.showInspector = true;
    theMesh.objDelegate = &objDelegate;
    
    // Prepass to determine memory reqs
    TK_ParseObj( objFileData, objFileSize, &objDelegate );
    printf("Scratch Mem: %zu\n", objDelegate.scratchMemSize );
    objDelegate.scratchMem = malloc( objDelegate.scratchMemSize );
    
    // Parse again with memory
    TK_ParseObj( objFileData, objFileSize, &objDelegate );
    
    // Setup window
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(1280, 720, "OBJ Viewer", NULL, NULL);
    glfwMakeContextCurrent(window);
    gl3wInit();
    
    // Setup ImGui binding
    ImGui_ImplGlfwGL3_Init(window, true);
        
//    bool show_test_window = true;
    bool show_another_window = false;
    bool show_test_window = false;
    ImVec4 clear_color = ImColor(25, 25, 40);
    
    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplGlfwGL3_NewFrame();
        
//        // 1. Show a simple window
//        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
//        {
//            static float f = 0.0f;
//            ImGui::Text("Hello, world!");
//            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
//            ImGui::ColorEdit3("clear color", (float*)&clear_color);
//            if (ImGui::Button("Test Window")) show_test_window ^= 1;
//            if (ImGui::Button("Another Window")) show_another_window ^= 1;
//            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
//        }
        
        // 2. Show another simple window, this time using an explicit Begin/End pair
        if (show_another_window)
        {
            ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiSetCond_FirstUseEver);
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::Text("Hello");
            ImGui::End();
        }
        
        // Show OBJ file inspector
        if (theMesh.showInspector)
        {
            ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiSetCond_FirstUseEver);
            ImGui::Begin( theMesh.objFilename, &theMesh.showInspector );

            if (ImGui::CollapsingHeader("Statistics"))
            {
                ImGui::BulletText( "Memory: %ld", theMesh.objDelegate->scratchMemSize );
                ImGui::BulletText( "Faces: %ld", theMesh.objDelegate->numFaces );
                ImGui::BulletText( "Triangles: %ld", theMesh.objDelegate->numTriangles );
                ImGui::BulletText( "Verts: %ld", theMesh.objDelegate->numVerts );
                ImGui::BulletText( "Materials: %ld", theMesh.groupCount );
                ImGui::Separator();
            }
            if (ImGui::CollapsingHeader("Display"))
            {
                ImGui::Checkbox("Wireframe", &theMesh.displayOpts.wireFrame );
            }
            
            ImGui::End();
            
        }
        
        // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
        if (show_test_window)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
            ImGui::ShowTestWindow(&show_test_window);
        }
        
        // Update
         ObjMesh_update( &theMesh );
                                                             
        // Rendering
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        
        ObjMesh_renderAll( &theMesh );
        
        ImGui::Render();
        glfwSwapBuffers(window);
    }
    
    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    glfwTerminate();
    
    return 0;
}
