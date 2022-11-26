#include "pdbsplit-private-pch.h"

static inline uint32_t binary_search_get_public_symbol_rva(
	const s_public_symbol* data,
	unsigned int search_middle)
{
	return data[search_middle].rva;
}

static inline void fill_public_names_for_range(
	const std::vector<s_public_symbol>& symbols,
	std::vector<s_chunk_name_offset>& names,
	uint32_t start, 
	uint32_t end)
{
	const s_public_symbol* data = symbols.data();
	const s_public_symbol* head = nullptr;

	_binary_search(data, symbols.size(), binary_search_get_public_symbol_rva, start, head);

	// 99% of cases: public name at the start of a section contribution
	if (head)
	{
		while (head->rva < end)
		{
			names.push_back(s_chunk_name_offset{ head->name, head->rva - start, true });
			head++;
		}
	}
	else
	{
		bool active = false;
		for (const s_public_symbol& symbol : symbols)
		{
			if (symbol.rva >= start)
			{
				active = true;
			}

			if (symbol.rva < end)
			{
				if (active)
				{
					names.push_back(s_chunk_name_offset{ symbol.name, symbol.rva - start, true });
				}
			}
			else
			{
				break;
			}
		}
	}
}

static inline void get_offset_contribution_for_rva(
	const std::vector<s_section_contribution>& contributions,
	uint32_t rva,
	uint32_t& offset_out,
	uint32_t& contribution_rva_out,
	uint32_t& last_contribution_size_out)
{
	offset_out = 0;
	contribution_rva_out = 0;
	last_contribution_size_out = 0;

	for (const s_section_contribution& contribution : contributions)
	{
		if (rva >= contribution.rva && rva < contribution.rva + contribution.size)
		{
			contribution_rva_out = contribution.rva;
			offset_out = rva - contribution.rva;
			last_contribution_size_out = contribution.size;
			break;
		}
	}
}

static inline uint32_t get_reloc_destination(
	uint32_t reloc_rva,
	const PDB::ImageSectionStream& image_section_stream,
	const c_exe_reader& exe_reader,
	uint32_t image_base)
{
	uint16_t one_based_section_index;
	uint32_t offset_in_section;
	assert(image_section_stream.ConvertRVAToSectionOffset(reloc_rva, one_based_section_index, offset_in_section));

	return *exe_reader.get_address<uint32_t>(one_based_section_index - 1, offset_in_section) - image_base;
}

static inline void fill_relocations_for_range(
	const PDB::ImageSectionStream& image_section_stream,
	const c_exe_reader& exe_reader,
	const std::vector<uint32_t>& reloc_rvas,
	const std::vector<s_section_contribution>& contributions,
	std::vector<s_chunk_reloc_reference>& chunk_relocations,
	uint32_t start,
	uint32_t end)
{
	uint32_t image_base = exe_reader.get_image_base();
	bool active = false;
	uint32_t offset = 0;
	uint32_t contribution_rva = 0;
	uint32_t last_contribution_size = 0;
	for (const uint32_t& rva : reloc_rvas)
	{
		if (rva >= start)
		{
			active = true;
		}

		if (rva < end)
		{
			if (active)
			{
				uint32_t dest = get_reloc_destination(rva, image_section_stream, exe_reader, image_base);

				if (dest >= contribution_rva && dest < contribution_rva + last_contribution_size)
				{
					// fast path, we are still in this contribution
					chunk_relocations.push_back(s_chunk_reloc_reference{ rva - start, contribution_rva, dest - contribution_rva });
				}
				else
				{
					// slow path, must find target contribution
					get_offset_contribution_for_rva(contributions, dest, offset, contribution_rva, last_contribution_size);
					if (!contribution_rva)
					{
						uint16_t one_based_section_index;
						uint32_t offset_in_section;
						assert(image_section_stream.ConvertRVAToSectionOffset(dest, one_based_section_index, offset_in_section));

						printf("contribution NOT found for relocation in image %08x -> %08x [%04hx:%08x] (chunk %08x->%08x)\n", rva + image_base, dest + image_base, one_based_section_index, offset_in_section, start, end);
					}

					chunk_relocations.push_back(s_chunk_reloc_reference{ rva - start, contribution_rva, offset });
				}
			}
		}
		else
		{
			break;
		}
	}
}

void populate_chunks(
	std::vector<s_chunk>& chunks,
	const std::vector<s_public_symbol>& symbols,
	const std::vector<s_section_contribution>& contributions,
	const std::vector<uint32_t>& reloc_rvas,
	const c_exe_reader& exe_reader,
	const PDB::RawFile& raw_pdb_file,
	const PDB::DBIStream& dbi_stream)
{
	const PDB::ImageSectionStream image_section_stream = dbi_stream.CreateImageSectionStream(raw_pdb_file);

	puts("Populating chunks");

	for (const s_section_contribution& contribution : contributions)
	{
		s_chunk chunk
		{
			contribution.object_file_index,
			contribution.debug_image_section_index,
			contribution.debug_image_section_offset,
			contribution.rva,
			contribution.size,
			contribution.characteristics
		};

		chunk.data = exe_reader.get_address<uint8_t>(chunk.debug_image_section_index - 1, chunk.debug_image_section_offset);

		fill_public_names_for_range(symbols, chunk.names, chunk.rva, chunk.rva + chunk.size);
		fill_relocations_for_range(
			image_section_stream,
			exe_reader,
			reloc_rvas,
			contributions,
			chunk.relocations,
			chunk.rva,
			chunk.rva + chunk.size);

		if (!chunk.names.size() || chunk.names[0].offset != 0)
		{
			char never_should_have_put_std_string_in[32];
			sprintf(never_should_have_put_std_string_in, "_unk_%08X", chunk.rva);
			chunk.names.emplace(chunk.names.begin(), s_chunk_name_offset{ never_should_have_put_std_string_in, 0, false });
		}

		chunks.push_back(chunk);
	}

	printf("%zu chunks\n", chunks.size());
}

static inline uint32_t binary_search_get_chunk_rva(
	const s_chunk* data,
	unsigned int search_middle)
{
	return data[search_middle].rva;
}

void analyise_chunk_usage(
	std::vector<s_chunk>& chunks)
{
	puts("Analysing chunk usage");

	// weak functions can have the code in one module, but a piece of data in another module
	// and the data will not have a public name and so be marked private despite being accessed by other modules.
	// go over every chunk's relocations and check that the dest chunk has the same object id
	// if the name is private. if it doesn't, mark the name as public.
	for (const s_chunk& chunk : chunks)
	{
		for (const s_chunk_reloc_reference& reference : chunk.relocations)
		{
			s_chunk* found_chunk = nullptr;
			s_chunk_name_offset* found_name = nullptr;
			_binary_search(chunks.data(), chunks.size(), binary_search_get_chunk_rva, reference.chunk_rva, found_chunk);
			assert(found_chunk);

			for (size_t i = found_chunk->names.size(); i > 0; i--)
			{
				s_chunk_name_offset* name = &found_chunk->names[i - 1];

				if (reference.chunk_offset >= name->offset)
				{
					found_name = name;
					break;
				}
			}
			assert(found_name);

			if (!found_name->is_public && found_chunk->object_file_index != chunk.object_file_index)
			{
				printf("Corrected access status of %s\n", found_name->string.c_str());
				found_name->is_public = true;
			}
		}
	}
}

void dump_chunks(
	const std::vector<s_chunk>& chunks,
	const PDB::ModuleInfoStream& module_info_stream)
{
	for (const s_chunk& chunk : chunks)
	{
		const PDB::ModuleInfoStream::Module& module = module_info_stream.GetModule(chunk.object_file_index);

		printf("chunk [%04hx:%08x] \"%s\" \"%s\" ->\n",
			chunk.debug_image_section_index,
			chunk.debug_image_section_offset,
			module.GetObjectName().Decay(),
			module.GetName().Decay());
		printf("\trva %08x size %08x characteristics %08x\n", chunk.rva, chunk.size, chunk.characteristics);

		puts("\tsymbols\n\t{");
		for (const s_chunk_name_offset& name : chunk.names)
		{
			printf("\t\t+%04x (%08x) \"%s\" %s,\n",
				name.offset,
				chunk.rva + name.offset,
				name.string.data(),
				name.is_public ? "public" : "private");
		}
		puts("\t}");

		puts("\trelocations\n\t{");
		for (const s_chunk_reloc_reference& reference : chunk.relocations)
		{
			const s_chunk* found_chunk = nullptr;
			_binary_search(chunks.data(), chunks.size(), binary_search_get_chunk_rva, reference.chunk_rva, found_chunk);
			assert(found_chunk);

			const char* referenced_public_name = "<no-name>";
			size_t referenced_name_offset = reference.chunk_rva;
			size_t referenced_offset = reference.chunk_offset;
			bool is_public = false;

			for (size_t i = found_chunk->names.size(); i > 0; i--)
			{
				const s_chunk_name_offset& name = found_chunk->names[i-1];

				if (reference.chunk_offset >= name.offset)
				{
					referenced_public_name = name.string.data();
					referenced_name_offset = reference.chunk_rva + name.offset;
					referenced_offset = reference.chunk_offset - name.offset;
					is_public = name.is_public;

					break;
				}
			}

			if (!is_public)
			{
				assert(found_chunk->object_file_index == chunk.object_file_index);
			}

			printf("\t\t+%04x (%08x) \"%s\" %s %08zx + %04zx,\n",
				reference.offset,
				chunk.rva + reference.offset,
				referenced_public_name,
				is_public ? "public" : "private",
				referenced_name_offset,
				referenced_offset);
		}
		puts("\t}");
	}
}
