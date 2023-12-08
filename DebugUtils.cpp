#include "DebugUtils.hpp"

typedef int (WINAPI* t_sprintf)(char* buffer, const char* format, ...);
t_sprintf f_sprintf = NULL;

typedef int (__cdecl* t_rand)(void);
t_rand f_rand = NULL;

VOID DebugUtils::Init()
{
	LPCSTR moduleName = xor ("msvcrt.dll");

	HMODULE hModule = reinterpret_cast<HMODULE>(ModuleUtils::GetModuleBase(moduleName));

	f_sprintf = (t_sprintf)(ModuleUtils::GetFuncAddress(hModule, xor ("sprintf")));
	f_rand = (t_rand)(ModuleUtils::GetFuncAddress(hModule, xor ("rand")));
}

VOID DebugUtils::InitString(char* buffer, const char* format, ...)
{
	f_sprintf(buffer, format);
}

int DebugUtils::Random()
{
	return f_rand();
}