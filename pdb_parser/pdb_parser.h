#pragma once

typedef NTSTATUS(*PFN_ZwQuerySystemInformationType)(
	__in       ULONG SystemInformationClass,
	__inout    PVOID SystemInformation,
	__in       ULONG SystemInformationLength,
	__out_opt  PULONG ReturnLength
	);

typedef struct _RTL_PROCESS_MODULE_INFORMATION {
	HANDLE Section;                 // Not filled in
	PVOID MappedBase;
	PVOID ImageBase;
	ULONG ImageSize;
	ULONG Flags;
	USHORT LoadOrderIndex;
	USHORT InitOrderIndex;
	USHORT LoadCount;
	USHORT OffsetToFileName;
	UCHAR  FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, * PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES {
	ULONG NumberOfModules;
	RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, * PRTL_PROCESS_MODULES;

typedef struct _module_info
{
	void* module_base;
	ULONG module_size;
}module_info;

namespace tools
{
	bool get_system_module_info(const char* module_name, module_info& out_module_info);

	std::string wstring2string(std::wstring ws_wd);
	std::wstring string2wstring(std::string st_wd);
}
#define  SystemModuleInformation 11
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L)    


enum class symbol_get_type
{
	public_sybol,
	type_offset
};

namespace pdb
{
	namespace parser
	{

		bool init(const wchar_t* searchPath);
		unsigned long long  load_image(const wchar_t* path, const wchar_t* module_name, uint64_t imageBase, uint32_t imageSize);
		bool set_search_path(const wchar_t* searchPath);
		ULONG64 find(const wchar_t* name, const wchar_t* child_name, unsigned long long base, symbol_get_type type);
		bool get_pdb_info(std::wstring path, std::wstring& url, std::wstring& pdbpath);
	}
}
