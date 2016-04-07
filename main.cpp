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


// needs initialization
static FreeTypeEncapsulate gFt;

static GLuint gProgramId;
static std::shared_ptr<FreeTypeAtlas> gAtlasPtr;



// for making program from shader collection
#include <string>
#include <fstream>
#include <sstream>

// for std::max(...) (used when calculating texture atlas dimensions)
#include <algorithm>

// for printf(...)
#include <stdio.h>


#define DEBUG     // uncomment to enable the registering of DebugFunc(...)

//// TODO: put this in an atlas
//struct point {
//    GLfloat x;
//    GLfloat y;
//    GLfloat s;
//    GLfloat t;
//};
//
//
//
//
//
//
//// these should really be encapsulated off in some structure somewhere, but for the sake of this
//// barebones demo, keep them here
//GLuint gVbo;        // move to a "FreeTypeContainment" class
//FT_Library gFtLib;  // move to a "FreeTypeContainment" class
//FT_Face gFtFace;    // move to a "FreeTypeContainment" class
//GLuint gProgramId;
//GLint gUniformTextSamplerLoc;   // uniform location within program
//GLint gUniformTextColorLoc;     // uniform location within program



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
//
///*-----------------------------------------------------------------------------------------------
//Description:
//    Encapsulates the creation of an OpenGL GPU program, including the compilation and linking of 
//    shaders.  It tries to cover all the basics and the error reporting and is as self-contained 
//    as possible, only returning a program ID when it is finished.
//Parameters: None
//Returns:
//    The OpenGL ID of the GPU program.
//Exception:  Safe
//Creator:
//    John Cox (2-13-2016)
//-----------------------------------------------------------------------------------------------*/
//GLuint CreateProgram()
//{
//    // hard-coded ignoring possible errors like a boss
//
//    // load up the vertex shader and compile it
//    // Note: After retrieving the file's contents, dump the stringstream's contents into a 
//    // single std::string.  Do this because, in order to provide the data for shader 
//    // compilation, pointers are needed.  The std::string that the stringstream::str() function 
//    // returns is a copy of the data, not a reference or pointer to it, so it will go bad as 
//    // soon as the std::string object disappears.  To deal with it, copy the data into a 
//    // temporary string.
//    std::ifstream shaderFile("shader.vert");
//    std::stringstream shaderData;
//    shaderData << shaderFile.rdbuf();
//    shaderFile.close();
//    std::string tempFileContents = shaderData.str();
//    GLuint vertShaderId = glCreateShader(GL_VERTEX_SHADER);
//    const GLchar *vertShaderBytes[] = { tempFileContents.c_str() };
//    const GLint vertShaderStrLengths[] = { (int)tempFileContents.length() };
//    glShaderSource(vertShaderId, 1, vertShaderBytes, vertShaderStrLengths);
//    glCompileShader(vertShaderId);
//    // alternately (if you are willing to include and link in glutil, boost, and glm), call 
//    // glutil::CompileShader(GL_VERTEX_SHADER, shaderData.str());
//
//    GLint isCompiled = 0;
//    glGetShaderiv(vertShaderId, GL_COMPILE_STATUS, &isCompiled);
//    if (isCompiled == GL_FALSE)
//    {
//        GLchar errLog[128];
//        GLsizei *logLen = 0;
//        glGetShaderInfoLog(vertShaderId, 128, logLen, errLog);
//        printf("vertex shader failed: '%s'\n", errLog);
//        glDeleteShader(vertShaderId);
//        return 0;
//    }
//
//    // load up the fragment shader and compiler it
//    shaderFile.open("shader.frag");
//    shaderData.str(std::string());      // because stringstream::clear() only clears error flags
//    shaderData.clear();                 // clear any error flags that may have popped up
//    shaderData << shaderFile.rdbuf();
//    shaderFile.close();
//    tempFileContents = shaderData.str();
//    GLuint fragShaderId = glCreateShader(GL_FRAGMENT_SHADER);
//    const GLchar *fragShaderBytes[] = { tempFileContents.c_str() };
//    const GLint fragShaderStrLengths[] = { (int)tempFileContents.length() };
//    glShaderSource(fragShaderId, 1, fragShaderBytes, fragShaderStrLengths);
//    glCompileShader(fragShaderId);
//
//    glGetShaderiv(fragShaderId, GL_COMPILE_STATUS, &isCompiled);
//    if (isCompiled == GL_FALSE)
//    {
//        GLchar errLog[128];
//        GLsizei *logLen = 0;
//        glGetShaderInfoLog(fragShaderId, 128, logLen, errLog);
//        printf("fragment shader failed: '%s'\n", errLog);
//        glDeleteShader(vertShaderId);
//        glDeleteShader(fragShaderId);
//        return 0;
//    }
//
//    GLuint programId = glCreateProgram();
//    glAttachShader(programId, vertShaderId);
//    glAttachShader(programId, fragShaderId);
//    glLinkProgram(programId);
//
//    // the program contains binary, linked versions of the shaders, so clean up the compile 
//    // objects
//    // Note: Shader objects need to be un-linked before they can be deleted.  This is ok because
//    // the program safely contains the shaders in binary form.
//    glDetachShader(programId, vertShaderId);
//    glDetachShader(programId, fragShaderId);
//    glDeleteShader(vertShaderId);
//    glDeleteShader(fragShaderId);
//
//    // check if the program was built ok
//    GLint isLinked = 0;
//    glGetProgramiv(programId, GL_LINK_STATUS, &isLinked);
//    if (isLinked == GL_FALSE)
//    {
//        printf("program didn't compile\n");
//        glDeleteProgram(programId);
//        return 0;
//    }
//
//    // done here
//    return programId;
//}
//
//class atlas
//{
//public:
//    // the FT_Face type is a pointer, so don't use a reference or pointer to an FT_Face type
//    // Note: The "face" argument is used to alter font size prior to loading, and it does this 
//    // through a function that takes it as a non-const argument.  But the compiler doesn't 
//    // complain and it runs, so go ahead and keep it const.
//    atlas(const FT_Face face, const int height);
//
//    ~atlas();
//
//    void RenderChar(const char c, const float x, const float y, const float userScaleX, const float userScaleY);
//    //void RenderText(const float x, const float y, const float userScaleX, 
//    //    const float userScaleY);
//private:
//    // have to reference it on every draw call, so keep it around
//    GLuint _textureId;
//
//    // which sampler to use (0 - GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS (??you sure??)
//    GLint _textureSamplerId;
//
//    // the FreeType glyph set provides the basic printable ASCII character set, and even though 
//    // the first 31 are not visible and will therefore not be loaded, the useless bytes are an 
//    // acceptable tradeoff for rapid ASCII character lookup
//    struct FreeTypeGlyphCharInfo {
//        // advance X and Y for screen coordinate calculations
//        float ax;
//        float ay;
//
//        // bitmap left and top for screen coordinate calculations
//        float bl;
//        float bt;
//
//        // glyph bitmap width and height for screen coordinate calculations
//        float bw;
//        float bh;
//
//        // glyph bitmap width and height normalized to [0.0,1.0] for texture coordinate calculations
//        // Note: It is better for performance to do the normalization (a division) at startup.
//        float nbw;
//        float nbh;
//
//        // TODO: change to "texture S" and "texture T" to be clear
//        float tx;	// x offset of glyph in texture coordinates (S on range [0.0,1.0])
//        float ty;	// y offset of glyph in texture coordinates (T on range [0.0,1.0])
//    } _glyphCharInfo[128];   
//};
//
//atlas *gAtlasPtr;
//
//atlas::atlas(const FT_Face face, const int height)
//{
//    // clear out the character memory to all 0s (standard practice for arrays)
//    memset(_glyphCharInfo, 0, sizeof(_glyphCharInfo));
//
//    // set font height to 48 pixels
//    // Note: Setting the pixel width (middle argument) to 0 lets FreeType determine font width 
//    // based on the provided height.
//    // http://learnopengl.com/#!In-Practice/Text-Rendering
//    //FT_Set_Pixel_Sizes(gFtFace, 0, height);
//    FT_Set_Pixel_Sizes(face, 0, height);
//
//    // save some dereferencing
//    //FT_GlyphSlot glyph = gFtFace->glyph;
//    FT_GlyphSlot glyph = face->glyph;
//
//    // before I begin...
//    // A texture atlas means that a bunch of different images are loaded into the same texture.  
//    // In many applications, such as games with sprites, the textures are all the same size, so 
//    // it is rather easy to make a perfect grid.  TrueType fonts are not so pretty for nice 
//    // grids, but when rendered properly they look very nice, so I'm willing to jump through 
//    // some hoops when loading them.  The characters in TrueType fonts are not the same size 
//    // because each character is designed individually and has its own dimensions.  To load 
//    // these into an atlas, I am going to run through all the characters and calculate how much 
//    // space will be required to load them side-by-side, but be aware that this row of 
//    // characters will be of uneven height (not all characters are the same height) and may run 
//    // up against the maximum byte width of a texture.  Then I'll allocate the bytes necessary 
//    // for a texture to hold all the character's glyphs, and finally I'll load them all 
//    // side-by-side in the same order as when I calculated the required texture size.
//    
//    // The GL standard defines a maximum size (in bytes) for textures.  This affects 1D and 2D 
//    // textures (I've been told that 3D textures have their own max values).  1D is just a line, 
//    // so it only applies to that one dimension.  For 2D textures, the max size applies to both 
//    // width and height.  This value is determined by the graphics API.  I checked this program
//    // (on 3-29-2016), and at this time OpenGL is telling me that my max texture size is 16384 
//    // bytes.  This means that I have 16384 bytes for width and 16384 bytes for height, and I 
//    // doubt that I am going to max out either with this FreeType library, although if font size (??font size -> bitmap size??)
//    // became enormous I might need to start a second row.  But while the GL standard does not 
//    // tell the graphics API an upper limit, it does tell them a lower limit for this max value, 
//    // which I believe is 1024 bytes.  That is, OpenGL is free to specify that a maximum texture 
//    // size is 16kb, or 32kb, or whatever, but the max texture size must not be below 1024 
//    // bytes, which should be sufficient to support rather old video or just incapable video 
//    // hardware.  A max size of 1024 bytes means that a 2D texture can 1024x1024 bytes, or 
//    // 1024x512 bytes, or 2x1024 (yes, 2 bytes), and it would be okay.  
//    
//    // for FreeType fonts under default rendering, 1 pixel == 1 byte
//    // Note: FreeType 2 (the header indicates that I am using 2.6.1 as of 3-29-2016) does not 
//    // provide RGB data for a texture, but it does provide monochrome (1-bit) or 8-bit greyscale 
//    // bitmaps.  
//    // http://www.freetype.org/freetype2/docs/ft2faq.html#general-donts
//    // Also Note: When loading a glyph, I use FT_LOAD_RENDER, which causes a character's glyph 
//    // to be loaded from the TrueType file into a small bitmap in the default render mode.  
//    // http://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_LOAD_RENDER
//    // Also Also Note: The default render mode loads an 8-bit greyscale bitmap with 256 levels 
//    // of opacity (2^8) (implicitly, this means 256 levels of opacity per pixel).  "Use linear 
//    // alpha blending and gamma correction to correctly render non-monochrome glyph bitmaps onto 
//    // a surface." (alpha blending is activated in the "render text" method) This means that 
//    // there is one 8-bit byte per pixel.  Keep this in mind when calculating the texture size.  
//    // http://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_Render_Mode
//
//    //DO THINGS WITH THIS
//    GLint max_texture_size;
//    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
//
//    // build up the dimensions for the atlas, then make a texture for it later
//    // Note: Not only are the dimensions needed before allocating space for the texture, but in 
//    // loading up these dimensions BEFORE creating the texture, I can make sure sure that all the
//    // characters load (that is, check that FT_Load_Char(...) does not return errors).
//    // Also Note: The row width builds additively with each glyph, but the row height is only the 
//    // height of the tallest glyph.
//    // Also Also Note:??mention max texture size??
//    //??why restrict to a max width? trying to mimic the actual bitmap layout??
//    unsigned int atlasPixelWidth = 0;
//    unsigned int atlasPixelHeight = 0;
//    unsigned int rowPixelWidth = 0;
//    unsigned int rowPixelHeight = 0;
//
//    // load only the visible characters of the basic ASCII set
//    // Note: The basic set is 2^7 = 128 items, and the first 32 are non-printable, so skip them.
//    for (size_t charCode = 32; charCode < 128; charCode++)
//    {
//        if (FT_Load_Char(face, charCode, FT_LOAD_RENDER))
//        {
//            fprintf(stderr, "Loading character '%c' failed\n", charCode);
//            continue;
//        }
//
//        // if this glyph would make this row's width exceed the max allowable texture size, 
//        // start a new row
//        // Note: Remember that FreeType provides an 8-bit alpha bitmap, so in this particular 
//        // application, each pixel is 1 byte and pixels and bytes can be compared.
//        // Also Also Note: The "+1" is a 1-pixel "gutter" between characters in case some kind 
//        // of float rounding at render time (specifically, calculating texture coordinates S and 
//        // T, each by definition on the range [0.0,1.0]) goes 1 pixel further than it should.  
//        // This 1-pixel "gutter" prevents that 1 pixel from infringing on the edges of another 
//        // glyph.
//        if ((rowPixelWidth + glyph->bitmap.width + 1) >= 1024)
//        {
//            // if this row is longer than any previous row, expand the atlas
//            // Note: Since this only happens if the next glyph will make the row exceed the max 
//            // allowable texture width, the difference between the two will probably only be a 
//            // handful of pixels, but these calculations need to be exact to look right.
//            atlasPixelWidth = std::max(atlasPixelWidth, rowPixelWidth);
//
//            // row height has already been calculated, so just add it
//            atlasPixelHeight += rowPixelHeight;
//
//            // reset row width and height before starting the new row
//            rowPixelWidth = 0;
//            rowPixelHeight = 0;
//        }
//
//        // expand the row's width by the width of this character's glyph + the 1-pixel "gutter"
//        rowPixelWidth += glyph->bitmap.width + 1;
//
//        // if the current glyph is taller than any other glyph in the row, accomodate it
//        // Note: Yes, this means that there will be wasted bytes, but unless I used some kind of 
//        // fancy closest-packing algorithm I am going to have wasted bytes.  But bytes are cheap 
//        // (for PCs, at least), so I'm willing to waste some bytes if it means that atlas 
//        // loading will be easier.
//        rowPixelHeight = std::max(rowPixelHeight, glyph->bitmap.rows);
//    }
//
//    // when the above loop exits, it will have adjusted the variables for row width and height, 
//    // but not for atlas width and height, so take care of the atlas width and height the same 
//    // way as it happens when a new row is created
//    atlasPixelWidth = std::max(atlasPixelWidth, rowPixelWidth);
//    atlasPixelHeight += rowPixelHeight;
//
//    // must have already created AND BOUND a program for this to work
//    glGenTextures(1, &_textureId);
//    glBindTexture(GL_TEXTURE_2D, _textureId);
//
//    // allocate space for the texture in GPU memory
//    // Note: This is why "atlas width" and "atlas height" had to be computed beforehand.
//
//    // some kind of detail thing; leave at default of 0
//    GLint level = 0;
//
//    // tell OpenGL that it should store the data as alpha values (no RGB)
//    // Note: That is, it should store [alpha, alpha, alpha, etc.].  If GL_RGBA (red, green, 
//    // blue, and alpha) were stated instead, then OpenGL would store the data as 
//    // [red, green, blue, alpha, red, green, blue, alpha, red, green, blue, alpha, etc.].
//    GLint internalFormat = GL_ALPHA;
//
//    // tell OpenGL that the data is being provided as alpha values
//    // Note: Similar to "internal format", but this is telling OpenGL how the data is being
//    // provided.  It is possible that many different image file formats store their RGBA 
//    // in many different formats, so "provided format" may differ from one file type to 
//    // another while "internal format" remains the same in order to provide consistency 
//    // after file loading.  In this case though, "internal format" and "provided format"
//    // are the same.
//    GLint providedFormat = GL_ALPHA;
//
//    // knowing the provided format is great and all, but OpenGL is getting the data in the 
//    // form of a void pointer, so it needs to be told if the data is a singed byte, unsigned 
//    // byte, unsigned integer, float, etc.
//    // Note: The shader uses values on the range [0.0, 1.0] for color and alpha values, but
//    // the FreeType face uses a single byte for each alpha value.  Why?  Because a single 
//    // unsigned byte (8 bits) can specify 2^8 = 256 (0 - 255) different values.  This is 
//    // good enough for what FreeType is trying to draw.  OpenGL compensates for the byte by
//    // dividing it by 256 to get it on the range [0.0, 1.0].  
//    // Also Note: Some file formats provide data as sets of floats, in which case this type
//    // would be GL_FLOAT.
//    GLenum providedFormatDataType = GL_UNSIGNED_BYTE;
//
//    // no border (??units? how does this work? play with it??)
//    GLint border = 0;
//
//    // the 0 (null (void *) pointer) at the end tells OpenGL that no data is provided for the 
//    // texture right now, so it should only allocate the required memory for now
//    glTexImage2D(GL_TEXTURE_2D, level, internalFormat, atlasPixelWidth, atlasPixelHeight,
//        border, providedFormat, providedFormatDataType, 0);
//
//    // tell the frag shader which texture sampler to use
//    // ??need to do this now or is it more appropriate to wait until render time??
//    glActiveTexture(GL_TEXTURE0);
//    _textureSamplerId = 0;
//    glUniform1i(gUniformTextSamplerLoc, _textureSamplerId);
//
//    // ??the heck is this??
//    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//
//    // texture configuration setup (I don't understand most of these)
//    // - clamp both S and T (texture's X and Y; they have their own axis names) to edges so that 
//    // any texture coordinates that are provided to OpenGL won't be allowed beyond the [-1, +1] 
//    // range that texture coordinates are restricted to
//    // - linear filtering when the texture needs to be magnified or (I cringe at the term) 
//    // "minified" based on scaling
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//
//    // paste all glyph bitmaps into the texture, but when loading them, I need to keep track of 
//    // where they are
//    int offsetX = 0;
//    int offsetY = 0;
//    // hijack the "row pixel height" and re-use it for helping to calculate Y offset
//    for (size_t charCode = 32; charCode < 128; charCode++)
//    {
//        if (FT_Load_Char(face, charCode, FT_LOAD_RENDER))
//        {
//            fprintf(stderr, "Loading character '%c' failed\n", charCode);
//            continue;
//        }
//
//        // this is the same dea as the "atlas pixel width/height" condition when determining 
//        // atlas size, but now it deals with the byte offsets into the loaded texture
//        if ((offsetX + glyph->bitmap.width + 1) >= 1024)
//        {
//            // vertical offset jumps to next row
//            offsetY += rowPixelHeight;
//
//            // row height and horizontal offset reset to beginning of row
//            rowPixelHeight = 0;
//            offsetX = 0;
//        }
//
//        // send the glyph's bitmap to its own place in the atlas' texture
//        GLint level = 0;
//        GLint internalFormat = GL_ALPHA;
//        GLint providedFormat = GL_ALPHA;
//        GLenum providedFormatDataType = GL_UNSIGNED_BYTE;
//        GLint border = 0;
//        glTexSubImage2D(GL_TEXTURE_2D, level, offsetX, offsetY, glyph->bitmap.width,
//            glyph->bitmap.rows, providedFormat, providedFormatDataType, glyph->bitmap.buffer);
//
//        // save glyph info for render time
//
//        // because "advance" (pixel distance to jump before next character that makes the text 
//        // appear according to font design) is stored, for reasons only the FreeType creator 
//        // knows, in 1/64 pixels, and multiplying by 2^6 (64) will generate it in pixels
//        // Note: The Y advance is only used in fonts that are meant to be written vertically.
//        // The Y advance does NOT describe the distance between lines of text.  Nevertheless,
//        // for the sake of generic font handling, record the Y advance too.
//        _glyphCharInfo[charCode].ax = (float)(glyph->advance.x >> 6);
//        _glyphCharInfo[charCode].ay = (float)(glyph->advance.y >> 6);
//
//        // pixel distance from font origin (a formally defined bottom-left point for the font 
//        // designer) to the bitmap origin (because rectangles outside OpenGL, including the 
//        // bitmap standard, define the top left as the rectangle origin) - yay for different 
//        // standards mixing it up in the same program
//        _glyphCharInfo[charCode].bl = (float)(glyph->bitmap_left);
//        _glyphCharInfo[charCode].bt = (float)(glyph->bitmap_top);
//
//        // don't know how FreeType stores glyphs in the TrueType file format and I don't need to 
//        // since the "load char" function work, but I do need to know where the glyph's data is 
//        // in the atlas' texture
//        // Note: Remember that OpenGL defines texture origin as the bottom left of a rectangle.
//        // Also Note: Remember that a texture has its own pixel coordinate system S and T that
//        // interpolate along a texture from [S=0.0, T=0.0] to [S=1.0T=1.0].
//        _glyphCharInfo[charCode].tx = (float)(offsetX / (float)atlasPixelWidth);
//        _glyphCharInfo[charCode].ty = (float)(offsetY / (float)atlasPixelHeight);
//
//        // rectangles (including OpenGL textures) are defined by an origin (already recorded) 
//        // and width and height
//        // Note: The portion of the atlas texture for this character needs an origin and width and height, and since the origin is in texture coordinates, the width and height for this portion of the texture must also be in texture coordinates.
//        _glyphCharInfo[charCode].bw = (float)(glyph->bitmap.width);
//        _glyphCharInfo[charCode].nbw = (float)(glyph->bitmap.width / (float)atlasPixelWidth);
//        _glyphCharInfo[charCode].bh = (float)(glyph->bitmap.rows);
//        _glyphCharInfo[charCode].nbh = (float)(glyph->bitmap.rows / (float)atlasPixelHeight);
//
//        // just like in the earlier loop
//        rowPixelHeight = std::max(rowPixelHeight, glyph->bitmap.rows);
//        offsetX += glyph->bitmap.width + 1;
//    }
//}
//
//atlas::~atlas()
//{
//    glDeleteTextures(1, &_textureId);
//}
//
//// x and y are screen coordinates (each on the range [-1,+1])
//void atlas::RenderChar(const char c, const float x, const float y, const float userScaleX, const float userScaleY)
//{
//    // the text will be drawn, in part, via a manipulation of pixel alpha values, and apparently
//    // OpenGL's blending does this
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//
//    // bind the texture that contains the atlas and tell OpenGL 
//    glBindTexture(GL_TEXTURE_2D, _textureId);
//    glUniform1i(gUniformTextSamplerLoc, _textureSamplerId);
//
//    // set the 
//    // 2 floats per screen coord, 2 floats per texture coord, so 1 variable will do
//    GLint itemsPerVertexAttrib = 2;
//
//    // how many bytes to "jump" until the next instance of the attribute
//    GLint bytesPerVertex = 4 * sizeof(float);
//
//    // this is cast as a pointer due to OpenGL legacy stuff
//    GLint bufferStartByteOffset = 0;
//
//    // shorthand for "vertex attribute index"
//    GLint vai = 0;
//
//    // need to create 1 quad (2 triangles) for each character, each of which occupies a 
//    // rectangle in the atlas texture
//    // Note: MUST bind BEFORE setting vertex attribute array pointers or it WILL crash.  The GPU 
//    // operates on the values set in the vertex attribute pointers and not on the buffer ID.  
//    // The buffer ID is simply a way to tell OpenGL to activate a certain part of the context, 
//    // and then the following vertex attribute data tells OpenGL how the next draw call is going 
//    // to go down.  This is a GL context thing and those will only work on the currently-set 
//    // buffer.  No error is thrown if the current buffer's ID is 0 (no buffer), and that just 
//    // begs for a silent bug.  A lot of OpenGL code does not clean up the buffer bindings at the 
//    // end of the draw call (why unbind if you're just going to bind another in a moment 
//    // anyway?), and in doing so this error might be swallowed.  
//    glBindBuffer(GL_ARRAY_BUFFER, gVbo);
//
//    // screen coordinates first
//    // Note: 2 floats starting 0 bytes from set start.
//    glEnableVertexAttribArray(vai);
//    glVertexAttribPointer(vai, itemsPerVertexAttrib, GL_FLOAT, GL_FALSE, bytesPerVertex,
//        (void *)bufferStartByteOffset);
//
//    // texture coordinates second
//    // Note: My approach (a common one) is to use the same kind and number of items for each
//    // vertex attribute, so this array's settings are nearly identical to the screen 
//    // coordinate's, the only difference being an offset (screen coordinate bytes first, then
//    // texture coordinate byte; see the box).
//    vai++;
//    bufferStartByteOffset += itemsPerVertexAttrib * sizeof(float);
//    glEnableVertexAttribArray(vai);
//    glVertexAttribPointer(vai, itemsPerVertexAttrib, GL_FLOAT, GL_FALSE, bytesPerVertex,
//        (void *)bufferStartByteOffset);
//
//    // X and Y screen coordinates are on the range [-1,+1]
//    float oneOverScreenPixelWidth = 2.0f / glutGet(GLUT_WINDOW_WIDTH);
//    float oneOverScreenPixelHeight = 2.0f / glutGet(GLUT_WINDOW_HEIGHT);
//
//    // figure out where the texture needs to start drawing in screen coordinates
//    // Note: A glyph has a formal origin point that we (humans) usually think of as being where
//    // the character "starts".  But as far as OpenGL is concerned, I have a 2D rectangle of pixel
//    // data, and that's it.  If I drew that texture at the user-provided x and y, then the 
//    // character would likely look like it isn't centered.  The TrueType format provides info 
//    // that FreeType extracts so that I can figure out where to draw the texture so that it 
//    // LOOKS like the glyph ('g', c, ';', etc.) "starts" at the user-provided x and y.
//    float scaledGlyphLeft = _glyphCharInfo[c].bl * oneOverScreenPixelWidth * userScaleX;
//    float scaledGlyphWidth = _glyphCharInfo[c].bw * oneOverScreenPixelWidth * userScaleX;
//    float scaledGlyphTop = _glyphCharInfo[c].bt * oneOverScreenPixelHeight * userScaleY;
//    float scaledGlyphHeight = _glyphCharInfo[c].bh * oneOverScreenPixelHeight * userScaleY;
//
//    // could these be condensed into the "scaled glyph" calulations? yes, but this is clearer to 
//    // me
//    float screenCoordLeft = x - scaledGlyphLeft;
//    float screenCoordRight = screenCoordLeft + scaledGlyphWidth;
//    float screenCoordTop = y + scaledGlyphTop;
//    float screenCoordBottom = screenCoordTop - scaledGlyphHeight;
//
//    // unlike my project "freeglut_glload_render_freetype", which loads glyphs into their own 
//    // textures one at a time (crude, but conveys basics), I can no longer use the whole texture 
//    // and must use the offset info that was stored when the atlas was created
//    // Note: Remember that textures use their own 2D coordinate system (S,T) to avoid confusion 
//    // with screen coordinates (X,Y).
//    float sLeft = _glyphCharInfo[c].tx;//0.0f;
//    float sRight = _glyphCharInfo[c].tx + _glyphCharInfo[c].nbw;//1.0f;
//    float tBottom = _glyphCharInfo[c].ty;
//    float tTop = _glyphCharInfo[c].ty + _glyphCharInfo[c].nbh;
//
//    // OpenGL draws triangles, but a rectangle needs to be drawn, so specify the four corners
//    // of the box in such a way that GL_LINE_STRIP will draw the two triangle halves of the 
//    // box
//    // Note: As long as the lines from point A to B to C to D don't cross each other (draw 
//    // it on paper), this is fine.  I am going with the pattern 
//    // bottom left -> bottom right -> top left -> top right.
//    // Also Note: Bitmap standards (and most other rectangle standards in programming, such 
//    // as for GUIs) understand the top left as the origin, and therefore the texture's pixels 
//    // are provided in a rectangle that goes from top left to bottom right.  OpenGL is the 
//    // odd one out in the world of rectangles, and it draws textures from lower left 
//    // ([S=0,T=0]) to upper right ([S=1,T=1]).  This means that the texture, from OpenGL's 
//    // perspective, is "upside down".  Yay for different standards.
//    point box[4] = {
//        { screenCoordLeft, screenCoordBottom, sLeft, tTop },
//        { screenCoordRight, screenCoordBottom, sRight, tTop },
//        { screenCoordLeft, screenCoordTop, sLeft, tBottom },
//        { screenCoordRight, screenCoordTop, sRight, tBottom }
//    };
//
//    // the vertex buffer's size is dependent upon string length, and in this demo that value is 
//    // not constant, so the vertex data needs to be completely refreshed every draw call, and 
//    // therefore glBufferData(...) is used instead of glBufferSubData(...) 
//    glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_DYNAMIC_DRAW);
//
//    // all that so that this one function call will work
//    // Note: Start at vertex 0 (that is, start at element 0 in the GL_ARRAY_BUFFER) and draw 
//    // 4 of them.
//    // Also Note: Due to the way that fonts work in texture atlases, only one character is 
//    // drawn at a time, which means that only 1 quad is drawn at a time.  Rather than set up 
//    // indexes and perform an element draw, just use a GL_TRIANGLE_STRIP.  That works great 
//    // for a single (and only a single) quad, hence the hard-coded vertex count (4) in the 
//    // draw call.  If it were not a quad, instancing and glDrawElements(...) should be used
//    // instead.
//    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//
//    // cleanup
//    glBindTexture(GL_TEXTURE_2D, 0);
//    glBindBuffer(GL_ARRAY_BUFFER, 0);
//    glDisable(GL_BLEND);
//    glBlendFunc(0, 0);
//}

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
    glUseProgram(gProgramId);

    // clear existing data
    glClearColor(0.2f, 0.2f, 0.2f, 0.0f);   // make background a dull grey
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //// the shader for this program uses a vec4 (implicit content type is float) for color, so 
    //// specify text color as a 4-float array and give it to shader 
    GLfloat color[4] = { 0.5f, 0.5f, 0, 1 };
    //glUniform4fv(gUniformTextColorLoc, 1, color);

    //// draw all the stuff that we want to, then swap the buffers
    //// Note: Viable X and Y values are on the range [-1, +1].  No world->camera->clip->screen 
    //// space conversions are performed here, so they must be specified in screen space. 
    //// (??you sure? X = -1, Y = -1 doesn't draw??)
    //float X = -0.99f;
    //float Y = -0.99f;
    //gAtlasPtr->RenderChar('{', X, Y, 1.0f, 1.0f);

    //X = -0.5f;
    //Y = -0.5f;
    //gAtlasPtr->RenderChar('L', X, Y, 2.0f, 2.0f);

    //X = +0.5f;
    //Y = -0.23f;
    //gAtlasPtr->RenderChar('p', X, Y, 4.0f, 4.0f);

    float X = -0.99f;
    float Y = -0.99f;
    gAtlasPtr->RenderChar('{', X, Y, 1.0f, 1.0f, color);

    X = -0.5f;
    Y = -0.5f;

    gAtlasPtr->RenderChar('L', X, Y, 2.0f, 2.0f, color);

    X = +0.5f;
    Y = -0.23f;
    gAtlasPtr->RenderChar('p', X, Y, 4.0f, 4.0f, color);


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
//
///*-----------------------------------------------------------------------------------------------
//Description:
//    Convenient for deleting anything that needs cleaning up.
//Parameters: None
//Returns:    None
//Exception:  Safe
//Creator:
//    John Cox (3-9-2016)
//-----------------------------------------------------------------------------------------------*/
//void FreeResources()
//{
//    glDeleteProgram(gProgramId);
//    glDeleteBuffers(1, &gVbo);
//}

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
        return 0;
    }
    else if (glext_ARB_debug_output)
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

    gProgramId = gFt.Init("FreeSans.ttf", "shader.vert", "shader.frag");
    if (0 == gProgramId)
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

    //// FreeType needs to load itself into particular variables
    //// Note: FT_Init_FreeType(...) returns something called an FT_Error, which VS can't find.
    //// Based on the useage, it is assumed that 0 is returned if something went wrong, otherwise
    //// non-zero is returned.  That is the only explanation for this kind of condition.
    //if (FT_Init_FreeType(&gFtLib))
    //{
    //    fprintf(stderr, "Could not init freetype library\n");
    //    return false;
    //}
    //
    //// Note: FT_New_Face(...) also returns an FT_Error.
    //const char *fontFilePath = "FreeSans.ttf";  // file path relative to solution directory
    //if (FT_New_Face(gFtLib, fontFilePath, 0, &gFtFace))
    //{
    //    fprintf(stderr, "Could not open font '%s'\n", fontFilePath);
    //    return false;
    //}

    //gProgramId = CreateProgram();
    //
    //// pick out the attributes and uniforms used in the FreeType GPU program

    //char textTextureName[] = "textureSamplerId";
    //gUniformTextSamplerLoc = glGetUniformLocation(gProgramId, textTextureName);
    //if (gUniformTextSamplerLoc == -1)
    //{
    //    fprintf(stderr, "Could not bind uniform '%s'\n", textTextureName);
    //    return false;
    //}

    ////char textColorName[] = "color";
    //char textColorName[] = "textureColor";
    //gUniformTextColorLoc = glGetUniformLocation(gProgramId, textColorName);
    //if (gUniformTextColorLoc == -1)
    //{
    //    fprintf(stderr, "Could not bind uniform '%s'\n", textColorName);
    //    return false;
    //}

    //// start up the font texture atlas now that the program is created and uniforms are recorded
    //gAtlasPtr = new atlas(gFtFace, 48);

    //// create the vertex buffer that will be used to create quads as a base for the FreeType 
    //// glyph textures
    //glGenBuffers(1, &gVbo);
    //if (gVbo == -1)
    //{
    //    fprintf(stderr, "could not generate vertex buffer object\n");
    //    return false;
    //}

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

    //// Note: Apparently there is a function FreeResource(...) (note the lack of an appending 's')
    //// off in some API (I don't know where).  Use the FreeResources() that was defined in this 
    //// file.
    //FreeResources();
    return 0;
}