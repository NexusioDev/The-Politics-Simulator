#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <map>
#include <string>

struct Character {
    GLuint     TextureID;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    GLuint     Advance;
};

// Globale Variablen
extern std::map<int, std::map<char, Character>> FontLibrary;
extern GLuint textVAO, textVBO;
extern GLuint textShader;
extern glm::mat4 projection;

// Funktionen
void RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color, int fontID = 0);
GLuint CompileShaderProgram(const char* vsSource, const char* fsSource);
bool InitTextRenderer(const std::string& fontPath = R"(C:\Windows\Fonts\bahnschrift.ttf)", int fontSize = 16, int screenWidth = 800, int screenHeight = 600);

#endif
