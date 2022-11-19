// Copyright 2011-2022, Molecular Matters GmbH <office@molecular-matters.com>
// See LICENSE.txt for licensing details (2-clause BSD License: https://opensource.org/licenses/BSD-2-Clause)

#pragma once

#ifndef _WIN32
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define INVALID_HANDLE_VALUE ((long)-1)
#endif

struct s_memory_mapped_file_handle
{
#ifdef _WIN32
	void* file;
	void* file_mapping;
#else
	int   file;
#endif
	void* base_address;
#ifndef _WIN32
	long len;
#endif
};

s_memory_mapped_file_handle open_memmapped_file(const char* path);
void close_memmapped_file(s_memory_mapped_file_handle& handle);
