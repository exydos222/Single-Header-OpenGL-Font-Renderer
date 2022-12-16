#pragma once
#include <map>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <vector>
#include <string>
#include <GL/glew.h>

unsigned int textureVBO, textureVAO, textureEBO;
unsigned int atlasTexture;

unsigned short chCount = 0, indCount = 0;
std::vector<float> vertices;
std::vector<unsigned int> indices;

struct vec2f {
    float x, y;
};

struct Character {
    vec2f Size;
    vec2f Bearing;
    vec2f Atlas;
    vec2f AtlasSize;
    float Advance;
};

std::map<char, Character> Characters;
unsigned int textureProgram;

const inline void InitFontRenderer(unsigned short windowWindow, unsigned short windowHeight, const char* fontName, unsigned short minASCII = 32, unsigned short maxASCII = 126) {
    FT_Library ft;
    FT_Init_FreeType(&ft);

    FT_Face face;

    FT_New_Face(ft, fontName, 0, &face);

    FT_Set_Pixel_Sizes(face, 0, windowHeight / 40);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    unsigned short roww = 0;
    unsigned char rowh = 0;
    unsigned short w = 0;
    unsigned char h = 0;

    glActiveTexture(GL_TEXTURE0);

    for (unsigned short c = 0; c < 255; c++) {
        FT_Load_Char(face, c, FT_LOAD_RENDER);

        if (roww + face->glyph->bitmap.width + 1 >= 1024) {
            w = std::max(w, roww);
            h += rowh;
            roww = 0;
            rowh = 0;
        }
        roww += face->glyph->bitmap.width + 1;
        rowh = std::max(rowh, (unsigned char)face->glyph->bitmap.rows);
    }
    w = std::max(w, roww);
    h += rowh;

    glGenTextures(1, &atlasTexture);
    glBindTexture(GL_TEXTURE_2D, atlasTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);

    unsigned short ox = 0;
    unsigned short oy = 0;

    rowh = 0;

    for (unsigned short c = minASCII; c < maxASCII; c++)
    {
        FT_Load_Char(face, c, FT_LOAD_RENDER);

        if (ox + face->glyph->bitmap.width + 1 >= 1024) {
            oy += rowh;
            rowh = 0;
            ox = 0;
        }

        glBindTexture(GL_TEXTURE_2D, atlasTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, ox, oy, face->glyph->bitmap.width, face->glyph->bitmap.rows, GL_ALPHA, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

        Characters.insert(std::pair<char, Character>(c, Character{ vec2f{ (float)face->glyph->bitmap.width / windowWindow, (float)face->glyph->bitmap.rows / windowHeight }, vec2f{ (float)face->glyph->bitmap_left / windowWindow, (float)face->glyph->bitmap_top / windowHeight }, vec2f{ (float)ox / w, (float)oy / h }, vec2f{ (float)face->glyph->bitmap.width / w, (float)face->glyph->bitmap.rows / h }, (float)face->glyph->advance.x / 64 / windowWindow }));

        rowh = std::max(rowh, (unsigned char)face->glyph->bitmap.rows);
        ox += face->glyph->bitmap.width + 1;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec2 aTexCoord;\n"
        "layout (location = 2) in vec3 aColor;\n"
        "out vec2 TexCoord;\n"
        "out vec3 TexColor;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = vec4(aPos.xyz, 1.0);\n"
        "   TexCoord = aTexCoord;\n"
        "	TexColor = aColor;\n"
        "}\0";

    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec2 TexCoord;\n"
        "in vec3 TexColor;\n"
        "uniform sampler2D Texture;\n"
        "void main()\n"
        "{\n"
        "	vec4 tex = texture(Texture, TexCoord);\n"
        "   FragColor = vec4(TexColor.r + tex.r, TexColor.g + tex.g, TexColor.b + tex.b, tex.a);\n"
        "}\0";

    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    textureProgram = glCreateProgram();

    glAttachShader(textureProgram, vertexShader);
    glAttachShader(textureProgram, fragmentShader);
    glLinkProgram(textureProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glGenVertexArrays(1, &textureVAO);
    glGenBuffers(1, &textureVBO);
    glGenBuffers(1, &textureEBO);

    glBindVertexArray(textureVAO);

    glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, textureEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // color attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

const inline void DrawString(std::string str, float x, float y, float wscale, float hscale, float red, float green, float blue) {
    for (unsigned short i = 0; i < str.size(); i++) {
        const Character ch = Characters[str[i]];
        const float xpos = x + ch.Bearing.x * wscale;
        const float ypos = y - (ch.Size.y - ch.Bearing.y) * hscale;

        const float w = ch.Size.x * wscale;
        const float h = ch.Size.y * hscale;
        // update for each character
        const float xposw = xpos + w;
        const float yposh = ypos + h;
        vertices.push_back(xposw);
        vertices.push_back(ypos);
        vertices.push_back(0);
        vertices.push_back(ch.Atlas.x + ch.AtlasSize.x);
        vertices.push_back(ch.Atlas.y + ch.AtlasSize.y);
        vertices.push_back(red);
        vertices.push_back(green);
        vertices.push_back(blue);
        vertices.push_back(xposw);
        vertices.push_back(yposh);
        vertices.push_back(0);
        vertices.push_back(ch.Atlas.x + ch.AtlasSize.x);
        vertices.push_back(ch.Atlas.y);
        vertices.push_back(red);
        vertices.push_back(green);
        vertices.push_back(blue);
        vertices.push_back(xpos);
        vertices.push_back(yposh);
        vertices.push_back(0);
        vertices.push_back(ch.Atlas.x);
        vertices.push_back(ch.Atlas.y);
        vertices.push_back(red);
        vertices.push_back(green);
        vertices.push_back(blue);
        vertices.push_back(xpos);
        vertices.push_back(ypos);
        vertices.push_back(0);
        vertices.push_back(ch.Atlas.x);
        vertices.push_back(ch.Atlas.y + ch.AtlasSize.y);
        vertices.push_back(red);
        vertices.push_back(green);
        vertices.push_back(blue);

        indices.push_back(indCount);
        indices.push_back(indCount + 1);
        indices.push_back(indCount + 3);
        indices.push_back(indCount + 1);
        indices.push_back(indCount + 2);
        indices.push_back(indCount + 3);
        x += ch.Advance * wscale;
        chCount += 6;
        indCount += 4;
    }
}

const inline void DrawString(std::string str, float x, float y, float red, float green, float blue) {
    for (unsigned short i = 0; i < str.size(); i++) {
        const Character ch = Characters[str[i]];
        const float xpos = x + ch.Bearing.x;
        const float ypos = y - (ch.Size.y - ch.Bearing.y);
        // update for each character
        const float xposw = xpos + ch.Size.x;
        const float yposh = ypos + ch.Size.y;
        vertices.push_back(xposw);
        vertices.push_back(ypos);
        vertices.push_back(0);
        vertices.push_back(ch.Atlas.x + ch.AtlasSize.x);
        vertices.push_back(ch.Atlas.y + ch.AtlasSize.y);
        vertices.push_back(red);
        vertices.push_back(green);
        vertices.push_back(blue);
        vertices.push_back(xposw);
        vertices.push_back(yposh);
        vertices.push_back(0);
        vertices.push_back(ch.Atlas.x + ch.AtlasSize.x);
        vertices.push_back(ch.Atlas.y);
        vertices.push_back(red);
        vertices.push_back(green);
        vertices.push_back(blue);
        vertices.push_back(xpos);
        vertices.push_back(yposh);
        vertices.push_back(0);
        vertices.push_back(ch.Atlas.x);
        vertices.push_back(ch.Atlas.y);
        vertices.push_back(red);
        vertices.push_back(green);
        vertices.push_back(blue);
        vertices.push_back(xpos);
        vertices.push_back(ypos);
        vertices.push_back(0);
        vertices.push_back(ch.Atlas.x);
        vertices.push_back(ch.Atlas.y + ch.AtlasSize.y);
        vertices.push_back(red);
        vertices.push_back(green);
        vertices.push_back(blue);

        indices.push_back(indCount);
        indices.push_back(indCount + 1);
        indices.push_back(indCount + 3);
        indices.push_back(indCount + 1);
        indices.push_back(indCount + 2);
        indices.push_back(indCount + 3);

        x += ch.Advance;
        chCount += 6;
        indCount += 4;
    }
}

const inline float GetStringWidth(std::string str) {
    float x = 0;
    for (unsigned short i = 0; i < str.size(); i++)
        x += (Characters[str[i]].Advance);
    return x;
}

const inline float GetStringWidth(std::string str, float wscale) {
    float x = 0;
    for (unsigned short i = 0; i < str.size(); i++)
        x += (Characters[str[i]].Advance);
    return x * wscale;
}

const inline float GetNegativeStringWidth(std::string str) {
    float x = 0;
    for (unsigned short i = 0; i < str.size(); i++)
        x -= (Characters[str[i]].Advance);
    return x;
}

const inline float GetNegativeStringWidth(std::string str, float wscale) {
    float x = 0;
    for (unsigned short i = 0; i < str.size(); i++)
        x -= (Characters[str[i]].Advance);
    return x * wscale;
}

const inline float GetStringHeight(std::string str) {
    float y = 0;
    for (unsigned short i = 0; i < str.size(); i++)
        y = std::max(Characters[str[i]].Bearing.y, y);
    return y;
}

const inline float GetStringHeight(std::string str, float hscale) {
    float y = 0;
    for (unsigned short i = 0; i < str.size(); i++)
        y = std::max(Characters[str[i]].Bearing.y, y);
    return y * hscale;
}

const inline void DrawCenteredString(std::string str, float x, float y, float wscale, float hscale, float red, float green, float blue) {
    x += GetNegativeStringWidth(str) / 2 * wscale;

    for (unsigned short i = 0; i < str.size(); i++) {
        const Character ch = Characters[str[i]];
        const float xpos = x + ch.Bearing.x * wscale;
        const float ypos = y - (ch.Size.y - ch.Bearing.y) * hscale;

        const float w = ch.Size.x * wscale;
        const float h = ch.Size.y * hscale;
        // update for each character
        const float xposw = xpos + w;
        const float yposh = ypos + h;
        vertices.push_back(xposw);
        vertices.push_back(ypos);
        vertices.push_back(0);
        vertices.push_back(ch.Atlas.x + ch.AtlasSize.x);
        vertices.push_back(ch.Atlas.y + ch.AtlasSize.y);
        vertices.push_back(red);
        vertices.push_back(green);
        vertices.push_back(blue);
        vertices.push_back(xposw);
        vertices.push_back(yposh);
        vertices.push_back(0);
        vertices.push_back(ch.Atlas.x + ch.AtlasSize.x);
        vertices.push_back(ch.Atlas.y);
        vertices.push_back(red);
        vertices.push_back(green);
        vertices.push_back(blue);
        vertices.push_back(xpos);
        vertices.push_back(yposh);
        vertices.push_back(0);
        vertices.push_back(ch.Atlas.x);
        vertices.push_back(ch.Atlas.y);
        vertices.push_back(red);
        vertices.push_back(green);
        vertices.push_back(blue);
        vertices.push_back(xpos);
        vertices.push_back(ypos);
        vertices.push_back(0);
        vertices.push_back(ch.Atlas.x);
        vertices.push_back(ch.Atlas.y + ch.AtlasSize.y);
        vertices.push_back(red);
        vertices.push_back(green);
        vertices.push_back(blue);

        indices.push_back(indCount);
        indices.push_back(indCount + 1);
        indices.push_back(indCount + 3);
        indices.push_back(indCount + 1);
        indices.push_back(indCount + 2);
        indices.push_back(indCount + 3);

        x += ch.Advance * wscale;
        chCount += 6;
        indCount += 4;
    }
}

const inline void DrawCenteredString(std::string str, float x, float y, float red, float green, float blue) {
    x += GetNegativeStringWidth(str) / 2;

    for (unsigned short i = 0; i < str.size(); i++) {
        const Character ch = Characters[str[i]];
        const float xpos = x + ch.Bearing.x;
        const float ypos = y - (ch.Size.y - ch.Bearing.y);
        // update for each character
        const float xposw = xpos + ch.Size.x;
        const float yposh = ypos + ch.Size.y;
        vertices.push_back(xposw);
        vertices.push_back(ypos);
        vertices.push_back(0);
        vertices.push_back(ch.Atlas.x + ch.AtlasSize.x);
        vertices.push_back(ch.Atlas.y + ch.AtlasSize.y);
        vertices.push_back(red);
        vertices.push_back(green);
        vertices.push_back(blue);
        vertices.push_back(xposw);
        vertices.push_back(yposh);
        vertices.push_back(0);
        vertices.push_back(ch.Atlas.x + ch.AtlasSize.x);
        vertices.push_back(ch.Atlas.y);
        vertices.push_back(red);
        vertices.push_back(green);
        vertices.push_back(blue);
        vertices.push_back(xpos);
        vertices.push_back(yposh);
        vertices.push_back(0);
        vertices.push_back(ch.Atlas.x);
        vertices.push_back(ch.Atlas.y);
        vertices.push_back(red);
        vertices.push_back(green);
        vertices.push_back(blue);
        vertices.push_back(xpos);
        vertices.push_back(ypos);
        vertices.push_back(0);
        vertices.push_back(ch.Atlas.x);
        vertices.push_back(ch.Atlas.y + ch.AtlasSize.y);
        vertices.push_back(red);
        vertices.push_back(green);
        vertices.push_back(blue);

        indices.push_back(indCount);
        indices.push_back(indCount + 1);
        indices.push_back(indCount + 3);
        indices.push_back(indCount + 1);
        indices.push_back(indCount + 2);
        indices.push_back(indCount + 3);

        x += ch.Advance;
        chCount += 6;
        indCount += 4;
    }
}

const inline void FinishStrings() {
    glUseProgram(textureProgram);

    glBindTexture(GL_TEXTURE_2D, atlasTexture);

    glBindVertexArray(textureVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, textureEBO);
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), &vertices[0], GL_DYNAMIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), &indices[0], GL_DYNAMIC_DRAW);

    glDrawElements(GL_TRIANGLES, chCount, GL_UNSIGNED_INT, 0);

    vertices.clear();
    indices.clear();

    chCount = 0;
    indCount = 0;
}
