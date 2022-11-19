#include "pdbsplit-private-pch.h"

struct s_image_base_relocation
{
	uint32_t virtual_address;
	uint32_t size_of_block;
};

void read_exe_relocations(
	std::vector<uint32_t>& reloc_rvas,
	const COFFI::section* reloc_section)
{
    const s_image_base_relocation* header = reinterpret_cast<const s_image_base_relocation*>(reloc_section->get_data());

    while (header->size_of_block != 0)
    {
        //printf("block vaddr %08x sizeof block %08x\n", header->virtual_address, header->size_of_block);

        for (size_t i = 0; ; i++)
        {
            const uint16_t* offset_ptr = reinterpret_cast<const uint16_t*>(
                reinterpret_cast<const uint8_t*>(header) + sizeof(s_image_base_relocation) + (i * 2));
            uint16_t ptr = *offset_ptr & 0x0fff;

            //printf("%p %04hx %08lx\n", offset_ptr, ptr, header->virtual_address + ptr);

            if (!ptr && i != 0)
            {
                break;
            }

            reloc_rvas.push_back(header->virtual_address + ptr);
        }

        header = reinterpret_cast<const s_image_base_relocation*>(
            reinterpret_cast<const uint8_t*>(header) + header->size_of_block);
    }

    printf("%zu relocations\n", reloc_rvas.size());
}
