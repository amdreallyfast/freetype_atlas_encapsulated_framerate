#pragma once

// there doesn't seem to be any way out of including all of FreeType without some shady forward 
// declarations of FreeType interna structures
// Note: FreeType has a single header for everything.  This is evil.
#include <ft2build.h>
#include FT_FREETYPE_H  // also defined relative to "freetype-2.6.1/include/"

class FreeTypeAtlas
{
public:
    // the FT_Face type is a pointer, so don't use a reference or pointer
    FreeTypeAtlas(const FT_Face face, const int height);

    ~FreeTypeAtlas();

    void RenderChar(const char c, const float x, const float y, const float userScaleX, const float userScaleY);
    //void RenderText(const float x, const float y, const float userScaleX, 
    //    const float userScaleY);
private:
    // have to reference it on every draw call, so keep it around
    // Note: It is actually a GLuint, which is a typedef of "unsigned int", but I don't want to 
    // include all of the OpenGL declarations in a header file, so just use the original type.
    // It is a tad shady, but keep an eye on build warnings about incompatible types and you
    // should be fine.
    unsigned int _textureId;

    // which sampler to use (0 - GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS (??you sure??)
    // Note: Technically it is a GLint, but is simple int here for the same reason 
    // "texture ID" is an unsigned int instead of GLuint.
    int _textureSamplerId;

    // the FreeType glyph set provides the basic printable ASCII character set, and even though 
    // the first 31 are not visible and will therefore not be loaded, the useless bytes are an 
    // acceptable tradeoff for rapid ASCII character lookup
    struct FreeTypeGlyphCharInfo {
        // advance X and Y for screen coordinate calculations
        float ax;
        float ay;

        // bitmap left and top for screen coordinate calculations
        float bl;
        float bt;

        // glyph bitmap width and height for screen coordinate calculations
        float bw;
        float bh;

        // glyph bitmap width and height normalized to [0.0,1.0] for texture coordinate calculations
        // Note: It is better for performance to do the normalization (a division) at startup.
        float nbw;
        float nbh;

        // TODO: change to "texture S" and "texture T" to be clear
        float tx;	// x offset of glyph in texture coordinates (S on range [0.0,1.0])
        float ty;	// y offset of glyph in texture coordinates (T on range [0.0,1.0])
    } _glyphCharInfo[128];
};

