#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"
#include <SDL.h>
#include <SDL_opengl.h>

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>

using namespace std;

struct Smartphone {
    int id;
    string name;
    int budgetCategory;
    vector<string> tags;
    int stock;

    int matchScore(const vector<string>& prefs) const {
        int score = 0;
        for (const auto& tag : prefs)
            for (const auto& phoneTag : tags)
                if (tag == phoneTag)
                    score++;
        return score;
    }
};

class SmartphoneBuilder {
public:
    vector<Smartphone>& allPhones;
    vector<string> softPrefs;
    int budget;

    SmartphoneBuilder(vector<Smartphone>& phones) : allPhones(phones), budget(0) {}

    void setBudget(int b) { budget = b; }
    void addSoftPref(const string& tag) { softPrefs.push_back(tag); }

    vector<Smartphone> build() {
        vector<Smartphone> results;
        int bestScore = 0;
        for (const auto& phone : allPhones) {
            if (phone.budgetCategory != budget) continue;
            int score = phone.matchScore(softPrefs);
            if (score > bestScore) bestScore = score;
        }

        for (const auto& phone : allPhones) {
            if (phone.budgetCategory != budget) continue;
            int score = phone.matchScore(softPrefs);
            if (score == bestScore)
                results.push_back(phone);
        }
        return results;
    }
};

int main() {
    srand(time(0));
    vector<Smartphone> phones = {
        {1, "Apple iPhone 17 Air", 4, {"Apple", "Slim"}, rand()%10+1},
        {2, "Samsung S25 Edge", 4, {"Snapdragon", "Slim"}, rand()%10+1},
        {3, "Samsung S25+", 4, {"Snapdragon", "Camera"}, rand()%10+1},
        {4, "Nothing Phone 2a", 2, {"Snapdragon", "Slim"}, rand()%10+1},
        {5, "Samsung A56", 1, {"Budget", "Camera"}, rand()%10+1},
        {6, "Redmi Note 13+", 1, {"Budget", "Camera"}, rand()%10+1},
        {7, "Samsung M53", 2, {"Mediatek", "LongBattery"}, rand()%10+1},
        {8, "Apple iPhone 15", 4, {"Apple", "Camera"}, rand()%10+1},
        {9, "Apple iPhone 15+", 4, {"Apple", "Camera"}, rand()%10+1}
    };

    // SDL + OpenGL context
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) return -1;
    SDL_Window* window = SDL_CreateWindow("Smartphone Recommender", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1000, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 130");

    // GUI State
    int role = 0;
    char adminPwd[32] = "";
    bool loggedIn = false;
    bool showCustomer = false;
    bool showAdmin = false;
    vector<Smartphone> recommendations;

    int budget = 0, ss = 0, soc = 0, battery = 0, extras = 0;
    int selectedPhoneId = -1;
    char purchaseStatus[64] = "";

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event))
            ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            running = false;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Smartphone Recommendation System");

        if (!loggedIn) {
            ImGui::Text("Login as:");
            if (ImGui::RadioButton("Admin", role == 1)) role = 1;
            if (ImGui::RadioButton("Customer", role == 2)) role = 2;

            if (role == 1) {
                ImGui::InputText("Admin Password", adminPwd, 32, ImGuiInputTextFlags_Password);
                if (ImGui::Button("Login")) {
                    if (string(adminPwd) == "admin123") {
                        loggedIn = true;
                        showAdmin = true;
                    }
                }
            } else if (role == 2) {
                if (ImGui::Button("Proceed")) {
                    loggedIn = true;
                    showCustomer = true;
                }
            }
        }

        // Admin GUI
        if (showAdmin) {
            ImGui::Text("Admin Dashboard");
            for (auto& phone : phones) {
                ImGui::Text("%d. %s (Stock: %d)%s", phone.id, phone.name.c_str(), phone.stock,
                            (phone.stock < 5 ? " [LOW STOCK]" : ""));
            }

            static int idToAdd = 0;
            static int qtyToAdd = 0;
            ImGui::InputInt("Phone ID", &idToAdd);
            ImGui::InputInt("Quantity", &qtyToAdd);
            if (ImGui::Button("Add Stock")) {
                for (auto& phone : phones) {
                    if (phone.id == idToAdd) {
                        phone.stock += qtyToAdd;
                        break;
                    }
                }
            }
        }

        // Customer GUI
        if (showCustomer) {
            ImGui::Text("Customer Preferences:");

            const char* screenOptions[] = {"<6.5", "6.5–6.7", ">6.7", "Slim"};
            const char* socOptions[] = {"Apple", "Snapdragon", "Mediatek", "No preference"};
            const char* batteryOptions[] = {"LongBattery", "FastCharging", "EfficientUI", "No preference"};
            const char* budgetOptions[] = {"<25k", "25–30k", "30–50k", "50k+"};
            const char* extraOptions[] = {"Slim", "Camera", "Performance", "None"};

            ImGui::Combo("Screen Size", &ss, screenOptions, IM_ARRAYSIZE(screenOptions));
            ImGui::Combo("SoC", &soc, socOptions, IM_ARRAYSIZE(socOptions));
            ImGui::Combo("Battery", &battery, batteryOptions, IM_ARRAYSIZE(batteryOptions));
            ImGui::Combo("Budget", &budget, budgetOptions, IM_ARRAYSIZE(budgetOptions));
            ImGui::Combo("Extras", &extras, extraOptions, IM_ARRAYSIZE(extraOptions));

            if (ImGui::Button("Get Recommendations")) {
                SmartphoneBuilder builder(phones);
                builder.setBudget(budget + 1);

                if (soc == 0) builder.addSoftPref("Apple");
                else if (soc == 1) builder.addSoftPref("Snapdragon");
                else if (soc == 2) builder.addSoftPref("Mediatek");

                if (battery == 0) builder.addSoftPref("LongBattery");
                else if (battery == 1) builder.addSoftPref("FastCharging");
                else if (battery == 2) builder.addSoftPref("EfficientUI");

                if (extras == 0) builder.addSoftPref("Slim");
                else if (extras == 1) builder.addSoftPref("Camera");
                else if (extras == 2) builder.addSoftPref("Performance");

                recommendations = builder.build();
                strcpy(purchaseStatus, "");
            }

            if (!recommendations.empty()) {
                ImGui::Separator();
                ImGui::Text("Recommendations:");
                for (const auto& phone : recommendations) {
                    ImGui::RadioButton((to_string(phone.id) + ". " + phone.name + " (Stock: " + to_string(phone.stock) + ")").c_str(), &selectedPhoneId, phone.id);
                }
                if (ImGui::Button("Buy Selected")) {
                    bool found = false;
                    for (auto& phone : recommendations) {
                        if (phone.id == selectedPhoneId && phone.stock > 0) {
                            phone.stock--;
                            snprintf(purchaseStatus, 64, "Order placed for: %s", phone.name.c_str());
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        strcpy(purchaseStatus, "Invalid ID selected or out of stock.");
                    }
                }
                ImGui::Text("%s", purchaseStatus);
            }
        }

        ImGui::End();
        ImGui::Render();

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
