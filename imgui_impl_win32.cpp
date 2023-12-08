#include "imgui.h"
#include "imgui_impl_win32.h"

#include "ModuleUtils.hpp"

#include <cstdint>

typedef BOOL (WINAPI* tQueryPerformanceCounter)(LARGE_INTEGER* lpPerformanceCount);
tQueryPerformanceCounter _QueryPerformanceCounter = nullptr;

typedef BOOL (WINAPI* tQueryPerformanceFrequency)(LARGE_INTEGER* lpFrequency);
tQueryPerformanceFrequency _QueryPerformanceFrequency = nullptr;

static void* im_memset(void* destination, int value, size_t size)
{
    auto data = static_cast<uint8_t*>(destination);

    __stosb(data, static_cast<uint8_t>(value), size);
    return static_cast<void*>(data);
}

struct ImGui_ImplWin32_Data
{
    INT64                       Time;
    INT64                       TicksPerSecond;

    ImGui_ImplWin32_Data()      { im_memset(this, 0, sizeof(*this)); }
};

static ImGui_ImplWin32_Data* ImGui_ImplWin32_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplWin32_Data*)ImGui::GetIO().BackendPlatformUserData : NULL;
}

bool ImGui_ImplWin32_InitData()
{
    LPCSTR lpNtdll = xor ("ntdll.dll");
    HMODULE hNtdll = (HMODULE)ModuleUtils::GetModuleBase(lpNtdll);

    if (!hNtdll)
        return false;

    _QueryPerformanceCounter = (tQueryPerformanceCounter)ModuleUtils::GetFuncAddress(hNtdll, xor ("RtlQueryPerformanceCounter"));
    _QueryPerformanceFrequency = (tQueryPerformanceFrequency)ModuleUtils::GetFuncAddress(hNtdll, xor ("RtlQueryPerformanceFrequency"));

    return true;
}

bool    ImGui_ImplWin32_Init(void* hwnd)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendPlatformUserData == NULL);

    INT64 perf_frequency, perf_counter;
    if (!_QueryPerformanceFrequency((LARGE_INTEGER*)&perf_frequency))
        return false;
    if (!_QueryPerformanceCounter((LARGE_INTEGER*)&perf_counter))
        return false;

    ImGui_ImplWin32_Data* bd = IM_NEW(ImGui_ImplWin32_Data)();
    
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = xor ("rbqx");
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

    bd->TicksPerSecond = perf_frequency;
    bd->Time = perf_counter;

    io.ImeWindowHandle = hwnd;

    io.KeyMap[ImGuiKey_Tab] = VK_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
    io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
    io.KeyMap[ImGuiKey_Home] = VK_HOME;
    io.KeyMap[ImGuiKey_End] = VK_END;
    io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
    io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
    io.KeyMap[ImGuiKey_Space] = VK_SPACE;
    io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = VK_RETURN;

    return true;
}

void ImGui_ImplWin32_Shutdown()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Data* bd = ImGui_ImplWin32_GetBackendData();

    io.BackendPlatformName = NULL;
    io.BackendPlatformUserData = NULL;
    IM_DELETE(bd);
}

void ImGui_ImplWin32_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Data* bd = ImGui_ImplWin32_GetBackendData();
    IM_ASSERT(bd != NULL);

    INT64 current_time = 0;
    _QueryPerformanceCounter((LARGE_INTEGER*)&current_time);

    io.DeltaTime = (float)(current_time - bd->Time) / bd->TicksPerSecond;
    bd->Time = current_time;
}

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif
#ifndef DBT_DEVNODES_CHANGED
#define DBT_DEVNODES_CHANGED 0x0007
#endif

int GetMouseButton(std::uint16_t key)
{
    auto button = 0;

    switch (key)
    {
    case XBUTTON1:
    {
        button = 3;
        break;
    }
    case XBUTTON2:
    {
        button = 4;
        break;
    }
    }

    return button;
}

int GetMouseButton(std::uint32_t message, std::uintptr_t wparam)
{
    auto button = 0;

    switch (message)
    {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONUP:
    {
        button = 0;
        break;
    }
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONUP:
    {
        button = 1;
        break;
    }
    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONUP:
    {
        button = 2;
        break;
    }
    case WM_XBUTTONDOWN:
    case WM_XBUTTONDBLCLK:
    case WM_XBUTTONUP:
    {
        button = GetMouseButton(GET_XBUTTON_WPARAM(wparam));
        break;
    }
    }

    return button;
}

ImVec2 GetMouseLocation(std::intptr_t lparam)
{
    const auto x = static_cast<std::uint16_t>(lparam);
    const auto y = static_cast<std::uint16_t>(lparam >> 16);

    ImVec2 location =
    {
        static_cast<float>(x),
        static_cast<float>(y),
    };

    return location;
}

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    auto current_context = ImGui::GetCurrentContext();

    if (!current_context)
        return FALSE;

    auto& io = ImGui::GetIO();

    switch (message)
    {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONDBLCLK:
    {
        const auto button = GetMouseButton(message, wparam);
        io.MouseDown[button] = true;
        return TRUE;
    }
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP:
    {
        const auto button = GetMouseButton(message, wparam);
        io.MouseDown[button] = false;
        return TRUE;
    }
    case WM_MOUSEWHEEL:
    {
        const auto amount = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam));
        io.MouseWheel += (amount / static_cast<float>(WHEEL_DELTA));
        return TRUE;
    }
    case WM_MOUSEHWHEEL:
    {
        const auto amount = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam));
        io.MouseWheelH += (amount / static_cast<float>(WHEEL_DELTA));
        return TRUE;
    }
    case WM_MOUSEMOVE:
    {
        io.MousePos = GetMouseLocation(lparam);
        return FALSE;
    }
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        if (wparam < 256)
        {
            io.KeysDown[wparam] = true;
        }
        return TRUE;
    }
    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        if (wparam < 256)
        {
            io.KeysDown[wparam] = false;
        }
        return TRUE;
    }
    case WM_CHAR:
    {
        const auto character = static_cast<std::uint32_t>(wparam);
        io.AddInputCharacter(character);
        return TRUE;
    }
    }

    return FALSE;
}