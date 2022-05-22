#include "public.h"

namespace pdb
{
	namespace global
	{
		HANDLE hProcess = (HANDLE)-1;
	}
	namespace parser
	{
		const wchar_t* extractFileName(const wchar_t* const path, const size_t length) noexcept
		{
			if (!length)
			{
				return path;
			}

			const wchar_t* name = &path[length - 1];
			while ((name != path) && (*name != L'\\') && (*name != L'/'))
			{
				--name;
			}

			return name;
		}

		bool makeFullptah(SYMSRV_INDEX_INFOW info, const wchar_t delimiter, std::wstring& fullpath)
		{
			bool result = false;
			do
			{
				bool	pdb_20 = ((info.sig == info.guid.Data1) && (info.guid.Data2 == 0) && (info.guid.Data3 == 0) && (*reinterpret_cast<const uint64_t*>(info.guid.Data4) == 0)) ? false : true;
				const size_t pathLength = wcslen(info.pdbfile);
				if (pathLength == 0) break;

				wchar_t* pdbPath = info.pdbfile;
				const wchar_t* const pdbName = extractFileName(pdbPath, pathLength);
				const unsigned int age = info.age;
				std::wstringstream stream;
				if (pdb_20)
				{
					const auto& guid = info.guid;


					stream << std::uppercase << std::hex << std::setfill(L'0')
						<< pdbName
						<< delimiter
						<< std::setw(8) << guid.Data1
						<< std::setw(4) << guid.Data2
						<< std::setw(4) << guid.Data3
						<< std::setw(2) << guid.Data4[0]
						<< std::setw(2) << guid.Data4[1]
						<< std::setw(2) << guid.Data4[2]
						<< std::setw(2) << guid.Data4[3]
						<< std::setw(2) << guid.Data4[4]
						<< std::setw(2) << guid.Data4[5]
						<< std::setw(2) << guid.Data4[6]
						<< std::setw(2) << guid.Data4[7]
						<< std::setw(1) << age
						<< delimiter
						<< pdbPath;
				}
				else
				{
					const auto sig = info.guid.Data1;
					stream << std::uppercase << std::hex << std::setfill(L'0')
						<< pdbName
						<< delimiter
						<< std::setw(8) << sig
						<< std::setw(1) << age
						<< delimiter
						<< pdbPath;

				}
				fullpath = stream.str();
				result = true;
			} while (false);
			return result;
		}

		bool get_pdb_info(std::wstring path, std::wstring& url, std::wstring &pdbpath)
		{
			SYMSRV_INDEX_INFOW info = { 0 };
			bool result = false;
			do
			{
				info.sizeofstruct = sizeof(info);
				if (SymSrvGetFileIndexInfoW(path.c_str(), &info, 0) == false) break;

				if (makeFullptah(info, '/', url) == false) break;

				if (makeFullptah(info, '\\', pdbpath) == false) break;

				result = true;
			} while (false);
			return result;
		}

		bool init(const wchar_t* searchPath)
		{
			if (SymInitializeW(pdb::global::hProcess, searchPath, false) == false)	return false;
			SymSetOptions(SymGetOptions() | SYMOPT_UNDNAME | SYMOPT_DEBUG | SYMOPT_LOAD_ANYTHING);

			LoadLibraryW(L"C:\\Windows\\System32\\dbghelp.dll");
			return true;

		}

		unsigned long long  load_image(const wchar_t* path, const wchar_t* module_name, uint64_t imageBase, uint32_t imageSize)
		{
			return SymLoadModuleExW(pdb::global::hProcess, nullptr, path, module_name, imageBase, imageSize, nullptr, 0);
		}

		bool set_search_path(const wchar_t * searchPath)
		{
			return	SymSetSearchPathW(pdb::global::hProcess, searchPath);
		}

		ULONG64 find(const wchar_t* name, const wchar_t *child_name,unsigned long long base, symbol_get_type type)
		{
			ULONG64 offset = 0;
			do
			{
				if (name == nullptr) break;

				constexpr auto k_size = sizeof(SYMBOL_INFOW) + MAX_SYM_NAME * sizeof(wchar_t);
				unsigned char buf[k_size]{};
				auto* const info = reinterpret_cast<SYMBOL_INFOW*>(buf);
				info->SizeOfStruct = k_size;
				info->MaxNameLen = MAX_SYM_NAME;
				
				if (SymGetTypeFromNameW(pdb::global::hProcess, base, name, info) == false) break;


				if (type == symbol_get_type::public_sybol)
				{
					if (SymGetTypeInfo(pdb::global::hProcess, base, info->TypeIndex, TI_GET_ADDRESS, &offset) == false)	break;
				}
				if (type == symbol_get_type::type_offset)
				{
					unsigned long child_count = 0;
					if (SymGetTypeInfo(pdb::global::hProcess, base, info->TypeIndex, TI_GET_CHILDRENCOUNT, &child_count) == false)	break;

					size_t	child_size  = sizeof(TI_FINDCHILDREN_PARAMS) + sizeof(ULONG) * child_count;
					TI_FINDCHILDREN_PARAMS* child_list = (TI_FINDCHILDREN_PARAMS*)malloc (child_size);

					memset(child_list, 0, child_size);
					child_list->Count = child_count;

					if (SymGetTypeInfo(pdb::global::hProcess, base, info->TypeIndex, TI_FINDCHILDREN, child_list) == false)	break;

					for (unsigned long i = 0; i < child_count; i++)
					{
						wchar_t* wc_symbol_name = NULL;
						std::wstring ws_symbol_name;
						if (SymGetTypeInfo(pdb::global::hProcess, base, child_list->ChildId[i], TI_GET_SYMNAME, &wc_symbol_name) == false)	continue;

						ws_symbol_name = wc_symbol_name;
						if (ws_symbol_name == child_name)
						{
							unsigned long temp;
							if (SymGetTypeInfo(pdb::global::hProcess, base, child_list->ChildId[i], TI_GET_OFFSET, &temp) == false)	break;
							offset = temp;
							break;
						}
						LocalFree(wc_symbol_name);
					}
					free(child_list);
				}
			} while (false);
			return offset;
		}


	}

}

namespace tools
{
	bool get_system_module_info(const char* module_name, module_info& out_module_info)
	{
		PRTL_PROCESS_MODULES	module_info = NULL;
		ULONG retLen = 0;
		NTSTATUS nStatus = STATUS_SUCCESS;
		bool resule = false;
		PFN_ZwQuerySystemInformationType fnZwQuerySystemInformation = (PFN_ZwQuerySystemInformationType)GetProcAddress(LoadLibraryA("ntdll.dll"), "ZwQuerySystemInformation");

		do
		{
			if (fnZwQuerySystemInformation == NULL) break;

			//取出系统模块的地址
			nStatus = fnZwQuerySystemInformation(SystemModuleInformation, module_info, 0, &retLen);
			if (nStatus != STATUS_INFO_LENGTH_MISMATCH) break;

			module_info = (PRTL_PROCESS_MODULES)malloc(retLen);
			memset(module_info, 0, retLen);
			nStatus = fnZwQuerySystemInformation(SystemModuleInformation, module_info, retLen, &retLen);

			if (nStatus != STATUS_SUCCESS) break;

			for (ULONG i = 0; i < module_info->NumberOfModules; i++)
			{
				//循环从链表对比
				if (strstr((const char*)module_info->Modules[i].FullPathName, module_name))
				{
					out_module_info.module_base = module_info->Modules[i].ImageBase;
					out_module_info.module_size = module_info->Modules[i].ImageSize;
					break;
				}
			}
			resule = true;
		} while (false);
		free(module_info);

		return resule;
	}


	std::string wstring2string(std::wstring ws_wd)
	{
		std::string st_out;
		char* m_char;
		int len = WideCharToMultiByte(CP_ACP, 0, ws_wd.c_str(), (int)ws_wd.length(), NULL, 0, NULL, NULL);
		m_char = new char[len + 1];
		WideCharToMultiByte(CP_ACP, 0, ws_wd.c_str(), (int)ws_wd.length(), m_char, len, NULL, NULL);
		m_char[len] = '\0';
		st_out.assign(m_char);
		delete[]m_char;
		return st_out;
	}

	std::wstring string2wstring(std::string st_wd)
	{
		std::wstring ws_out;
		wchar_t* m_wchar;
		int len = MultiByteToWideChar(CP_ACP, 0, st_wd.c_str(), (int)st_wd.length(), NULL, 0);
		m_wchar = new wchar_t[len + 1];
		MultiByteToWideChar(CP_ACP, 0, st_wd.c_str(), (int)st_wd.length(), m_wchar, len);
		m_wchar[len] = '\0';
		ws_out.assign(m_wchar);
		delete[]m_wchar;
		return ws_out;
	}

}