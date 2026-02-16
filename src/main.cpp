#include <iostream>
#include <algorithm>
#include <cstring>
#include <csignal>
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

    int verbosity = 0;

    Query* currentQuery = nullptr;
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--case") == 0 || strcmp(argv[i], "-c") == 0)
        {
            g_options.caseSensitive = true;
            continue;
        }

        if (strcmp(argv[i], "--verbose") == 0)
        {
            ++verbosity;
            continue;
        }

        if (strcmp(argv[i], "--steamdir") == 0 || strcmp(argv[i], "-s") == 0)
        {
            ++i;
            if (i < argc)
            {
                if (std::filesystem::is_directory(argv[i]))
                {
                    g_options.steamDir = argv[i];
                    continue;
                }
                logger.error("%s was not a directory", argv[i]);
                exit(EXIT_FAILURE);
            }

            logger.error("Missing directory parameter for %s argument", argv[i - 1]);
            exit(EXIT_FAILURE);
        }


        if (currentQuery && strcmp(toLowerCase(argv[i]).c_str(), "or") == 0)
            continue;
        if (currentQuery && strcmp(toLowerCase(argv[i]).c_str(), "and") == 0)
        {
            currentQuery->type = Query::QueryAnd;
            continue;
        }


        if (auto query = std::make_unique<Query>(argv[i]); query->valid)
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


    if (g_options.steamDir.empty())
        g_options.steamDir = getSteamDir();

    if (std::filesystem::is_directory(g_options.steamDir / "steamapps/common"))
        g_options.steamCommonDir = g_options.steamDir / "steamapps/common";


    if (verbosity > 2)
        logger.setLevel(Logging::LogLevel::Debug);
    else if (verbosity > 1)
        logger.setLevel(Logging::LogLevel::Log);
    else if (verbosity > 0)
        logger.setLevel(Logging::LogLevel::Warning);


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
            "The value can be left out to only match the key,"
            "\nor the key can be left out to search for any matching value.\n"
            "Different operators can be used:\n"
            << style(brightBlack) <<
            " =              Match key/value starting with these terms\n"
            " ==             Match only exact key/value\n"
            " !=             Match only different key/value\n"
            " <, >, <=, >=   Numerical comparison on the value (less than, greater than, etc)\n"
            << style(success) <<
            "Keys with multiple space-separated values can be indexed with square brackets,\n"
            "e.g.: origin[1] to query the second value.\n"
            "Queries are implicitly or-chained. Use the AND keyword to and-chain queries.\nExample: "
            << style(brightBlack) << "classname=monster AND =argument AND origin[2]<200\n" << style(info)
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

extern "C" void signalHandler(int sig)
{
    g_receivedSignal.store(sig);
}


int main(const int argc, char* argv[])
{
    logger.setFileHandler(nullptr);
    logger.setLevel(Logging::LogLevel::Error);

    handleArgs(argc, argv);

    /*
      Use a custom handler to break checkMaps loop without stopping application completely,
      that way we can report on the matches found so far if interrupted early.
    */
    std::signal(SIGINT, signalHandler);

    g_options.findGlobs();

    if (g_options.globs.empty())
    {
        std::cout << style(warning) << "No .bsp files were found.\n"
            "Is '" << g_options.steamDir.string() << "' your Steam install directory? (y/N) " << style();
        if (!confirm_dialogue(false))
        {
            g_options.steamDir = getSteamDir(true);
            g_options.steamCommonDir = g_options.steamDir / "steamapps/common";
            g_options.findGlobs();
        }
    }

    if (g_options.globs.empty())
    {
        std::cerr << style(warning) << "No .bsp files were found.\n" << style() << std::endl;
        return EXIT_SUCCESS;
    }

    g_options.checkMaps();

    // Return signal handler to default
    std::signal(SIGINT, SIG_DFL);

    if (g_options.entries.empty())
    {
        std::cout << "No matches were found, checked " << g_options.globs.size() << " .bsp files" << std::endl;
        return EXIT_SUCCESS;
    }

    std::cout << "Number of matches found: " << g_options.foundEntries << '\n' << std::endl;


    // Flatten to vector and sort our entries by map name
    std::vector<std::pair<std::filesystem::path, std::vector<EntityEntry>>> entEntries;
    entEntries.reserve(g_options.entries.size());
    for (auto& [map, entries] : g_options.entries)
        entEntries.emplace_back( map, std::move(entries) );

    std::ranges::sort(entEntries, [](const auto& a, const auto& b) { return a.first < b.first; });


    for (const auto& [map, entries] : entEntries)
    {
        std::cout << (g_options.absoluteDir ? map.filename() : map).string() << ": [\n";

        for (const auto& [index, flags, classname, targetname, queryMatches, matched] : entries)
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
