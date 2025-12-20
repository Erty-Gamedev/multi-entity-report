#pragma once

#include <array>
#include <vector>
#include <filesystem>


static inline const std::array<std::string, 3> c_SteamPipes{
	"_addon", "_hd", "_downloads"
};

std::filesystem::path getSteamDir();

bool confirm_dialogue(const bool yesDefault = true);

std::string toLowerCase(std::string str);
std::string toUpperCase(std::string str);
std::string unSteampipe(std::string str);
void trim(std::string& str, const char* trim = " \t\n\r");
std::vector<std::string> splitString(const std::string& str, const char delimiter = ' ');
void replaceToken(std::string& str, const std::string& token, const std::string& newValue);
