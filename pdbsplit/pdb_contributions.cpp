#include "pdbsplit-private-pch.h"

void read_pdb_contributions(
	std::vector<s_section_contribution>& contributions,
	const PDB::RawFile& raw_pdb_file,
	const PDB::DBIStream& dbi_stream)
{
	const PDB::ImageSectionStream image_section_stream = dbi_stream.CreateImageSectionStream(raw_pdb_file);
	const PDB::SectionContributionStream section_contribution_stream = dbi_stream.CreateSectionContributionStream(raw_pdb_file);

	const PDB::ArrayView<PDB::DBI::SectionContribution> section_contributions = section_contribution_stream.GetContributions();
	const size_t count = section_contributions.GetLength();

	contributions.reserve(count);

	for (const PDB::DBI::SectionContribution& contribution : section_contributions)
	{
		const uint32_t rva = image_section_stream.ConvertSectionOffsetToRVA(contribution.section, contribution.offset);
		if (rva == 0u)
		{
			printf("Contribution has invalid RVA\n");
			continue;
		}

		contributions.push_back(
			s_section_contribution
			{
				contribution.moduleIndex,
				contribution.section,
				contribution.offset,
				rva,
				contribution.size,
				contribution.characteristics,
				contribution.dataCrc
			});
	}
}

// Because nothing is perfect. #TODO: In the future, catch these properly with a herustic.
void hack_pdb_contributions(
	std::vector<s_section_contribution>& contributions)
{
	static const s_section_contribution hack_contributions[] =
	{
		// contribution NOT found for relocation in image 0045382a -> 006b7f84[0014:00001504](chunk 00053810->000538c0)
		s_section_contribution
		{
			0x01a5,
			0x0014,
			0x00001504,
			0x002b7f84,
			4,
			0,
			0
		},
		// contribution NOT found for relocation in image 004a2186 -> 0083ecac[0014:0018822c](chunk 000a2150->000a21e0)
		s_section_contribution
		{
			0x012c,
			0x0014,
			0x0018822c,
			0x0043ecac,
			4,
			0,
			0
		},
		// contribution NOT found for relocation in image 004e3010 -> 009921c4[0014:002db744](chunk 000e2fd0->000e30e0)
		s_section_contribution
		{
			0x00fa,
			0x0014,
			0x002db744,
			0x005921c4,
			4,
			0,
			0
		},
		// contribution NOT found for relocation in image 0053d0c6 -> 00990a9e[0014:002da01e](chunk 0013d090->0013d190)
		s_section_contribution
		{
			0x0089,
			0x0014,
			0x002da01e,
			0x00590a9e,
			2,
			0,
			0
		}
	};

	for (const s_section_contribution& contribution : hack_contributions)
	{
		contributions.push_back(contribution);
	}

	std::sort(contributions.begin(), contributions.end(), [](const s_section_contribution& lhs, const s_section_contribution& rhs)
		{
			return lhs.rva < rhs.rva;
		});
}

void verify_pdb_contributions(
	const std::vector<s_section_contribution>& contributions,
	const COFFI::coffi& exe_file)
{
	//const COFFI::sections& sections = exe_file.get_sections();
	//
	//for (const s_section_contribution& contribution : contributions)
	//{
	//	const COFFI::section* section = sections[contribution.debug_image_section_index-1];
	//	assert(section);
	//
	//	const uint8_t* data = reinterpret_cast<const uint8_t*>(section->get_data()) + contribution.debug_image_section_offset;
	//	uint32_t crc = crc32(data, contribution.size);
	//	assert(crc == contribution.data_crc);
	//}
}
