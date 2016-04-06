#pragma once

// there doesn't seem to be any way out of including all of FreeType without some shady forward 
// declarations of FreeType interna structures
// Note: FreeType has a single header for everything.  This is evil.
#include <ft2build.h>
#include FT_FREETYPE_H  // also defined relative to "freetype-2.6.1/include/"

#include <string>

class FreeTypeEncapsulate
{
public:
    FreeTypeEncapsulate();
    ~FreeTypeEncapsulate();

    bool Init(const std::string &vertShaderPath, const std::string &fragShaderPath);

private:
    FT_Library _ftLib;  // move to a "FreeTypeContainment" class
    FT_Face _ftFace;    // move to a "FreeTypeContainment" class

    unsigned int CreateFreeTypeProgram(const std::string &vertShaderPath, 
        const std::string &fragShaderPath);

    // these are actually GLuint and GLint values, but I didn't want to include all of OpenGL 
    // just for the typedefs, which is unfortunately necessary since only readily available 
    // header files for OpenGL include everything
    // Note: Each character glyph will be a part of a larger texture, and any texture drawing 
    // requires its own OpenGL draw call.  Therefore only character can draw at a time, and 
    // therefore only one atlas can draw at a time.  This FreeType texture atlas stuff will 
    // require its own OpenGL program, so this class is a great place to put said program's ID, 
    // the locations of the shader uniforms for that program, and the vertex buffer that all 
    // atlas drawing will share.
    
    unsigned int _programId;
    int _uniformTextSamplerLoc;   // uniform location within program
    int _uniformTextColorLoc;     // uniform location within program
    unsigned int _vbo;

};