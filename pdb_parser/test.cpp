#include "public.h"

int main()
{
	wchar_t wc_current_dir[MAX_PATH] = { 0 };
	std::wstring urlfile;
	std::wstring pdbpath;
	std::wstring ws_current_dir;
	std::wstring image_path = L"C:\\Windows\\System32\\ntoskrnl.exe";
	do
	{
		if (GetCurrentDirectoryW(MAX_PATH, wc_current_dir) == 0) break;
		ws_current_dir = wc_current_dir;
		ws_current_dir += L"\\";

		if (pdb::parser::init(wc_current_dir) == false) break;

		if (pdb::parser::get_pdb_info(image_path, urlfile, pdbpath) == false) break;

		std::wstring url = L"https://msdl.microsoft.com/download/symbols";
		url = url+ L"/" + urlfile;

		std::wstring pdbFullPath = ws_current_dir + pdbpath;
		Pdb::WinInetFileDownloader downloader(pdbFullPath.c_str());
		if (Pdb::SymLoader::download(url.c_str(), downloader) == false) break;

		module_info system_module_info = { 0 };
		if (tools::get_system_module_info(tools::wstring2string(L"ntoskrnl.exe").c_str(), system_module_info) == false) 	break;

		unsigned long long base = 0;
		base = pdb::parser::load_image(image_path.c_str(), L"ntoskrnl.exe", (ULONG64)system_module_info.module_base, system_module_info.module_size);
		PVOID address = (PVOID)pdb::parser::find(L"KiNmiCallbackListHead", NULL, base, symbol_get_type::public_sybol);
		ULONG64 offset = 	pdb::parser::find(L"_EPROCESS", L"RundownProtect", base, symbol_get_type::type_offset);

	} while (false);

}