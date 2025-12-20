#include <vector>
#include <filesystem>
#include "mer.h"
#include "utils.h"
#include "logging.h"
#include "bspentreader.h"


namespace fs = std::filesystem;
using namespace Styling;


void printUsage()
{
#ifdef _WIN32
    std::cout << "Usage: mer.exe maps_dir <search query> [options...]\n\n";
#else
    std::cout << "Usage: mer maps_dir <search query> [options...]\n\n";
#endif
    std::cout << style(bold) << "REQUIRED ARGUMENTS\n" << style()
        << " * MAPS DIR\t\t(path)\t"
        << "path to the maps directory e.g. \".../steamapps/Half-Life/valve/maps\"\n\n"

        << style(bold) << "SEARCH QUERY\n" << style()
        << "  --classname\t-c\t\t"
        << "classnames that must match\n"
        << "  --key\t-k\t\t"
        << "keys that must match\n"
        << "  --value\t-v\t\t"
        << "values that must match\n"
        << "  --flags\t-f\t\t"
        << "spawnflags that must match (ALL must match unless --flags_or is used)\n"

        << style(bold) << "OPTIONS\n" << style()
        << "  --flags_or\t-o\t\t"
        << "change spawnflag check mode to ANY\n"
        << "  --exact\t-e\t\t"
        << "matches must be exact (whole term)\n"
        << "  --case\t-s\t\t"
        << "make matches case sensitive\n"
        << "  --version\t\t\t"
        << "print application version\n"
        << "  --help\t-h\t\t"
        << "print this message and exit"
        << std::endl;
}

void Options::findGlobsInMod(fs::path modPath)
{
    if (!fs::is_directory(modPath / "maps"))
        return;

    for (const auto& entry : fs::directory_iterator(modPath / "maps"))
    {
        fs::path entryPath = entry.path();
        if (toLowerCase(entryPath.extension().string()) == ".bsp")
            globs.push_back(entryPath);
    }
}

void Options::findGlobsInPipes(std::string baseMod)
{
    fs::path modPath = gamePath / baseMod;
    if (fs::is_directory(modPath))
        findGlobsInMod(modPath);

    for (const auto& pipe : c_SteamPipes)
    {
        modPath = gamePath / (baseMod + pipe);
        if (fs::is_directory(modPath))
            findGlobsInMod(modPath);
    }
}

void Options::findAllMods()
{
    if (fs::is_directory(steamCommonDir / "Sven Co-op" / "svencoop"))
        modDirs.emplace_back(steamCommonDir / "Sven Co-op" / "svencoop");

    if (!fs::is_directory(steamCommonDir / "Half-Life"))
        return;

    for (const auto& entry : fs::directory_iterator(steamCommonDir / "Half-Life"))
    {
        if (!fs::exists(steamCommonDir / "Half-Life" / entry / "liblist.gam"))
            continue;

        modDirs.emplace_back(steamCommonDir / "Half-Life" / entry);
    }
}

bool Options::matchInList(std::string needle, const std::vector<std::string>& haystack) const
{
    if (!caseSensitive)
        needle = toLowerCase(needle);

    if (exact)
        return std::find(haystack.begin(), haystack.end(), needle) != haystack.end();

    for (std::string query : haystack)
    {
        if (needle.starts_with(query))
            return true;
    }

    return false;
}

void Options::findGlobs()
{
    if (globalSearch)
    {
        findAllMods();
        for (const auto& modDir : modDirs)
        {
            gamePath = modDir.parent_path();
            mod = modDir.stem().string();
            findGlobsInPipes(mod);
        }
        return;
    }

    findGlobsInPipes(unSteampipe(mod));
}

void Options::checkMaps()
{
    using namespace BSPFormat;

    for (const auto& glob : globs)
    {
#ifdef _DEBUG
        std::cout << "\r\033[0K" << "Reading " << glob.filename();
#endif

        try
        {
            const Bsp reader{ glob };

            for (int i = 0; i < reader.entities.size(); ++i)
            {
                const auto& entity = reader.entities[i];

                if (!classnames.empty())
                    if (!matchInList(entity.at("classname"), classnames))
                        continue;

                if (!keys.empty())
                {
                    bool skip = true;
                    for (const auto& [key, value] : entity)
                    {
                        if (matchInList(key, keys))
                        {
                            skip = false;
                            break;
                        }
                    }
                    if (skip)
                        continue;
                }

                if (!values.empty())
                {
                    bool skip = true;
                    for (const auto& [key, value] : entity)
                    {
                        if (matchInList(value, values))
                        {
                            skip = false;
                            break;
                        }
                    }
                    if (skip)
                        continue;
                }

                if (flags > 0)
                {
                    int spawnflags = 0;
                    if (entity.contains("spawnflags"))
                        spawnflags = std::stoi(entity.at("spawnflags"));
                    int matches = spawnflags & flags;
                    if (matches == 0 || (!flagsOr && matches != flags))
                        continue;
                }

                if (!entries.contains(glob))
                    entries.insert_or_assign(glob, std::vector<EntityEntry>{});

                std::string targetname = entity.contains("targetname") ? entity.at("targetname") : "";
                entries.at(glob).emplace_back(i, entity.at("classname"), targetname);
            }
        }
        catch (const std::runtime_error& e)
        {
            std::cerr << "\r\033[0K" << style(warning) << "WARNING: Could not read " << glob.filename() << ". Reason: " << e.what() << style() << std::endl;
            continue;
        }
    }
#ifdef _DEBUG
    std::cout << "\r\033[0K" << std::endl;
#endif
}
