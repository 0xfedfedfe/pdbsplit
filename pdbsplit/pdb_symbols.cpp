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

// nothing is perfect. #TODO: In the future, catch these properly with a herustic.
void hack_pdb_names(
	std::vector<s_public_symbol>& symbols)
{
	static const s_public_symbol hack_symbols[] =
	{
		// contribution NOT found for relocation in image 0045382a -> 006b7f84[0014:00001504](chunk 00053810->000538c0)
		s_public_symbol
		{
			"_pdbsplit_error_002B7F84",
			0x0014,
			0x00001504,
			0x002b7f84,
		},
		// contribution NOT found for relocation in image 004a2186 -> 0083ecac[0014:0018822c](chunk 000a2150->000a21e0)
		s_public_symbol
		{
			"_pdbsplit_error_0043ECAC",
			0x0014,
			0x0018822c,
			0x0043ecac,
		},
		// contribution NOT found for relocation in image 004e3010 -> 009921c4[0014:002db744](chunk 000e2fd0->000e30e0)
		s_public_symbol
		{
			"_pdbsplit_error_005921C4",
			0x0014,
			0x002db744,
			0x005921c4,
		},
		// contribution NOT found for relocation in image 0053d0c6 -> 00990a9e[0014:002da01e](chunk 0013d090->0013d190)
		s_public_symbol
		{
			"_pdbsplit_error_00590A9E",
			0x0014,
			0x002da01e,
			0x00590a9e,
		}
	};

	for (const s_public_symbol& symbol : hack_symbols)
	{
		symbols.push_back(symbol);
	}

	// must sort symbols to binary search later
	std::sort(symbols.begin(), symbols.end(), [](const s_public_symbol& lhs, const s_public_symbol& rhs)
		{
			return lhs.rva < rhs.rva;
		});
}
