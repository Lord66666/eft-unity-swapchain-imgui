#include "../tyler.hpp"

#include "../libs/imgui_tricks.hpp"

static const char* PatchFormatStringFloatToInt(const char* fmt)
{
	if (fmt[0] == '%' && fmt[1] == '.' && fmt[2] == '0' && fmt[3] == 'f' && fmt[4] == 0) // Fast legacy path for "%.0f" which is expected to be the most common case.
		return "%d";
	const char* fmt_start = ImParseFormatFindStart(fmt);    // Find % (if any, and ignore %%)
	const char* fmt_end = ImParseFormatFindEnd(fmt_start);  // Find end of format specifier, which itself is an exercise of confidence/recklessness (because snprintf is dependent on libc or user).
	if (fmt_end > fmt_start && fmt_end[-1] == 'f')
	{
		if (fmt_start == fmt && fmt_end[0] == 0)
			return "%d";
		ImGuiContext& g = *GImGui;
		ImFormatString(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), "%.*s%%d%s", (int)(fmt_start - fmt), fmt, fmt_end); // Honor leading and trailing decorations, but lose alignment/precision.
		return g.TempBuffer;
	}
	return fmt;
}

bool SliderScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, float power)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);

	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
	const ImRect frame_bb(window->DC.CursorPos + ImVec2(0, 20), window->DC.CursorPos + ImVec2(160, 30));
	const ImRect total_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(160, 30));

	ImGui::ItemSize(total_bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(total_bb, id, &frame_bb))
		return false;

	if (format == NULL)
		format = ImGui::DataTypeGetInfo(data_type)->PrintFmt;
	else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0)
		format = PatchFormatStringFloatToInt(format);

	const bool hovered = ImGui::ItemHoverable(frame_bb, id);
	bool temp_input_is_active = ImGui::IsActiveIdUsingNavInput(id); //TempInputIsActive
	bool temp_input_start = false;
	if (!temp_input_is_active)
	{
		const bool focus_requested = ImGui::FocusableItemRegister(window, id);
		const bool clicked = (hovered && g.IO.MouseClicked[0]);
		if (focus_requested || clicked || g.NavActivateId == id || g.NavInputId == id)
		{
			ImGui::SetActiveID(id, window);
			ImGui::SetFocusID(id, window);
			ImGui::FocusWindow(window);
			g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
			if (focus_requested || (clicked && g.IO.KeyCtrl) || g.NavInputId == id)
			{
				temp_input_start = true;
				ImGui::FocusableItemUnregister(window);
			}
		}
	}


	if (hovered)
		ImGui::SetMouseCursor(7);

	float Active = ImTricks::Animations::FastFloatLerp(id, hovered, 0.f, 1.f, 0.05f);

	ImColor color_frame = ImTricks::Animations::FastColorLerp(ImColor(68, 78, 88), ImColor(68, 78, 88), Active);
	ImGui::RenderFrame(frame_bb.Min + ImVec2(0, 2), frame_bb.Max + ImVec2(0, -2), color_frame, true, 2);

	ImRect grab_bb;
	const bool value_changed = ImGui::SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, ImGuiSliderFlags_None, &grab_bb);

	if ((grab_bb.Max.x > grab_bb.Min.x) && (frame_bb.Min.x + 5 < grab_bb.Max.x)) {
		window->DrawList->AddRectFilled(frame_bb.Min + ImVec2(1, 2), grab_bb.Max + ImVec2(1, 0), ImColor(149, 137, 255), 2);
	}

	window->DrawList->AddRectFilled(grab_bb.Min - ImVec2(1, 4), grab_bb.Max + ImVec2(2, 3), ImColor(130, 131, 187), 2);

	char value_buf[64];
	const char* value_buf_end = value_buf + ImGui::DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);

	window->DrawList->AddText(total_bb.Min + ImVec2(0, 7 - ImGui::CalcTextSize(label).y / 2), ImColor(255,255,255), label);
	ImColor Text = ImTricks::Animations::FastColorLerp(ImColor(139, 139, 139), ImColor(255, 255, 255), Active);
	window->DrawList->AddText(total_bb.Max + ImVec2(-ImGui::CalcTextSize(value_buf).x, -30), Text, value_buf);

	return value_changed;
}

bool TylerUI::Elements::SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, float power)
{
	return SliderScalar(label, ImGuiDataType_Float, v, &v_min, &v_max, format, power);
}

bool TylerUI::Elements::SliderInt(const char* label, int* v, int v_min, int v_max, const char* format)
{
	return SliderScalar(label, ImGuiDataType_S32, v, &v_min, &v_max, format, 1);
}