#pragma once
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <string>

namespace Flux
{

struct SplashConfig
{
    const char *title = "Flux Engine";
    const char *subtitle = "v0.1 Alpha";
    float durationSec = 2.8f;
    ImVec4 bgColor = {0.07f, 0.07f, 0.09f, 1.f};
    ImVec4 titleColor = {1.f, 0.85f, 0.35f, 1.f}; // gold
    ImVec4 subColor = {0.6f, 0.6f, 0.6f, 1.f};
    ImVec4 barColor = {1.f, 0.85f, 0.35f, 1.f};
    float barHeight = 4.f;
    float titleSize = 2.4f;
    float subSize = 1.1f;
};

inline void RunSplashScreen(GLFWwindow *window, const SplashConfig &cfg = {})
{
    double start = glfwGetTime();
    while (!glfwWindowShouldClose(window))
    {
        double elapsed = glfwGetTime() - start;
        float t = (float)(elapsed / cfg.durationSec);
        if (t >= 1.f)
            break;

        float alpha = (t < 0.2f) ? (t / 0.2f) : (t > 0.8f) ? ((1.f - t) / 0.2f) : 1.f;

        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiIO &io = ImGui::GetIO();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::SetNextWindowBgAlpha(1.f);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, cfg.bgColor);
        ImGui::Begin("##splash", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav |
                         ImGuiWindowFlags_NoMove);

        ImGui::SetWindowFontScale(cfg.titleSize);
        ImVec4 tc = cfg.titleColor;
        tc.w *= alpha;
        ImGui::SetCursorPosY(io.DisplaySize.y * 0.38f);
        float tw = ImGui::CalcTextSize(cfg.title).x;
        ImGui::SetCursorPosX((io.DisplaySize.x - tw) * 0.5f);
        ImGui::TextColored(tc, "%s", cfg.title);

        ImGui::SetWindowFontScale(cfg.subSize);
        ImVec4 sc = cfg.subColor;
        sc.w *= alpha;
        float sw = ImGui::CalcTextSize(cfg.subtitle).x;
        ImGui::SetCursorPosX((io.DisplaySize.x - sw) * 0.5f);
        ImGui::TextColored(sc, "%s", cfg.subtitle);

        ImDrawList *dl = ImGui::GetWindowDrawList();
        float barW = io.DisplaySize.x * t;
        ImVec4 bc = cfg.barColor;
        bc.w *= alpha;
        dl->AddRectFilled({0, io.DisplaySize.y - cfg.barHeight}, {barW, io.DisplaySize.y},
                          ImGui::ColorConvertFloat4ToU32(bc));

        ImGui::SetWindowFontScale(1.f);
        ImGui::End();
        ImGui::PopStyleColor();

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(cfg.bgColor.x, cfg.bgColor.y, cfg.bgColor.z, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
}

} // namespace Flux