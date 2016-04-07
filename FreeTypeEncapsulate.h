#pragma once

// there doesn't seem to be any way out of including all of FreeType without some shady forward 
// declarations of FreeType interna structures
// Note: FreeType has a single header for everything.  This is evil.
#include <ft2build.h>
#include FT_FREETYPE_H  // also defined relative to "freetype-2.6.1/include/"

// because the FreeType encapsulation contains info necessary to create the atlas
#include "FreeTypeAtlas.h"

#include <string>
#include <memory>   // for the shared pointer

class FreeTypeEncapsulate
{
public:
    FreeTypeEncapsulate();
    ~FreeTypeEncapsulate();

    // takes: file path relative to solution directory
    // returns: shader program ID if initialization successful, otherwise 0
    int Init(const std::string &trueTypeFontFilePath, 
        const std::string &vertShaderPath, const std::string &fragShaderPath);

    // the shared pointer will encapsulate the atlas' pointer and clean up after it is 
    // unecessary, and it is const so that the user can't even try to re-initialize it
    const std::shared_ptr<FreeTypeAtlas> GenerateAtlas(const int fontSize);

private:
    bool _haveInitialized;

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
};