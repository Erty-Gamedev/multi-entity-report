#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include "logging.h"
#include "utils.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <pwd.h>
#endif

using namespace Styling;


static std::filesystem::path getExeDir()
{
#ifdef _WIN32
    std::vector<wchar_t> pathBuffer;
    DWORD copied = 0;
    do {
        pathBuffer.resize(pathBuffer.size() + MAX_PATH);
        copied = GetModuleFileNameW(NULL, &pathBuffer.at(0), static_cast<DWORD>(pathBuffer.size()));
    } while (copied >= pathBuffer.size());
    pathBuffer.resize(copied);
    std::wstring exeDir(pathBuffer.begin(), pathBuffer.end());
    return std::filesystem::path{ exeDir }.parent_path();
#else
    return std::filesystem::canonical("/proc/self/exe").parent_path();
#endif
}
static inline const std::filesystem::path c_exedir = getExeDir();
static inline const std::filesystem::path configFilePath = c_exedir / "mer.conf";


static inline std::unordered_map<std::string, std::string> readConfigFile()
{
    std::unordered_map<std::string, std::string> config;

    std::ifstream file;
    file.open(configFilePath);
    if (!file.is_open() || !file.good())
    {
        file.close();
        return config;
    }

    std::string line, key, value;
    line.reserve(1024);
    int lineNumber = 0;
    while (std::getline(file, line))
    {
        ++lineNumber;

        // Skip comments and empty lines
        if (line.starts_with("//") || line.starts_with(";") || line.starts_with("#") || line.empty())
            continue;

        const std::vector<std::string>& parts = splitString(line, '=');
        if (parts.empty())
            continue;

        key = parts.at(0);
        trim(key);

        if (parts.size() > 1) {
            value = parts.at(1);
            trim(value);
        }
        else { value = ""; }

        config.insert_or_assign(key, value);
    }

    file.close();
    return config;
}
static inline std::unordered_map<std::string, std::string> g_configs = readConfigFile();

static inline bool saveConfigFile()
{
    std::ofstream file;
    file.open(configFilePath);
    if (!file.is_open() || !file.good())
    {
        file.close();
        return false;
    }

    for (auto& kv : g_configs)
        file << kv.first << "=" << kv.second << "\n";

    file.close();
    return true;
}


#ifdef _WIN32
static inline const std::filesystem::path c_defaultSteamDir = "C:/Program Files (x86)/Steam";
#else
struct passwd *pw = getpwuid(getuid());
static inline std::filesystem::path userHome{ pw->pw_dir };
static inline const std::filesystem::path c_defaultSteamDir = userHome / ".local/share/Steam";
static inline const std::filesystem::path c_steamDirSnap = userHome / "snap/steam/common/.local/share/Steam";
#endif
std::filesystem::path getSteamDir()
{
    if (g_configs.contains("steamdir"))
    {
        if (std::filesystem::is_directory(g_configs["steamdir"]))
            return g_configs["steamdir"];
        std::cout << style(warning) << "\"" << g_configs["steamdir"] << "\" is not a directory" << style() << std::endl;
    }

    if (std::filesystem::is_directory(c_defaultSteamDir))
    {
        std::cout << "Is " << c_defaultSteamDir << " your Steam directory? (Y/n) ";
        if (confirm_dialogue(true))
        {
            g_configs.insert_or_assign("steamdir", c_defaultSteamDir.string());
            saveConfigFile();
            return c_defaultSteamDir;
        }
    }
#ifndef _WIN32
    if (std::filesystem::is_directory(c_steamDirSnap))
    {
        std::cout << "Is " << c_steamDirSnap << " your Steam directory? (Y/n) ";
        if (confirm_dialogue(true))
        {
            g_configs.insert_or_assign("steamdir", c_steamDirSnap.string());
            saveConfigFile();
            return c_steamDirSnap;
        }
    }
#endif

    std::string buffer;
    for (int i = 0; i < 3; ++i)
    {
        std::cout << "Enter path to Steam directory:  ";

        std::getline(std::cin, buffer);
        if (std::filesystem::is_directory(buffer))
        {
            g_configs.insert_or_assign("steamdir", buffer);
            saveConfigFile();
            return buffer;
        }

        std::cout << style(warning) << "\"" << buffer << "\" is not a directory" << style() << std::endl;
    }

    std::cerr << style(error) << "Could not set Steam directory" << style() << std::endl;
    exit(EXIT_FAILURE);
}

bool confirm_dialogue(const bool yesDefault)
{
    static std::string buffer;
    std::getline(std::cin, buffer);

    if (buffer.empty()) { return yesDefault; }
    if (tolower(buffer.at(0)) == 'y') { return true; }
    return false;
}

std::string toLowerCase(std::string str)
{
    std::ranges::transform(str, str.begin(), [] (const unsigned char c) {
        return std::tolower(c);
    });
    return str;
}

std::string toUpperCase(std::string str)
{
    std::ranges::transform(str, str.begin(), [] (const unsigned char c) {
        return std::toupper(c);
    });
    return str;
}

std::string unSteampipe(std::string str)
{
    if (str.empty())
        return str;

    for (auto const& steampipe : c_SteamPipes)
    {
        if (const size_t matchPosition = str.rfind(steampipe); matchPosition != std::string::npos)
        {
            str.replace(matchPosition, steampipe.length(), "");
            return str;
        }
    }
    return str;
}

void trim(std::string& str, const char* trim)
{
    str.erase(0, str.find_first_not_of(trim));
    str.erase(str.find_last_not_of(trim) + 1);
}

std::vector<std::string> splitString(const std::string& str, const char delimiter)
{
    std::istringstream strStream{ str };
    std::vector<std::string> segments;
    std::string segment;
    while (std::getline(strStream, segment, delimiter))
    {
        segments.push_back(segment);
    }
    return segments;
}
