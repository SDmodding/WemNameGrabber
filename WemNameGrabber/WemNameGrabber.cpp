#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <Windows.h>
#include <map>
#include <string>

#include "SimpleXMLWriter.hpp"

void OnExit()
{
	printf("\nPress any key to exit...");
	int m_Dummy = getchar();
}

int main()
{
	SetConsoleTitleA("WemNameGrabber");
	atexit(OnExit);

	WIN32_FIND_DATAA m_FindData = { 0 };
	HANDLE m_FindHandle = FindFirstFileA("Files\\*.wem", &m_FindData);
	if (!m_FindHandle || m_FindHandle == INVALID_HANDLE_VALUE)
	{
		printf("Couldn't open 'Files\\.'");
		return 1;
	}

	std::map<std::string, uint32_t> m_WemFileNames;
	size_t m_NumWemNamesFound = 0;
	size_t m_NumWemFiles = 0;

	while (1)
	{
		char m_FilePath[MAX_PATH];
		sprintf_s(m_FilePath, sizeof(m_FilePath), "Files\\%s", m_FindData.cFileName);

		char m_FileHashString[16];
		memset(m_FileHashString, 0, sizeof(m_FileHashString));
		memcpy(m_FileHashString, m_FindData.cFileName, reinterpret_cast<size_t>(strchr(m_FindData.cFileName, '.')) - reinterpret_cast<size_t>(m_FindData.cFileName));

		uint32_t m_FileHash = strtoul(m_FileHashString, nullptr, 0);

		FILE* m_File = fopen(m_FilePath, "rb");
		if (m_File)
		{
			uint8_t m_Bytes[0x200];
			fread(m_Bytes, sizeof(uint8_t), sizeof(m_Bytes), m_File);

			fclose(m_File);

			const char* m_DataString = "data";
			uintptr_t m_LatestDataOffset = 0;
			for (uintptr_t i = 0; (sizeof(m_Bytes) - sizeof(m_DataString)) > i; ++i)
			{
				if (strncmp(reinterpret_cast<char*>(&m_Bytes[i]), m_DataString, sizeof(m_DataString)) == 0)
					m_LatestDataOffset = i;
			}

			uintptr_t m_StringBeginOffset = 0, m_StringEndOffset = 0;

			for (uintptr_t i = (m_LatestDataOffset - 1); i != 0; --i)
			{
				if (m_Bytes[i] == '\0')
				{
					if (m_StringEndOffset)
					{
						m_StringBeginOffset = i + 1;
						break;
					}
				}
				else if (!m_StringEndOffset)
					m_StringEndOffset = i;
			}

			const char* m_String = reinterpret_cast<char*>(&m_Bytes[m_StringBeginOffset]);
			size_t m_StringLength = strlen(m_String);

			bool m_IsValidString = true;
			for (size_t i = 0; m_StringLength > i; ++i)
			{
				char m_Char = m_String[i];
				if (m_Char >= ' ' && m_Char <= 'z')
					continue;

				m_IsValidString = false;
				break;
			}

			if (m_IsValidString)
			{
				m_WemFileNames[m_String] = m_FileHash;

				++m_NumWemNamesFound;
				printf("%u - %s\n", m_FileHash, m_String);
			}
			else
				m_WemFileNames["Unknown_" + std::string(m_FileHashString)] = m_FileHash;
		}

		++m_NumWemFiles;
		if (!FindNextFileA(m_FindHandle, &m_FindData))
			break;
	}

	FindClose(m_FindHandle);

	printf("Found {%d/%d} names\n", m_NumWemNamesFound, m_NumWemFiles);
	if (m_WemFileNames.size() > 0)
	{
		printf("Outputting to file...\n");

		CXMLWriter m_Writer;
		CXMLWriterNode& m_RootNode = m_Writer["WemFiles"];
			
		for (auto m_Pair : m_WemFileNames)
		{
			CXMLWriterNode& m_Node = m_RootNode[&std::string("File##" + m_Pair.first)[0]];
			m_Node("name", &m_Pair.first[0]);
			m_Node("id", &std::to_string(m_Pair.second)[0]);
		}

		m_Writer.OutputFile("output.xml");
	}

	return 0;
}