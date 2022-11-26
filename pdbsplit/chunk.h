#pragma once

// for specifying externs...
// binary search for chunk with rva == chunk_rva
// then with chunk offset pick appropiate label to offset from
struct s_chunk_reloc_reference
{
	uint32_t offset;
	uint32_t chunk_rva;
	uint32_t chunk_offset;
};

// section contributions may hold multiple names
struct s_chunk_name_offset
{
	std::string string;
	uint32_t offset;
	bool is_public;
};

struct s_chunk
{
	// contribution
	uint16_t object_file_index;
	uint16_t debug_image_section_index;
	uint32_t debug_image_section_offset;
	uint32_t rva;
	uint32_t size;
	uint32_t characteristics;

	// public symbol(s)
	std::vector<s_chunk_name_offset> names;

	// chunk data
	const uint8_t* data;
	std::vector<s_chunk_reloc_reference> relocations;
};

void populate_chunks(
	std::vector<s_chunk>& chunks,
	const std::vector<s_public_symbol>& symbols,
	const std::vector<s_section_contribution>& contributions,
	const std::vector<uint32_t>& reloc_rvas,
	const c_exe_reader& exe_reader,
	const PDB::RawFile& raw_pdb_file,
	const PDB::DBIStream& dbi_stream);

void analyise_chunk_usage(
	std::vector<s_chunk>& chunks);

void dump_chunks(
	const std::vector<s_chunk>& chunks,
	const PDB::ModuleInfoStream& module_info_stream);
