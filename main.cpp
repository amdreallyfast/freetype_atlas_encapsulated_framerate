// the OpenGL version include also includes all previous versions
// Build note: Due to a minefield of preprocessor build flags, the gl_load.hpp must come after 
// the version include.
// Build note: Do NOT mistakenly include _int_gl_4_4.h.  That one doesn't define OpenGL stuff 
// first.
// Build note: Also need to link opengl32.lib (unknown directory, but VS seems to know where it 
// is, so don't put in an "Additional Library Directories" entry for it).
// Build note: Also need to link glload/lib/glloadD.lib.
#include "glload/include/glload/gl_4_4.h"
#include "glload/include/glload/gl_load.hpp"

// Build note: Must be included after OpenGL code (in this case, glload).
// Build note: Also need to link freeglut/lib/freeglutD.lib.  However, the linker will try to 
// find "freeglut.lib" (note the lack of "D") instead unless the following preprocessor 
// directives are set either here or in the source-building command line (VS has a
// "Preprocessor" section under "C/C++" for preprocessor definitions).
// Build note: Also need to link winmm.lib (VS seems to know where it is, so don't put in an 
// "Additional Library Directories" entry).
#define FREEGLUT_STATIC
#define _LIB
#define FREEGLUT_LIB_PRAGMAS 0
#include "freeglut/include/GL/freeglut.h"

// FreeType builds everything with a lot of pre-defined header paths that are all relative to the
// "freetype-2.6.1/include/" folder, so unfortunately (for the sake of a barebones demo) that 
// folder has to be added to the list of "additional include directories" that the project looks
// through when compiling
#include <ft2build.h>
#include FT_FREETYPE_H  // also defined relative to "freetype-2.6.1/include/"

// this linking approach is very useful for portable, crude, barebones demo code, but it is 
// better to link through the project building properties
#pragma comment(lib, "glload/lib/glloadD.lib")
#pragma comment(lib, "opengl32.lib")            // needed for glload::LoadFunctions()
#pragma comment(lib, "freeglut/lib/freeglutD.lib")
#ifdef WIN32
#pragma comment(lib, "winmm.lib")               // Windows-specific; freeglut needs it
#endif

// apparently the FreeType lib also needs a companion file, "freetype261d.pdb"
#pragma comment (lib, "freetype-2.6.1/objs/vc2010/Win32/freetype261d.lib")

#include "FreeTypeEncapsulate.h"
#include "FreeTypeAtlas.h"
#include "Stopwatch.h"


// needs initialization
static FreeTypeEncapsulate gFt;

static GLuint gTextTextureProgramId;
static std::shared_ptr<FreeTypeAtlas> gAtlasPtr;

static Timing::Stopwatch gTimer;

// for making program from shader collection
#include <string>
#include <fstream>
#include <sstream>

// for std::max(...) (used when calculating texture atlas dimensions)
#include <algorithm>

// for printf(...)
#include <stdio.h>


#define DEBUG     // uncomment to enable the registering of DebugFunc(...)


/*-----------------------------------------------------------------------------------------------
Description:
    For use with glutInitContextFlags(flagA | flagB | ... | GLUT_DEBUG).  If GLUT_DEBUG is 
    enabled, then apparently the OpenGL global glext_ARB_debug_output will be enabled, which in
    turn will cause the init(...) function to register this function in the OpenGL pipeline.  
    Then, if an error occurs, this function is called automatically to report the error.  This 
    is handy so that glGetError() doesn't have to be called everywhere, complete with debug flags 
    for every single call.

Parameters:
    Unknown.  The function pointer is provided to glDebugMessageCallbackARB(...), and that
    function is responsible for calling this one as it sees fit.
Returns:    None
Exception:  Safe
Creator:
    John Cox (2014)
-----------------------------------------------------------------------------------------------*/
void APIENTRY DebugFunc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    const GLchar* message, const GLvoid* userParam)
{
    std::string srcName;
    switch (source)
    {
    case GL_DEBUG_SOURCE_API_ARB: srcName = "API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: srcName = "Window System"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: srcName = "Shader Compiler"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: srcName = "Third Party"; break;
    case GL_DEBUG_SOURCE_APPLICATION_ARB: srcName = "Application"; break;
    case GL_DEBUG_SOURCE_OTHER_ARB: srcName = "Other"; break;
    }

    std::string errorType;
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR_ARB: errorType = "Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: errorType = "Deprecated Functionality"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: errorType = "Undefined Behavior"; break;
    case GL_DEBUG_TYPE_PORTABILITY_ARB: errorType = "Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE_ARB: errorType = "Performance"; break;
    case GL_DEBUG_TYPE_OTHER_ARB: errorType = "Other"; break;
    }

    std::string typeSeverity;
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH_ARB: typeSeverity = "High"; break;
    case GL_DEBUG_SEVERITY_MEDIUM_ARB: typeSeverity = "Medium"; break;
    case GL_DEBUG_SEVERITY_LOW_ARB: typeSeverity = "Low"; break;
    }

    printf("%s from %s,\t%s priority\nMessage: %s\n",
        errorType.c_str(), srcName.c_str(), typeSeverity.c_str(), message);
}

/*-----------------------------------------------------------------------------------------------
Description:
    This is the rendering function.  It tells OpenGL to clear out some color and depth buffers,
    to set up the data to draw, to draw than stuff, and to report any errors that it came across.
    This is not a user-called function.

    This function is registered with glutDisplayFunc(...) during glut's initialization.
Parameters: None
Returns:    None
Exception:  Safe
Creator:
    John Cox (2-13-2016)
-----------------------------------------------------------------------------------------------*/
void display()
{
    glUseProgram(gTextTextureProgramId);

    // clear existing data
    glClearColor(0.2f, 0.2f, 0.2f, 0.0f);   // make background a dull grey
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // only report frame rate every second
    static int elapsedFrames = 0;
    static double elapsedTime = 0.0;
    static double frameRate = 0.0;
    elapsedFrames++;
    elapsedTime += gTimer.lap();
    if (elapsedTime > 1.0)
    {
        frameRate = (double)elapsedFrames / elapsedTime;
        
        // reset the counters
        elapsedFrames = 0;
        elapsedTime = 0.0;
    }

    // the shader for this program uses a vec4 (implicit content type is float) for color, so 
    // specify text color as a 4-float array and give it to shader 
    // Note: Even though color only needs RGB, use an alpha value as well in case some text
    // transparency is desired.
    GLfloat color[4] = { 0.5f, 0.5f, 0.0f, 1.0f };
    char str[8];
    sprintf(str, "%.2lf", frameRate);
    float xy[2] = { -0.99f, -0.99f };
    float scaleXY[2] = { 1.0f, 1.0f };
    //gAtlasPtr->RenderText("{123}", xy, scaleXY, color);
    gAtlasPtr->RenderText(str, xy, scaleXY, color);

    //xy[0] = -0.5f;
    //xy[1] = -0.5f;
    //scaleXY[0] = 2.0f;
    //scaleXY[1] = 2.0f;
    //gAtlasPtr->RenderChar('L', xy, scaleXY, color);

    //xy[0] = +0.5f;
    //xy[1] = -0.23f;
    //scaleXY[0] = 4.0f;
    //scaleXY[1] = 4.0f;
    //gAtlasPtr->RenderChar('p', xy, scaleXY, color);


    // tell the GPU to swap out the displayed buffer with the one that was just rendered
    glutSwapBuffers();

    // tell glut to call this display() function again on the next iteration of the main loop
    // Note: https://www.opengl.org/discussion_boards/showthread.php/168717-I-dont-understand-what-glutPostRedisplay()-does
    glutPostRedisplay();

    // clean up bindings
    // Note: This is just good practice, but in reality the bindings can be left as they were 
    // and re-bound on each new call to this rendering function.
    glUseProgram(0);
}

/*-----------------------------------------------------------------------------------------------
Description:
    Tell's OpenGL to resize the viewport based on the arguments provided.  This is not a 
    user-called function.

    This function is registered with glutReshapeFunc(...) during glut's initialization.
Parameters: 
    w   The width of the window in pixels.
    h   The height of the window in pixels.
Returns:    None
Exception:  Safe
Creator:
    John Cox (2-13-2016)
-----------------------------------------------------------------------------------------------*/
void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
}

/*-----------------------------------------------------------------------------------------------
Description:
    Executes some kind of command when the user presses a key on the keyboard.  This is not a 
    user-called function.

    This function is registered with glutKeyboardFunc(...) during glut's initialization.
Parameters: 
    key     The ASCII code of the key that was pressed (ex: ESC key is 27)
    x       ??
    y       ??
Returns:    None
Exception:  Safe
Creator:
    John Cox (2-13-2016)
-----------------------------------------------------------------------------------------------*/
void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 27:
    {
        // ESC key
        glutLeaveMainLoop();
        return;
    }
    default:
        break;
    }
}

/*-----------------------------------------------------------------------------------------------
Description:
    ??what does it do? I picked it up awhile back and haven't changed it??
Parameters:
    displayMode     ??
    width           ??
    height          ??
Returns:    
    ??what??
Exception:  Safe
Creator:
    John Cox (2-13-2016)
-----------------------------------------------------------------------------------------------*/
unsigned int defaults(unsigned int displayMode, int &width, int &height) 
{ 
    return displayMode; 
}

/*-----------------------------------------------------------------------------------------------
Description:
    Governs window creation, the initial OpenGL configuration (face culling, depth mask, even
    though this is a 2D demo and that stuff won't be of concern), the creation of geometry, and
    the creation of a texture.
Parameters:
    argc    (From main(...)) The number of char * items in argv.  For glut's initialization.
    argv    (From main(...)) A collection of argument strings.  For glut's initialization.
Returns:
    False if something went wrong during initialization, otherwise true;
Exception:  Safe
Creator:
    John Cox (3-7-2016)
-----------------------------------------------------------------------------------------------*/
bool init(int argc, char *argv[])
{
    glutInit(&argc, argv);

    // I don't know what this is doing, but it has been working, so I'll leave it be for now
    int windowWidth = 500;  // square 500x500 pixels
    int windowHeight = 500;
    unsigned int displayMode = GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL;
    displayMode = defaults(displayMode, windowWidth, windowHeight);
    glutInitDisplayMode(displayMode);

    // create the window
    // ??does this have to be done AFTER glutInitDisplayMode(...)??
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(300, 200);   // X = 0 is screen left, Y = 0 is screen top
    int window = glutCreateWindow(argv[0]);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);

    // OpenGL 4.3 was where internal debugging was enabled, freeing the user from having to call
    // glGetError() and analyzing it after every single OpenGL call, complete with surrounding it
    // with #ifdef DEBUG ... #endif blocks
    // Note: https://blog.nobel-joergensen.com/2013/02/17/debugging-opengl-part-2-using-gldebugmessagecallback/
    int glMajorVersion = 4;
    int glMinorVersion = 4;
    glutInitContextVersion(glMajorVersion, glMinorVersion);
    glutInitContextProfile(GLUT_CORE_PROFILE);
#ifdef DEBUG
    glutInitContextFlags(GLUT_DEBUG);   // if enabled, 
#endif

    // glload must load AFTER glut loads the context
    glload::LoadTest glLoadGood = glload::LoadFunctions();
    if (!glLoadGood)    // apparently it has an overload for "bool type"
    {
        printf("glload::LoadFunctions() failed\n");
        return false;
    }
    else if (!glload::IsVersionGEQ(glMajorVersion, glMinorVersion))
    {
        // the "is version" check is an "is at least version" check
        printf("Your OpenGL version is %i, %i. You must have at least OpenGL %i.%i to run this tutorial.\n",
            glload::GetMajorVersion(), glload::GetMinorVersion(), glMajorVersion, glMinorVersion);
        glutDestroyWindow(window);
        return false;
    }

    // no problems initializing glload, so now register the debugging function (if necessary)
    if (glext_ARB_debug_output)
    {
        // condition will be true if GLUT_DEBUG is a context flag
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        glDebugMessageCallbackARB(DebugFunc, (void*)15);
    }

    // these OpenGL initializations are for 3D stuff, where depth matters and multiple shapes can
    // be "on top" of each other relative to the most distant thing rendered, and this barebones 
    // code is only for 2D stuff, but initialize them anyway as good practice (??bad idea? only 
    // use these once 3D becomes a thing??)
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);

    gTextTextureProgramId = gFt.Init("FreeSans.ttf", "shader.vert", "shader.frag");
    if (0 == gTextTextureProgramId)
    {
        fprintf(stderr, "FreeType could not be initialized\n");
        return false;
    }

    // set font height to 48 pixels
    gAtlasPtr = gFt.GenerateAtlas(48);
    if (0 == gAtlasPtr)
    {
        fprintf(stderr, "FreeType atlas could not be initialized\n");
        return false;
    }

    // initialize the timer, which will be used for frame rate calculations
    if (!gTimer.initialize())
    {
        fprintf(stderr, "Stopwatch could not initialized\n");
        return false;
    }
    gTimer.start();

    // all went well
    return true;
}

/*-----------------------------------------------------------------------------------------------
Description:
    Program start and end.
Parameters: 
    argc    The number of strings in argv.
    argv    A pointer to an array of null-terminated, C-style strings.
Returns:
    0 if program ended well, which it always does or it crashes outright, so returning 0 is fune
Exception:  Safe
Creator:
    John Cox (2-13-2016)
-----------------------------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    if (!init(argc, argv))
    {
        // bad initialization; it will take care of it's own error reporting
        return 1;
    }

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMainLoop();

    return 0;
}