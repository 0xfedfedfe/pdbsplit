#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "create_directory.h"

void create_directory(const char* directory_path)
{
	CreateDirectoryA(directory_path, nullptr);
}
