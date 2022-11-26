#include "pdbsplit-private-pch.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

const void* c_exe_reader::get_address(uint16_t section, uint32_t offset) const
{
	return exe_data + sections[section].physical_offset + offset;
}

uint32_t c_exe_reader::get_image_base() const
{
	return image_base;
}

size_t c_exe_reader::get_num_sections() const
{
	return sections.size();
}

c_exe_reader::c_exe_reader(const void* _exe_data)
	: exe_data(reinterpret_cast<const uint8_t*>(_exe_data))
{
	const IMAGE_DOS_HEADER* image_dos_header = reinterpret_cast<const IMAGE_DOS_HEADER*>(exe_data);
	const IMAGE_NT_HEADERS32* image_nt_headers = reinterpret_cast<const IMAGE_NT_HEADERS32*>(exe_data + image_dos_header->e_lfanew);

	image_base = image_nt_headers->OptionalHeader.ImageBase;

	for (size_t i = 0; i < image_nt_headers->FileHeader.NumberOfSections; i++)
	{
		const IMAGE_SECTION_HEADER* section = reinterpret_cast<const IMAGE_SECTION_HEADER*>(
			reinterpret_cast<const uint8_t*>(IMAGE_FIRST_SECTION(image_nt_headers)) + i * sizeof(IMAGE_SECTION_HEADER));

		s_exe_section section_struct
		{
			section->PointerToRawData,
			section->VirtualAddress,
			section->SizeOfRawData
		};

		sections.push_back(section_struct);
	}
}

c_exe_reader::~c_exe_reader()
{

}
