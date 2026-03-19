#ifndef UI_ELEMENTS_H
#define UI_ELEMENTS_H

#include <windows.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <functional>
#include <string>
#include "../text/TextRenderer.h"
#include "../GameState.h"
#include "UIColors.h"

// Forward declarations
struct UIElement;
struct Panel;

// Hilfsfunktion für absolute Position
glm::vec2 GetAbsolutePos(UIElement* e);

// Basisklasse
struct UIElement {
    virtual ~UIElement() {}
    float x, y, w, h;
    bool visible = true;
    bool active = true;
    UIElement* parent = nullptr;

    virtual void Draw() = 0;
    virtual void Update(float dt) {}

    virtual void Toggle() { visible = !visible; }

    bool IsVisibleRecursive() const {
        if (!visible) return false;
        if (parent) return parent->IsVisibleRecursive();
        return true;
    }
};

// Panel Container
struct Panel : UIElement {
    float r, g, b;
    std::vector<std::unique_ptr<UIElement>> children;

    Panel(float px, float py, float pw, float ph, float pr, float pg, float pb) {
        x = px; y = py; w = pw; h = ph;
        r = pr; g = pg; b = pb;
    }

    void Draw() override {
        if (!visible) return;

        glUseProgram(0);
        glDisable(GL_TEXTURE_2D);

        glColor3f(r, g, b);
        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
        glEnd();

        glPushMatrix();
        glTranslatef(x, y, 0);

        for (auto& child : children)
            child->Draw();

        glPopMatrix();
    }

    void Update(float dt) override {
        if (!visible) return;
        for (auto& child : children)
            child->Update(dt);
    }

    void AddChild(UIElement* e) {
        e->parent = this;
        children.push_back(std::unique_ptr<UIElement>(e));
    }

    template<typename T, typename... Args>
    T* CreateChild(Args&&... args) {
        static_assert(std::is_base_of<UIElement, T>::value, "T must derive from UIElement");

        auto child = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = child.get();
        child->parent = this;
        children.push_back(std::move(child));
        return ptr;
    }
};

// Label für Text
struct Label : UIElement {
    std::function<std::string()> textSource;
    std::function<glm::vec3()> colorSource = nullptr;
    float r = 1, g = 1, b = 1;
    int fontID = 0;
    bool centered = false;

    Label(float px, float py, float pw, std::function<std::string()> source, int fID = 0, bool center = false, float textColorR = 1, float textColorG = 1, float textColorB = 1) {
        x = px; y = py; w = pw;
        r = textColorR; g = textColorG; b = textColorB;
        textSource = source;
        fontID = fID;
        centered = center;
    }

    float GetTextWidth(const std::string& text, float scale = 1.0f) {
        float width = 0.0f;

        auto it_font = FontLibrary.find(fontID);
        if (it_font == FontLibrary.end()) return 0.0f;

        auto& currentChars = it_font->second;

        for (char c : text) {
            auto it_char = currentChars.find(c);
            if (it_char != currentChars.end()) {
                width += it_char->second.Advance * scale;
            }
        }
        return width;
    }

    void Draw() override {
        if (!visible || !textSource) return;

        if (colorSource) {
            glm::vec3 dynamicColor = colorSource();
            r = dynamicColor.r; g = dynamicColor.g; b = dynamicColor.b;
        }

        std::string txt = textSource();

        float drawX = x;

        if (centered && w > 0) {
            float tw = GetTextWidth(txt, 1.0f);
            drawX = x + (w - tw) * 0.5f;
        }

        RenderText(txt, drawX, y + 20, 1.0f, glm::vec3(r, g, b), fontID);
    }
};

// Button
struct Button : UIElement {
    std::string text;
    int fontID = 0;
    bool hovered = false;
    bool pressed = false;
    std::function<void()> onClick = nullptr;

    Button(float px, float py, float pw, float ph, const std::string& t, int fID = 0) {
        x = px; y = py; w = pw; h = ph;
        text = t;
        fontID = fID;
    }

    void Update(float) override {
        if (!IsVisibleRecursive()) {
            hovered = false;
            pressed = false;
        }
    }

    void Draw() override {
        if (!visible) return;

        float m[16];
        glGetFloatv(GL_MODELVIEW_MATRIX, m);
        float currentAbsX = m[12] + x;
        float currentAbsY = m[13] + y;

        POINT p;
        GetCursorPos(&p);
        ScreenToClient(GetActiveWindow(), &p);

        hovered = (p.x >= currentAbsX && p.x <= currentAbsX + w &&
                   p.y >= currentAbsY && p.y <= currentAbsY + h);

        SHORT mouseState = GetAsyncKeyState(VK_LBUTTON);
        if (hovered && (mouseState & 0x8000)) {
            pressed = true;
        } else if (pressed && !(mouseState & 0x8000)) {
            pressed = false;
            if (hovered && onClick) onClick();
        }

        glUseProgram(0);
        glDisable(GL_TEXTURE_2D);
        if (pressed)      glColor3f(0.1f, 0.1f, 0.15f);
        else if (hovered) glColor3f(0.3f, 0.3f, 0.4f);
        else              glColor3f(0.2f, 0.2f, 0.25f);

        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
        glEnd();

        RenderText(text, x + 12, y + (h / 2.0f) + 5.0f, 1.0f, glm::vec3(1, 1, 1), fontID);
    }
};

// Horizontale Linie
struct Line : UIElement {
    float thickness;
    float r, g, b;

    Line(float px, float py, float pw, float pthickness = 2.0f,
         float pr = 0.5f, float pg = 0.5f, float pb = 0.5f) {
        x = px; y = py; w = pw; h = pthickness;
        thickness = pthickness;
        r = pr; g = pg; b = pb;
    }

    void Draw() override {
        if (!visible) return;
        glUseProgram(0);
        glDisable(GL_TEXTURE_2D);
        glColor3f(r, g, b);
        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + thickness);
        glVertex2f(x, y + thickness);
        glEnd();
    }
};

// Hilfsfunktions-Implementierung
inline glm::vec2 GetAbsolutePos(UIElement* e) {
    if (!e) return glm::vec2(0, 0);
    float absX = e->x;
    float absY = e->y;
    UIElement* temp = e->parent;
    while (temp) {
        absX += temp->x;
        absY += temp->y;
        temp = temp->parent;
    }
    return glm::vec2(absX, absY);
}

#endif
