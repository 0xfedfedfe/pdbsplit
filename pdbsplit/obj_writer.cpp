#include "pdbsplit-private-pch.h"

struct s_coff_symbol
{
	uint32_t original_rva;
	uint32_t obj_offset;

	const char* name;
	uint32_t flags;
	bool is_public;
};

struct s_coff_relocation
{
	uint32_t dest_chunk_rva;
	uint32_t dest_chunk_offset;
	uint32_t obj_offset;
};

struct s_coff_section_data
{
	uint16_t original_debug_image_section_index;
	std::vector<uint8_t> data;
	std::vector<s_coff_symbol> symbols;
	std::vector<s_coff_relocation> relocations;
};

struct s_coff_data
{
	s_coff_section_data* get_section(size_t debug_image_section_index)
	{
		for (s_coff_section_data& section : sections)
		{
			if (section.original_debug_image_section_index == debug_image_section_index)
			{
				return &section;
			}
		}

		return nullptr;
	}

	std::vector<s_coff_section_data> sections;
};

static inline void populate_obj(
	s_coff_data& data,
	const std::vector<s_chunk>& chunks,
	const std::vector<const s_chunk*> candidate_chunks,
	const PDB::ArrayView<PDB::IMAGE_SECTION_HEADER> pdb_sections,
	const c_exe_reader& exe_reader)
{
	//writer.get_header()->set_flags(IMAGE_FILE_32BIT_MACHINE | IMAGE_FILE_LINE_NUMS_STRIPPED);

	for (const s_chunk* chunk : candidate_chunks)
	{
		s_coff_section_data* section = data.get_section(chunk->debug_image_section_index);
		if (!section)
		{
			s_coff_section_data new_section
			{
				chunk->debug_image_section_index
			};

			data.sections.push_back(new_section);
			section = &data.sections[data.sections.size()-1];
		}

		uint32_t base_offset = section->data.size();

		for (const s_chunk_name_offset& name : chunk->names)
		{
			s_coff_symbol symbol
			{
				chunk->rva + name.offset,
				base_offset + name.offset,

				name.string.data(),
				chunk->characteristics,
				name.is_public
			};

			section->symbols.push_back(symbol);
		}

		for (const s_chunk_reloc_reference& reloc : chunk->relocations)
		{
			s_coff_relocation relocation
			{
				reloc.chunk_rva,
				reloc.chunk_offset,
				base_offset + reloc.offset
			};

			section->relocations.push_back(relocation);
		}

		section->data.resize(base_offset + chunk->size);
		memcpy(&section->data[base_offset], exe_reader.get_address(chunk->debug_image_section_index-1, chunk->debug_image_section_offset), chunk->size);
	}
}

void write_all_objects(
	const char* output_directory,
	const std::vector<s_chunk>& chunks,
	const PDB::RawFile& raw_pdb_file,
	const PDB::DBIStream& dbi_stream,
	const c_exe_reader& exe_reader)
{
	const PDB::ImageSectionStream image_section_stream = dbi_stream.CreateImageSectionStream(raw_pdb_file);
	PDB::ArrayView<PDB::IMAGE_SECTION_HEADER> pdb_sections = image_section_stream.GetImageSections();

	const PDB::ModuleInfoStream module_info_stream = dbi_stream.CreateModuleInfoStream(raw_pdb_file);
	PDB::ArrayView<PDB::ModuleInfoStream::Module> pdb_modules = module_info_stream.GetModules();
	size_t num_modules = pdb_modules.GetLength();

	for (size_t i = 0; i < num_modules; i++)
	{
		const PDB::ModuleInfoStream::Module& module = pdb_modules[i];
		// i have stopped caring
		std::string object_name = module.GetName().Decay();
		std::string library_name = module.GetObjectName().Decay();

		if (object_name == library_name)
		{
			library_name = "halobetacache";
		}
		else
		{
			library_name = library_name.substr(library_name.find_last_of("/\\") + 1);
		}
		object_name = object_name.substr(object_name.find_last_of("/\\") + 1);

		bool is_linker_common = false;
		if (!library_name.length())
		{
			is_linker_common = true;
			object_name = "linker_common.obj";
		}

		bool is_export_def = false;
		if (object_name.find(".obj") == std::string::npos)
		{
			is_export_def = true;
		}

		printf("[%04zx] \"%s%s%s\"%s",
			i,
			library_name.c_str(),
			is_linker_common ? "" : "/",
			object_name.c_str(),
			is_export_def ? " (export definition, skipped)\n" : "");
		if (is_export_def)
		{
			continue;
		}

		std::vector<const s_chunk*> candidate_chunks;
		for (const s_chunk& chunk : chunks)
		{
			if (chunk.object_file_index == i)
			{
				candidate_chunks.push_back(&chunk);
			}
		}

		printf(" -> %zu chunk(s)\n", candidate_chunks.size());
		if (!candidate_chunks.size())
		{
			continue;
		}

		s_coff_data data;
		populate_obj(data, chunks, candidate_chunks, pdb_sections, exe_reader);

		if (!is_linker_common)
		{
			char output_filedir[260];
			snprintf(
				output_filedir,
				sizeof(output_filedir),
				"%s/%s",
				output_directory,
				library_name.c_str());
			create_directory(output_filedir);
		}

		char output_filepath[260];
		snprintf(
			output_filepath,
			sizeof(output_filepath),
			"%s/%s%s%s",
			output_directory,
			library_name.c_str(),
			is_linker_common ? "" : "/",
			object_name.c_str());

		//writer.save(output_filepath);
	}
}
