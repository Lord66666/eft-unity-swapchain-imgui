#include "ModuleUtils.hpp"

VOID ModuleUtils::EraseHeaders(HMODULE hModule)
{
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
	PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader + (ULONG)pDosHeader->e_lfanew);

	ULONG OldProtect = NULL;
	ULONG DosImageSize = sizeof(IMAGE_DOS_HEADER);

	if (LI_FN(VirtualProtectEx).forwarded_safe_cached()((HANDLE)-1, pDosHeader, DosImageSize, PAGE_READWRITE, &OldProtect))
	{
		for (ULONG i = 0; i < DosImageSize; i++)
			*(BYTE*)((BYTE*)pDosHeader + i) = 0;
	}

	ULONG NtImageSize = sizeof(IMAGE_NT_HEADERS);

	if (pDosHeader && LI_FN(VirtualProtectEx).forwarded_safe_cached()((HANDLE)-1, pDosHeader, NtImageSize, PAGE_READWRITE, &OldProtect))
	{
		for (ULONG i = 0; i < NtImageSize; i++)
			*(BYTE*)((BYTE*)pDosHeader + i) = 0;
	}
}

BYTE* ModuleUtils::GetFuncAddress(HMODULE hModule, const char* szFunc)
{
	if (!hModule)
		return nullptr;

	BYTE* pBase = reinterpret_cast<BYTE*>(hModule);

	auto* pNT = reinterpret_cast<IMAGE_NT_HEADERS*>(pBase + reinterpret_cast<IMAGE_DOS_HEADER*>(pBase)->e_lfanew);
	auto* pExportDir = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(pBase + pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	ULONG max = pExportDir->NumberOfNames;

	for (ULONG i = 0; i < max; ++i)
	{
		ULONG CurrNameRVA = reinterpret_cast<ULONG*>(pBase + pExportDir->AddressOfNames)[i];
		char* szName = reinterpret_cast<char*>(pBase + CurrNameRVA);

		if (CRT::StrCmp(szName, szFunc) == 0)
		{
			USHORT Ordinal = reinterpret_cast<USHORT*>(pBase + pExportDir->AddressOfNameOrdinals)[i];
			ULONG RVA = reinterpret_cast<ULONG*>(pBase + pExportDir->AddressOfFunctions)[Ordinal];

			return pBase + RVA;
		}
	}

	return nullptr;
}

PBYTE ModuleUtils::PatternScan(LPCSTR Pattern)
{
#define InRange(x, a, b) (x >= a && x <= b) 
#define GetBits(x) (InRange(x, '0', '9') ? (x - '0') : ((x - 'A') + 0xA))
#define GetByte(x) ((BYTE)(GetBits(x[0]) << 4 | GetBits(x[1])))

	auto ModuleStart = (PBYTE)ModuleUtils::GetModuleBase(nullptr);
	
	if (!ModuleStart)
		return nullptr;

	auto pNtHeader = ((PIMAGE_NT_HEADERS)(ModuleStart + ((PIMAGE_DOS_HEADER)ModuleStart)->e_lfanew));
	auto ModuleEnd = (PBYTE)(ModuleStart + pNtHeader->OptionalHeader.SizeOfImage - 0x1000); 
	
	ModuleStart += 0x1000;

	PBYTE FirstMatch = nullptr;
	const char* CurPatt = Pattern;
	
	for (; ModuleStart < ModuleEnd; ++ModuleStart)
	{
		bool SkipByte = (*CurPatt == '\?');
		
		if (SkipByte || *ModuleStart == GetByte(CurPatt))
		{
			if (!FirstMatch) 
				FirstMatch = ModuleStart;
			
			SkipByte ? CurPatt += 2 : CurPatt += 3;
			
			if (CurPatt[-1] == 0) 
				return FirstMatch;
		}
		else if (FirstMatch)
		{
			ModuleStart = FirstMatch;
			FirstMatch = nullptr;
			CurPatt = Pattern;
		}
	}

	return nullptr;
}

PBYTE ModuleUtils::PatternScanInModule(uint64_t ModuleBase, LPCSTR Pattern)
{
#define InRange(x, a, b) (x >= a && x <= b) 
#define GetBits(x) (InRange(x, '0', '9') ? (x - '0') : ((x - 'A') + 0xA))
#define GetByte(x) ((BYTE)(GetBits(x[0]) << 4 | GetBits(x[1])))

	auto ModuleStart = (PBYTE)ModuleBase;

	if (!ModuleStart)
		return nullptr;

	auto pNtHeader = ((PIMAGE_NT_HEADERS)(ModuleStart + ((PIMAGE_DOS_HEADER)ModuleStart)->e_lfanew));
	auto ModuleEnd = (PBYTE)(ModuleStart + pNtHeader->OptionalHeader.SizeOfImage - 0x1000);

	ModuleStart += 0x1000;

	PBYTE FirstMatch = nullptr;
	const char* CurPatt = Pattern;

	for (; ModuleStart < ModuleEnd; ++ModuleStart)
	{
		bool SkipByte = (*CurPatt == '\?');

		if (SkipByte || *ModuleStart == GetByte(CurPatt))
		{
			if (!FirstMatch)
				FirstMatch = ModuleStart;

			SkipByte ? CurPatt += 2 : CurPatt += 3;

			if (CurPatt[-1] == 0)
				return FirstMatch;
		}
		else if (FirstMatch)
		{
			ModuleStart = FirstMatch;
			FirstMatch = nullptr;
			CurPatt = Pattern;
		}
	}

	return nullptr;
}