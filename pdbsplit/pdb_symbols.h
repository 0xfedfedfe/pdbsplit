#pragma once

struct s_public_symbol
{
	std::string name;
	uint16_t debug_image_section_index;
	uint32_t debug_image_section_offset;
	uint32_t rva;
};

void read_pdb_symbols(
	std::vector<s_public_symbol>& symbols,
	const PDB::RawFile& raw_pdb_file,
	const PDB::DBIStream& dbi_stream);

void hack_pdb_names(
	std::vector<s_public_symbol>& symbols);
