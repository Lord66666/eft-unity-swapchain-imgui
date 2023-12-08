#include "ModuleUtils.hpp"

class DebugUtils
{
public:
	static VOID Init();

	static VOID InitString(char* buffer, const char* format, ...);

	template<typename... Args>
	static VOID Log(LPCSTR text, Args... args)
	{
		//CHAR Buffer[4096] = { 0 };
		//DebugUtils::InitString(Buffer, text, args...);

		LI_FN(OutputDebugStringA).forwarded_safe_cached()(text);
	}

	static int Random();
};