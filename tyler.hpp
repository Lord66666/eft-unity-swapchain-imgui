
#include "../imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui_internal.h"

#include <functional>
#include "libs/imgui_tricks.hpp"


#include "font/faprolight.hpp"
#include "font/hashes.h"
#include "font/sffont.hpp"

namespace TylerUI {

	namespace Elements
	{
		extern void BeginChild(const char* str_id, const ImVec2 size_arg, bool border = false, ImGuiWindowFlags extra_flags = NULL);
		extern void EndChild();
		extern void Checkbox(const char* label, bool& v);
		extern bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, float power);
		extern bool SliderInt(const char* label, int* v, int v_min, int v_max, const char* format);

		extern bool BeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags = 0);
		extern void EndCombo();
		extern bool ComboElement(const char* label, bool selected);
		extern bool Combo(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1);
		extern bool Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int popup_max_height_in_items = -1);
		extern bool Combo(const char* label, int* current_item, bool(*items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int popup_max_height_in_items = -1);

		extern bool Button(const char* label, ImVec2 size_arg);

		extern bool Tab(const char* label, int& tab, int index);
	}

	namespace Fonts {
		inline ImFont* DrukWideBold;
		inline ImFont* OriginalBig;
		inline ImFont* OriginalMedium;
		inline ImFont* Logotype;
	}

}
