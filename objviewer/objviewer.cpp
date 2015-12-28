
#include <string.h>

#include <imgui.h>

#include "imgui_impl_glfw_gl3.h"

#include <stdio.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#define TK_OBJFILE_IMPLEMENTATION
#include "tk_objfile.h"

typedef struct ObjMeshGroupStruct
{
    char *mtlName;
    size_t numTriangles;
    TK_IndexedTriangle *triangles;
    
    ObjMeshGroupStruct *next;
    
} ObjMeshGroup;

struct ObjDisplayOptions
{
    bool wireFrame;
};

struct ObjMesh
{
    // General info
    TK_ObjDelegate *objDelegate;
    char *objFilename;
    
    // Obj geometry
    TK_Geometry *geom;
    ObjMeshGroup *rootGroup;
    
    // GUI stuff
    ObjDisplayOptions displayOpts;
    bool showInspector;
};

void ObjMesh_addGroup( ObjMesh *mesh, ObjMeshGroup *group)
{
    group->next = mesh->rootGroup;
    mesh->rootGroup = group;
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
    
    group->mtlName = strdup( mtlname );
    group->numTriangles = numTriangles;
    group->triangles = triangles;
    
    ObjMesh_addGroup( mesh, group );
}



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
    objDelegate.geometry = objviewerGeometry;
    objDelegate.triangleGroup = objviewerTriangleGroup;
    
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
    bool show_test_window = true;
    ImVec4 clear_color = ImColor(114, 144, 154);
    
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
                ImGui::BulletText( "Materials: %ld", theMesh.geom->numMaterials );
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
        
        // Rendering
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        glfwSwapBuffers(window);
    }
    
    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    glfwTerminate();
    
    return 0;
}
