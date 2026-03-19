#include "TextRenderer.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "../stb_truetype.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

std::map<int, std::map<char, Character>> FontLibrary;
GLuint textVAO = 0, textVBO = 0;
GLuint textShader = 0;
glm::mat4 projection;

std::map<char, Character> LoadFontFace(const std::string& fontPath, int fontSize) {
    std::map<char, Character> characters;

    FILE* file = nullptr;
    if (fopen_s(&file, fontPath.c_str(), "rb") != 0) return characters;

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    std::vector<unsigned char> fontBuffer(size);
    fread(fontBuffer.data(), 1, size, file);
    fclose(file);

    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, fontBuffer.data(), 0)) return characters;

    float scale = stbtt_ScaleForPixelHeight(&font, (float)fontSize);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned char c = 32; c < 128; ++c) {
        int x1, y1, x2, y2;
        stbtt_GetCodepointBitmapBox(&font, c, scale, scale, &x1, &y1, &x2, &y2);
        int w = x2 - x1, h = y2 - y1;

        unsigned char* bitmap = stbtt_GetCodepointBitmap(&font, scale, scale, c, &w, &h, 0, 0);

        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        int advance;
        stbtt_GetCodepointHMetrics(&font, c, &advance, nullptr);

        characters[c] = { tex, {w, h}, {x1, y1}, (GLuint)(advance * scale) };
        stbtt_FreeBitmap(bitmap, nullptr);
    }
    return characters;
}

void RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color, int fontID) {
    auto& currentChars = FontLibrary[fontID];

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(textShader);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    float m[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, m);
    glm::mat4 model = glm::make_mat4(m);

    glUniformMatrix4fv(glGetUniformLocation(textShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection * model));
    glUniform3f(glGetUniformLocation(textShader, "textColor"), color.x, color.y, color.z);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    float currentX = x;

    for (char c : text) {
        auto it = currentChars.find(c);
        if (it == currentChars.end()) continue;
        Character ch = it->second;

        float xpos = std::floor(currentX + ch.Bearing.x * scale);
        float ypos = std::floor(y + (ch.Bearing.y + ch.Size.y) * scale);

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        float vertices[6][4] = {
            { xpos,     std::floor(ypos - h),   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { std::floor(xpos + w), ypos,       1.0f, 1.0f },

            { xpos,     std::floor(ypos - h),   0.0f, 0.0f },
            { std::floor(xpos + w), ypos,       1.0f, 1.0f },
            { std::floor(xpos + w), std::floor(ypos - h),   1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        currentX += std::floor(ch.Advance * scale);
    }

    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_BLEND);
}

GLuint CompileShaderProgram(const char* vsSource, const char* fsSource) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSource, nullptr);
    glCompileShader(vs);

    GLint success;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(vs, 512, nullptr, info);
        std::cerr << "VS Fehler:\n" << info << std::endl;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSource, nullptr);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(fs, 512, nullptr, info);
        std::cerr << "FS Fehler:\n" << info << std::endl;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512];
        glGetProgramInfoLog(program, 512, nullptr, info);
        std::cerr << "Link Fehler:\n" << info << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

bool InitTextRenderer(const std::string& fontPath, int fontSize, const int screenWidth, const int screenHeight) {
    FontLibrary[0] = LoadFontFace(R"(C:\Windows\Fonts\bahnschrift.ttf)", 16);
    FontLibrary[1] = LoadFontFace(R"(C:\Windows\Fonts\bahnschrift.ttf)", 22);
    FontLibrary[2] = LoadFontFace(R"(C:\Windows\Fonts\segoeuib.ttf)", 32);

    if (FontLibrary[0].empty()) return false;

    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), static_cast<void *>(0));

    const char* vsSource = R"(
    #version 330 core
    layout (location = 0) in vec4 vertex;
    out vec2 TexCoords;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
        TexCoords = vertex.zw;
    }
    )";

    const char* fsSource = R"(
    #version 330 core
    in vec2 TexCoords;
    out vec4 FragColor;
    uniform sampler2D text;
    uniform vec3 textColor;

    void main() {
        float alpha = texture(text, TexCoords).r;
        if(alpha < 0.1) discard;
        FragColor = vec4(textColor, alpha);
    }
    )";
    textShader = CompileShaderProgram(vsSource, fsSource);

    projection = glm::ortho(0.0f, static_cast<float>(screenWidth), 0.0f, static_cast<float>(screenHeight), -1.0f, 1.0f);

    return true;
}
