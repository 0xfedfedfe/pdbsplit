#include "pdbsplit-private-pch.h"

static inline void populate_coff(
	const std::vector<const s_chunk*> candidate_chunks,
	const PDB::ArrayView<PDB::IMAGE_SECTION_HEADER> pdb_sections,
	COFFI::coffi& writer)
{
	writer.get_header()->set_flags(IMAGE_FILE_32BIT_MACHINE | IMAGE_FILE_LINE_NUMS_STRIPPED);

	for (const s_chunk* chunk : candidate_chunks)
	{

	}
}

void write_all_objects(
	const char* output_directory,
	const std::vector<s_chunk>& chunks,
	const PDB::RawFile& raw_pdb_file,
	const PDB::DBIStream& dbi_stream)
{
	const PDB::ImageSectionStream image_section_stream = dbi_stream.CreateImageSectionStream(raw_pdb_file);
	PDB::ArrayView<PDB::IMAGE_SECTION_HEADER> pdb_sections = image_section_stream.GetImageSections();

	const PDB::ModuleInfoStream module_info_stream = dbi_stream.CreateModuleInfoStream(raw_pdb_file);
	PDB::ArrayView<PDB::ModuleInfoStream::Module> pdb_modules = module_info_stream.GetModules();
	size_t num_modules = pdb_modules.GetLength();

	for (size_t i = 0; i < num_modules; i++)
	{
		const PDB::ModuleInfoStream::Module& module = pdb_modules[i];
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

		COFFI::coffi writer;
		writer.create(COFFI::COFFI_ARCHITECTURE_PE);
		populate_coff(candidate_chunks, pdb_sections, writer);

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
		writer.save(output_filepath);
	}
}
