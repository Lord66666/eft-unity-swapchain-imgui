#include "../tyler.hpp"
#include "../libs/imgui_tricks.hpp"

#include <map>

struct TabStruct {
	float size;
	float hovered;
};

bool TylerUI::Elements::Tab(const char* label,int& tab, int index)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	const ImGuiID id = window->GetID(label);

	static std::map<ImGuiID, TabStruct> circle_anim;


	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos;
	ImVec2 size = ImGui::CalcItemSize({ 120, 30 }, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	ImRect bb(pos, pos + size);
	ImGui::InvisibleButton(label, size);

	if (ImGui::IsItemHovered())
		ImGui::SetMouseCursor(7);

	if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		tab = index;

	float Selected = ImTricks::Animations::FastFloatLerp(id, tab == index, 0, 1.f, 0.05f);
	ImColor TabText = ImTricks::Animations::FastColorLerp(ImColor(34, 44, 54), ImColor(149, 137, 255), Selected);

	window->DrawList->PushClipRect(bb.Min, bb.Max, false);
	{	
		window->DrawList->AddRectFilled(bb.Min, bb.Max, ImColor(149, 137, 255), 4);
		window->DrawList->AddRectFilled(bb.Min + ImVec2(2, 2), bb.Max - ImVec2(2, 2), TabText, 4);
		window->DrawList->AddText(bb.Min + ImVec2(60, 15) - ImGui::CalcTextSize(label) / 2, ImColor(180, 191, 197), label);
	}
	window->DrawList->PopClipRect();
}