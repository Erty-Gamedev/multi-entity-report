#include <cmath>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <ranges>
#include "logging.h"
#include "mer.h"
#include "utils.h"


namespace fs = std::filesystem;
using namespace Styling;
static inline Logging::Logger& logger = Logging::Logger::getLogger("mer");

Options g_options{};


void printUsage()
{
#ifdef _WIN32
    std::cout << "Usage: mer.exe [mods...] <search query> [options...]\n";
#else
    std::cout << "Usage: mer [mods...] <search query> [options...]\n";
#endif
    std::cout
        << style(brightBlack |italic) << "Run without any arguments to start interactive mode\n\n"

        << style(bold) << "ARGUMENTS\n" << style()
        << "  mods                 filter search to these mods only, global search otherwise (e.g. cstrike)\n"

        << style(bold) << "SEARCH QUERY\n" << style()
        << "  --classname  -c      classnames that must match\n"
        << "  --key        -k      keys that must match\n"
        << "  --value      -v      values that must match\n"
        << "  --flags      -f      spawnflags that must match (ALL must match unless --flags_or is used)\n"

        << style(bold) << "OPTIONS\n" << style()
        << "  --flags_or   -o      change spawnflag check mode to ANY\n"
        << "  --exact      -e      matches must be exact (whole term)\n"
        << "  --case       -s      make matches case sensitive\n"
        << "  --help       -h      print this message and exit\n"
        << "  --version            print application version and exit\n"
        << "  --verbose            enable verbose logging\n\n"

        << style(italic) << "Example:\n" << style()
        << "> " << style(brightBlack) << "mer" << style() << " valve -c monster_gman -v argument\n"
        << "Half-Life/valve/maps/c1a0.bsp: [\n  monster_gman(index 55, targetname 'argumentg')\n]\n"
        << std::endl;
}

void Options::findGlobsInMod(const fs::path& modDir)
{
    if (!fs::is_directory(modDir / "maps"))
        return;

    for (const auto& entry : fs::directory_iterator(modDir / "maps"))
    {
        if (const fs::path& entryPath = entry.path(); toLowerCase(entryPath.extension().string()) == ".bsp")
        {
            fs::path shortGlob = entryPath.parent_path().parent_path().parent_path().stem()
                / entryPath.parent_path().parent_path().stem() / entryPath.parent_path().stem() / entryPath.filename();
            globs.push_back(shortGlob);
        }
    }
}

void Options::findGlobsInPipes(fs::path modDir)
{
    std::string baseMod = modDir.stem().string();
    gamePath = modDir.parent_path();

    if (fs::is_directory(modDir))
        findGlobsInMod(modDir);

    for (const auto& pipe : c_SteamPipes)
    {
        modDir = gamePath / (baseMod + pipe);
        if (fs::is_directory(modDir))
            findGlobsInMod(modDir);
    }
}

void Options::findAllMods()
{
    if (fs::is_directory(steamCommonDir / "Sven Co-op/svencoop"))
        modDirs.emplace_back(steamCommonDir / "Sven Co-op/svencoop");

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
        return std::ranges::find(haystack, needle) != haystack.end();

    return std::ranges::any_of(haystack, [needle](const std::string& query) {
        return needle.starts_with(query);
    });
}

bool Options::matchValueInList(std::string needle, const std::vector<std::string>& haystack) const
{
    if (!caseSensitive)
        needle = toLowerCase(needle);

    char* err;
    const double needleNum = std::strtod(needle.c_str(), &err);

    if (*err)
    {
        if (exact)
            return std::ranges::find(haystack, needle) != haystack.end();

        return std::ranges::any_of(haystack, [needle](const std::string& query) {
            return needle.starts_with(query);
        });
    }

    // If we get here, needle is a number. Check if we should do a numeric comparison
    for (const std::string& query : haystack)
    {
        if (query.starts_with(">="))
            return needleNum >= std::stod(query.substr(2));
        if (query.starts_with("<="))
            return needleNum <= std::stod(query.substr(2));
        if (query.starts_with(">"))
            return needleNum > std::stod(query.substr(1));
        if (query.starts_with("<"))
            return needleNum < std::stod(query.substr(1));
        if (query.starts_with("="))
            return fabs(needleNum - std::stod(query.substr(1))) < 0.01;

        if (needle.starts_with(query))
            return true;
    }

    return false;
}

void Options::findGlobs()
{
    if (globalSearch)
        findAllMods();
    else
    {
        for (const auto& mod : mods)
        {
            if (mod == "svencoop")
                gamePath = g_options.steamCommonDir / "Sven Co-op";
            else
                gamePath = g_options.steamCommonDir / "Half-Life";

            fs::path modDir = gamePath / mod;
            if (!std::filesystem::is_directory(modDir))
            {
                logger.warning("\"" + modDir.string() + "\" is not a directory");
                continue;
            }
            modDirs.push_back(modDir);
        }
    }

    for (const auto& modDir : modDirs)
        findGlobsInPipes(modDir);
}

void Options::checkMaps()
{
    using namespace BSPFormat;

    for (const auto& glob : globs)
    {
        std::cout << "\r\033[0K" << "Reading " << glob.string();

        try { const Bsp reader{ glob }; }
        catch (const std::runtime_error& e)
        {
            if (logger.getLevel() > Logging::LogLevel::LOG_WARNING)
                continue;
            std::cerr << "\r\033[0K";
            logger.warning("Could not read " + glob.string() + ". Reason: " + e.what());
            continue;
        }
    }

    std::cout << "\r\033[0K" << std::endl;
}


using namespace BSPFormat;


Bsp::Bsp(const std::filesystem::path& filepath) {
    m_filepath = filepath;
    m_file.open(g_options.steamCommonDir / filepath, std::ios::binary);
    if (!m_file.is_open() || !m_file.good())
    {
        m_file.close();
        throw std::runtime_error("Could not open file for reading");
    }

    m_file.read(reinterpret_cast<char*>(&m_header), sizeof(BspHeader));

    if (m_header.version != 30 && m_header.version != 29)
        throw std::runtime_error("Unexpected BSP version: " + style(info) + std::to_string(m_header.version) + style());

    parse();
    m_file.close();
}

bool Bsp::readComment()
{
    if (m_file.peek() != '/')
        return false;

    m_file.get();  // Consume first slash

    if (m_file.peek() != '/')  // If next character is not slash, undo and return
    {
        m_file.unget();
        return false;
    }

    char c;
    while (m_file.get(c))
    {
        if (c == '\n')
            break;
    }
    return true;
}

void Bsp::parse()
{
    BspLump& entLump = m_header.lumps[LumpIndex::Entities];
    m_file.seekg(entLump.offset, std::ios::beg);

    while (isspace(m_file.peek()))  // Skip whitepaces
        m_file.get();

    // I've found at least one example of BSP29 using comments as headers over each entity, skip these
    while (readComment()) {}


    // If the next byte isn't {, check if we need to flip planes and entities lumps, we might have a bshift BSP
    if (m_file.peek() != '{')
    {
        entLump = m_header.lumps[LumpIndex::Planes];
        m_file.seekg(entLump.offset, std::ios::beg);
        while (isspace(m_file.peek()))  // Skip whitepaces
            m_file.get();
        if (m_file.peek() != '{')
            throw std::runtime_error("Unexpected BSP format");
    }


    const int lumpEnd = entLump.offset + entLump.length;

    char c;
    int i = 0;
    while (m_file.get(c) && m_file.tellg() < lumpEnd)
    {
        if (c == '{')
        {
            int index = i++;
            Entity entity = readEntity();

            if (!entity.contains("classname"))
                continue;  // Just in case. Entities should always have a classname, but you never know

            if (!g_options.classnames.empty())
                if (!g_options.matchInList(entity.at("classname"), g_options.classnames))
                    continue;

            if (!g_options.keys.empty()
                && !std::ranges::any_of(entity | std::views::keys, [](const std::string& key) {
                    return g_options.matchInList(key, g_options.keys);
                }))
                continue;

            if (!g_options.values.empty()
                && !std::ranges::any_of(entity | std::views::values, [](const std::string& value) {
                    return g_options.matchInList(value, g_options.values);
                }))
                continue;

            if (g_options.flags > 0)
            {
                unsigned int spawnflags = 0;
                if (entity.contains("spawnflags"))
                    spawnflags = std::stoi(entity.at("spawnflags"));
                if (const unsigned int matches = spawnflags & g_options.flags; matches == 0
                    || (!g_options.flagsOr && matches != g_options.flags))
                    continue;
            }

            if (!g_options.entries.contains(m_filepath))
                g_options.entries.insert_or_assign(m_filepath, std::vector<EntityEntry>{});

            std::string targetname = entity.contains("targetname") ? entity.at("targetname") : "";
            g_options.entries.at(m_filepath).emplace_back(index, entity.at("classname"), targetname);
        }
    }
}

std::string Bsp::readToken(int maxLength)
{
    std::string token;
    token.reserve(maxLength);

    char c;
    int i = 0;
    while (m_file.get(c))
    {
        ++i;

        if (c == '"')
            break;

        if (c == '\n')
        {
            token += "\\n";
            continue;
        }

        if (c == '\r')
            continue;

        token.push_back(c);
    }

    return token;
}

Entity Bsp::readEntity()
{
    Entity entity;
    const std::streamoff start = m_file.tellg();

    char c;
    while (m_file.get(c))
    {
        if (isspace(c)) continue;
        if (c == '}') break;

        if (c == '"')
        {
            std::string key = readToken(c_MaxKeyLength);

            while (m_file.get(c))
            {
                if (isspace(c)) continue;

                if (c == '"')
                {
                    std::string value = readToken(c_MaxValueLength);
                    entity.insert_or_assign(key, value);

                    // Skip comments at end of line (occurs in certain SC maps)
                    if (m_file.peek() == '/')
                        while (m_file.get(c))
                            if (c == '\n')
                                break;

                    break;
                }

                // Print out the rest of the entity before the unexpected data was encountered
                const std::streamoff offset = m_file.tellg();
                std::vector<unsigned char> rawEntBuffer;
                rawEntBuffer.resize(offset - start);
                m_file.seekg(start);
                m_file.read(reinterpret_cast<char*>(&rawEntBuffer[0]), offset - start);

                throw std::runtime_error("Unexpected entity data near " + style(info|dim) + std::string(rawEntBuffer.begin(), rawEntBuffer.end()) + style());
            }
        }
    }

    return entity;
}
