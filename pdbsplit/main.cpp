#include "pdbsplit-private-pch.h"

int main(int argc, char* argv[])
{
	assert(argc == 4);

	COFFI::coffi exe_file;
	assert(exe_file.load(argv[1]));
	assert(exe_file.get_architecture() == COFFI::COFFI_ARCHITECTURE_PE);

	s_memory_mapped_file_handle pdb_file;
	pdb_file = open_memmapped_file(argv[2]);
	assert(PDB::ValidateFile(pdb_file.base_address) == PDB::ErrorCode::Success);

	const PDB::RawFile raw_pdb_file = PDB::CreateRawFile(pdb_file.base_address);
	const PDB::DBIStream dbi_stream = PDB::CreateDBIStream(raw_pdb_file);

	std::vector<s_section_contribution> contributions;
	read_pdb_contributions(contributions, raw_pdb_file, dbi_stream);
	hack_pdb_contributions(contributions);
	verify_pdb_contributions(contributions, exe_file);

	std::vector<s_public_symbol> symbols;
	read_pdb_symbols(symbols, raw_pdb_file, dbi_stream);

	std::vector<uint32_t> reloc_rvas;
	const COFFI::sections& exe_sections = exe_file.get_sections();
	read_exe_relocations(reloc_rvas, exe_sections[exe_sections.size()-1]);

	std::vector<s_chunk> chunks;
	populate_chunks(chunks, symbols, contributions, reloc_rvas, exe_file, raw_pdb_file, dbi_stream);

	const PDB::ModuleInfoStream module_info_stream = dbi_stream.CreateModuleInfoStream(raw_pdb_file);
	dump_chunks(chunks, module_info_stream);

	write_all_objects(argv[3], chunks, raw_pdb_file, dbi_stream);

	close_memmapped_file(pdb_file);

	return EXIT_SUCCESS;
}
