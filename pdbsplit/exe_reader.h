#pragma once

struct s_exe_section
{
	uint32_t physical_offset;
	uint32_t virtual_offset;
	uint32_t virtual_size;
};

class c_exe_reader
{
private:
	uint32_t image_base;
	const uint8_t* exe_data;
	std::vector<s_exe_section> sections;

public:
	const void* get_address(uint16_t section, uint32_t offset = 0) const;
	
	template <typename T>
	inline const T* get_address(uint16_t section, uint32_t offset) const
	{
		return reinterpret_cast<const T*>(get_address(section, offset));
	}

	uint32_t get_image_base() const;
	size_t get_num_sections() const;

	c_exe_reader(const void* exe_data);
	~c_exe_reader();
};
