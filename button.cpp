#include "../tyler.hpp"

#include "../libs/imgui_tricks.hpp"

bool TylerUI::Elements::Button(const char* label, ImVec2 size_arg)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	int flags = 0;
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos;
	ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, pos + size);
	ImGui::ItemSize(bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(bb, id))
		return false;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, 0);

	if (hovered)
		ImGui::SetMouseCursor(7);

	float Active = ImTricks::Animations::FastFloatLerp(id, hovered, 0.f, 1.f, 0.05f);
	ImColor Text = ImTricks::Animations::FastColorLerp(ImColor(176, 176, 176), ImColor(255,255,255), Active);
	ImColor Rect = ImTricks::Animations::FastColorLerp(ImColor(70, 80, 90), ImColor(70, 80, 90), Active);

	window->DrawList->AddRectFilled(bb.Min, bb.Max, Rect, 4);

	window->DrawList->AddText(bb.Min + ImVec2(size_arg.x / 2 - ImGui::CalcTextSize(label).x / 2, size_arg.y / 2 - ImGui::CalcTextSize(label).y / 2), Text, label);

	return pressed;
}
