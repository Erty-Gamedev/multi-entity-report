#pragma once

#include <array>
#include <vector>
#include <filesystem>


static inline const std::array<std::string, 3> c_SteamPipes{
	"_addon", "_hd", "_downloads"
};

std::filesystem::path getSteamDir(bool resetConfig = false);

bool confirm_dialogue(bool yesDefault = true);

std::string toLowerCase(std::string str);
std::string toUpperCase(std::string str);
std::string unSteampipe(std::string str);
void trim(std::string& str, const char* trim = " \t\n\r");
std::vector<std::string> splitString(const std::string& str, char delimiter = ' ');
