#include <sstream>
#include "bspentreader.h"
#include "utils.h"

using namespace BSPFormat;



Bsp::Bsp(const std::filesystem::path& filepath) {
	m_filepath = filepath;
	m_file.open(filepath, std::ios::binary);
	if (!m_file.is_open() || !m_file.good())
	{
		m_file.close();
		throw std::runtime_error("Could not open " + filepath.string());
	}

	m_file.read(reinterpret_cast<char*>(&m_header), sizeof(BspHeader));

	if (m_header.version != 30)
		throw std::runtime_error("Unexpected BSP version: " + std::to_string(m_header.version));

	parse();
	m_file.close();
}

void Bsp::parse()
{
	BspLump& entLump = m_header.lumps[LumpIndex::ENTITIES];

	m_file.seekg(entLump.offset, std::ios::beg);
	int lumpEnd = entLump.offset + entLump.length;
	std::string lineBuffer;
	lineBuffer.reserve(1024);

	while (std::getline(m_file, lineBuffer) && m_file.tellg() < lumpEnd)
	{
		if (lineBuffer.starts_with('{'))
			readEntity();
	}
}

void Bsp::readEntity()
{
	Entity entity;

	std::string lineBuffer;
	lineBuffer.reserve(512);
	while (std::getline(m_file, lineBuffer))
	{
		if (lineBuffer.starts_with('"'))
		{
			const auto& parts = splitString(lineBuffer, '"');
			if (parts.size() != 4)
				throw std::runtime_error("Invalid entity property: \"" + lineBuffer + "\"");

			entity.insert_or_assign(parts.at(1), parts.at(3));
			continue;
		}

		if (lineBuffer.starts_with('}'))
			break;

		throw std::runtime_error("Unexpected entity data: \"" + lineBuffer + "\"");
	}

	entities.push_back(entity);
}
