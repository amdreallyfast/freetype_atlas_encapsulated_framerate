#version 440

// must have the same name as its corresponding "out" item in the vert shader
smooth in vec2 texturePos;
uniform sampler2D textureSamplerId;
uniform vec4 textureColor;

// because gl_FragColor is officially deprecated by 4.4
out vec4 finalColor;

void main(void) {
    // the texture only provides us with alpha values, so only draw out the .a
    finalColor = vec4(1, 1, 1, texture2D(textureSamplerId, texturePos).a) * textureColor;
}
    