#pragma once

void write_all_objects(
	const char* output_directory,
	const std::vector<s_chunk>& chunks,
	const PDB::RawFile& raw_pdb_file,
	const PDB::DBIStream& dbi_stream);
