#pragma once
#include <set>
#include <vector>
#include <string>
#include <atomic>
#include <csignal>
#include <fstream>
#include <unordered_map>
#include <filesystem>
#include <memory>


struct EntityEntry
{
	unsigned int index;
	unsigned int flags = 0;
	std::string classname, targetname, key, value;
	std::string queryMatches;
	bool matched = false;
};

using Entity = std::unordered_map<std::string, std::string>;


class Query
{
public:
	enum QueryType
	{
		QueryOr,
		QueryAnd
	};
	enum QueryOperator
	{
		QueryEquals,
		QueryExact,
		QueryNotEquals,
		QueryGreater,
		QueryLess,
		QueryGreaterEquals,
		QueryLessEquals
	};

	bool valid = true;
	bool elementAccess = false;
	bool valueIsNumeric = false;
	QueryType type = QueryOr;
	QueryOperator op = QueryEquals;
	unsigned int flags = 0u;
	std::string key, value;
	double valueNumeric;
	int valueIndex = 0;
	Query* next = nullptr;

	Query(const std::string_view& rawQuery);

	EntityEntry testEntity(const Entity& entity, unsigned int index = 0u) const;
	EntityEntry testChain(const Entity& entity, unsigned int index = 0u);
private:
	void parse(const std::string_view& rawQuery);
	void checkIndexedKey();
};


struct Options
{
	unsigned int flags = 0;
	unsigned int foundEntries = 0;
	bool globalSearch = false;
	bool flagsOr = false;
	bool exact = false;
	bool caseSensitive = false;
	bool interactiveMode = false;
	Query* firstQuery;
	std::vector<std::string> mods;
	std::filesystem::path gamePath;
	std::filesystem::path steamDir;
	std::filesystem::path steamCommonDir;
	std::set<std::filesystem::path> globs;
	std::vector<std::unique_ptr<Query>> queries;
	std::vector<std::filesystem::path> modDirs;
	std::unordered_map<std::filesystem::path, std::vector<EntityEntry>> entries;

	void findGlobs();
	void checkMaps() const;
private:
	void findGlobsInPipes(std::filesystem::path modDir);
	void findGlobsInMapsDir(const std::filesystem::path& mapsDir);
	void findAllMods();
};
extern Options g_options;
extern std::atomic<int> g_receivedSignal;

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
		Entities = 0,
		Planes = 1,
		Textures = 2,
		Vertices = 3,
		Visibility = 4,
		Nodes = 5,
		Texinfo = 6,
		Faces = 7,
		Lighting = 8,
		Clipnodes = 9,
		Leaves = 10,
		Marksurfaces = 11,
		Edges = 12,
		Surfedges = 13,
		Models = 14,
		Headerlumps = 15
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
		BspLump lumps[Headerlumps];
	};
#pragma pack(pop)


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
