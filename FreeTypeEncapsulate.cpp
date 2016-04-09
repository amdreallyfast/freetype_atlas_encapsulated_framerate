#include "FreeTypeEncapsulate.h"

// the OpenGL version include also includes all previous versions
// Build note: Do NOT mistakenly include _int_gl_4_4.h.  That one doesn't define OpenGL stuff 
// first.
#include "glload/include/glload/gl_4_4.h"

// for making program from shader collection
#include <string>
#include <fstream>
#include <sstream>


FreeTypeEncapsulate::FreeTypeEncapsulate()
    :
    _haveInitialized(0),
    _programId(0),
    _uniformTextSamplerLoc(0),
    _uniformTextColorLoc(0)
{
}

FreeTypeEncapsulate::~FreeTypeEncapsulate()
{
    // cleanup
    glDeleteProgram(_programId);
}

int FreeTypeEncapsulate::Init(const std::string &trueTypeFontFilePath, 
    const std::string &vertShaderPath, const std::string &fragShaderPath)
{
    // FreeType needs to load itself into particular variables
    // Note: FT_Init_FreeType(...) returns something called an FT_Error, which VS can't find.
    // Based on the useage, it is assumed that 0 is returned if something went wrong, otherwise
    // non-zero is returned.  That is the only explanation for this kind of condition.
    if (FT_Init_FreeType(&_ftLib))
    {
        fprintf(stderr, "Could not init freetype library\n");
        return false;
    }

    // Note: FT_New_Face(...) also returns an FT_Error.
    if (FT_New_Face(_ftLib, trueTypeFontFilePath.c_str(), 0, &_ftFace))
    {
        fprintf(stderr, "Could not open font '%s'\n", trueTypeFontFilePath.c_str());
        return false;
    }

    _programId = CreateFreeTypeProgram(vertShaderPath, fragShaderPath);

    // pick out the attributes and uniforms used in the FreeType GPU program

    char textTextureName[] = "textureSamplerId";
    _uniformTextSamplerLoc = glGetUniformLocation(_programId, textTextureName);
    if (_uniformTextSamplerLoc == -1)
    {
        fprintf(stderr, "Could not bind uniform '%s'\n", textTextureName);
        return false;
    }

    //char textColorName[] = "color";
    char textColorName[] = "textureColor";
    _uniformTextColorLoc = glGetUniformLocation(_programId, textColorName);
    if (_uniformTextColorLoc == -1)
    {
        fprintf(stderr, "Could not bind uniform '%s'\n", textColorName);
        return false;
    }

    _haveInitialized = true;
    return _programId;
}

const std::shared_ptr<FreeTypeAtlas> FreeTypeEncapsulate::GenerateAtlas(const int fontSize)
{
    if (!_haveInitialized)
    {
        fprintf(stderr, "FreeTypeEncapsulate object has not been initialized\n");
        return false;
    }

    std::shared_ptr<FreeTypeAtlas> newAtlasPtr = std::make_shared<FreeTypeAtlas>(
        _uniformTextSamplerLoc, _uniformTextColorLoc);
    newAtlasPtr->Init(_ftFace, fontSize);

    return newAtlasPtr;
}

/*-----------------------------------------------------------------------------------------------
Description:
    Encapsulates the creation of an OpenGL GPU program, including the compilation and linking of
    shaders.  It tries to cover all the basics and the error reporting and is as self-contained
    as possible, only returning a program ID when it is finished.
Parameters: None
Returns:
    The OpenGL ID of the GPU program.
Exception:  Safe
Creator:
    John Cox (2-13-2016)
-----------------------------------------------------------------------------------------------*/
unsigned int FreeTypeEncapsulate::CreateFreeTypeProgram(const std::string &vertShaderPath, 
    const std::string &fragShaderPath)
{
    // hard-coded and ignoring possible errors like a boss

    // load up the vertex shader and compile it
    // Note: After retrieving the file's contents, dump the stringstream's contents into a 
    // single std::string.  Do this because, in order to provide the data for shader 
    // compilation, pointers are needed.  The std::string that the stringstream::str() function 
    // returns is a copy of the data, not a reference or pointer to it, so it will go bad as 
    // soon as the std::string object disappears.  To deal with it, copy the data into a 
    // temporary string.
    std::ifstream shaderFile(vertShaderPath);
    std::stringstream shaderData;
    shaderData << shaderFile.rdbuf();
    shaderFile.close();
    std::string tempFileContents = shaderData.str();
    GLuint vertShaderId = glCreateShader(GL_VERTEX_SHADER);
    const GLchar *vertShaderBytes[] = { tempFileContents.c_str() };
    const GLint vertShaderStrLengths[] = { (int)tempFileContents.length() };
    glShaderSource(vertShaderId, 1, vertShaderBytes, vertShaderStrLengths);
    glCompileShader(vertShaderId);
    // alternately (if you are willing to include and link in glutil, boost, and glm), call 
    // glutil::CompileShader(GL_VERTEX_SHADER, shaderData.str());

    GLint isCompiled = 0;
    glGetShaderiv(vertShaderId, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE)
    {
        GLchar errLog[128];
        GLsizei *logLen = 0;
        glGetShaderInfoLog(vertShaderId, 128, logLen, errLog);
        printf("vertex shader failed: '%s'\n", errLog);
        glDeleteShader(vertShaderId);
        return 0;
    }

    // load up the fragment shader and compiler it
    shaderFile.open(fragShaderPath);
    shaderData.str(std::string());      // because stringstream::clear() only clears error flags
    shaderData.clear();                 // clear any error flags that may have popped up
    shaderData << shaderFile.rdbuf();
    shaderFile.close();
    tempFileContents = shaderData.str();
    GLuint fragShaderId = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar *fragShaderBytes[] = { tempFileContents.c_str() };
    const GLint fragShaderStrLengths[] = { (int)tempFileContents.length() };
    glShaderSource(fragShaderId, 1, fragShaderBytes, fragShaderStrLengths);
    glCompileShader(fragShaderId);

    glGetShaderiv(fragShaderId, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE)
    {
        GLchar errLog[128];
        GLsizei *logLen = 0;
        glGetShaderInfoLog(fragShaderId, 128, logLen, errLog);
        printf("fragment shader failed: '%s'\n", errLog);
        glDeleteShader(vertShaderId);
        glDeleteShader(fragShaderId);
        return 0;
    }

    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertShaderId);
    glAttachShader(programId, fragShaderId);
    glLinkProgram(programId);

    // the program contains binary, linked versions of the shaders, so clean up the compile 
    // objects
    // Note: Shader objects need to be un-linked before they can be deleted.  This is ok because
    // the program safely contains the shaders in binary form.
    glDetachShader(programId, vertShaderId);
    glDetachShader(programId, fragShaderId);
    glDeleteShader(vertShaderId);
    glDeleteShader(fragShaderId);

    // check if the program was built ok
    GLint isLinked = 0;
    glGetProgramiv(programId, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE)
    {
        printf("program didn't compile\n");
        glDeleteProgram(programId);
        return 0;
    }

    // done here
    return programId;
}

