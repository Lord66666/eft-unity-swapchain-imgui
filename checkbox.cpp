#include "../tyler.hpp"
#include "../libs/imgui_tricks.hpp"
#include <map>

void TylerUI::Elements::Checkbox(const char* label, bool& v)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;

	int flags = 0;
	ImGuiContext& g = *GImGui; 
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos;
	ImVec2 size = ImGui::CalcItemSize({ 160, 18 }, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, pos + size);
	ImGui::ItemSize(bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(bb, id))
		return;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, 0);

	if (ImGui::IsMouseHoveringRect(bb.Min + ImVec2(0, 0), bb.Min + ImVec2(18, 18)))
		ImGui::SetMouseCursor(7);

	if (ImGui::IsMouseHoveringRect(bb.Min + ImVec2(0, 0), bb.Min + ImVec2(18, 18)) && pressed)
		v = !v;

	float Active = ImTricks::Animations::FastFloatLerp(id, v, 0, 1.f, 0.05f);

	ImColor back = ImTricks::Animations::FastColorLerp(ImColor(73, 83, 93), ImColor(149, 137, 255), Active);
	ImColor Text = ImTricks::Animations::FastColorLerp(ImColor(176, 176, 176), ImColor(255,255,255), Active);

	window->DrawList->AddRectFilled(bb.Min, bb.Min + ImVec2(18, 18), back, 4);
	window->DrawList->AddText(bb.Min + ImVec2(25, 2), Text, label);
}
