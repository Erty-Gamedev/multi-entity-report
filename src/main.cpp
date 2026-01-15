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
static inline Logging::Logger& logger = Logging::Logger::getLogger("mer");


static void handleArgs(const int argc, char* argv[])
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
    g_options.steamCommonDir = g_options.steamDir / "steamapps/common";

    int verbosity = 0;

    Query* currentQuery = nullptr;
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--classname") == 0 || strcmp(argv[i], "-c") == 0)
        {
            ++i;
            if (i < argc)
            {
                g_options.classnames.emplace_back(argv[i]);
                continue;
            }

            logger.error("Missing parameter for %s argument", argv[i - 1]);
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

            logger.error("Missing parameter for %s argument", argv[i - 1]);
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

            logger.error("Missing parameter for %s argument", argv[i - 1]);
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

            logger.error("Missing parameter for %s argument", argv[i - 1]);
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

        if (strcmp(argv[i], "--verbose") == 0)
        {
            ++verbosity;
            continue;
        }


        if (currentQuery && strcmp(toLowerCase(argv[i]).c_str(), "or") == 0)
            continue;
        if (currentQuery && strcmp(toLowerCase(argv[i]).c_str(), "and") == 0)
        {
            currentQuery->type = Query::QueryAnd;
            continue;
        }


        std::unique_ptr<Query> query = std::make_unique<Query>(argv[i]);
        if (query->valid)
        {
            g_options.queries.push_back(std::move(query));
            Query* newQuery = g_options.queries.back().get();
            if (currentQuery)
                currentQuery->next = newQuery;
            currentQuery = newQuery;
        }
        else
            g_options.mods.emplace_back(unSteampipe(argv[i]));
    }

    if (verbosity > 2)
        logger.setLevel(Logging::LogLevel::LOG_DEBUG);
    else if (verbosity > 1)
        logger.setLevel(Logging::LogLevel::LOG_LOG);
    else if (verbosity > 0)
        logger.setLevel(Logging::LogLevel::LOG_WARNING);


    if (g_options.queries.empty())
    {
        g_options.interactiveMode = true;
        std::string buffer;

        if (g_options.mods.empty())
        {
            std::cout << style(info) << "Specify mods to narrow search by (leave empty for global search): " << style();
            std::getline(std::cin, buffer);
            if (buffer.empty())
                g_options.globalSearch = true;
            else
            {
                const std::vector<std::string>& parts = splitString(buffer, ' ');
                for (const auto& part : parts)
                    g_options.mods.push_back(unSteampipe(part));
            }
        }

        std::cout << style(success) <<
            "Search queries are key=value pairs. Multiple queries can be entered separated by spaces.\n"
            "The value can be left out to search for any matching key,"
            "\nor the key can be left out to search for any matching value.\n"
            "Different operators can be used:\n"
            << style(brightBlack) <<
            " =              Match key/value starting with these values\n"
            " ==             Match only exact key/value\n"
            " <, >, <=, >=   Numerical comparison on the value (less than, greater than, etc)\n"
            << style(success) <<
            "Keys with multiple space-separated values can be indexed with square brackets,\n"
            "e.g.: origin[1] to query the second value.\n"
            "Queries are implicitly or-chained. Use the AND keyword to and-chain queries.\nExample: "
            << style(brightBlack) << "classname=monster AND =argument origin[2]<200\n" << style(info)
            << "Enter search queries: " << style();

        std::getline(std::cin, buffer);
        if (!buffer.empty())
        {
            const std::vector<std::string>& parts = splitString(buffer, ' ');
            for (const auto& part : parts)
            {
                if (currentQuery && strcmp(toLowerCase(part).c_str(), "or") == 0)
                    continue;
                if (currentQuery && strcmp(toLowerCase(part).c_str(), "and") == 0)
                {
                    currentQuery->type = Query::QueryAnd;
                    continue;
                }

                std::unique_ptr<Query> query = std::make_unique<Query>(part);
                if (query->valid)
                {
                    g_options.queries.push_back(std::move(query));
                    Query* newQuery = g_options.queries.back().get();
                    if (currentQuery)
                        currentQuery->next = newQuery;
                    currentQuery = newQuery;
                }
                else
                    std::cout << style(warning) << "Invalid query: " << part << std::endl;
            }
        }

        if (g_options.queries.empty())
        {
            std::cout << style(warning) << "Please specify a search query\n" << style() << std::endl;
            printUsage();
            std::cout << "\nPress any key to close this window...";
            confirm_dialogue();
            exit(EXIT_SUCCESS);
        }
    }

    g_options.firstQuery = g_options.queries.front().get();

    if (g_options.mods.empty())
        g_options.globalSearch = true;
}



int main(const int argc, char* argv[])
{
    logger.setFileHandler(nullptr);
    logger.setLevel(Logging::LogLevel::LOG_ERROR);

    handleArgs(argc, argv);
    g_options.findGlobs();
    g_options.checkMaps();

    for (const auto& [map, entries] : g_options.entries)
    {
        std::cout << map.string() << ": [\n";

        for (const auto&[index, flags, classname, targetname, key, value, queryMatches, matched] : entries)
        {
            std::cout << "  " << classname << " (index " << index;
            if (!targetname.empty())
                std::cout << ", targetname '" << targetname << "'";
            if (!queryMatches.empty())
                std::cout << ", " << queryMatches;
            std::cout << ")\n";
        }

        std::cout << "]\n";
    }

    if (g_options.interactiveMode)
    {
        std::cout << "\nPress any key to close this window...";
        confirm_dialogue();
    }
}
