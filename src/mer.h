#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include "utils.h"
#include "bspentreader.h"


struct EntityEntry
{
	int index;
	std::string classname;
	std::string targetname;
};


struct Options
{
	std::string mod = "";
	std::filesystem::path gamePath;
	std::filesystem::path steamDir;
	std::filesystem::path steamCommonDir;
	std::vector<std::string> classnames;
	std::vector<std::string> keys;
	std::vector<std::string> values;
	std::vector<std::filesystem::path> globs;
	std::vector<std::filesystem::path> modDirs;
	std::unordered_map<std::filesystem::path, std::vector<EntityEntry>> entries;
	unsigned int flags = 0;
	bool globalSearch = false;
	bool flagsOr = false;
	bool exact = false;
	bool caseSensitive = false;

	void findGlobs();
	void checkMaps();
private:
	void findGlobsInPipes(std::string baseMod);
	void findGlobsInMod(std::filesystem::path modPath);
	void findAllMods();
	bool matchInList(std::string needle, const std::vector<std::string>& haystack) const;
};
extern Options g_options;

void printUsage();
