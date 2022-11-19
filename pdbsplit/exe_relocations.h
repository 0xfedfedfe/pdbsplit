#pragma once

void read_exe_relocations(
	std::vector<uint32_t>& reloc_rvas,
	const COFFI::section* reloc_section);
