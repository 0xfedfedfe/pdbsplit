#pragma once

struct s_section_contribution
{
	uint16_t object_file_index;
	uint16_t debug_image_section_index;
	uint32_t debug_image_section_offset;
	uint32_t rva;
	uint32_t size;
	uint32_t characteristics;
	uint32_t data_crc;
};

void read_pdb_contributions(
	std::vector<s_section_contribution>& contributions,
	const PDB::RawFile& raw_pdb_file,
	const PDB::DBIStream& dbi_stream);

void hack_pdb_contributions(
	std::vector<s_section_contribution>& contributions);

void verify_pdb_contributions(
	const std::vector<s_section_contribution>& contributions,
	const COFFI::coffi& exe_file);
