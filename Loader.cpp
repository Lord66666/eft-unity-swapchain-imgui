#include "EFTBase.hpp"
#include "EFTPriceMap.hpp"
#include "tyler/tyler.hpp"

#include <array>
#include <string_view>

#include "tyler/font/DrukWideBold.hpp"
#include "tyler/font/Poppins.hpp"
#include "tyler/font/Logotype.hpp"

#include <iostream>
#include <string>

#include <sstream>

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#define M_PI 3.14159265358979323846

using namespace std;

static HWND hwnd = 0;

ImFont* guiFont = nullptr;
ImFont* lootFont = nullptr;

ID3D11Device* Device = nullptr;

ID3D11DeviceContext* DeviceContext = nullptr;
ID3D11RenderTargetView* RenderTargetView = nullptr;

typedef HRESULT (__fastcall* tPresentScene)(IDXGISwapChain* SwapChain, UINT SyncInterval, UINT Flags);
tPresentScene PresentScene_Original = NULL;

typedef HRESULT (__fastcall* tResizeBuffers)(IDXGISwapChain* SwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
tResizeBuffers ResizeBuffers_Original = NULL;

typedef BOOL (WINAPI* tGetClientRect)(HWND hWnd, LPRECT lpRect);
tGetClientRect _GetClientRect = NULL;

static WNDPROC UnityWindowProc = nullptr;

typedef SHORT (WINAPI* tGetAsyncKeyState)(int vKey);
tGetAsyncKeyState _GetAsyncKeyState = nullptr;

typedef LRESULT (WINAPI* tCallWindowProcW)(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
tCallWindowProcW _CallWindowProcW = nullptr;

typedef LONG_PTR (WINAPI* tGetWindowLongPtrW)(HWND hWnd, int nIndex);
tGetWindowLongPtrW _GetWindowLongPtrW = nullptr;

typedef LONG_PTR (WINAPI* tSetWindowLongPtrW)(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
tSetWindowLongPtrW _SetWindowLongPtrW = nullptr;

static BOOL ImMenu = FALSE;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WndProc_Hook(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == WM_KEYDOWN && wParam == VK_INSERT)
	{
		ImMenu = !ImMenu;
	}

	if (Msg == WM_KEYDOWN && wParam == VK_ESCAPE && ImMenu == TRUE)
	{
		ImMenu = FALSE;
	}

	if (ImMenu == TRUE)
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam))
			return TRUE;

		if (Msg != WM_LBUTTONDOWN && Msg != WM_LBUTTONUP && Msg != WM_RBUTTONDOWN
			&& Msg != WM_RBUTTONUP && Msg != WM_MOUSEWHEEL && Msg != WM_MOUSEMOVE
			&& Msg != 0x20 && Msg != 0x8 && Msg != 0x21 && Msg != 0x7)
		{
			return _CallWindowProcW(UnityWindowProc, hWnd, Msg, wParam, lParam);
		}

		return TRUE;
	}

	return _CallWindowProcW(UnityWindowProc, hWnd, Msg, wParam, lParam);
}

int AimBotKey = VK_RBUTTON;
bool AimBotEnabled = false;

static int AimBotFOV = 50;
static int AimBotSmooth = 0;
static int AimBone = 1;
static int AimType = 1;

bool DrawFOV = true, PlayerESP = true, ShowPlayerPrice = true, BoneESP = true;
bool DrawExits = true, ExitsOpenedOnly = true, HealthBar = true, DistanceESP = true, Nicknames = true, ActiveWeaponESP = true;

static int BoxStyle = 0;

static bool hideHud = false;
bool LootESP = true;
static int MinItemPrice = 25000;
static int MinContPrice = 25000;
static int MinCorpsPrice = 25000;
static int LootDistance = 100;
static int LootCorpsDistance = 100;
static int ContainerDistance = 100;

static int NightRed = 111, NightGreen = 147, NightBlue = 245;

bool ShowItems = true, ShowCorpses = true, ShowContainers = true;

bool InfinityStamina = true;
bool NoVisor = true;
bool ThermalVision = false;
bool NightVision = false;
bool NightVisionCustom = false;
bool NoRecoil = true, NoSway = true, NoSpread = true, NoBreath = true, FastSpeed = true;

static int SpeedKey = VK_LSHIFT;

static UnityColor NightVisionColor;

bool IsThermalSet = true;
bool IsNightSet = true;
bool IsVisorSet = true;
bool IsSpeedSet = true;
bool IsStaminaSet = true;

std::string ReplaceString(std::string subject, const std::string& search, const std::string& replace)
{
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos)
	{
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}

	return subject;
}

inline int GetItemPrice(const std::uint64_t hash)
{
	for (auto& entry : m_market_items_array)
	{
		if (entry.m_hash == hash)
			return entry.m_price;
	}
	return 0;
}

inline const char* GetItemName(std::uint64_t hash)
{
	for (auto& entry : m_market_items_array)
	{
		if (entry.m_hash == hash)
			return entry.m_name;
	}

	return { };
}

inline float GetItemDistance(Vector3 cameraPos, Vector3 itemPos)
{
	if (EFTCore::m_Camera == nullptr)
		return 0;

	if (cameraPos.Zero())
		return 0;

	return itemPos.Distance(cameraPos);
}

ImGuiWindow* BeginScene()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
		ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.0f, 0.0f, 0.0f, 0.0f });

		ImGui::Begin(xor ("PLZmc"), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs);
		{
			ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
			ImGui::SetWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y), ImGuiCond_Always);
		}
	}

	return ImGui::GetCurrentWindow();
}

VOID EndScene(ImGuiWindow* window)
{
	window->DrawList->PushClipRectFullScreen();

	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);

	ImGui::Render();
}

void DrawCornerESP(float X, float Y, float W, float H, const ImU32& color, float thickness)
{
	float lineW = (W / 4);
	float lineH = (H / 4);

	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddLine(ImVec2(X, Y), ImVec2(X, Y + lineH), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 255.0f / 255.0f)), 3);
	window->DrawList->AddLine(ImVec2(X, Y), ImVec2(X + lineW, Y), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 255.0f / 255.0f)), 3);
	window->DrawList->AddLine(ImVec2(X + W - lineW, Y), ImVec2(X + W, Y), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 255.0f / 255.0f)), 3);
	window->DrawList->AddLine(ImVec2(X + W, Y), ImVec2(X + W, Y + lineH), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 255.0f / 255.0f)), 3);
	window->DrawList->AddLine(ImVec2(X, Y + H - lineH), ImVec2(X, Y + H), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 255.0f / 255.0f)), 3);
	window->DrawList->AddLine(ImVec2(X, Y + H), ImVec2(X + lineW, Y + H), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 255.0f / 255.0f)), 3);
	window->DrawList->AddLine(ImVec2(X + W - lineW, Y + H), ImVec2(X + W, Y + H), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 255.0f / 255.0f)), 3);
	window->DrawList->AddLine(ImVec2(X + W, Y + H - lineH), ImVec2(X + W, Y + H), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 255.0f / 255.0f)), 3);

	window->DrawList->AddLine(ImVec2(X, Y), ImVec2(X, Y + lineH), ImGui::GetColorU32(color), thickness);
	window->DrawList->AddLine(ImVec2(X, Y), ImVec2(X + lineW, Y), ImGui::GetColorU32(color), thickness);
	window->DrawList->AddLine(ImVec2(X + W - lineW, Y), ImVec2(X + W, Y), ImGui::GetColorU32(color), thickness);
	window->DrawList->AddLine(ImVec2(X + W, Y), ImVec2(X + W, Y + lineH), ImGui::GetColorU32(color), thickness);
	window->DrawList->AddLine(ImVec2(X, Y + H - lineH), ImVec2(X, Y + H), ImGui::GetColorU32(color), thickness);
	window->DrawList->AddLine(ImVec2(X, Y + H), ImVec2(X + lineW, Y + H), ImGui::GetColorU32(color), thickness);
	window->DrawList->AddLine(ImVec2(X + W - lineW, Y + H), ImVec2(X + W, Y + H), ImGui::GetColorU32(color), thickness);
	window->DrawList->AddLine(ImVec2(X + W, Y + H - lineH), ImVec2(X + W, Y + H), ImGui::GetColorU32(color), thickness);
	window->DrawList->AddLine(ImVec2(X + W, Y + H - lineH), ImVec2(X + W, Y + H), ImGui::GetColorU32(color), thickness);
}

float DrawOutlinedText(ImFont* pFont, const std::string& text, const ImVec2& pos, float size, ImU32 color, bool center)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	std::stringstream stream(text);
	std::string line;

	float y = 0.0f;
	int i = 0;

	while (std::getline(stream, line))
	{
		ImVec2 textSize = pFont->CalcTextSizeA(size, FLT_MAX, 0.0f, line.c_str());

		if (center)
		{
			window->DrawList->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) + 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			window->DrawList->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) - 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			window->DrawList->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) + 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			window->DrawList->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) - 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());

			window->DrawList->AddText(pFont, size, ImVec2(pos.x - textSize.x / 2.0f, pos.y + textSize.y * i), ImGui::GetColorU32(color), line.c_str());
		}
		else
		{
			window->DrawList->AddText(pFont, size, ImVec2((pos.x) + 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			window->DrawList->AddText(pFont, size, ImVec2((pos.x) - 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			window->DrawList->AddText(pFont, size, ImVec2((pos.x) + 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			window->DrawList->AddText(pFont, size, ImVec2((pos.x) - 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());

			window->DrawList->AddText(pFont, size, ImVec2(pos.x, pos.y + textSize.y * i), ImGui::GetColorU32(color), line.c_str());
		}

		y = pos.y + textSize.y * (i + 1);
		i++;
	}

	return y;
}

VOID DrawLine(Vector3 PositionToStart, Vector3 PositionToEnd, float Thickness, ImU32 Color)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddLine(ImVec2(PositionToStart.x, PositionToStart.y), ImVec2(PositionToEnd.x, PositionToEnd.y), Color, Thickness);
}

VOID DrawBone(Player* player, HumanBoneType startBone, HumanBoneType endBone, ImU32 color)
{
	auto startPos = player->GetHumanBonePosition(startBone);
	
	if (startPos.Zero())
		return;

	auto endPos = player->GetHumanBonePosition(endBone);

	if (endPos.Zero())
		return;

	DrawLine(startPos, endPos, 1.2f, color);
}

VOID DrawSkeleton(Player* player, ImU32 color)
{
	DrawBone(player, HumanBoneType::HEAD, HumanBoneType::NECK, color);
	DrawBone(player, HumanBoneType::NECK, HumanBoneType::SPINE, color);
	DrawBone(player, HumanBoneType::SPINE, HumanBoneType::PELVIS, color);

	DrawBone(player, HumanBoneType::NECK, HumanBoneType::L_HAND0, color);
	DrawBone(player, HumanBoneType::NECK, HumanBoneType::R_HAND0, color);

	DrawBone(player, HumanBoneType::R_HAND0, HumanBoneType::R_HAND1, color);
	DrawBone(player, HumanBoneType::L_HAND0, HumanBoneType::L_HAND1, color);

	DrawBone(player, HumanBoneType::R_HAND1, HumanBoneType::R_HAND2, color);
	DrawBone(player, HumanBoneType::L_HAND1, HumanBoneType::L_HAND2, color);

	DrawBone(player, HumanBoneType::PELVIS, HumanBoneType::R_KNEE, color);
	DrawBone(player, HumanBoneType::PELVIS, HumanBoneType::L_KNEE, color);

	DrawBone(player, HumanBoneType::R_KNEE, HumanBoneType::R_FOOT, color);
	DrawBone(player, HumanBoneType::L_KNEE, HumanBoneType::L_FOOT, color);
}

__forceinline VOID DrawFilledRect(float x, float y, float w, float h, ImU32 color)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddRectFilled(ImVec2(x, y - 1), ImVec2(x + w, y + h), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 255.0f / 255.0f)), 0, 0);
	window->DrawList->AddRectFilled(ImVec2(x, y + 1), ImVec2(x + w, y + h), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 255.0f / 255.0f)), 0, 0);
	window->DrawList->AddRectFilled(ImVec2(x - 1, y), ImVec2(x + w, y + h), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 255.0f / 255.0f)), 0, 0);
	window->DrawList->AddRectFilled(ImVec2(x + 1, y), ImVec2(x + w, y + h), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 255.0f / 255.0f)), 0, 0);
	window->DrawList->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), color, 0, 0);
}

__forceinline VOID DrawBorderBox(int x, int y, int x2, int y2, int thickness, ImU32 color)
{
	DrawFilledRect(x, y, x2, thickness, color);
	DrawFilledRect(x, y + y2, x2, thickness, color);
	DrawFilledRect(x, y, thickness, y2, color);
	DrawFilledRect(x + x2, y, thickness, y2 + thickness, color);
}

__forceinline VOID Draw2DBox(float x, float y, float width, float height, int thickness, ImU32 boxColor)
{
	DrawBorderBox(x, y, width, height, thickness, boxColor);
}

__forceinline VOID DrawHealthBar(Vector3 HeadPos, Vector3 FootPos, WORD Health, WORD MaxHealth)
{
	auto BarHeight = FootPos.y - HeadPos.y;
	auto BarWidth = (BarHeight / 2) * 1.24f;

	int BarX = (int)HeadPos.x - (BarWidth / 2);
	int BarY = (int)HeadPos.y;

	auto Percentage = Health * (BarHeight / MaxHealth);
	auto Deduction = BarHeight - Percentage;

	auto HealthColor = IM_COL32(0.0f, 255.0f, 0.0f, 255.0f);

	if (Health > 75)
		HealthColor = IM_COL32(0.0f, 255.0f, 0.0f, 255.0f);
	else if (Health > 40)
		HealthColor = IM_COL32(255.0f, 155.0f, 0.0f, 255.0f);
	else
		HealthColor = IM_COL32(255.0f, 0.0f, 0.0f, 255.0f);

	DrawBorderBox(BarX - 6, BarY, 3, (int)BarHeight, 1.5, IM_COL32(0.0f, 0.0f, 0.0f, 70.0f));
	DrawFilledRect(BarX - 6, BarY + Deduction, 3, Percentage, HealthColor);
}

__declspec(noinline) Player* GetLocalPlayer(LocalGameWorld* m_LocalGameWorld)
{
	List<Player>* online_users = m_LocalGameWorld->RegisteredPlayers();
	
	if (!MemoryUtils::IsUserAddress(online_users))
		return nullptr;

	auto playerList = online_users->listBase;
	int playerCount = online_users->itemCount;

	if (playerCount <= 0 || !playerList)
		return nullptr;

	for (int i = 0; i < playerCount; i++)
	{
		Player* player = playerList->entries[i];

		if (player->IsLocal())
			return player;
	}

	return nullptr;
}

typedef int (WINAPI* tMultiByteToWideChar)(UINT CodePage, DWORD dwFlags, LPCCH lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar);
tMultiByteToWideChar fnMultiByteToWideChar = nullptr;

typedef int (WINAPI* tWideCharToMultiByte)(UINT CodePage, DWORD dwFlags, LPCWCH lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCCH lpDefaultChar, LPBOOL lpUsedDefaultChar);
tWideCharToMultiByte fnWideCharToMultiByte = nullptr;

std::wstring ToUnicodes(std::string input)
{
	int strLen = (int)input.length() + 1;
	int size = fnMultiByteToWideChar(CP_ACP, 0, input.c_str(), strLen, 0, 0);

	if (size == 0)
		return L"";

	wchar_t* buffer = new wchar_t[size];

	int result = fnMultiByteToWideChar(CP_ACP, 0, input.c_str(), strLen, buffer, size);

	if (result == 0)
		return L"";

	std::wstring output(buffer);
	delete[] buffer;

	return output;
}

std::string ToANSI(const wchar_t* str)
{
	char buf[512];
	fnWideCharToMultiByte(CP_ACP, 0, str, -1, buf, sizeof(buf), NULL, NULL);
	return buf;
}

#include <array>

float screenWidth = 0;
float screenHeight = 0;

Vector3 CameraLocation;
Vector3 CameraRotation;
float CameraFov = 65.0f;

BOOL UnityCameraUpdate()
{
	if (EFTCore::m_Camera == nullptr)
		return FALSE;

	auto CameraTransform = EFTCore::m_Camera->GetComponentTransform();

	if (!MemoryUtils::IsAddressValid(CameraTransform) || !CameraTransform)
		return FALSE;

	CameraLocation = Vector3();

	CameraTransform->Position(&CameraLocation);

	if (CameraLocation.Zero())
		return FALSE;

	CameraRotation = Vector3();

	CameraTransform->Rotation(&CameraRotation);

	if (CameraRotation.Zero())
		return FALSE;

	CameraFov = EFTCore::m_Camera->GetFieldOfView();

	return TRUE;
}

double GetCrossDistance(double x1, double y1, double x2, double y2)
{
	return CRT::m_sqrtf(CRT::m_powf((float)(x1 - x2), (float)(2)) + CRT::m_powf((float)(y1 - y2), (float)2));
}

Player* GetClosestPlayerToCrossHair(Vector3 playerPos, float& maxDistance, Player* entity)
{
	if (entity)
	{
		float crossDistance = GetCrossDistance(playerPos.x, playerPos.y, (screenWidth / 2), (screenHeight / 2));

		if (crossDistance < maxDistance)
		{
			float Radius = (AimBotFOV * screenWidth / 2 / CameraFov) / 2;

			if (playerPos.x <= ((screenWidth / 2)+Radius) &&
				playerPos.x >= ((screenWidth / 2)-Radius) &&
				playerPos.y <= ((screenHeight / 2)+Radius) &&
				playerPos.y >= ((screenHeight / 2)-Radius))
			{
				maxDistance = crossDistance;

				return entity;
			}

			return nullptr;
		}
	}

	return nullptr;
}

Player* GetPlayerByFOV(Vector3 CameraPosition, Player* PlayerEntity, float& max)
{
	if (!PlayerEntity)
		return nullptr;

	if (CameraPosition.Zero())
		return nullptr;

	auto HeadPos = PlayerEntity->GetBonePosition(PlayerBoneType::Head);

	if (HeadPos.Zero())
		return nullptr;

	auto ClosestPlayer = GetClosestPlayerToCrossHair(HeadPos, max, PlayerEntity);

	if (ClosestPlayer)
		return ClosestPlayer;

	return nullptr;
}

static __forceinline auto GetAimBone() -> HumanBoneType
{
	return HumanBoneType::NECK_MODEL;
	/*auto Bone = HumanBoneType::NECK_MODEL;

	if (AimBone == 0)
		Bone = HumanBoneType::PELVIS;
	else if (AimBone == 1)
		Bone = HumanBoneType::NECK_MODEL;
	else if (AimBone == 2)
		Bone = HumanBoneType::NECK;

	return Bone;*/
}

Vector3 CalcAngles(const Vector3& src, const Vector3& dst)
{
	Vector3 angles = Vector3();

	Vector3 delta = src - dst;

	float magnitude = delta.Length();

	angles.x = -CRT::m_atan2f(delta.x, -delta.z) * 57.29578f;
	angles.y = CRT::m_asinf(delta.y / magnitude) * 57.29578;
	angles.z = 0.0f;

	return angles;
}

Vector3 inline SmoothAngles(Vector3 StartAngle, Vector3 EndAngle)
{
	Vector3 SmoothAngle = Vector3();

	SmoothAngle.x = (EndAngle.x - StartAngle.x) / 8 + StartAngle.x;
	SmoothAngle.y = (EndAngle.y - StartAngle.y) / 8 + StartAngle.y;

	return SmoothAngle;
}

VOID FaceEntity(Player* localPlayer, Player* targetPlayer)
{
	if (!localPlayer)
		return;

	if (!targetPlayer)
		return;

	auto localMovement = localPlayer->GetMovementContext();

	if (!MemoryUtils::IsAddressValid(localMovement) || !localMovement)
		return;

	Vector3 LocalViewAngles = Vector3();

	LocalViewAngles.x = localMovement->ViewAngles.x;
	LocalViewAngles.y = localMovement->ViewAngles.y;

	auto WeaponAnimation = localPlayer->GetProceduralWeaponAnimation();

	if (!MemoryUtils::IsAddressValid(WeaponAnimation) || !WeaponAnimation)
		return;

	auto HandsContainer = WeaponAnimation->GetHandsContainer();

	if (!MemoryUtils::IsAddressValid(HandsContainer) || !HandsContainer)
		return;

	auto FireportTransform = HandsContainer->Fireport();

	if (!MemoryUtils::IsAddressValid(FireportTransform) || !FireportTransform)
		return;

	auto FireportPos = Vector3();

	FireportTransform->Position(&FireportPos);

	if (FireportPos.Zero())
		return;

	auto HeadPos3D = targetPlayer->GetHumanBonePosition3D(GetAimBone());

	if (HeadPos3D.Zero())
		return;

	auto Angles = CalcAngles(FireportPos, HeadPos3D);

	if (Angles.Zero())
		return;

	if (AimBotSmooth > 0)
		Angles = SmoothAngles(FireportPos, Angles);

	localMovement->ViewAngles.x = Angles.x;
	localMovement->ViewAngles.y = Angles.y;
}

VOID Log(LPCSTR text, ...)
{
	CHAR logbuf[4096] = { 0 };

	va_list va_alist;
	va_start(va_alist, text);
	_vsnprintf(logbuf, sizeof(logbuf) - 1, text, va_alist);
	va_end(va_alist);

	LI_FN(OutputDebugStringA).forwarded_safe_cached()(logbuf);
}

Vector3 GetObjectLocation(uint64_t interactive)
{
	if (!MemoryUtils::IsAddressValid(interactive))
		return Vector3();

	const auto base_object = MemoryUtils::Read<uint64_t>(interactive + 0x10);
	
	if (!MemoryUtils::IsAddressValid(base_object))
		return Vector3();

	const auto game_object = MemoryUtils::Read<uint64_t>(base_object + 0x30);
	
	if (!MemoryUtils::IsAddressValid(game_object))
		return Vector3();

	const auto object_class = MemoryUtils::Read<uint64_t>(game_object + 0x30);
	
	if (!MemoryUtils::IsAddressValid(object_class))
		return Vector3();

	auto transform = MemoryUtils::Read<uint64_t>(object_class + 0x8);
	
	if (!MemoryUtils::IsAddressValid(transform))
		return Vector3();

	Transform* containerTransform = *reinterpret_cast<Transform**>(uintptr_t(transform) + 0x28);

	if (!MemoryUtils::IsAddressValid(containerTransform))
		return Vector3();

	Vector3 containerLocation = Vector3();
	containerTransform->Position(&containerLocation);

	return containerLocation;
}

uintptr_t GetLootArray(LocalGameWorld* m_LocalGameWorld)
{
	if (!MemoryUtils::IsAddressValid(m_LocalGameWorld) || !m_LocalGameWorld)
		return 0;

	uintptr_t registered_loot = MemoryUtils::Read<uintptr_t>((uintptr_t)m_LocalGameWorld + 0x60);

	if (!MemoryUtils::IsAddressValid(registered_loot) || !registered_loot)
		return 0;

	return MemoryUtils::Read<uintptr_t>(registered_loot + 0x10);
}

uint32_t GetLootCount(LocalGameWorld* m_LocalGameWorld)
{
	if (!MemoryUtils::IsAddressValid(m_LocalGameWorld) || !m_LocalGameWorld)
		return 0;

	uintptr_t registered_loot = MemoryUtils::Read<uintptr_t>((uintptr_t)m_LocalGameWorld + 0x60);

	if (!MemoryUtils::IsAddressValid(registered_loot) || !registered_loot)
		return 0;

	return MemoryUtils::Read<uint32_t>(registered_loot + 0x18);
}

uintptr_t GetLootObject(uintptr_t m_LootArray, uint32_t index)
{
	return MemoryUtils::Read<uintptr_t>(m_LootArray + (index * 0x8) + 0x20);
}

bool IsSecuredSlot(const char* slot, uintptr_t itemTemplate)
{
	if (CRT::StrStr(slot, xor ("SecuredContainer")))
		return true;

	bool unlootable = MemoryUtils::Read<bool>(itemTemplate + 0xF1);

	return unlootable;
}

bool IsValidSlot(const char* slot, uintptr_t itemTemplate)
{
	if (!CRT::StrStr(slot, xor ("Pockets")) && !CRT::StrStr(slot, xor ("TacticalVest"))
		&& !CRT::StrStr(slot, xor ("Backpack")))
	{
		return false;
	}

	if (CRT::StrStr(slot, xor ("Scabbard")))
	{
		bool unlootable = MemoryUtils::Read<bool>(itemTemplate + 0xF1);

		return !unlootable;
	}

	return true;
}

uintptr_t GetGridsInSlot(uintptr_t containedItem)
{
	if (!MemoryUtils::IsAddressValid(containedItem) || !containedItem)
		return 0;

	uintptr_t lootGrids = MemoryUtils::Read<uintptr_t>(containedItem + 0x68);

	if (!MemoryUtils::IsAddressValid(lootGrids) || !lootGrids)
		return 0;

	return lootGrids;
}

std::string GetUnityString(uintptr_t stringPtr)
{
	std::string unityString = "";

	if (!MemoryUtils::IsAddressValid(stringPtr) || !stringPtr)
		return unityString;

	int32_t stringLength = MemoryUtils::Read<int32_t>(stringPtr + 0x10);

	if (stringLength <= 0)
		return unityString;

	std::string stringBase = MemoryUtils::GetUnicodeString(stringPtr + 0x14, stringLength);

	if (stringBase.empty())
		return unityString;

	std::wstring baseUtf = ToUnicodes(stringBase);
	std::string baseAnsi = ToANSI(baseUtf.c_str());

	if (baseAnsi.empty())
		return unityString;

	unityString = baseAnsi;

	return unityString;
}

enum EExfiltrationStatus : uint8_t
{
	NotPresent = 1,
	UncompleteRequirements,
	Countdown,
	RegularMode,
	Pending,
	AwaitsManualActivation
};

inline bool IsExitOpened(uint64_t exitPoint)
{
	if (!MemoryUtils::IsAddressValid(exitPoint) || !exitPoint)
		return false;

	auto exitStatus = MemoryUtils::Read<uint8_t>(exitPoint + 0xA8);

	if (ExitsOpenedOnly)
	{
		if (exitStatus == EExfiltrationStatus::RegularMode)
			return true;
	}
	else
	{
		if (exitStatus == EExfiltrationStatus::UncompleteRequirements || exitStatus == EExfiltrationStatus::RegularMode || exitStatus == EExfiltrationStatus::AwaitsManualActivation)
			return true;
	}

	return false;
}

__forceinline VOID DrawExitPoints(LocalGameWorld* m_LocalGameWorld, Vector3 cameraPosition)
{
	if (!MemoryUtils::IsAddressValid(m_LocalGameWorld) || !m_LocalGameWorld)
		return;

	if (!MemoryUtils::IsAddressValid(EFTCore::m_Camera) || !EFTCore::m_Camera)
		return;

	if (cameraPosition.Zero())
		return;

	auto exitController = MemoryUtils::Read<uint64_t>(reinterpret_cast<uint64_t>(m_LocalGameWorld) + 0x18);

	if (!MemoryUtils::IsAddressValid(exitController) || !exitController)
		return;

	auto exitPoints = MemoryUtils::Read<uint64_t>(exitController + 0x20);

	if (!MemoryUtils::IsAddressValid(exitPoints) || !exitPoints)
		return;

	auto exitPointCount = MemoryUtils::Read<int32_t>(exitPoints + 0x18);

	if (exitPointCount > 0)
	{
		std::string exitName = "";
		Vector3 exitLocation = Vector3();

		for (int i = 0; i < exitPointCount; i++)
		{
			uintptr_t exitPoint = MemoryUtils::Read<uintptr_t>(exitPoints + (0x20 + (i * 0x8)));

			if (!MemoryUtils::IsAddressValid(exitPoint) || !exitPoint)
				continue;

			if (!IsExitOpened(exitPoint))
				continue;

			uintptr_t exitTriggerSettings = MemoryUtils::ReadMemory<uint64_t>(exitPoint + 0x58);

			if (!MemoryUtils::IsAddressValid(exitTriggerSettings) || !exitTriggerSettings)
				continue;

			auto exitPointLocation = GetObjectLocation(exitPoint);

			if (exitPointLocation.Zero())
				continue;

			uintptr_t exitPointString = MemoryUtils::Read<uintptr_t>(exitTriggerSettings + 0x10);
			std::string exitPointName = GetUnityString(exitPointString);

			if (exitPointName.empty())
				continue;

			exitName = exitPointName;
			exitLocation = exitPointLocation;

			Vector3 extractPosition = EFTCore::m_Camera->WorldToScreen(exitLocation);

			if (extractPosition.Zero())
				continue;

			auto extractDistance = GetItemDistance(cameraPosition, exitLocation);

			std::string exitInfo = exitName + std::string(xor (" | ")) + std::to_string((int)extractDistance) + std::string(xor ("m"));

			DrawOutlinedText(lootFont, exitInfo, ImVec2(extractPosition.x, extractPosition.y), 12.0f, IM_COL32(92.0f, 255.0f, 120.0f, 255.0f), true);
		}
	}
}

__forceinline VOID DrawLootCorpses(LocalGameWorld* m_LocalGameWorld, Vector3 cameraPosition)
{
	if (!MemoryUtils::IsAddressValid(m_LocalGameWorld) || !m_LocalGameWorld)
		return;

	if (!MemoryUtils::IsAddressValid(EFTCore::m_Camera) || !EFTCore::m_Camera)
		return;

	if (cameraPosition.Zero())
		return;

	auto lootCount = GetLootCount(m_LocalGameWorld);

	if (lootCount <= 0)
		return;

	auto lootArray = GetLootArray(m_LocalGameWorld);

	if (!MemoryUtils::IsAddressValid(lootArray) || !lootArray)
		return;

	for (auto i = 0; i < lootCount; i++)
	{
		auto lootObject = GetLootObject(lootArray, i);

		if (!MemoryUtils::IsAddressValid(lootObject) || !lootObject)
			continue;

		uintptr_t lootProfile = MemoryUtils::Read<uintptr_t>(lootObject + 0x10);

		if (!MemoryUtils::IsAddressValid(lootProfile) || !lootProfile)
			continue;

		uintptr_t lootInteractive = MemoryUtils::Read<uintptr_t>(lootProfile + 0x28);

		if (!MemoryUtils::IsAddressValid(lootInteractive) || !lootInteractive)
			continue;
		{
			uintptr_t lootInteractItem = MemoryUtils::Read<uintptr_t>(lootInteractive + 0x50);

			if (!MemoryUtils::IsAddressValid(lootInteractItem) || !lootInteractItem)
				continue;

			uintptr_t lootItemTemplate = MemoryUtils::Read<uintptr_t>(lootInteractItem + 0x40);

			if (!MemoryUtils::IsAddressValid(lootItemTemplate) || !lootItemTemplate)
				continue;

			auto containerPosition = GetObjectLocation(lootObject);

			if (containerPosition.Zero())
				continue;

			auto containterDistance = GetItemDistance(cameraPosition, containerPosition);

			if (containterDistance > LootCorpsDistance)
				continue;

			uintptr_t templateName = MemoryUtils::Read<uintptr_t>(lootItemTemplate + 0x50);
			std::string templateHash = GetUnityString(templateName);

			if (templateHash.empty())
				continue;

			auto lootHash = HASH(templateHash.c_str());
			auto lootName = GetItemName(lootHash);

			if (lootName == NULL)
				continue;

			const auto corpseHash = HASH(xor (L"55d7217a4bdc2d86028b456d"));

			if (lootHash == corpseHash)
			{
				uintptr_t lootItemOwner = MemoryUtils::Read<uintptr_t>(lootInteractive + 0x48);

				if (!MemoryUtils::IsAddressValid(lootItemOwner) || !lootItemOwner)
					continue;

				uintptr_t lootItemRoot = MemoryUtils::Read<uintptr_t>(lootItemOwner + 0xA0);

				if (!MemoryUtils::IsAddressValid(lootItemRoot) || !lootItemRoot)
					continue;

				uintptr_t lootSlots = MemoryUtils::Read<uintptr_t>(lootItemRoot + 0x70);

				if (!MemoryUtils::IsAddressValid(lootSlots) || !lootSlots)
					continue;

				uintptr_t lootSlotArray = MemoryUtils::Read<uintptr_t>(lootSlots + 0x20);

				if (!MemoryUtils::IsAddressValid(lootSlotArray) || !lootSlotArray)
					continue;

				int32_t slotCount = MemoryUtils::Read<int32_t>(lootSlots + 0x18);

				if (slotCount > 0)
				{
					Vector3 interactPosition = Vector3();
					ULONG interactPrice = 0;

					Transform* corpsePelvisTransform = *reinterpret_cast<Transform**>(lootInteractive + 0x110);

					if (!MemoryUtils::IsAddressValid(corpsePelvisTransform) || !corpsePelvisTransform)
						continue;

					corpsePelvisTransform->Position(&interactPosition);

					for (int j = 0; j < slotCount; j++)
					{
						uintptr_t lootSlot = MemoryUtils::Read<uintptr_t>(lootSlots + (0x20 + (j * 0x8)));

						if (!MemoryUtils::IsAddressValid(lootSlot) || !lootSlot)
							continue;

						uintptr_t slotContainedItem = MemoryUtils::Read<uintptr_t>(lootSlot + 0x38);

						if (!MemoryUtils::IsAddressValid(slotContainedItem) || !slotContainedItem)
							continue;

						uintptr_t containedItemTemplate = MemoryUtils::Read<uintptr_t>(slotContainedItem + 0x40);

						if (!MemoryUtils::IsAddressValid(containedItemTemplate) || !containedItemTemplate)
							continue;

						uintptr_t lootSlotId = MemoryUtils::Read<uintptr_t>(lootSlot + 0x10);

						if (!MemoryUtils::IsAddressValid(lootSlotId) || !lootSlotId)
							continue;

						std::string slotName = GetUnityString(lootSlotId);

						if (slotName.empty())
							continue;

						if (IsValidSlot(slotName.c_str(), containedItemTemplate))
						{
							auto slotGrids = GetGridsInSlot(slotContainedItem);

							if (!MemoryUtils::IsAddressValid(slotGrids) || !slotGrids)
								continue;

							uintptr_t slotGridArray = MemoryUtils::Read<uintptr_t>(slotGrids + 0x20);

							if (!MemoryUtils::IsAddressValid(slotGridArray) || !slotGridArray)
								continue;

							int32_t slotGridCount = MemoryUtils::Read<int32_t>(slotGrids + 0x18);

							if (slotGridCount > 0)
							{
								for (int k = 0; k < slotGridCount; k++)
								{
									uintptr_t slotGrid = MemoryUtils::Read<uintptr_t>(slotGrids + (0x20 + (k * 0x8)));

									if (!MemoryUtils::IsAddressValid(slotGrid) || !slotGrid)
										continue;

									uintptr_t gridEnumerable = MemoryUtils::Read<uintptr_t>(slotGrid + 0x40);

									if (!MemoryUtils::IsAddressValid(gridEnumerable) || !gridEnumerable)
										continue;

									uintptr_t gridItemList = MemoryUtils::Read<uintptr_t>(gridEnumerable + 0x18);

									if (!MemoryUtils::IsAddressValid(gridItemList) || !gridItemList)
										continue;

									uintptr_t gridItemArray = MemoryUtils::Read<uintptr_t>(gridItemList + 0x10);

									if (!MemoryUtils::IsAddressValid(gridItemArray) || !gridItemArray)
										continue;

									int32_t gridItemCount = MemoryUtils::Read<int32_t>(gridItemList + 0x18);

									for (auto l = 0; l < gridItemCount; l++)
									{
										auto gridItemObject = GetLootObject(gridItemArray, l);

										if (MemoryUtils::IsAddressValid(gridItemObject) && gridItemObject)
										{
											uintptr_t gridItemTemplate = MemoryUtils::Read<uintptr_t>(gridItemObject + 0x40);

											if (!MemoryUtils::IsAddressValid(gridItemTemplate) || !gridItemTemplate)
												continue;

											uintptr_t gridTemplateId = MemoryUtils::Read<uintptr_t>(gridItemTemplate + 0x50);
											std::string gridItemHash = GetUnityString(gridTemplateId);

											if (gridItemHash.empty())
												continue;

											auto gridHash = HASH(gridItemHash.c_str());

											interactPrice += GetItemPrice(gridHash);
										}
									}
								}
							}
						}

						if (IsSecuredSlot(slotName.c_str(), containedItemTemplate))
							continue;

						uintptr_t slotTemplateName = MemoryUtils::Read<uintptr_t>(containedItemTemplate + 0x50);
						std::string slotTemplateHash = GetUnityString(slotTemplateName);

						if (slotTemplateHash.empty())
							continue;

						auto slotLootHash = HASH(slotTemplateHash.c_str());

						interactPrice += GetItemPrice(slotLootHash);
					}

					auto price = interactPrice >= 10000 ? (interactPrice - 2300) : interactPrice;

					if (price >= MinCorpsPrice && !interactPosition.Zero())
					{
						Vector3 containerPosition2D = EFTCore::m_Camera->WorldToScreen(interactPosition);

						if (containerPosition2D.Zero())
							continue;

						auto corpseDistance = GetItemDistance(cameraPosition, interactPosition);

						if (corpseDistance > LootCorpsDistance)
							continue;

						std::string formatted = std::string(to_string(price));

						std::string corpseName = xor ("Corpse");

						std::string containerName = corpseName + std::string(xor (" | "));
						std::string containerDelim = std::string(xor (" RUB | ") + std::to_string((int)corpseDistance) + std::string(xor ("m")));

						std::string lootInfo = std::string(containerName) + formatted + containerDelim;

						auto displayInfo = lootInfo;

						int middle[3] = { containerPosition2D.y, containerPosition2D.y, containerPosition2D.y };

						int fontsize = 12;
						float offset = (displayInfo.size() * fontsize) / 5;

						DrawOutlinedText(lootFont, displayInfo, ImVec2(containerPosition2D.x - offset, middle[1]), 12.0f, IM_COL32(255, 120, 92, 255), false);

						middle[1] += fontsize;
					}
				}
			}
		}
	}
}

__forceinline VOID DrawLootItems(LocalGameWorld* m_LocalGameWorld, Vector3 cameraPosition)
{
	if (!MemoryUtils::IsAddressValid(m_LocalGameWorld) || !m_LocalGameWorld)
		return;

	if (!MemoryUtils::IsAddressValid(EFTCore::m_Camera) || !EFTCore::m_Camera)
		return;

	if (cameraPosition.Zero())
		return;

	auto lootCount = GetLootCount(m_LocalGameWorld);

	if (lootCount <= 0)
		return;

	auto lootArray = GetLootArray(m_LocalGameWorld);

	if (!MemoryUtils::IsAddressValid(lootArray) || !lootArray)
		return;

	for (auto i = 0; i < lootCount; i++)
	{
		auto lootObject = GetLootObject(lootArray, i);

		if (!MemoryUtils::IsAddressValid(lootObject) || !lootObject)
			continue;

		uintptr_t lootProfile = MemoryUtils::Read<uintptr_t>(lootObject + 0x10);

		if (!MemoryUtils::IsAddressValid(lootProfile) || !lootProfile)
			continue;

		uintptr_t lootInteractive = MemoryUtils::Read<uintptr_t>(lootProfile + 0x28);

		if (!MemoryUtils::IsAddressValid(lootInteractive) || !lootInteractive)
			continue;
		{
			uintptr_t lootInteractItem = MemoryUtils::Read<uintptr_t>(lootInteractive + 0x50);

			if (!MemoryUtils::IsAddressValid(lootInteractItem) || !lootInteractItem)
				continue;

			uintptr_t lootItemTemplate = MemoryUtils::Read<uintptr_t>(lootInteractItem + 0x40);

			if (!MemoryUtils::IsAddressValid(lootItemTemplate) || !lootItemTemplate)
				continue;

			auto containerPosition = GetObjectLocation(lootObject);

			if (containerPosition.Zero())
				continue;

			auto containterDistance = GetItemDistance(cameraPosition, containerPosition);

			if (containterDistance > LootDistance)
				continue;

			uintptr_t templateName = MemoryUtils::Read<uintptr_t>(lootItemTemplate + 0x50);
			std::string templateHash = GetUnityString(templateName);

			if (templateHash.empty())
				continue;

			auto lootHash = HASH(templateHash.c_str());
			auto lootName = GetItemName(lootHash);

			if (lootName == NULL)
				continue;

			std::string lootGroundName = "";
			lootGroundName = std::string(lootName);

			ULONG interactPrice = 0;

			interactPrice += GetItemPrice(lootHash);

			auto price = interactPrice >= 10000 ? (interactPrice - 2300) : interactPrice;

			if (price >= MinItemPrice && !containerPosition.Zero() && !lootGroundName.empty())
			{
				Vector3 containerPosition2D = EFTCore::m_Camera->WorldToScreen(containerPosition);

				if (containerPosition2D.Zero())
					continue;

				std::string formatted = std::string(to_string(price));

				std::string containerName = lootGroundName + std::string(xor (" | "));
				std::string containerDelim = std::string(xor (" RUB | ") + std::to_string((int)containterDistance) + std::string(xor ("m")));

				std::string lootInfo = std::string(containerName) + formatted + containerDelim;

				auto displayInfo = lootInfo;

				int middle[3] = { containerPosition2D.y, containerPosition2D.y, containerPosition2D.y };

				int fontsize = 12;
				float offset = (displayInfo.size() * fontsize) / 5;

				DrawOutlinedText(lootFont, displayInfo, ImVec2(containerPosition2D.x - offset, middle[1]), 12.0f, IM_COL32(239, 239, 239, 239), false);

				middle[1] += fontsize;
			}
		}
	}
}

__forceinline VOID DrawLootContainers(LocalGameWorld* m_LocalGameWorld, Vector3 cameraPosition)
{
	if (!MemoryUtils::IsAddressValid(m_LocalGameWorld) || !m_LocalGameWorld)
		return;

	if (!MemoryUtils::IsAddressValid(EFTCore::m_Camera) || !EFTCore::m_Camera)
		return;

	if (cameraPosition.Zero())
		return;

	auto lootCount = GetLootCount(m_LocalGameWorld);

	if (lootCount <= 0)
		return;

	auto lootArray = GetLootArray(m_LocalGameWorld);

	if (!MemoryUtils::IsAddressValid(lootArray) || !lootArray)
		return;

	for (auto i = 0; i < lootCount; i++)
	{
		auto lootObject = GetLootObject(lootArray, i);

		if (!MemoryUtils::IsAddressValid(lootObject) || !lootObject)
			continue;

		uintptr_t lootProfile = MemoryUtils::Read<uintptr_t>(lootObject + 0x10);

		if (!MemoryUtils::IsAddressValid(lootProfile) || !lootProfile)
			continue;

		uintptr_t lootInteractive = MemoryUtils::Read<uintptr_t>(lootProfile + 0x28);

		if (!MemoryUtils::IsAddressValid(lootInteractive) || !lootInteractive)
			continue;

		uintptr_t lootInteractItem = MemoryUtils::Read<uintptr_t>(lootInteractive + 0x50);

		if (lootInteractItem != 0)
			continue;

		uintptr_t lootItemOwner = MemoryUtils::Read<uintptr_t>(lootInteractive + 0x108);

		if (!MemoryUtils::IsAddressValid(lootItemOwner) || !lootItemOwner)
			continue;

		uintptr_t lootContainer = MemoryUtils::Read<uintptr_t>(lootItemOwner + 0xA0);

		if (!MemoryUtils::IsAddressValid(lootContainer) || !lootContainer)
			continue;

		uintptr_t lootGrids = MemoryUtils::Read<uintptr_t>(lootContainer + 0x68);

		if (!MemoryUtils::IsAddressValid(lootGrids) || !lootGrids)
			continue;

		auto containerPosition = GetObjectLocation(lootObject);

		if (containerPosition.Zero())
			continue;

		auto containterDistance = GetItemDistance(cameraPosition, containerPosition);

		if (containterDistance > ContainerDistance)
			continue;

		uintptr_t lootGrid = MemoryUtils::Read<uintptr_t>(lootGrids + 0x20);

		if (!MemoryUtils::IsAddressValid(lootGrid) || !lootGrid)
			continue;

		uintptr_t lootDict = MemoryUtils::Read<uintptr_t>(lootGrid + 0x40);

		if (!MemoryUtils::IsAddressValid(lootDict) || !lootDict)
			continue;

		uintptr_t lootDictionary = MemoryUtils::Read<uintptr_t>(lootDict + 0x10);

		if (!MemoryUtils::IsAddressValid(lootDictionary) || !lootDictionary)
			continue;

		uintptr_t lootEntries = MemoryUtils::Read<uintptr_t>(lootDictionary + 0x18);

		if (!MemoryUtils::IsAddressValid(lootEntries) || !lootEntries)
			continue;

		int lootEntryCount = MemoryUtils::Read<int32_t>(lootDictionary + 0x40);

		if (lootEntryCount > 0)
		{
			ULONG containerPrice = 0;

			for (int j = 0; j < lootEntryCount; j++)
			{
				uintptr_t lootItem = MemoryUtils::Read<uintptr_t>(lootEntries + (0x28 + (j * 0x18)));

				if (!MemoryUtils::IsAddressValid(lootItem) || !lootItem)
					continue;

				uintptr_t lootItemTemplate = MemoryUtils::Read<uintptr_t>(lootItem + 0x40);

				if (!MemoryUtils::IsAddressValid(lootItemTemplate) || !lootItemTemplate)
					continue;

				uintptr_t templateName = MemoryUtils::Read<uintptr_t>(lootItemTemplate + 0x50);
				std::string templateHash = GetUnityString(templateName);

				if (templateHash.empty())
					continue;

				auto lootHash = HASH(templateHash.c_str());

				containerPrice += GetItemPrice(lootHash);
			}

			auto price = containerPrice >= 10000 ? (containerPrice - 2300) : containerPrice;

			if (price >= MinContPrice && !containerPosition.Zero())
			{
				Vector3 containerPosition2D = EFTCore::m_Camera->WorldToScreen(containerPosition);

				if (containerPosition2D.Zero())
					continue;

				std::string formatted = std::string(to_string(price));

				std::string containerName = std::string(xor ("Container | "));
				std::string containerDelim = std::string(xor (" RUB | ") + std::to_string((int)containterDistance) + std::string(xor ("m")));

				std::string lootInfo = std::string(containerName) + formatted + containerDelim;

				auto displayInfo = lootInfo;

				int middle[3] = { containerPosition2D.y, containerPosition2D.y, containerPosition2D.y };

				int fontsize = 12;
				float offset = (displayInfo.size() * fontsize) / 5;

				DrawOutlinedText(lootFont, displayInfo, ImVec2(containerPosition2D.x - offset, middle[1]), 12.0f, IM_COL32(239, 239, 239, 255), false);
				
				middle[1] += fontsize;
			}
		}
	}
}

typedef void(__fastcall* tSetTimeScale)(float timeScale);
tSetTimeScale SetTimeScale = nullptr;

void SetGameSpeed(float timeScale)
{
	uintptr_t procedure = Unity::UnityPlayer + 0x990F80;

	if (!MemoryUtils::IsUserAddress(procedure))
		return;

	SetTimeScale = (tSetTimeScale)procedure;

	SetTimeScale(timeScale);
}

uint32_t GetInventoryPrice(uintptr_t InventoryController)
{
	if (!MemoryUtils::IsAddressValid(InventoryController) || !InventoryController)
		return 0;

	uintptr_t lootItemRoot = MemoryUtils::Read<uintptr_t>(InventoryController + 0xA0);

	if (!MemoryUtils::IsAddressValid(lootItemRoot) || !lootItemRoot)
		return 0;

	uintptr_t lootSlots = MemoryUtils::Read<uintptr_t>(lootItemRoot + 0x70);

	if (!MemoryUtils::IsAddressValid(lootSlots) || !lootSlots)
		return 0;

	uintptr_t lootSlotArray = MemoryUtils::Read<uintptr_t>(lootSlots + 0x20);

	if (!MemoryUtils::IsAddressValid(lootSlotArray) || !lootSlotArray)
		return 0;

	int32_t slotCount = MemoryUtils::Read<int32_t>(lootSlots + 0x18);

	if (slotCount > 0)
	{
		ULONG playerPrice = 0;

		for (int j = 0; j < slotCount; j++)
		{
			uintptr_t lootSlot = MemoryUtils::Read<uintptr_t>(lootSlots + (0x20 + (j * 0x8)));

			if (!MemoryUtils::IsAddressValid(lootSlot) || !lootSlot)
				continue;

			uintptr_t slotContainedItem = MemoryUtils::Read<uintptr_t>(lootSlot + 0x38);

			if (!MemoryUtils::IsAddressValid(slotContainedItem) || !slotContainedItem)
				continue;

			uintptr_t containedItemTemplate = MemoryUtils::Read<uintptr_t>(slotContainedItem + 0x40);

			if (!MemoryUtils::IsAddressValid(containedItemTemplate) || !containedItemTemplate)
				continue;

			uintptr_t lootSlotId = MemoryUtils::Read<uintptr_t>(lootSlot + 0x10);

			if (!MemoryUtils::IsAddressValid(lootSlotId) || !lootSlotId)
				continue;

			std::string slotName = GetUnityString(lootSlotId);

			if (slotName.empty())
				continue;

			if (IsValidSlot(slotName.c_str(), containedItemTemplate))
			{
				auto slotGrids = GetGridsInSlot(slotContainedItem);

				if (!MemoryUtils::IsAddressValid(slotGrids) || !slotGrids)
					continue;

				uintptr_t slotGridArray = MemoryUtils::Read<uintptr_t>(slotGrids + 0x20);

				if (!MemoryUtils::IsAddressValid(slotGridArray) || !slotGridArray)
					continue;

				int32_t slotGridCount = MemoryUtils::Read<int32_t>(slotGrids + 0x18);

				if (slotGridCount > 0)
				{
					for (int k = 0; k < slotGridCount; k++)
					{
						uintptr_t slotGrid = MemoryUtils::Read<uintptr_t>(slotGrids + (0x20 + (k * 0x8)));

						if (!MemoryUtils::IsAddressValid(slotGrid) || !slotGrid)
							continue;

						uintptr_t gridEnumerable = MemoryUtils::Read<uintptr_t>(slotGrid + 0x40);

						if (!MemoryUtils::IsAddressValid(gridEnumerable) || !gridEnumerable)
							continue;

						uintptr_t gridItemList = MemoryUtils::Read<uintptr_t>(gridEnumerable + 0x18);

						if (!MemoryUtils::IsAddressValid(gridItemList) || !gridItemList)
							continue;

						uintptr_t gridItemArray = MemoryUtils::Read<uintptr_t>(gridItemList + 0x10);

						if (!MemoryUtils::IsAddressValid(gridItemArray) || !gridItemArray)
							continue;

						int32_t gridItemCount = MemoryUtils::Read<int32_t>(gridItemList + 0x18);

						for (auto l = 0; l < gridItemCount; l++)
						{
							auto gridItemObject = GetLootObject(gridItemArray, l);

							if (MemoryUtils::IsAddressValid(gridItemObject) && gridItemObject)
							{
								uintptr_t gridItemTemplate = MemoryUtils::Read<uintptr_t>(gridItemObject + 0x40);

								if (!MemoryUtils::IsAddressValid(gridItemTemplate) || !gridItemTemplate)
									continue;

								uintptr_t gridTemplateId = MemoryUtils::Read<uintptr_t>(gridItemTemplate + 0x50);
								std::string gridItemHash = GetUnityString(gridTemplateId);

								if (gridItemHash.empty())
									continue;

								auto gridHash = HASH(gridItemHash.c_str());

								playerPrice += GetItemPrice(gridHash);
							}
						}
					}
				}
			}

			if (IsSecuredSlot(slotName.c_str(), containedItemTemplate))
				continue;

			uintptr_t slotTemplateName = MemoryUtils::Read<uintptr_t>(containedItemTemplate + 0x50);
			std::string slotTemplateHash = GetUnityString(slotTemplateName);

			if (slotTemplateHash.empty())
				continue;

			auto slotLootHash = HASH(slotTemplateHash.c_str());

			playerPrice += GetItemPrice(slotLootHash);
		}

		auto price = playerPrice >= 10000 ? (playerPrice - 2300) : playerPrice;

		if (price > 100)
			return price;
	}

	return 0;
}

void SetStyles()
{
	ImGui::GetStyle().WindowPadding = { 0,0 };
	ImGui::GetStyle().WindowMinSize = { 1,1 };
	ImGui::GetStyle().WindowBorderSize = 0;
	ImGui::GetStyle().PopupBorderSize = 1;
	ImGui::GetStyle().PopupRounding = 4;
	ImGui::GetStyle().WindowRounding = 4;
	ImGui::GetStyle().GrabMinSize = 1;
	ImGui::GetStyle().ItemSpacing = { 5, 10 };
	ImGui::GetStyle().ColorButtonPosition = ImGuiDir_Right;

	ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImColor{ 34, 44, 54 };
	ImGui::GetStyle().Colors[ImGuiCol_PopupBg] = ImColor{ 17, 17, 17 };
	ImGui::GetStyle().Colors[ImGuiCol_Border] = ImColor{ 33, 33, 33 };
	ImGui::GetStyle().Colors[ImGuiCol_Text] = ImColor{ 255, 255, 255 };
	ImGui::GetStyle().Colors[ImGuiCol_ScrollbarBg] = ImColor{ 119, 119, 119, 0 };

	ImGui::GetStyle().Colors[ImGuiCol_Header] = ImColor{ 35, 42, 51 };
	ImGui::GetStyle().Colors[ImGuiCol_HeaderActive] = ImColor{ 35, 42, 51 };
	ImGui::GetStyle().Colors[ImGuiCol_HeaderHovered] = ImColor{ 35, 42, 51 };

	static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	static const ImWchar ranges[] = {
		0x0020, 0x00FF,0x2000, 0x206F,0x3000, 0x30FF,0x31F0, 0x31FF, 0xFF00,
		0xFFEF,0x4e00, 0x9FAF,0x0400, 0x052F,0x2DE0, 0x2DFF,0xA640, 0xA69F, 0
	};

	ImFontConfig icons_config;
	icons_config.MergeMode = true;
	icons_config.PixelSnapH = true;

	ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(base85_compressed_data, base85_compressed_size, 14, NULL, ranges);
	ImGui::GetIO().Fonts->AddFontFromMemoryTTF(faprolight, sizeof faprolight, 18, &icons_config, icon_ranges);

	TylerUI::Fonts::OriginalBig = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(base85_compressed_data, base85_compressed_size, 18, NULL, ranges);
	TylerUI::Fonts::OriginalMedium = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(base85_compressed_data, base85_compressed_size, 15, NULL, ranges);
	TylerUI::Fonts::Logotype = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(base85_compressed_data, base85_compressed_size, 24, NULL, ranges);
}

void RenderMenu()
{
	auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;

	static auto selected_tab = 0;

	ImGui::Begin("Interface", nullptr, flags);
	{
		auto draw = ImGui::GetWindowDrawList();
		auto pos = ImGui::GetWindowPos();

		ImGui::SetWindowSize({ 550, 350 });

		draw->AddLine(pos + ImVec2(0, 40), pos + ImVec2(550, 40), ImColor(43, 53, 63), 1);
		draw->AddLine(pos + ImVec2(140, 40), pos + ImVec2(140, 350), ImColor(43, 53, 63), 1);
		draw->AddText(pos + ImVec2(275, 20) - ImGui::CalcTextSize("Oblivion") / 2, ImColor(180, 191, 197), "Oblivion");

		ImGui::SetCursorPos({ 10, 50 });
		ImGui::BeginGroup();
		{
			
			ImGui::Checkbox("Aimbot", &AimBotEnabled);

			static int slider = 0;
			ImGui::SliderInt("Slider", &slider, 0, 100, "%i");

			static int combo = 0;
			const char* combo_items[] = { "Default", "Not Default" };
			ImGui::Combo("Combo", &combo, combo_items, 2);

			ImGui::Button("Button", { 160, 30 });
		}
		ImGui::EndGroup();
	}
	ImGui::End();
}


VOID OnRender(ImGuiWindow* ImWindow)
{
	Unity::displayWidth = screenWidth;
	Unity::displayHeight = screenHeight;

	EFTCore::m_Camera = EFTCore::GetCurrentCamera();

	if (!MemoryUtils::IsAddressValid(EFTCore::m_Camera) || !EFTCore::m_Camera)
		return;

	EFTCore::m_GameObjectManager = EFTCore::GetObjectManager();

	if (!MemoryUtils::IsAddressValid(EFTCore::m_GameObjectManager) || !EFTCore::m_GameObjectManager)
		return;

	auto active_objects = MemoryUtils::ReadMemory<std::array<uint64_t, 2>>(reinterpret_cast<uint64_t>(EFTCore::m_GameObjectManager) + offsetof(GameObjectManager, lastActiveObject));

	if (!active_objects[0] || !active_objects[1])
		return;

	if (!MemoryUtils::IsAddressValid(active_objects[0]) || !MemoryUtils::IsAddressValid(active_objects[1]))
		return;

	auto m_GameWorld = EFTCore::GetObjectFromList(active_objects[1], active_objects[0], xor ("GameWorld"));

	if (!MemoryUtils::IsAddressValid(m_GameWorld) || !m_GameWorld)
		return;

	uintptr_t ptr = MemoryUtils::Read<uintptr_t>((uintptr_t)m_GameWorld + 0x30);

	if (!MemoryUtils::IsAddressValid(ptr) || !ptr)
		return;

	ptr = MemoryUtils::Read<uintptr_t>(ptr + 0x18);
	
	if (!MemoryUtils::IsAddressValid(ptr) || !ptr)
		return;

	LocalGameWorld* m_LocalGameWorld = MemoryUtils::Read<LocalGameWorld*>(ptr + 0x28);
	
	if (!MemoryUtils::IsAddressValid(m_LocalGameWorld) || !m_LocalGameWorld)
		return;

	auto tagged_objects = MemoryUtils::ReadMemory<std::array<uint64_t, 2>>(reinterpret_cast<uint64_t>(EFTCore::m_GameObjectManager) + offsetof(GameObjectManager, lastTaggedObject));

	if (!tagged_objects[0] || !tagged_objects[1])
		return;

	if (!MemoryUtils::IsAddressValid(tagged_objects[0]) || !MemoryUtils::IsAddressValid(tagged_objects[1]))
		return;
	
	auto m_FPSCameras = EFTCore::GetObjectFromList(tagged_objects[1], tagged_objects[0], xor ("FPS Camera"));

	if (!MemoryUtils::IsAddressValid(m_FPSCameras) || !m_FPSCameras)
		return;
	
	uint64_t m_FPSCamera = m_FPSCameras;

	uintptr_t fps_ptr = MemoryUtils::Read<uintptr_t>(m_FPSCamera + 0x30);

	if (!MemoryUtils::IsAddressValid(fps_ptr) || !fps_ptr)
		return;

	uintptr_t fps_ptr2 = MemoryUtils::Read<uintptr_t>((uintptr_t)fps_ptr + 0x18);

	if (!MemoryUtils::IsAddressValid(fps_ptr2) || !fps_ptr2)
		return;

	EFTCore::m_FPSCamera = fps_ptr2;

	auto LocalPlayer = GetLocalPlayer(m_LocalGameWorld);
	
	if (!LocalPlayer)
		return;

	auto WeaponAnimation = LocalPlayer->GetProceduralWeaponAnimation();

	if (WeaponAnimation && MemoryUtils::IsAddressValid(WeaponAnimation))
	{
		if (NoRecoil || NoSpread)
		{
			MemoryUtils::Write<Vector3>(reinterpret_cast<uint64_t>(WeaponAnimation) + 0x20C, Vector3(0.0f, 0.0f, 0.0f));
			MemoryUtils::Write<Vector3>(reinterpret_cast<uint64_t>(WeaponAnimation) + 0x218, Vector3(0.0f, 0.0f, 0.0f));

			MemoryUtils::Write<float>(reinterpret_cast<uint64_t>(WeaponAnimation) + 0x228, 0.0f);
			MemoryUtils::Write<float>(reinterpret_cast<uint64_t>(WeaponAnimation) + 0x22C, 0.0f);

			auto ShotEffect = MemoryUtils::Read<uint64_t>(reinterpret_cast<uint64_t>(WeaponAnimation) + 0x48);

			if (ShotEffect && MemoryUtils::IsAddressValid(ShotEffect))
			{
				MemoryUtils::Write<Vector2>(ShotEffect + 0x40, Vector2(0.0f, 0.0f));
				MemoryUtils::Write<Vector2>(ShotEffect + 0x48, Vector2(0.0f, 0.0f));
				MemoryUtils::Write<float>(ShotEffect + 0x70, 0.0f);
			}
		}

		if (NoSway || NoBreath)
		{
			auto BreathEffect = MemoryUtils::Read<uint64_t>(reinterpret_cast<uint64_t>(WeaponAnimation) + 0x28);

			if (BreathEffect && MemoryUtils::IsAddressValid(BreathEffect))
			{
				MemoryUtils::Write<float>(BreathEffect + 0xA4, 0.0f);
			}

			if (NoSway)
			{
				MemoryUtils::Write<uint32_t>(reinterpret_cast<uint64_t>(WeaponAnimation) + 0x100, 1);

				auto WalkEffect = MemoryUtils::Read<uint64_t>(reinterpret_cast<uint64_t>(WeaponAnimation) + 0x30);

				if (WalkEffect && MemoryUtils::IsAddressValid(WalkEffect))
				{
					MemoryUtils::Write<float>(WalkEffect + 0x44, 0.0f);
				}

				auto MotionEffect = MemoryUtils::Read<uint64_t>(reinterpret_cast<uint64_t>(WeaponAnimation) + 0x38);

				if (MotionEffect && MemoryUtils::IsAddressValid(MotionEffect))
				{
					MemoryUtils::Write<float>(MotionEffect + 0xD0, 0.0f);
				}

				auto ForceEffect = MemoryUtils::Read<uint64_t>(reinterpret_cast<uint64_t>(WeaponAnimation) + 0x40);

				if (ForceEffect && MemoryUtils::IsAddressValid(ForceEffect))
				{
					MemoryUtils::Write<float>(ForceEffect + 0x30, 0.0f);
				}
			}
		}
	}


	List<Player>* onlineUsers = m_LocalGameWorld->RegisteredPlayers();
	
	if (!MemoryUtils::IsAddressValid(onlineUsers) || !onlineUsers)
		return;

	auto playerList = onlineUsers->listBase;
	int playerCount = onlineUsers->itemCount;

	if (playerCount <= 0 || !playerList)
		return;

	auto CameraPosition = LocalPlayer->GetCameraPosition();

	if (CameraPosition.Zero())
		return;

	if (DrawFOV)
	{
		float fovRadius = (AimBotFOV * screenWidth / 2 / CameraFov) / 2;
		ImWindow->DrawList->AddCircle(ImVec2(screenWidth / 2, screenHeight / 2), fovRadius, IM_COL32(238, 238, 238, 255), 128);
	}

	/*if (!UnityCameraUpdate())
		return;*/

	if (LootESP && ShowContainers)
		DrawLootContainers(m_LocalGameWorld, CameraPosition);

	if (LootESP && ShowItems  )
		DrawLootItems(m_LocalGameWorld, CameraPosition);

	if (LootESP && ShowCorpses)
		DrawLootCorpses(m_LocalGameWorld, CameraPosition);

	if (DrawExits)
		DrawExitPoints(m_LocalGameWorld, CameraPosition);

	if (ThermalVision && !IsThermalSet && !NightVision)
	{
		const auto GameObject = EFTCore::GetGameObject(reinterpret_cast<uint64_t>(EFTCore::m_Camera));

		if (MemoryUtils::IsAddressValid(GameObject) && GameObject)
		{
			auto ThermalComponent = EFTCore::GetComponentFromObject(GameObject, xor ("ThermalVision"));

			if (MemoryUtils::IsAddressValid(ThermalComponent) && ThermalComponent)
			{
				MemoryUtils::Write<bool>(ThermalComponent + 0xD1, false);
				MemoryUtils::Write<bool>(ThermalComponent + 0xD2, false);
				MemoryUtils::Write<bool>(ThermalComponent + 0xD3, false);
				MemoryUtils::Write<bool>(ThermalComponent + 0xD4, false);
				MemoryUtils::Write<bool>(ThermalComponent + 0xD5, false);

				uintptr_t ThermalUtils = MemoryUtils::Read<uintptr_t>(ThermalComponent + 0x18);

				if (MemoryUtils::IsAddressValid(ThermalUtils) && ThermalUtils)
				{
					MemoryUtils::Write<float>(ThermalUtils + 0x34, 0.f);

					uintptr_t NoiseParams = MemoryUtils::Read<uintptr_t>(ThermalUtils + 0x20);

					if (MemoryUtils::IsAddressValid(NoiseParams) && NoiseParams)
					{
						uintptr_t TextureMask = MemoryUtils::Read<uintptr_t>(ThermalUtils + 0x10);

						if (MemoryUtils::IsAddressValid(TextureMask) && TextureMask)
						{
							auto MaskColor = UnityColor(0.f, 0.f, 0.f, 0.f);
							MemoryUtils::Write<UnityColor>(TextureMask + 0x38, MaskColor);

							MemoryUtils::Write<bool>(TextureMask + 0x48, false);
							MemoryUtils::Write<float>(TextureMask + 0x4C, 0.f);
						}

						MemoryUtils::Write<float>(NoiseParams + 0x18, 0.f);
					}

					uintptr_t ValuesCoefs = MemoryUtils::Read<uintptr_t>(ThermalUtils + 0x18);

					if (MemoryUtils::IsAddressValid(ValuesCoefs) && ValuesCoefs)
						MemoryUtils::Write<float>(ValuesCoefs + 0x18, 0.f);
				}

				MemoryUtils::Write<bool>(ThermalComponent + 0xD0, true);

				IsThermalSet = true;
			}
		}
	}

	if (!ThermalVision && IsThermalSet)
	{
		const auto GameObject = EFTCore::GetGameObject(reinterpret_cast<uint64_t>(EFTCore::m_Camera));

		if (MemoryUtils::IsAddressValid(GameObject) && GameObject)
		{
			auto ThermalComponent = EFTCore::GetComponentFromObject(GameObject, xor ("ThermalVision"));

			if (MemoryUtils::IsAddressValid(ThermalComponent) && ThermalComponent)
			{
				MemoryUtils::Write<bool>(ThermalComponent + 0xD0, false);

				IsThermalSet = false;
			}
		}
	}

	if (NightVision && !IsNightSet)
	{
		const auto GameObject = EFTCore::GetGameObject(reinterpret_cast<uint64_t>(EFTCore::m_Camera));

		if (MemoryUtils::IsAddressValid(GameObject) && GameObject)
		{
			auto NightComponent = EFTCore::GetComponentFromObject(GameObject, xor ("NightVision"));

			if (MemoryUtils::IsAddressValid(NightComponent) && NightComponent)
			{
				MemoryUtils::Write<bool>(NightComponent + 0xCC, true);
				IsNightSet = true;
				MemoryUtils::Write<float>(NightComponent + 0xB8, 0.f);

				uintptr_t TextureMask = MemoryUtils::Read<uintptr_t>(NightComponent + 0x30);

				if (MemoryUtils::IsAddressValid(TextureMask) && TextureMask)
				{
					auto MaskColor = UnityColor(0.f, 0.f, 0.f, 0.f);
					MemoryUtils::Write<UnityColor>(TextureMask + 0x38, MaskColor);

					MemoryUtils::Write<bool>(TextureMask + 0x48, false);
					MemoryUtils::Write<float>(TextureMask + 0x4C, 0.f);
				}

				if (NightVisionCustom)
				{
					NightVisionColor = UnityColor(NightRed, NightGreen, NightBlue, 0.f);
					MemoryUtils::Write<UnityColor>(NightComponent + 0xBC, NightVisionColor);
				}

				MemoryUtils::Write<float>(NightComponent + 0xA8, 0.f);
				MemoryUtils::Write<float>(NightComponent + 0xB0, 0.f);
			}
		}
	}

	if (!NightVision && IsNightSet)
	{
		const auto GameObject = EFTCore::GetGameObject(reinterpret_cast<uint64_t>(EFTCore::m_Camera));

		if (MemoryUtils::IsAddressValid(GameObject) && GameObject)
		{
			auto NightComponent = EFTCore::GetComponentFromObject(GameObject, xor ("NightVision"));

			if (MemoryUtils::IsAddressValid(NightComponent) && NightComponent)
			{
				MemoryUtils::Write<bool>(NightComponent + 0xCC, false);
				IsNightSet = false;
			}
		}
	}

	if (NoVisor)
	{
		const auto GameObject = EFTCore::GetGameObject(reinterpret_cast<uint64_t>(EFTCore::m_Camera));

		if (MemoryUtils::IsAddressValid(GameObject) && GameObject)
		{
			auto VisorComponent = EFTCore::GetComponentFromObject(GameObject, xor ("VisorEffect"));

			if (MemoryUtils::IsAddressValid(VisorComponent) && VisorComponent)
			{
				MemoryUtils::Write<float>(VisorComponent + 0xB8, 0.0f);
				IsVisorSet = true;
			}
		}
	}

	if (!NoVisor && IsVisorSet)
	{
		const auto GameObject = EFTCore::GetGameObject(reinterpret_cast<uint64_t>(EFTCore::m_Camera));

		if (MemoryUtils::IsAddressValid(GameObject) && GameObject)
		{
			auto VisorComponent = EFTCore::GetComponentFromObject(GameObject, xor ("VisorEffect"));

			if (MemoryUtils::IsAddressValid(VisorComponent) && VisorComponent)
			{
				MemoryUtils::Write<float>(VisorComponent + 0xB8, 1.0f);
				IsVisorSet = false;
			}
		}
	}

	if (FastSpeed)
	{
		if ((_GetAsyncKeyState(SpeedKey) & 0x8000))
		{
			SetGameSpeed(1.5f);
			IsSpeedSet = true;
		}
		else
		{
			SetGameSpeed(1.0f);
		}
	}

	if (!FastSpeed && IsSpeedSet)
	{
		SetGameSpeed(1.0f);
	}

	float maxFov = 999.0f;

	for (int i = 0; i < playerCount; i++)
	{
		Player* player = playerList->entries[i];
		
		if (!MemoryUtils::IsAddressValid(player) || !player)
			continue;

		if (player != LocalPlayer)
		{
			float distance = player->GetDistance();
			bool inRange = (distance <= 300.f);
			
			if (inRange)
			{
				{
					auto FootPos3D = player->GetHumanBonePosition(HumanBoneType::ROOT);

					if (FootPos3D.Zero())
						continue;

					auto FootPos = player->GetHumanBonePosition(HumanBoneType::ROOT);

					if (FootPos.Zero())
						continue;

					auto HeadPos = player->GetHumanBonePosition(HumanBoneType::HEAD);

					if (HeadPos.Zero())
						continue;

					float CornerHeight = CRT::m_abs(HeadPos.y - FootPos.y);
					float CornerWidth = CornerHeight * 0.66f;

					auto GreenAcidColor = IM_COL32(119.0f, 255.0f, 119.0f, 255.0f);
					auto WhitePastelColor = IM_COL32(239, 239, 239, 255);

					auto BoxColor = WhitePastelColor;

					if (PlayerESP)
					{
						if (BoxStyle == 1)
						{
							DrawCornerESP(HeadPos.x - (CornerWidth / 2), HeadPos.y, CornerWidth, CornerHeight, BoxColor, 1.5f);
						}
						else
						{
							Draw2DBox(HeadPos.x - (CornerWidth / 2), HeadPos.y, CornerWidth, CornerHeight, 1, BoxColor);
						}
					}

					if (BoneESP)
						DrawSkeleton(player, BoxColor);

					if (DistanceESP)
					{
						std::string distanceText = std::string("[") + (std::to_string((int)distance) + "]");

						DrawOutlinedText(guiFont, distanceText, ImVec2(HeadPos.x, FootPos.y + 5), 14, BoxColor, true);
					}

					auto Profile = player->GetPlayerProfile();
					
					if (!MemoryUtils::IsAddressValid(Profile) || !Profile)
						continue;

					auto Info = Profile->m_PlayerInfo;

					if (!MemoryUtils::IsAddressValid(Info) || !Info)
						continue;

					auto Name = Info->m_pPlayerName;

					if (!MemoryUtils::IsAddressValid(Name) || !Name)
						continue;

					if (Name->size <= 0)
						continue;

					int32_t NameLength = MemoryUtils::Read<int32_t>(reinterpret_cast<uint64_t>(Name) + 0x10);
					auto PlayerNameCh = MemoryUtils::GetUnicodeString(reinterpret_cast<uint64_t>(Name) + 0x14, NameLength);

					std::wstring PlayerNameUTF = ToUnicodes(PlayerNameCh);
					std::string PlayerName = ToANSI(PlayerNameUTF.c_str());

					auto HealthController = player->GetHealthController();

					if (!MemoryUtils::IsAddressValid(HealthController) || !HealthController)
						continue;

					auto HealthBody = HealthController->m_HealthBody;

					if (!MemoryUtils::IsAddressValid(HealthBody) || !HealthBody)
						continue;

					auto BodyController = HealthBody->m_BodyController;

					if (!MemoryUtils::IsAddressValid(BodyController) || !BodyController)
						continue;

					if (HealthBar)
					{
						float MaxHealth = 0.0f;
						float CurrentHealth = 0.0f;

						for (int i = 0; i < NUM_BODY_PARTS; i++)
						{
							auto BodyPart = BodyController->m_BodyParts[i].m_BodyPart;

							if (!MemoryUtils::IsAddressValid(BodyPart) || !BodyPart)
								continue;

							auto BodyHealth = BodyPart->m_Health;

							if (!MemoryUtils::IsAddressValid(BodyHealth) || !BodyHealth)
								continue;

							MaxHealth += BodyHealth->HealthMax;
							CurrentHealth += BodyHealth->Health;
						}

						DrawHealthBar(HeadPos, FootPos, CurrentHealth, MaxHealth);
					}

					if (ShowPlayerPrice)
					{
						auto InventoryController = MemoryUtils::Read<uintptr_t>((uintptr_t)player + 0x500);

						auto InventoryPrice = GetInventoryPrice(InventoryController);

						if (InventoryPrice > 0)
						{
							std::string formatted = std::string(to_string(InventoryPrice));

							std::string containerName = PlayerName + std::string(xor (" | "));//Nicknames ? PlayerName + std::string(xor (" [")) : std::string(xor ("["));
							std::string containerDelim = std::string(xor (" RUB"));

							std::string inventoryInfo = std::string(containerName) + formatted + containerDelim;

							DrawOutlinedText(guiFont, inventoryInfo, ImVec2(HeadPos.x, HeadPos.y - 20), 14, BoxColor, true);
						}
						else if (Nicknames)
						{
							DrawOutlinedText(guiFont, PlayerName, ImVec2(HeadPos.x, HeadPos.y - 20), 14, BoxColor, true);
						}
					}

					if (Nicknames && !ShowPlayerPrice)
					{
						DrawOutlinedText(guiFont, PlayerName, ImVec2(HeadPos.x, HeadPos.y - 20), 14, BoxColor, true);
					}

					if (AimBotEnabled)
					{
						auto TargetPlayer = GetPlayerByFOV(CameraPosition, player, maxFov);

						if (!TargetPlayer)
							continue;

						auto TargetHeadPos = TargetPlayer->GetHumanBonePosition(HumanBoneType::HEAD);

						if (TargetHeadPos.Zero())
							continue;

						auto TargetFootPos = TargetPlayer->GetHumanBonePosition(HumanBoneType::ROOT);

						if (TargetFootPos.Zero())
							continue;

						float tCornerHeight = CRT::m_abs(TargetHeadPos.y - TargetFootPos.y);
						float tCornerWidth = tCornerHeight * 0.65f;

						if (BoneESP)
							DrawSkeleton(TargetPlayer, GreenAcidColor);

						if (PlayerESP)
						{
							if (BoxStyle == 1)
							{
								DrawCornerESP(TargetHeadPos.x - (tCornerWidth / 2), TargetHeadPos.y, tCornerWidth, tCornerHeight, GreenAcidColor, 1.5f);
							}
							else
							{
								Draw2DBox(TargetHeadPos.x - (tCornerWidth / 2), TargetHeadPos.y, tCornerWidth, tCornerHeight, 1, GreenAcidColor);
							}
						}

						if ((_GetAsyncKeyState(AimBotKey) & 0x8000))
						{
							FaceEntity(LocalPlayer, TargetPlayer);
						}
					}
				}
			}
		}
	}
}

static bool IsWin32Init = false;
constexpr std::uint8_t g_font_key = 0xEDLL;

template< typename Type >
inline void CSafeDeleteArray(Type& object)
{
	if (MemoryUtils::IsAddressValid(object))
	{
		delete[] object;
		object = nullptr;
	}
}

struct XShader
{
	XShader(uint8_t key, const uint8_t* const code, size_t size)
		: m_key(key)
		, m_code(code)
		, m_data(nullptr)
		, m_size(size)
	{
		m_data = new uint8_t[m_size];
	}

	~XShader()
	{
		CSafeDeleteArray(m_data);
	}

	bool empty() const
	{
		return (m_size == 0);
	}

	size_t size() const
	{
		return m_size;
	}

	const uint8_t* decrypt() const
	{
		for (size_t index = 0; index < m_size; index++)
		{
			m_data[index] = m_code[index] ^ m_key;
		}

		return m_data;
	}

	uint8_t m_key = 0;
	const uint8_t* m_code = nullptr;
	uint8_t* m_data = nullptr;
	size_t m_size = 0;
};

template< typename Type >
inline void CrSafeRelease(Type& object)
{
	if (MemoryUtils::IsAddressValid(object))
	{
		object->Release();
		object = nullptr;
	}
}

HRESULT __fastcall PresentScene_Hook(IDXGISwapChain* SwapChain, UINT SyncInterval, UINT Flags)
{
	if (!Device)
	{
		if (SUCCEEDED(SwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&Device)))
		{
			SwapChain->GetDevice(__uuidof(Device), (void**)&Device);
			Device->GetImmediateContext(&DeviceContext);
		}

		ID3D11Texture2D* renderTargetTexture = nullptr;
		if (SUCCEEDED(SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&renderTargetTexture))))
		{
			Device->CreateRenderTargetView(renderTargetTexture, nullptr, &RenderTargetView);

			renderTargetTexture->Release();
		}

		ID3D11Texture2D* backBuffer = 0;
		SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (PVOID*)&backBuffer);

		D3D11_TEXTURE2D_DESC backBufferDesc = { 0 };
		backBuffer->GetDesc(&backBufferDesc);

		screenWidth = (float)backBufferDesc.Width;
		screenHeight = (float)backBufferDesc.Height;

		//CrSafeRelease(backBuffer);

		backBuffer->Release();

		hwnd = EFTCore::GetGameWindow();

		if (!hwnd || !MemoryUtils::IsAddressValid(hwnd))
		{
			DebugUtils::Log(xor ("[DWE] HWD not found."));
			Exit;
		}


		auto imGuiContext = ImGui::CreateContext();

		SetStyles();

		bool hwndInit = ImGui_ImplWin32_Init(hwnd);
		bool d3dInit = ImGui_ImplDX11_Init(Device, DeviceContext);

		guiFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(xor ("C:\\Windows\\Fonts\\Arial.ttf"), 15.0f, NULL, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
		lootFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(xor ("C:\\Windows\\Fonts\\Arial.ttf"), 11.0f, NULL, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());

		bool deviceObjectsInit = ImGui_ImplDX11_CreateDeviceObjects();
	}

	DeviceContext->OMSetRenderTargets(1, &RenderTargetView, nullptr);

	auto ImWindow = BeginScene();

	if (ImWindow)
	{
		OnRender(ImWindow);

		EndScene(ImWindow);
	}

	if (!ImMenu)
	{
		auto ImWindow = BeginScene();

		if (ImWindow)
		{
			OnRender(ImWindow);

			EndScene(ImWindow);
		}
	}

	if (ImMenu)
	{
		RenderMenu();
	}

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return PresentScene_Original(SwapChain, SyncInterval, Flags);
}

HRESULT __fastcall ResizeBuffers_Hook(IDXGISwapChain* SwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	if (Device != nullptr)
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();

		ImGui::DestroyContext();

		if (RenderTargetView)
			RenderTargetView->Release();

		if (DeviceContext)
			DeviceContext->Release();

		if (Device)
			Device->Release();

		Device = nullptr;
	}

	return ResizeBuffers_Original(SwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

VOID WINAPI EFT_OnLoad(LPCSTR Kek, LPCSTR Kek2)
{
	DebugUtils::Init();

	if (!Unity::Init())
	{
		DebugUtils::Log(xor ("[DWE] Unity init failed."));
		Exit;
	}

	hwnd = EFTCore::GetGameWindow();

	if (!hwnd || !MemoryUtils::IsAddressValid(hwnd))
	{
		DebugUtils::Log(xor ("[DWE] HWD not found."));
		Exit;
	}

	D3D11SwapChain* m_d3d11_swap_chain = Unity::GetD3D11SwapChain();
	
	if (!m_d3d11_swap_chain || !MemoryUtils::IsAddressValid(m_d3d11_swap_chain))
	{
		DebugUtils::Log(xor ("[DWE] DSWC not found."));
		Exit;
	}

	IDXGISwapChain* m_swap_chain = m_d3d11_swap_chain->GetSwapChain();

	if (!m_swap_chain || !MemoryUtils::IsAddressValid(m_swap_chain))
	{
		DebugUtils::Log(xor ("[DWE] SW not found."));
		Exit;
	}

	EFTCore::m_GameObjectManager = EFTCore::GetObjectManager();

	if (!EFTCore::m_GameObjectManager)
	{
		DebugUtils::Log(xor ("[DWE] GMR not found."));
		Exit;
	}

	LPCSTR lpUser32Name = xor ("user32.dll");

	HMODULE hUser32 = (HMODULE) ModuleUtils::GetModuleBase(lpUser32Name);
	LPVOID lpGetWindowRect = ModuleUtils::GetFuncAddress(hUser32, xor ("GetClientRect"));

	//_GetClientRect = (tGetClientRect)lpGetWindowRect;

	_CallWindowProcW = (tCallWindowProcW)ModuleUtils::GetFuncAddress(hUser32, xor ("CallWindowProcW"));
	_GetAsyncKeyState = (tGetAsyncKeyState)ModuleUtils::GetFuncAddress(hUser32, xor ("GetAsyncKeyState"));

	//_GetWindowLongPtrW = (tGetWindowLongPtrW)ModuleUtils::GetFuncAddress(hUser32, xor ("GetWindowLongPtrW"));
	_SetWindowLongPtrW = (tSetWindowLongPtrW)ModuleUtils::GetFuncAddress(hUser32, xor ("SetWindowLongPtrW"));

	LPCSTR lpKernel32 = xor ("kernel32.dll");
	HMODULE hKernel32 = (HMODULE)ModuleUtils::GetModuleBase(lpKernel32);

	if (!hKernel32)
	{
		DebugUtils::Log(xor ("[DWE] Kr not found."));
		Exit;
	}

	fnMultiByteToWideChar = (tMultiByteToWideChar)ModuleUtils::GetFuncAddress(hKernel32, xor ("MultiByteToWideChar"));
	fnWideCharToMultiByte = (tWideCharToMultiByte)ModuleUtils::GetFuncAddress(hKernel32, xor ("WideCharToMultiByte"));

	if (!ImGui::InitHeapFunctions())
	{
		DebugUtils::Log(xor ("[DWE] Hef init failed."));
		Exit;
	}

	if (!ImGui::InitApiFunctions())
	{
		DebugUtils::Log(xor ("[DWE] Afi init failed."));
		Exit;
	}

	if (!ImGui_ImplWin32_InitData())
	{
		DebugUtils::Log(xor ("[DWE] Win init failed."));
		Exit;
	}

	CameraLocation = Vector3();
	CameraRotation = Vector3();
	CameraFov = 0.0f;

	NightVisionColor = UnityColor(NightRed, NightGreen, NightBlue, 0.f);

	UnityWindowProc = WNDPROC(_SetWindowLongPtrW(hwnd, GWLP_WNDPROC, LONG_PTR(&WndProc_Hook)));

	VMTHook* hook = new VMTHook();

	hook->Initialize(reinterpret_cast<PVOID>(m_swap_chain));
	hook->CreateHook(reinterpret_cast<PVOID>(PresentScene_Hook), 8);

	PresentScene_Original = hook->GetFunctionPtr<tPresentScene>(8);

	VMTHook* hook2 = new VMTHook();

	hook2->Initialize(reinterpret_cast<PVOID>(m_swap_chain));
	hook2->CreateHook(reinterpret_cast<PVOID>(ResizeBuffers_Hook), 13);

	ResizeBuffers_Original = hook2->GetFunctionPtr<tResizeBuffers>(13);

	LI_FN(Beep).forwarded_safe_cached()(1110, 130);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	auto meme = xor ("bfffqgqgqegqe");

	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		ModuleUtils::EraseHeaders(hinstDLL);
		auto meme2 = xor ("bbgr444q");
		EFT_OnLoad(meme, meme2);

		return TRUE;
	}

	return TRUE;
}