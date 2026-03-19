#include <windows.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <chrono>
#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <functional>
#include <algorithm>

#include "text/TextRenderer.h"
#include "ui/UIElements.h"
#include "ui/UIColors.h"
#include "GameState.h"

// Globale Variablen
int bitmapWidth = 800;
int bitmapHeight = 600;

auto lastTime = std::chrono::high_resolution_clock::now();
auto fpsTimer = std::chrono::high_resolution_clock::now();
auto simTimer = std::chrono::high_resolution_clock::now();
float fps = 0.0f;
int frameCount = 0;
long long lastSimTimeUs = 0;
float simInterval = 1.0f;
float simSpeed = 1.0f;

static bool qWasDown = false;
static bool eWasDown = false;
static bool inGame = false;

std::vector<std::unique_ptr<UIElement>> ui;
std::vector<Panel*> menus;

std::string formatValue(int64_t value, std::string_view unit = "") {
    std::string s = std::to_string(value);
    int n = static_cast<int>(s.length());
    int insertCount = (n - 1) / 3;

    for (int i = 1; i <= insertCount; i++) {
        s.insert(n - i * 3, ".");
    }

    return unit.empty() ? s : s + " " + std::string(unit);
}

void OpenMenu(Panel* target) {
    const bool wasVisible = target->visible;

    for (const auto m : menus) {
        m->visible = false;
    }

    target->visible = !wasVisible;
}

template<typename T, typename... Args>
T* CreateUIElement(Args&&... args) {
    auto element = std::make_unique<T>(std::forward<Args>(args)...);
    T* ptr = element.get();
    ui.push_back(std::move(element));
    return ptr;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        ui.clear();
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
    {
        bitmapWidth = LOWORD(lParam);
        bitmapHeight = HIWORD(lParam);
        if (bitmapHeight == 0) bitmapHeight = 1;

        glViewport(0, 0, bitmapWidth, bitmapHeight);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, bitmapWidth, bitmapHeight, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        projection = glm::ortho(0.0f, static_cast<float>(bitmapWidth), static_cast<float>(bitmapHeight), 0.0f, -1.0f, 1.0f);

        return 0;
    }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) { //TODO: Das Game muss für jede auflösung geeignet sein
    WNDCLASS wc = {};
    SetProcessDPIAware();
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MyGameWindow";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, L"MyGameWindow", L"The Politics Simulator",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, bitmapWidth, bitmapHeight,
        nullptr, nullptr, hInstance, nullptr);

    HDC hDC = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd); pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA; pfd.cColorBits = 32; pfd.cDepthBits = 24;
    int pf = ChoosePixelFormat(hDC, &pfd); SetPixelFormat(hDC, pf, &pfd);
    HGLRC hRC = wglCreateContext(hDC); wglMakeCurrent(hDC, hRC);

    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glOrtho(0, bitmapWidth, bitmapHeight, 0, -1, 1);
    projection = glm::ortho(0.0f, static_cast<float>(bitmapWidth), static_cast<float>(bitmapHeight), 0.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        MessageBoxA(nullptr, reinterpret_cast<const char *>(glewGetErrorString(err)), "GLEW Error", MB_OK);
        return -1;
    }

    if (!InitTextRenderer(R"(C:\Windows\Fonts\bahnschrift.ttf)", 16, bitmapWidth, bitmapHeight)) {
        MessageBoxA(nullptr, "Text-Renderer konnte nicht initialisiert werden!", "Fehler", MB_OK);
        return -1;
    }

    // Shader für UI
    const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec3 aColor;
    uniform mat4 uProjection;
    out vec3 vColor;
    void main() {
        gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
        vColor = aColor;
    }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 vColor;
        out vec4 FragColor;
        void main() {
            FragColor = vec4(vColor, 1.0);
        }
    )";

    GLuint uiShader = CompileShaderProgram(vertexShaderSource, fragmentShaderSource);

    ShowWindow(hwnd, SW_SHOW);

    /*
    ===== UI CREATION
    */
    auto* mainMenuUI = CreateUIElement<Panel>(0, 0, 0, 0, 0.2f, 0.2f, 0.25f);
    auto* gameUI = CreateUIElement<Panel>(0, 0, 0, 0, 0.2f, 0.2f, 0.25f);

    auto* leftBar = gameUI->CreateChild<Panel>(0, 0, 240, bitmapHeight, 0.15f, 0.15f, 0.18f);
    auto* topBar = gameUI->CreateChild<Panel>(240, 0, bitmapWidth - 240, 50, 0.1f, 0.1f, 0.12f);

    auto* taxesMenu = gameUI->CreateChild<Panel>(260, 60, 300, 400, 0.2f, 0.2f, 0.25f);
    auto* infrastructureMenu = gameUI->CreateChild<Panel>(260, 60, 300, 500, 0.2f, 0.2f, 0.25f);
    auto* populationPolicyMenu = gameUI->CreateChild<Panel>(260, 60, 300, 400, 0.2f, 0.2f, 0.25f);
    auto* healthcarePolicyMenu = gameUI->CreateChild<Panel>(260, 60, 300, 400, 0.2f, 0.2f, 0.25f);
    auto* militaryPolicyMenu = gameUI->CreateChild<Panel>(260, 60, 300, 400, 0.2f, 0.2f, 0.25f);
    auto* educationPolicyMenu = gameUI->CreateChild<Panel>(260, 60, 300, 400, 0.2f, 0.2f, 0.25f);
    auto* servicesPolicyMenu = gameUI->CreateChild<Panel>(260, 60, 300, 400, 0.2f, 0.2f, 0.25f);

    auto* taxesInfoMenu = taxesMenu->CreateChild<Panel>(310, 0, 200, 400, 0.2f, 0.2f, 0.25f);
    auto* infrastructureInfoMenu = infrastructureMenu->CreateChild<Panel>(310, 0, 200, 500, 0.2f, 0.2f, 0.25f);
    auto* populationInfoMenu = populationPolicyMenu->CreateChild<Panel>(310, 0, 200, 400, 0.2f, 0.2f, 0.25f);
    auto* healthcareInfoMenu = healthcarePolicyMenu->CreateChild<Panel>(310, 0, 200, 400, 0.2f, 0.2f, 0.25f);
    auto* servicesInfoMenu = servicesPolicyMenu->CreateChild<Panel>(310, 0, 200, 400, 0.2f, 0.2f, 0.25f);
    auto* educationInfoMenu = educationPolicyMenu->CreateChild<Panel>(310, 0, 200, 400, 0.2f, 0.2f, 0.25f);
    auto* militaryInfoMenu = militaryPolicyMenu->CreateChild<Panel>(310, 0, 200, 400, 0.2f, 0.2f, 0.25f);
    //TODO: LeftBar auch übersichtlicher gestalten
    leftBar->CreateChild<Label>(20, 10, 250, []() { return "FPS:" + std::to_string(static_cast<int>(fps)); });
    auto* mainMoneyLabel = leftBar->CreateChild<Label>(20, 30, 250, []() { return "Money: " + formatValue(state.money, "EUR"); });
    leftBar->CreateChild<Label>(20, 50, 250, []() { return "Government Trust: " + std::to_string(static_cast<int>(state.stability)); });
    leftBar->CreateChild<Label>(20, 70, 250, []() { return "Population: " + formatValue(state.population()); });

    leftBar->CreateChild<Label>(20, 440, 200, []() { return "Sim-Logic: " + std::to_string(lastSimTimeUs) + " us"; }, 0);
    leftBar->CreateChild<Label>(20, 460, 250, []() { return "City Transit: " + formatValue(state.cityTransitUsage); });
    leftBar->CreateChild<Label>(20, 480, 250, []() { return "Intercity: " + formatValue(state.intercityUsage); });
    leftBar->CreateChild<Label>(20, 500, 250, []() { return "National Users: " + formatValue(state.nationalTicketUsers); });

    leftBar->CreateChild<Label>(20, 530, 200, []() {
        const char* days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
        const int dayIndex = state.currentDay % 7;
        return std::string("Day: ") + days[dayIndex] + " (Total: " + std::to_string(state.currentDay) + ")";
    }, 1);

    auto* speedPanel = topBar->CreateChild<Panel>(5, 5, 110, 40, 0.2f, 0.2f, 0.2f);

    speedPanel->CreateChild<Button>(5, 5, 30, 30, "||")->onClick = []() { simSpeed = 0.0f; };
    speedPanel->CreateChild<Button>(40, 5, 30, 30, ">")->onClick = []() { simSpeed = 1.0f; };
    speedPanel->CreateChild<Button>(75, 5, 30, 30, ">>")->onClick = []() { simSpeed = 4.0f; };


    auto* topBarNotificationLabel = topBar->CreateChild<Label>(5, 15, bitmapWidth - 250, []() {
        std::vector<const char*> warnings;
        if (state.stability < 11)           warnings.push_back("Low Trust");
        if (state.money < 0)                warnings.push_back("Debt");
        if (state.averageCityTravelSpeed() < 20.0f)  warnings.push_back("High City Traffic");
        if (state.averageLongDistanceSpeed() < 50.0f) warnings.push_back("Bad Long-Distance Infra");
        if (state.averageHealthcareCosts() > 120) warnings.push_back("High Health Costs");

        if (warnings.empty()) return std::string("");

        std::string result = (warnings.size() == 1) ? "Warning! " :
                             std::to_string(warnings.size()) + " Warnings! ";

        for (size_t i = 0; i < warnings.size(); ++i) {
            result += warnings[i];
            if (i < warnings.size() - 1) {
                result += ", ";
            }
        }
        return result;
    }, 1, true);

    mainMoneyLabel->colorSource = []() -> glm::vec3 {
        return (state.money >= 0) ? UIColors::Neutral : UIColors::Negative;
    };

    // --- HAUPTMENÜ BUTTONS ---
    leftBar->CreateChild<Button>(20, 110, 200, 40, "Taxes Menu")->onClick = [taxesMenu]() { OpenMenu(taxesMenu); };
    leftBar->CreateChild<Button>(20, 150, 200, 40, "Infra Menu")->onClick = [infrastructureMenu]() { OpenMenu(infrastructureMenu); };
    leftBar->CreateChild<Button>(20, 190, 200, 40, "Pop Menu")->onClick = [populationPolicyMenu]() { OpenMenu(populationPolicyMenu); };
    leftBar->CreateChild<Button>(20, 230, 200, 40, "Healthcare Menu")->onClick = [healthcarePolicyMenu]() { OpenMenu(healthcarePolicyMenu); };
    leftBar->CreateChild<Button>(20, 270, 200, 40, "Services Menu")->onClick = [servicesPolicyMenu]() { OpenMenu(servicesPolicyMenu); };
    leftBar->CreateChild<Button>(20, 310, 200, 40, "Education Menu")->onClick = [educationPolicyMenu]() { OpenMenu(educationPolicyMenu); };
    leftBar->CreateChild<Button>(20, 350, 200, 40, "Military Menu")->onClick = [militaryPolicyMenu]() { OpenMenu(militaryPolicyMenu); };

    // TAX PANEL
    taxesInfoMenu->CreateChild<Label>(0, 20, 200, []() {
        return "Taxation Rate: ";
        }, 0, true);
    taxesInfoMenu->CreateChild<Label>(0, 60, 200, []() {
        return std::to_string(static_cast<int>(state.residentTax)) + "% ";
        }, 2, true);
    auto* incomeLabel = taxesInfoMenu->CreateChild<Label>(0, 115, 200, []() {
        const int64_t net = state.GetNetIncome();
        return (net >= 0 ? "+" : "") + formatValue(net, "EUR");
    }, 0, true);

    incomeLabel->colorSource = []() -> glm::vec3 {
        return (state.GetNetIncome() >= 0) ? UIColors::Positive : UIColors::Negative;
    };

    taxesInfoMenu->CreateChild<Line>(0, 140, 200, 2);

    auto* taxIncomeLabel = taxesInfoMenu->CreateChild<Label>(0, 150, 200, []() {
        const int64_t net = (state.popGrownUps() * 2000LL * state.residentTax) / 100LL;
        return (net >= 0 ? "+" : "") + formatValue(net, "EUR");
    }, 0, true);
    auto* taxUpkeepIncomeLabel = taxesInfoMenu->CreateChild<Label>(0, 175, 200, []() {
        const int64_t net = -((state.population() * 15LL) / 100LL);
        return (net >= 0 ? "+" : "") + formatValue(net, "EUR");
    }, 0, true);
    auto* infrastructureIncomeLabel = taxesInfoMenu->CreateChild<Label>(0, 200, 200, []() {
        const int64_t net = -state.stateHighwayCosts() + state.getTransitBalance();
        return (net >= 0 ? "+" : "") + formatValue(net, "EUR");
    }, 0, true);
    auto* healthcareIncomeLabel = taxesInfoMenu->CreateChild<Label>(0, 225, 200, []() {
        const int64_t net = -state.stateHealthcareCosts();
        return (net > 0 ? "+" : "") + formatValue(net, "EUR");
    }, 0, true);
    auto* servicesIncomeLabel = taxesInfoMenu->CreateChild<Label>(0, 250, 200, []() {
        const int64_t net = -state.stateServicesCosts();
        return (net >= 0 ? "+" : "") + formatValue(net, "EUR");
    }, 0, true);
    auto* educationIncomeLabel = taxesInfoMenu->CreateChild<Label>(0, 275, 200, []() {
        const int64_t net = -state.stateEducationCosts();
        return (net >= 0 ? "+" : "") + formatValue(net, "EUR");
    }, 0, true);
    auto* militaryIncomeLabel = taxesInfoMenu->CreateChild<Label>(0, 300, 200, []() {
        const int64_t net = -state.stateMilitaryCosts();
        return (net >= 0 ? "+" : "") + formatValue(net, "EUR");
    }, 0, true);

    taxIncomeLabel->colorSource = []() -> glm::vec3 {
        return ((state.popGrownUps() * 2000LL * state.residentTax) / 100LL >= 0) ? UIColors::Positive : UIColors::Negative;
    };
    taxUpkeepIncomeLabel->colorSource = []() -> glm::vec3 {
        return (-((state.population() * 15LL) / 100LL) >= 0) ? UIColors::Positive : UIColors::Negative;
    };
    infrastructureIncomeLabel->colorSource = []() -> glm::vec3 {
        return (-state.stateHighwayCosts() + state.getTransitBalance() >= 0) ? UIColors::Positive : UIColors::Negative;
    };
    healthcareIncomeLabel->colorSource = []() -> glm::vec3 {
        if (-state.stateHealthcareCosts() == 0) return UIColors::NeutralYellow;
        return (-state.stateHealthcareCosts() >= 0) ? UIColors::Positive : UIColors::Negative;
    };
    servicesIncomeLabel->colorSource = []() -> glm::vec3 {
        return (-state.stateServicesCosts() >= 0) ? UIColors::Positive : UIColors::Negative;
    };
    educationIncomeLabel->colorSource = []() -> glm::vec3 {
        return (-state.GetNetIncome() >= 0) ? UIColors::Positive : UIColors::Negative;
    };
    militaryIncomeLabel->colorSource = []() -> glm::vec3 {
        return (-state.GetNetIncome() >= 0) ? UIColors::Positive : UIColors::Negative;
    };

    auto* incTaxBtn = taxesMenu->CreateChild<Button>(10, 20, 260, 40, "Raise Taxes");
    incTaxBtn->onClick = []() { state.residentTax += 1; state.stability -= 10; };

    auto* decTaxBtn = taxesMenu->CreateChild<Button>(10, 60, 260, 40, "Decrease Taxes");
    decTaxBtn->onClick = []() { state.residentTax -= 1; state.stability += 2; };

    // POPULATION PANEL
    populationInfoMenu->CreateChild<Label>(0, 20, 200, []() {
        return "Population: " + formatValue(state.population());
        }, 0, true);
    populationInfoMenu->CreateChild<Label>(0, 50, 200, []() {
        return "Children: " + std::to_string(state.popChildren);
        }, 0, true);
    populationInfoMenu->CreateChild<Label>(0, 75, 200, []() {
        return "Adults: " + std::to_string(state.popAdults);
        }, 0, true);
    populationInfoMenu->CreateChild<Label>(0, 100, 200, []() {
        return "Seniors: " + std::to_string(state.popSeniors);
        }, 0, true);

    populationPolicyMenu->CreateChild<Button>(10, 20, 260, 40, "Build Housing")->onClick = []() {
        state.money -= 100000000;
        state.popChildren += 50000;
        state.popAdults += 30000;
        state.popSeniors += 10000;
        };
    populationPolicyMenu->CreateChild<Button>(10, 60, 260, 40, "Population Subsidy")->onClick = []() {
        state.money -= 100000000;
        state.stability += 10;
        };

    // INFRASTRUCTURE PANEL //TODO: Panel übersichtlicher gestalten
    infrastructureInfoMenu->CreateChild<Label>(0, 10, 200, []() {
        return std::to_string(state.cityRoads) + " City Roads";
        }, 0, true);
    auto* highwayCount = infrastructureInfoMenu->CreateChild<Label>(0, 35, 200, []() {
        return std::to_string(state.highways) + " Highways";
        }, 2, true);
    auto* cityAverageSpeed = infrastructureInfoMenu->CreateChild<Label>(0, 60, 200, []() {
        float speed = state.averageCityTravelSpeed();
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f", speed);
        return "City: " + std::string(buf) + " km/h";
    }, 0, true);
    auto* longDistanceAverageSpeed = infrastructureInfoMenu->CreateChild<Label>(0, 85, 200, []() {
        float speed = state.averageLongDistanceSpeed();
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f", speed);
        return "Long-distance: " + std::string(buf) + " km/h";
    }, 0, true);
    infrastructureInfoMenu->CreateChild<Label>(0, 120, 200, []() {
        return "City Subsidy: " + std::to_string(state.cityTransitSubsidyRate) + "%";
        }, 0, true);
    infrastructureInfoMenu->CreateChild<Label>(0, 145, 200, []() {
        return "Intercity Subsidy: " + std::to_string(state.intercityRailSubsidyRate) + "%";
        }, 0, true);
    infrastructureInfoMenu->CreateChild<Label>(0, 170, 200, []() {
        return state.isTransitPrivatized ? "System: Privatized" : "System: Nationalized";
    }, 0, true);
    infrastructureInfoMenu->CreateChild<Label>(0, 195, 200, []() {
        return "City Ticket: " + std::to_string(state.getCityTicketPrice()).substr(0,4) + " EUR";
    }, 0, true);
    infrastructureInfoMenu->CreateChild<Label>(0, 220, 200, []() {
        return "Intercity: " + std::to_string(state.getIntercityTicketPrice()).substr(0,4) + " EUR";
    }, 0, true);
    infrastructureInfoMenu->CreateChild<Label>(0, 245, 200, []() {
        return "National: " + std::to_string(state.getNationalTicketPrice()).substr(0,4) + " EUR";
    }, 0, true);
    auto* transitNetLabel = infrastructureInfoMenu->CreateChild<Label>(0, 270, 200, []() {
        int64_t balance = state.getTransitBalance();
        return std::string("Transit Net: ") + (balance >= 0 ? "+" : "") + formatValue(balance, "EUR");
    }, 0, true);

    highwayCount->colorSource = []() -> glm::vec3 {
        return UIColors::Blue;
    };
    transitNetLabel->colorSource = []() -> glm::vec3 {
        return (state.getTransitBalance() >= 0) ? UIColors::Positive : UIColors::Negative;
    };
    cityAverageSpeed->colorSource = []() -> glm::vec3 {
        if (state.averageCityTravelSpeed() > 30) { return UIColors::Positive; }
        else if (state.averageCityTravelSpeed() < 20) { return UIColors::Negative; }
        else { return UIColors::NeutralYellow; }
    };
    longDistanceAverageSpeed->colorSource = []() -> glm::vec3 {
        if (state.averageLongDistanceSpeed() > 125) { return UIColors::Positive; }
        else if (state.averageLongDistanceSpeed() < 110) { return UIColors::Negative; }
        else { return UIColors::NeutralYellow; }
    };

    infrastructureMenu->CreateChild<Button>(10, 20, 260, 35, "Build City Roads")->onClick = []() {
        int64_t cost = 80000000;
        if (state.money >= cost) {
            state.money -= cost;
            state.popAdults -= 5000;
            state.stability -= 2;
            state.cityRoads += 1;
        }
    };

    infrastructureMenu->CreateChild<Button>(10, 55, 260, 35, "Build Highway")->onClick = []() {
        int64_t cost = 100000000;
        if (state.money >= cost) {
            state.money -= cost;
            state.popAdults -= 20000;
            state.stability -= 5;
            state.highways += 1;
        }
    };

    infrastructureMenu->CreateChild<Button>(10, 90, 260, 35, "Improve Train Infrastructure")->onClick = []() {
        int64_t cost = 100000000;
        if (state.money >= cost) {
            state.money -= cost;
            state.popAdults += 5000;
            state.stability += 1;
            state.intercityRail += 1;
        }
    };

    infrastructureMenu->CreateChild<Button>(10, 125, 260, 35, "Expand Tram Network")->onClick = []() {
        int64_t cost = 150000000;
        if (state.money >= cost) {
            state.money -= cost;
            state.cityTransit += 1;
            state.stability += 2;
        }
    };

    infrastructureMenu->CreateChild<Button>(10, 160, 260, 35, "Inc City Subsidy (+5%)")->onClick = []() {
        if (state.cityTransitSubsidyRate < 100) state.cityTransitSubsidyRate += 5;
    };
    infrastructureMenu->CreateChild<Button>(10, 195, 260, 35, "Dec City Subsidy (-5%)")->onClick = []() {
        if (state.cityTransitSubsidyRate > 0) state.cityTransitSubsidyRate -= 5;
    };
    infrastructureMenu->CreateChild<Button>(10, 230, 260, 35, "Inc Intercity Subsidy (+5%)")->onClick = []() {
        if (state.intercityRailSubsidyRate < 100) state.intercityRailSubsidyRate += 5;
    };
    infrastructureMenu->CreateChild<Button>(10, 265, 260, 35, "Dec Intercity Subsidy (-5%)")->onClick = []() {
        if (state.intercityRailSubsidyRate > 0) state.intercityRailSubsidyRate -= 5;
    };

    infrastructureMenu->CreateChild<Button>(10, 300, 260, 35, "Toggle Privatization")->onClick = []() {
        state.isTransitPrivatized = !state.isTransitPrivatized;
        state.stability -= 15;
    };

    // HEALTHCARE PANEL
    healthcareInfoMenu->CreateChild<Label>(0, 20, 200, []() {
        return "Total Costs: " + formatValue(state.totalHealthcareCosts, "EUR");
        }, 0, true);
    healthcareInfoMenu->CreateChild<Label>(0, 50, 200, []() {
        return "State Costs: " + formatValue(state.stateHealthcareCosts(), "EUR");
        }, 0, true);
    healthcareInfoMenu->CreateChild<Label>(0, 80, 200, []() {
        return "Cost per Adult: " + formatValue(state.averageHealthcareCosts(), "EUR");
        }, 0, true);
    healthcareInfoMenu->CreateChild<Label>(0, 110, 200, []() {
        return "People per Hospital: " + formatValue(state.population() / state.hospitals);
        }, 0, true);

    healthcarePolicyMenu->CreateChild<Button>(10, 20, 260, 40, "Build a new Hospital")->onClick = []() {
        state.money -= 100000000;
        state.hospitals += 1;
        state.stability += 3;
    };
    healthcarePolicyMenu->CreateChild<Button>(10, 60, 260, 40, "Increase Healthcare Subsidy (+2%)")->onClick = []() {
        if (state.healthcareSubsidyRate < 100) {
            state.healthcareSubsidyRate = std::clamp(state.healthcareSubsidyRate + 2, 0, 100);
            state.stability += 4;
        }
    };
    healthcarePolicyMenu->CreateChild<Button>(10, 100, 260, 40, "Reduce Healthcare Subsidy (-2%)")->onClick = []() {
        if (state.healthcareSubsidyRate > 0) {
            state.healthcareSubsidyRate = std::clamp(state.healthcareSubsidyRate - 2, 0, 100);
            state.stability -= 5;
        }
    };

    // Services PANEL
    servicesInfoMenu->CreateChild<Label>(0, 20, 200, []() {
        return "Fire Houses: " + formatValue(state.fireHouses);
        }, 0, true);
    servicesInfoMenu->CreateChild<Line>(0, 45, 200, 2);
    servicesInfoMenu->CreateChild<Label>(0, 50, 200, []() {
        return "Police Stations: " + formatValue(state.policeStations);
        }, 0, true);
    servicesInfoMenu->CreateChild<Line>(0, 75, 200, 2);
    servicesInfoMenu->CreateChild<Label>(0, 80, 200, []() {
        return "Recycling Centers: " + formatValue(state.trashRecycling);
        }, 0, true);
    servicesInfoMenu->CreateChild<Line>(0, 105, 200, 2);
    auto* serviceInfoIncomeLabel = servicesInfoMenu->CreateChild<Label>(0, 110, 200, []() {
        const int64_t net = -state.stateServicesCosts();
        return (net >= 0 ? "+" : "") + formatValue(net, "EUR");
        }, 0, true);

    serviceInfoIncomeLabel->colorSource = []() -> glm::vec3 {
        return (-state.stateServicesCosts() >= 0) ? UIColors::Positive : UIColors::Negative;
    };

    servicesPolicyMenu->CreateChild<Button>(10, 20, 260, 40, "Build a new Firehouse")->onClick = []() {
        state.money -= 10000000;
        state.fireHouses += 1;
        state.stability += 3;
    };
    servicesPolicyMenu->CreateChild<Button>(10, 60, 260, 40, "Build a new Police Station")->onClick = []() {
        state.money -= 10000000;
        state.policeStations += 1;
        state.stability += 3;
    };
    servicesPolicyMenu->CreateChild<Button>(10, 100, 260, 40, "Build a new Recycling Center")->onClick = []() {
        state.money -= 10000000;
        state.trashRecycling += 1;
        state.stability += 3;
    };

    // Education PANEL
    educationInfoMenu->CreateChild<Label>(0, 20, 200, []() {
        return "Elementary Schools: " + formatValue(state.elementarySchools);
        }, 0, true);
    educationInfoMenu->CreateChild<Label>(0, 40, 200, []() {
        return "High Schools: " + formatValue(state.highSchools);
        }, 0, true);
    educationInfoMenu->CreateChild<Label>(0, 60, 200, []() {
        return "Universities: " + formatValue(state.universities);
        }, 0, true);
    educationInfoMenu->CreateChild<Line>(0, 105, 200, 2);
    auto* educationInfoIncomeLabel = educationInfoMenu->CreateChild<Label>(0, 125, 200, []() {
        const int64_t net = -state.stateEducationCosts();
        return (net >= 0 ? "+" : "") + formatValue(net, "EUR");
        }, 0, true);

    educationInfoIncomeLabel->colorSource = []() -> glm::vec3 {
        return (-state.stateEducationCosts() >= 0) ? UIColors::Positive : UIColors::Negative;
    };

    educationPolicyMenu->CreateChild<Button>(10, 20, 260, 40, "Build a new Elementary School")->onClick = []() {
        state.money -= 10000000;
        state.elementarySchools += 1;
        state.stability += 2;
    };
    educationPolicyMenu->CreateChild<Button>(10, 60, 260, 40, "Build a new High School")->onClick = []() {
        state.money -= 10000000;
        state.highSchools += 1;
        state.stability += 3;
    };
    educationPolicyMenu->CreateChild<Button>(10, 100, 260, 40, "Build a new University")->onClick = []() {
        state.money -= 100000000;
        state.universities += 1;
        state.stability += 5;
    };

    // Military PANEL
    militaryInfoMenu->CreateChild<Label>(0, 20, 200, []() {
        return "Solders: " + formatValue(state.solders);
        }, 0, true);
    militaryInfoMenu->CreateChild<Label>(0, 40, 200, []() {
        return "Military Bases: " + formatValue(state.militaryStations);
        }, 0, true);
    militaryInfoMenu->CreateChild<Label>(0, 60, 200, []() {
        return "Wars: " + formatValue(state.wars);
        }, 0, true);
    militaryInfoMenu->CreateChild<Line>(0, 105, 200, 2);
    auto* militaryInfoIncomeLabel = militaryInfoMenu->CreateChild<Label>(0, 125, 200, []() {
        const int64_t net = -state.stateMilitaryCosts();
        return (net >= 0 ? "+" : "") + formatValue(net, "EUR");
        }, 0, true);

    militaryInfoIncomeLabel->colorSource = []() -> glm::vec3 {
        return (-state.stateMilitaryCosts() >= 0) ? UIColors::Positive : UIColors::Negative;
    };

    // Initial visibility
    gameUI->visible = false;
    taxesMenu->visible = false;
    infrastructureMenu->visible = false;
    populationPolicyMenu->visible = false;
    healthcarePolicyMenu->visible = false;
    servicesPolicyMenu->visible = false;
    educationPolicyMenu->visible = false;
    militaryPolicyMenu->visible = false;
    topBarNotificationLabel->visible = false;

    // Menus list
    menus.push_back(taxesMenu);
    menus.push_back(infrastructureMenu);
    menus.push_back(populationPolicyMenu);
    menus.push_back(healthcarePolicyMenu);
    menus.push_back(servicesPolicyMenu);
    menus.push_back(educationPolicyMenu);
    menus.push_back(militaryPolicyMenu);

    MSG msg = {};
    while (true) {
        glClear(GL_COLOR_BUFFER_BIT);
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        //FPS Berechnung
        frameCount++;
        if (std::chrono::duration<float>(currentTime - fpsTimer).count() >= 1.0f) {
            fps = static_cast<float>(frameCount);
            frameCount = 0;
            fpsTimer = currentTime;

            wchar_t title[256]; //TODO: Die Funfacts vielleicht am Anfang irgendwo speichern, damit nicht jede Sekunde ein neues Wort steht.
            const wchar_t* funFacts[] = { L"Now with 20% more corruption!", L"Calculating taxes...", L"Citizens are complaining!", L"Adding more potholes..." };
            swprintf_s(title, L"The Politics Simulator | FPS: %.1f | %s", fps, funFacts[rand() % 4]);
            SetWindowText(hwnd, title);
        }

        // Simulation
        if (inGame && simSpeed > 0.0f) {
            float effectiveInterval = simInterval / simSpeed;
            float simDelta = std::chrono::duration<float>(currentTime - simTimer).count();

            if (simDelta >= effectiveInterval) {
                auto startSim = std::chrono::high_resolution_clock::now();

                state.UpdateSimulation();
                state.currentDay++;

                auto endSim = std::chrono::high_resolution_clock::now();
                lastSimTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(endSim - startSim).count();

                simTimer = currentTime;
            }
        }

        bool qDown = GetAsyncKeyState('Q') & 0x8000;
        if (qDown && !qWasDown) {
            mainMenuUI->visible = !mainMenuUI->visible;
            gameUI->visible = !gameUI->visible;
        }
        qWasDown = qDown;

        bool eDown = GetAsyncKeyState('E') & 0x8000;
        if (eDown && !eWasDown) {
            inGame = true;
            state = GameState::CreateSmallVillage();
        }
        eWasDown = eDown;

        // UI Update
        for (int i = static_cast<int>(ui.size()) - 1; i >= 0; --i) {
            if (ui[i]->parent == nullptr && ui[i]->IsVisibleRecursive()) {
                ui[i]->Update(deltaTime);
            }
        }

        // UI Draw
        for (auto& e : ui) {
            if (e->parent == nullptr) {
                if (e->IsVisibleRecursive()) {
                    e->Draw();
                }
            }
        }

        const bool lowTrust = state.stability < 11;
        const bool inDebt = state.money < 0;
        const bool tooHighCityTraffic = state.averageCityTravelSpeed() < 20.0f;
        const bool tooHighStateTraffic = state.averageLongDistanceSpeed() < 50.0f;
        const bool tooExpensiveHealthcare = 120 < state.averageHealthcareCosts();
        bool isWarning = lowTrust || inDebt || tooHighCityTraffic || tooHighStateTraffic || tooExpensiveHealthcare;

        if (isWarning)
        {
            topBarNotificationLabel->visible = true;
            topBar->r = 1.0f;
        }
        else if (topBar->r >= 0.9f)
        {
            topBar->r = 0.1f;
            topBarNotificationLabel->visible = false;
        }

        if (state.stability <= 0.0f) {
            MessageBoxW(nullptr, L"Die Regierung hat jegliche Unterstützung verloren.\nGame Over.", L"Game Over", MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
            PostQuitMessage(0);
        }

        SwapBuffers(hDC);

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return 0;
            TranslateMessage(&msg); DispatchMessage(&msg);
        }
    }
}
