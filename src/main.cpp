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


static inline Options handleArgs(int argc, char* argv[])
{
    Options options;

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

    options.steamDir = getSteamDir();
    options.steamCommonDir = options.steamDir / "steamapps" / "common";

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--mod") == 0 || strcmp(argv[i], "-m") == 0)
        {
            ++i;
            if (i < argc)
            {
                options.mod = unSteampipe(argv[i]);
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
                options.classnames.emplace_back(argv[i]);
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
                options.keys.emplace_back(argv[i]);
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
                options.values.emplace_back(argv[i]);
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
                options.flags |= std::atoi(argv[i]);
                continue;
            }

            std::cerr << "Missing parameter for " << argv[i - 1] << " argument" << std::endl;
            exit(EXIT_FAILURE);
        }


        if (strcmp(argv[i], "--flags-or") == 0 || strcmp(argv[i], "-o") == 0)
        {
            options.flagsOr = true;
            continue;
        }

        if (strcmp(argv[i], "--exact") == 0 || strcmp(argv[i], "-e") == 0)
        {
            options.exact = true;
            continue;
        }

        if (strcmp(argv[i], "--case") == 0 || strcmp(argv[i], "-s") == 0)
        {
            options.caseSensitive = true;
            continue;
        }
    }

    if (options.classnames.empty() && options.values.empty() && options.flags == 0)
    {
        std::cout << style(info) << "Please specify a search query\n" << style() << std::endl;
        printUsage();
        exit(EXIT_SUCCESS);
    }

    if (options.mod.empty())
        options.globalSearch = true;

    if (options.mod == "svencoop")
        options.gamePath = options.steamCommonDir / "Sven Co-op";
    else
        options.gamePath = options.steamCommonDir / "Half-Life";

    if (!std::filesystem::is_directory(options.gamePath / options.mod))
    {
        std::cerr << style(error) << "\"" << (options.gamePath / options.mod).string() << "\" is not a directory" << style() << std::endl;
        exit(EXIT_FAILURE);
    }


    if (!options.caseSensitive)
    {
        for (std::string& str : options.classnames)
            str = toLowerCase(str);
        for (std::string& str : options.keys)
            str = toLowerCase(str);
        for (std::string& str : options.values)
            str = toLowerCase(str);
    }

    return options;
}



int main(int argc, char* argv[])
{
    Options options = handleArgs(argc, argv);
    options.findGlobs();
    options.checkMaps();

    for (const auto& [map, entries] : options.entries)
    {
        std::cout << map.filename().string() << ": [\n";

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
