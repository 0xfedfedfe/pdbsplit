#include "pdbsplit-private-pch.h"

static inline std::string create_string_from_array_view(const char* string, size_t string_length)
{
	char* string_copy = reinterpret_cast<char*>(_alloca(string_length+1));
	string_copy[string_length] = '\0';
	memcpy(string_copy, string, string_length);
	return string_copy;
}

void read_pdb_symbols(
	std::vector<s_public_symbol>& symbols,
	const PDB::RawFile& raw_pdb_file,
	const PDB::DBIStream& dbi_stream)
{
	const PDB::ImageSectionStream image_section_stream = dbi_stream.CreateImageSectionStream(raw_pdb_file);
	const PDB::CoalescedMSFStream symbol_record_stream = dbi_stream.CreateSymbolRecordStream(raw_pdb_file);
	const PDB::PublicSymbolStream public_symbol_stream = dbi_stream.CreatePublicSymbolStream(raw_pdb_file);
	{
		const PDB::ArrayView<PDB::HashRecord> hash_records = public_symbol_stream.GetRecords();

		for (const PDB::HashRecord& hash_record : hash_records)
		{
			const PDB::CodeView::DBI::Record* record = public_symbol_stream.GetRecord(symbol_record_stream, hash_record);
			if (!record)
			{
				// end of data?
				break;
			}

			const uint32_t rva = image_section_stream.ConvertSectionOffsetToRVA(
				record->data.S_PUB32.section, record->data.S_PUB32.offset);
			if (rva == 0u)
			{
				// certain symbols (e.g. control-flow guard symbols) don't have a valid RVA, ignore those
				continue;
			}

			symbols.push_back(
				s_public_symbol
				{
					create_string_from_array_view(
						record->data.S_PUB32.name.vc60.string, record->data.S_PUB32.name.vc60.length),
					record->data.S_PUB32.section,
					record->data.S_PUB32.offset,
					rva
				});
		}
	}

	// must sort symbols to binary search later
	std::sort(symbols.begin(), symbols.end(), [](const s_public_symbol& lhs, const s_public_symbol& rhs)
		{
			return lhs.rva < rhs.rva;
		});
}
