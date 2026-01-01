#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>
#include <filesystem>


struct EntityEntry
{
	int index;
	std::string classname;
	std::string targetname;
};


struct Options
{
	std::filesystem::path gamePath;
	std::filesystem::path steamDir;
	std::filesystem::path steamCommonDir;
	std::vector<std::string> classnames;
	std::vector<std::string> keys;
	std::vector<std::string> values;
	std::vector<std::string> mods;
	std::vector<std::filesystem::path> globs;
	std::vector<std::filesystem::path> modDirs;
	std::unordered_map<std::filesystem::path, std::vector<EntityEntry>> entries;
	unsigned int flags = 0;
	bool globalSearch = false;
	bool flagsOr = false;
	bool exact = false;
	bool caseSensitive = false;
	bool interactiveMode = false;

	void findGlobs();
	void checkMaps();
	bool matchInList(std::string needle, const std::vector<std::string>& haystack) const;
private:
	void findGlobsInPipes(std::filesystem::path modDir);
	void findGlobsInMod(const std::filesystem::path& modDir);
	void findAllMods();
};
extern Options g_options;

void printUsage();


/*
	Based on the Unofficial BSP v30 File Spec by dixxi1 (Bernhard Gruber)
	https://web.archive.org/web/20240313170323/https://hlbsp.sourceforge.net/index.php?content=bspdef
*/
namespace BSPFormat
{
	static constexpr int c_MaxMapHulls =			  4;
	static constexpr int c_MaxMapModels =           400;
	static constexpr int c_MaxMapBrushes =         4096;
	static constexpr int c_MaxMapEntities =        1024;
	static constexpr int c_MaxMapEntString = 128 * 1024;
	static constexpr int c_MaxMapPlanes =		  32767;
	static constexpr int c_MaxMapNodes =		  32767;
	static constexpr int c_MaxMapClipNodes =	  32767;
	static constexpr int c_MaxMapLeaves =		   8192;
	static constexpr int c_MaxMapVertices =		  65535;
	static constexpr int c_MaxMapFaces =		  65535;
	static constexpr int c_MaxMapMarkSurfaces =	  65535;
	static constexpr int c_MaxMapTexInfo =		   8192;
	static constexpr int c_MaxMapEdges =		 256000;
	static constexpr int c_MaxMapSurfEdges =	 512000;
	static constexpr int c_MaxMapTextures =		    512;
	static constexpr int c_MaxMapMipTex =	   0x200000;
	static constexpr int c_MaxMapLighting =    0x200000;
	static constexpr int c_MaxMapVisibility =  0x200000;
	static constexpr int c_MaxMapPortals =		  65535;

	static constexpr int c_MaxKeyLength =	  32;
	static constexpr int c_MaxValueLength =	1024;


	enum LumpIndex
	{
		ENTITIES = 0,
		PLANES = 1,
		TEXTURES = 2,
		VERTICES = 3,
		VISIBILITY = 4,
		NODES = 5,
		TEXINFO = 6,
		FACES = 7,
		LIGHTING = 8,
		CLIPNODES = 9,
		LEAVES = 10,
		MARKSURFACES = 11,
		EDGES = 12,
		SURFEDGES = 13,
		MODELS = 14,
		HEADERLUMPS = 15
	};

#pragma pack(push, 1)
	struct BspLump
	{
		std::int32_t offset;
		std::int32_t length;
	};

	struct BspHeader
	{
		std::int32_t version;
		BspLump lumps[LumpIndex::HEADERLUMPS];
	};
#pragma pack(pop)

	using Entity = std::unordered_map<std::string, std::string>;

	class Bsp
	{
	public:
		std::filesystem::path m_filepath;

		Bsp(const std::filesystem::path& filepath);
		~Bsp() { if (m_file.is_open()) m_file.close(); }
	private:
		BspHeader m_header{};
		std::ifstream m_file;
		void parse();
		std::string readToken(int maxLength = c_MaxKeyLength);
		Entity readEntity();
		bool readComment();
	};
}
