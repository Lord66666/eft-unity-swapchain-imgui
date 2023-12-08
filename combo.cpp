#include "../tyler.hpp"

#include "../libs/imgui_tricks.hpp"

static float CalcMaxPopupHeightFromItemCount(int items_count)
{ 
	ImGuiContext& g = *GImGui;
	if (items_count <= 0)
		return FLT_MAX;
	return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
}

bool TylerUI::Elements::BeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags)
{
	ImGuiContext& g = *GImGui;
	bool has_window_size_constraint = (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint) != 0;
	g.NextWindowData.Flags &= ~ImGuiNextWindowDataFlags_HasSizeConstraint;

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);

	const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : ImGui::GetFrameHeight();
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

	const ImRect frame_bb(window->DC.CursorPos + ImVec2(0, 0), window->DC.CursorPos + ImVec2(160, 20));
	const ImRect total_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(160, 20));

	ImGui::ItemSize(total_bb, 0);
	if (!ImGui::ItemAdd(total_bb, id, &frame_bb))
		return false;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(frame_bb, id, &hovered, &held);
	bool popup_open = ImGui::IsPopupOpen(id, ImGuiPopupFlags_None);

	const ImU32 frame_col = ImGui::GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
	const float value_x2 = ImMax(frame_bb.Min.x, frame_bb.Max.x - arrow_size);

	float Hovered = ImTricks::Animations::FastFloatLerp(id, hovered || popup_open, 0.f, 1.f, 0.09f);

	if ((pressed || g.NavActivateId == id) && !popup_open)
	{
		if (window->DC.NavLayerCurrent == 0)
			window->NavLastIds[0] = id;

		ImGui::OpenPopupEx(id, ImGuiPopupFlags_None);
		popup_open = true;
	}

	float Active = ImTricks::Animations::FastFloatLerp(id, hovered, 0.f, 1.f, 0.05f);
	ImColor color_frame = ImTricks::Animations::FastColorLerp(ImColor(70, 80, 90), ImColor(70, 80, 90), Active);

	ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, color_frame, true, 4);

	ImColor Text = ImTricks::Animations::FastColorLerp(ImColor(176, 176, 176), ImColor(255, 255, 255), Active);
	
	if (preview_value != NULL && !(flags & ImGuiComboFlags_NoPreview))
		window->DrawList->AddText(ImVec2(frame_bb.Min.x + 10, frame_bb.Min.y + 3), Text, preview_value);

	window->DrawList->AddText(total_bb.Min + ImVec2(165, 3), ImColor(255,255,255), label);

	if (hovered)
		ImGui::SetMouseCursor(7);

	if (!popup_open)
		return false;


	if ((flags & ImGuiComboFlags_HeightMask_) == 0)
		flags |= ImGuiComboFlags_HeightRegular;
	IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiComboFlags_HeightMask_));
	int popup_max_height_in_items = -1;
	if (flags & ImGuiComboFlags_HeightRegular)     popup_max_height_in_items = 8;
	else if (flags & ImGuiComboFlags_HeightSmall)  popup_max_height_in_items = 4;
	else if (flags & ImGuiComboFlags_HeightLarge)  popup_max_height_in_items = 20;
	ImGui::SetNextWindowSizeConstraints(ImVec2(160, 0.0f), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));

	char name[16];
	ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size);

	if (ImGuiWindow* popup_window = ImGui::FindWindowByName(name))
		if (popup_window->WasActive)
		{
			ImVec2 size_expected = ImGui::CalcWindowNextAutoFitSize(popup_window);
			if (flags & ImGuiComboFlags_PopupAlignLeft)
				popup_window->AutoPosLastDirection = ImGuiDir_Left;
			ImRect r_outer = ImGui::GetPopupAllowedExtentRect(popup_window);
			ImVec2 pos = ImGui::FindBestWindowPosForPopupEx(frame_bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, frame_bb, ImGuiPopupPositionPolicy_ComboBox);
			ImGui::SetNextWindowPos(pos - ImVec2(0, -5));
		}

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

	bool ret = ImGui::Begin(name, NULL, window_flags);
	ImGui::PopStyleVar(2);

	if (!ret)
	{
		ImGui::EndPopup();
		return false;
	}
	return true;
}

void TylerUI::Elements::EndCombo()
{
	ImGui::EndPopup();
}

static bool Items_ArrayGetter(void* data, int idx, const char** out_text)
{
	const char* const* items = (const char* const*)data;
	if (out_text)
		*out_text = items[idx];
	return true;
}

static bool Items_SingleStringGetter(void* data, int idx, const char** out_text)
{
	// FIXME-OPT: we could pre-compute the indices to fasten this. But only 1 active combo means the waste is limited.
	const char* items_separated_by_zeros = (const char*)data;
	int items_count = 0;
	const char* p = items_separated_by_zeros;
	while (*p)
	{
		if (idx == items_count)
			break;
		p += strlen(p) + 1;
		items_count++;
	}
	if (!*p)
		return false;
	if (out_text)
		*out_text = p;
	return true;
}

bool TylerUI::Elements::ComboElement(const char* label, bool selected)
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
	ImVec2 size = ImGui::CalcItemSize({ ImGui::GetWindowSize().x, 20 }, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, pos + size);
	ImGui::ItemSize(bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(bb, id))
		return false;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, 0);

	if (hovered)
		ImGui::SetMouseCursor(7);

	float Selected = ImTricks::Animations::FastFloatLerp(id, selected, 0.f, 1.f, 0.09f);
	float Hovered = ImTricks::Animations::FastFloatLerp(ImGui::GetID(std::string(label).append("hover").c_str()), hovered, 0.f, 1.f, 0.09f);
	ImColor color = ImTricks::Animations::FastColorLerp(ImColor(25, 25, 25), ImColor(27, 27, 27), selected);
	ImColor color_text = ImTricks::Animations::FastColorLerp(ImColor(139, 139, 139), ImColor(200, 200, 200), selected);
	window->DrawList->AddRectFilled(bb.Min, bb.Max, color, Hovered);
	window->DrawList->AddText(bb.Min + ImVec2(10, 4), color_text, label);

	return pressed;
}

bool TylerUI::Elements::Combo(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int popup_max_height_in_items)
{
	ImGuiContext& g = *GImGui;

	// Call the getter to obtain the preview string which is a parameter to BeginCombo()
	const char* preview_value = NULL;
	if (*current_item >= 0 && *current_item < items_count)
		items_getter(data, *current_item, &preview_value);

	// The old Combo() API exposed "popup_max_height_in_items". The new more general BeginCombo() API doesn't have/need it, but we emulate it here.
	if (popup_max_height_in_items != -1 && !(g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint))
		ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));

	if (!BeginCombo(label, preview_value, ImGuiComboFlags_None))
		return false;

	// Display items
	// FIXME-OPT: Use clipper (but we need to disable it on the appearing frame to make sure our call to SetItemDefaultFocus() is processed)
	bool value_changed = false;
	for (int i = 0; i < items_count; i++)
	{
		ImGui::PushID((void*)(intptr_t)i);
		const bool item_selected = (i == *current_item);
		const char* item_text;
		if (!items_getter(data, i, &item_text))
			item_text = "*Unknown item*";
		ImGui::SetCursorPos({ 0, float(i * 20) });
		if (ComboElement(item_text, item_selected))
		{
			value_changed = true;
			*current_item = i;
		}
		if (item_selected)
			ImGui::SetItemDefaultFocus();
		ImGui::PopID();
	}

	EndCombo();
	return value_changed;
}

// Combo box helper allowing to pass an array of strings.
bool TylerUI::Elements::Combo(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items)
{
	const bool value_changed = Combo(label, current_item, Items_ArrayGetter, (void*)items, items_count, height_in_items);
	return value_changed;
}

// Combo box helper allowing to pass all items in a single string literal holding multiple zero-terminated items "item1\0item2\0"
bool TylerUI::Elements::Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int height_in_items)
{
	int items_count = 0;
	const char* p = items_separated_by_zeros;       // FIXME-OPT: Avoid computing this, or at least only when combo is open
	while (*p)
	{
		p += strlen(p) + 1;
		items_count++;
	}
	bool value_changed = Combo(label, current_item, Items_SingleStringGetter, (void*)items_separated_by_zeros, items_count, height_in_items);
	return value_changed;
}
