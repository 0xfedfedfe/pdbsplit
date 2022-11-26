cmake_minimum_required(VERSION 3.22)

add_executable(pdbsplit
	pdbsplit-private-pch.h

	memory_mapped_file.h
	memory_mapped_file.cpp

	create_directory.h
	create_directory.cpp

	crc32.h
	crc32.cpp

	binary_search.h

	pdb_contributions.h
	pdb_contributions.cpp

	pdb_symbols.h
	pdb_symbols.cpp

	chunk.h
	chunk.cpp

	exe_relocations.h
	exe_relocations.cpp

	exe_reader.h
	exe_reader.cpp

	obj_writer.h
	obj_writer.cpp
	
	main.cpp)

set_source_files_properties(memory_mapped_file.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
set_source_files_properties(create_directory.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
target_precompile_headers(pdbsplit PRIVATE pdbsplit-private-pch.h)

target_link_libraries(pdbsplit PRIVATE raw_pdb)
