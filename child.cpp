#include "../tyler.hpp"

void shadow(ImDrawList* drawlist, ImVec2 pos, ImVec2 size, ImVec4 color)
{
	while (true)
	{
		if (color.w <= 0.019f)
			break;

		drawlist->AddRect(pos, pos + size, ImGui::GetColorU32(color), 8);
		color.w -= color.w / 8;
		pos -= ImVec2(1.f, 1.f);
		size += ImVec2(2.f, 2.f);
	}
}

bool BeginChildEx(const char* name, ImGuiID id, const ImVec2& size_arg, bool border, ImGuiWindowFlags flags)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* parent_window = g.CurrentWindow;

	flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_ChildWindow;
	flags |= (parent_window->Flags & ImGuiWindowFlags_NoMove);  // Inherit the NoMove flag

	// Size
	const ImVec2 content_avail = ImGui::GetContentRegionAvail();
	ImVec2 size = ImFloor(size_arg);
	const int auto_fit_axises = ((size.x == 0.0f) ? (1 << ImGuiAxis_X) : 0x00) | ((size.y == 0.0f) ? (1 << ImGuiAxis_Y) : 0x00);
	if (size.x <= 0.0f)
		size.x = ImMax(content_avail.x + size.x, 4.0f); // Arbitrary minimum child size (0.0f causing too much issues)
	if (size.y <= 0.0f)
		size.y = ImMax(content_avail.y + size.y, 4.0f);
	ImGui::SetNextWindowSize(size);

	// Build up name. If you need to append to a same child from multiple location in the ID stack, use BeginChild(ImGuiID id) with a stable value.
	char title[256];
	if (name)
		ImFormatString(title, IM_ARRAYSIZE(title), "%s/%s_%08X", parent_window->Name, name, id);
	else
		ImFormatString(title, IM_ARRAYSIZE(title), "%s/%08X", parent_window->Name, id);

	const float backup_border_size = g.Style.ChildBorderSize;
	if (!border)
		g.Style.ChildBorderSize = 0.0f;
	bool ret = ImGui::Begin(title, NULL, flags);
	g.Style.ChildBorderSize = backup_border_size;

	ImGuiWindow* child_window = g.CurrentWindow;
	child_window->ChildId = id;
	child_window->AutoFitChildAxises = (ImS8)auto_fit_axises;

	// Set the cursor to handle case where the user called SetNextWindowPos()+BeginChild() manually.
	// While this is not really documented/defined, it seems that the expected thing to do.
	if (child_window->BeginCount == 1)
		parent_window->DC.CursorPos = child_window->Pos;

	// Process navigation-in immediately so NavInit can run on first frame
	if (g.NavActivateId == id && !(flags & ImGuiWindowFlags_NavFlattened) && (child_window->DC.NavLayersActiveMask != 0 || child_window->DC.NavHasScroll))
	{
		ImGui::FocusWindow(child_window);
		ImGui::NavInitWindow(child_window, false);
		ImGui::SetActiveID(id + 1, child_window); // Steal ActiveId with a dummy id so that key-press won't activate child item
		g.ActiveIdSource = ImGuiInputSource_Nav;
	}

	parent_window->DrawList->AddRectFilled(ImGui::GetWindowPos(), ImGui::GetWindowPos() + size_arg, ImColor(27, 27, 27), 0);
	parent_window->DrawList->AddRectFilled(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImVec2(size_arg.x, 24), ImColor(32, 32, 32));
	parent_window->DrawList->AddRect(ImGui::GetWindowPos(), ImGui::GetWindowPos() + size_arg, ImColor(16, 16, 16, 25));
	parent_window->DrawList->AddText(ImGui::GetWindowPos() + ImVec2(9, 5), ImColor(255, 255, 255), name);

	return ret;
}

bool BeginChild(const char* str_id, const ImVec2& size_arg, bool border, ImGuiWindowFlags extra_flags)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	return BeginChildEx(str_id, window->GetID(str_id), size_arg, border, extra_flags);
}

bool BeginChild(ImGuiID id, const ImVec2& size_arg, bool border, ImGuiWindowFlags extra_flags)
{
	IM_ASSERT(id != 0);
	return BeginChildEx(NULL, id, size_arg, border, extra_flags);
}

void EndChild()
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;

	IM_ASSERT(g.WithinEndChild == false);
	IM_ASSERT(window->Flags & ImGuiWindowFlags_ChildWindow);   // Mismatched BeginChild()/EndChild() calls

	g.WithinEndChild = true;
	if (window->BeginCount > 1)
	{
		ImGui::End();
	}
	else
	{
		ImVec2 sz = window->Size;
		if (window->AutoFitChildAxises & (1 << ImGuiAxis_X)) // Arbitrary minimum zero-ish child size of 4.0f causes less trouble than a 0.0f
			sz.x = ImMax(4.0f, sz.x);
		if (window->AutoFitChildAxises & (1 << ImGuiAxis_Y))
			sz.y = ImMax(4.0f, sz.y);
		ImGui::End();

		ImGuiWindow* parent_window = g.CurrentWindow;
		ImRect bb(parent_window->DC.CursorPos, parent_window->DC.CursorPos + sz);
		ImGui::ItemSize(sz); //NavLayerActiveMask
		if ((window->DC.NavLayersActiveMask != 0 || window->DC.NavHasScroll) && !(window->Flags & ImGuiWindowFlags_NavFlattened))
		{
			ImGui::ItemAdd(bb, window->ChildId);
			ImGui::RenderNavHighlight(bb, window->ChildId);

			// When browsing a window that has no activable items (scroll only) we keep a highlight on the child
			if (window->DC.NavLayersActiveMask == 0 && window == g.NavWindow)
				ImGui::RenderNavHighlight(ImRect(bb.Min - ImVec2(2, 2), bb.Max + ImVec2(2, 2)), g.NavId, ImGuiNavHighlightFlags_TypeThin);
		}
		else
		{
			// Not navigable into
			ImGui::ItemAdd(bb, 0);
		}
	}
	g.WithinEndChild = false;
}

void TylerUI::Elements::BeginChild(const char* str_id, const ImVec2 size_arg, bool border, ImGuiWindowFlags extra_flags)
{
	::BeginChild(str_id, size_arg, border, extra_flags);
	ImGui::SetCursorPos({ 18, 24 + 14 });
	ImGui::BeginGroup();
}

void TylerUI::Elements::EndChild()
{
	ImGui::EndGroup();
	::EndChild();
}