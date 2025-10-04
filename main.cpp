#include "imgui.h"
#include <d3d11.h>
#include <tchar.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>
#include <imgui_internal.h>
#include <addons/imgui_addons.h>
#include <sstream>
#include <random>
#include <iomanip>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static bool  glowingToggle = false;
static float glowingValue = 10;

int main(int, char**)
{
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"interface", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowExW(WS_EX_TOPMOST, wc.lpszClassName, L"interface", WS_POPUP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImGuiStyle& style = ImGui::GetStyle();
    auto colors = style.Colors;
    ImVec4 accent = ImAdd::HexToColorVec4(0xFF69B4, 1);

    colors[ImGuiCol_WindowBg] = ImAdd::HexToColorVec4(0x1A0F1A, 1);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = accent;
    colors[ImGuiCol_Separator] = accent;
    colors[ImGuiCol_FrameBg] = ImAdd::HexToColorVec4(0x2D1A2D, 1);
    colors[ImGuiCol_FrameBgHovered] = ImAdd::HexToColorVec4(0x3D2A3D, 1);
    colors[ImGuiCol_FrameBgActive] = ImAdd::HexToColorVec4(0x4D3A4D, 1);
    colors[ImGuiCol_HeaderHovered] = ImAdd::HexToColorVec4(0x3D2A3D, 1);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.3f, 0.3f, 0.3f, 0);
    colors[ImGuiCol_Text] = ImColor(220, 200, 220);
    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_WindowShadow] = accent;
    colors[ImGuiCol_ButtonHovered] = accent;

    style.WindowBorderSize = 1.0f;
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.ChildRounding = 8.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 6.0f;

    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Tahoma.ttf", 14);

    static bool draw = false;
    ImVec4 clear_color = ImVec4(0.f, 0, 0.f, 0);

    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (GetAsyncKeyState(VK_INSERT) & 1) draw = !draw;
        if (GetAsyncKeyState(VK_END) & 1) exit(0);

        if (draw)
        {
            if (glowingToggle) {
                style.WindowShadowSize = glowingValue;
            }
            else {
                style.WindowShadowSize = 0;
            }

            ImGui::SetNextWindowSize(ImVec2(650, 550), ImGuiCond_Once);
            ImGui::SetNextWindowBgAlpha(0.98f);
            ImGui::Begin("##main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
            {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5);
                ImAdd::Text(ImVec4(accent), "fat fuck");

                static int selected_tab = 0;
                const char* tabNames[] = { "aimbot", "visuals", "misc", "config" };
                const int tabCount = sizeof(tabNames) / sizeof(tabNames[0]);

                float total_width = tabCount * 80;
                float start_x = (ImGui::GetWindowWidth() - total_width) * 0.5f;
                ImGui::SetCursorPosX(start_x);

                for (int i = 0; i < tabCount; ++i) {
                    ImGui::PushID(i);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(accent.x, accent.y, accent.z, 0.1f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(accent.x, accent.y, accent.z, 0.2f));


                    if (selected_tab == i) {
                        ImGui::PushStyleColor(ImGuiCol_Text, accent);
                    }
                    else {
                        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.145f, 0.082f, 0.145f, 0.8f));
                    }

                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);
                    if (ImAdd::TabButton(tabNames[i], ImVec2(80, 30), selected_tab == i)) {
                        selected_tab = i;
                    }

                    ImGui::PopStyleColor(1);
                    ImGui::PopStyleColor(3);
                    if (i < tabCount - 1) ImGui::SameLine();
                    ImGui::PopID();
                }

                ImGui::Spacing();
                ImGui::Separator();

                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImAdd::HexToColorVec4(0x251525, 0.8f));
                ImGui::BeginChild("##content", ImVec2(0, 0), false);
                {
                    ImGui::Columns(2, nullptr, false);
                    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.5f - 10);

                    ImGui::Spacing(); ImGui::Spacing();
                    ImAdd::BeginChild("##features", ImVec2(0, 0));
                    {
                        static int ref;
                        static const char* items[3] = { "smooth", "predictive", "silent" };
                        static bool aimEnabled = false;
                        static float fovValue = 90.0f;
                        static float smoothValue = 5.0f;

                        ImAdd::CheckBox("enable aimbot", &aimEnabled);
                        ImAdd::Combo("aim mode", &ref, items, IM_ARRAYSIZE(items));
                        ImAdd::SliderFloat("aim fov", &fovValue, 1, 360);
                        ImAdd::SliderFloat("smoothness", &smoothValue, 1, 20);
                        ImAdd::CheckBox("enable glow", &glowingToggle);
                        ImAdd::SliderFloat("glow spread", &glowingValue, 1, 500);
                    }
                    ImAdd::EndChild();

                    ImGui::NextColumn();
                    ImGui::Spacing(); ImGui::Spacing();
                    ImAdd::BeginChild("##settings", ImVec2(0, 0));
                    {
                        static bool espEnabled = true;
                        static bool showBox = true;
                        static bool showSkeleton = false;
                        static bool showDistance = true;
                        static float maxDistance = 250.0f;

                        ImAdd::CheckBox("enable esp", &espEnabled);
                        ImAdd::CheckBox("show box", &showBox);
                        ImAdd::CheckBox("show skeleton", &showSkeleton);
                        ImAdd::CheckBox("show distance", &showDistance);
                        ImAdd::SliderFloat("max distance", &maxDistance, 50, 1000);
                        ImGui::Spacing();
                        ImAdd::Button("save config", ImVec2(120, 35));
                        ImGui::SameLine();
                        ImAdd::Button("load config", ImVec2(120, 35));
                    }
                    ImAdd::EndChild();

                    ImGui::Columns(1);
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }
            ImGui::End();
        }

        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 165;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}