#include <iostream>
#include <cstring>
#include "logging.h"
#include "utils.h"
#include "mer.h"

int _CRT_glob = 0;

#ifndef MER_NAME_VERSION
#define MER_NAME_VERSION "Multi Entity Report v0.0.0"
#endif


using namespace Styling;


static inline void handleArgs(int argc, char* argv[])
{
    // Eager args
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--version") == 0)
        {
            std::cout << MER_NAME_VERSION << std::endl;
            exit(EXIT_SUCCESS);
        }
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            printUsage();
            exit(EXIT_SUCCESS);
        }
    }

    g_options.steamDir = getSteamDir();
    g_options.steamCommonDir = g_options.steamDir / "steamapps" / "common";

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--mod") == 0 || strcmp(argv[i], "-m") == 0)
        {
            ++i;
            if (i < argc)
            {
                g_options.mod = unSteampipe(argv[i]);
                continue;
            }

            std::cerr << "Missing directory parameter for " << argv[i - 1] << " argument" << std::endl;
            exit(EXIT_FAILURE);
        }

        if (strcmp(argv[i], "--classname") == 0 || strcmp(argv[i], "-c") == 0)
        {
            ++i;
            if (i < argc)
            {
                g_options.classnames.emplace_back(argv[i]);
                continue;
            }

            std::cerr << "Missing parameter for " << argv[i - 1] << " argument" << std::endl;
            exit(EXIT_FAILURE);
        }

        if (strcmp(argv[i], "--key") == 0 || strcmp(argv[i], "-k") == 0)
        {
            ++i;
            if (i < argc)
            {
                g_options.keys.emplace_back(argv[i]);
                continue;
            }

            std::cerr << "Missing parameter for " << argv[i - 1] << " argument" << std::endl;
            exit(EXIT_FAILURE);
        }

        if (strcmp(argv[i], "--value") == 0 || strcmp(argv[i], "-v") == 0)
        {
            ++i;
            if (i < argc)
            {
                g_options.values.emplace_back(argv[i]);
                continue;
            }

            std::cerr << "Missing parameter for " << argv[i - 1] << " argument" << std::endl;
            exit(EXIT_FAILURE);
        }

        if (strcmp(argv[i], "--flag") == 0 || strcmp(argv[i], "-f") == 0)
        {
            ++i;
            if (i < argc)
            {
                g_options.flags |= std::atoi(argv[i]);
                continue;
            }

            std::cerr << "Missing parameter for " << argv[i - 1] << " argument" << std::endl;
            exit(EXIT_FAILURE);
        }


        if (strcmp(argv[i], "--flags-or") == 0 || strcmp(argv[i], "-o") == 0)
        {
            g_options.flagsOr = true;
            continue;
        }

        if (strcmp(argv[i], "--exact") == 0 || strcmp(argv[i], "-e") == 0)
        {
            g_options.exact = true;
            continue;
        }

        if (strcmp(argv[i], "--case") == 0 || strcmp(argv[i], "-s") == 0)
        {
            g_options.caseSensitive = true;
            continue;
        }
    }

    if (g_options.classnames.empty() && g_options.values.empty() && g_options.flags == 0)
    {
        std::cout << style(info) << "Please specify a search query\n" << style() << std::endl;
        printUsage();
        exit(EXIT_SUCCESS);
    }

    if (g_options.mod.empty())
        g_options.globalSearch = true;

    if (g_options.mod == "svencoop")
        g_options.gamePath = g_options.steamCommonDir / "Sven Co-op";
    else
        g_options.gamePath = g_options.steamCommonDir / "Half-Life";

    if (!std::filesystem::is_directory(g_options.gamePath / g_options.mod))
    {
        std::cerr << style(error) << "\"" << (g_options.gamePath / g_options.mod).string() << "\" is not a directory" << style() << std::endl;
        exit(EXIT_FAILURE);
    }


    if (!g_options.caseSensitive)
    {
        for (std::string& str : g_options.classnames)
            str = toLowerCase(str);
        for (std::string& str : g_options.keys)
            str = toLowerCase(str);
        for (std::string& str : g_options.values)
            str = toLowerCase(str);
    }
}



int main(int argc, char* argv[])
{
    handleArgs(argc, argv);
    g_options.findGlobs();
    g_options.checkMaps();

    for (const auto& [map, entries] : g_options.entries)
    {
        std::cout << map.string() << ": [\n";

        for (const auto& entry : entries)
        {
            std::cout << "  " << entry.classname << " (index " << entry.index;
            if (!entry.targetname.empty())
                std::cout << ", targetname '" << entry.targetname << "'";
            std::cout << ")\n";
        }

        std::cout << "]\n";
    }
}
