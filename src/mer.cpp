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
    std::cout << "Usage: mer.exe [mods... [search queries... [options...]]]\n";
#else
    std::cout << "Usage: mer [mods... [search queries... [options...]]]\n";
#endif
    std::cout
        << style(brightBlack|italic) << "Run without any arguments to start interactive mode\n\n"

        << style(bold) << "ARGUMENTS\n" << style()
        << "  mods                 filter search to these mods only, global search otherwise (e.g. cstrike)\n\n"

        << style(bold) << "SEARCH QUERIES\n" << style() <<
           "  key=value pairs separated by spaces. Implicitly or-chained,\n"
           "  use the AND keyword inbetween queries to and-chain the queries.\n"
           "  Use square brackets on a key with multiple values to access a specific element,\n"
           "  e.g. origin[1] to query the second element.\n"
           "  Use == instead of = for exact matches only, != not matching,\n"
           "  or </>/>=/<= for numeric comparisons.\n\n"

        << style(bold) << "OPTIONS\n" << style()
        << "  --case       -s      make matches case sensitive\n"
        << "  --help       -h      print this message and exit\n"
        << "  --version            print application version and exit\n"
        << "  --verbose            enable verbose logging\n\n"

        << style(italic) << "Example:\n" << style()
        << "> " << style(brightBlack) << "mer" << style() << " valve classname=monster_gman AND =argument\n"
        << "Half-Life/valve/maps/c1a0.bsp: [\n  monster_gman (index 55, targetname 'argumentg', classname=monster_gman AND targetname=argumentg)\n]\n"
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

std::string Options::matchInList(std::string needle, const std::vector<std::string>& haystack) const
{
    if (!caseSensitive)
        needle = toLowerCase(needle);

    if (exact)
    {
        if (const auto match = std::ranges::find(haystack, needle); match != haystack.end())
            return *match;
        return "";
    }

    for (const auto& query : haystack)
    {
        if (needle.starts_with(query))
            return needle;
    }

    return "";
}

std::string Options::matchValueInList(std::string needle, const std::vector<std::string>& haystack) const
{
    std::string needleFull = needle;
    if (const auto hashPos = needle.find('#'); hashPos != std::string::npos)
        needle = needle.substr(0, hashPos);

    char* err;
    const double needleNum = std::strtod(needle.c_str(), &err);

    if (*err)
    {
        if (!caseSensitive)
            needle = toLowerCase(needle);

        if (exact)
        {
            if (const auto match = std::ranges::find(haystack, needle); match != haystack.end())
                return *match;
            return "";
        }

        for (const auto& query : haystack)
        {
            if (needle.starts_with(query))
                return needle;
        }

        return "";
    }

    // If we get here, needle is a number. Check if we should do a numeric comparison
    for (const std::string& query : haystack)
    {
        if ((query.starts_with(">=") && needleNum >= std::stod(query.substr(2))) ||
            (query.starts_with("<=") && needleNum <= std::stod(query.substr(2))) ||
            (query.starts_with(">") && needleNum > std::stod(query.substr(1))) ||
            (query.starts_with("<") && needleNum < std::stod(query.substr(1))) ||
            (query.starts_with("=") && fabs(needleNum - std::stod(query.substr(1))) < 0.01))
            return needleFull;

        if (needle.starts_with(query))
            return needleFull;
    }

    return "";
}


std::string Options::matchKey(const Entity& entity) const
{
    for (const auto& needle : entity | std::views::keys)
    {
        if (auto match = matchInList(needle, keys); !match.empty())
            return match;
    }

    return "";
}

std::string Options::matchValue(const Entity& entity) const
{
    for (const auto& needle : entity | std::views::values)
    {
        if (auto match = matchValueInList(needle, values); !match.empty())
            return match;
    }

    return "";
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

void Options::checkMaps() const
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
    BspLump& entLump = m_header.lumps[Entities];
    m_file.seekg(entLump.offset, std::ios::beg);

    while (isspace(m_file.peek()))  // Skip whitepaces
        m_file.get();

    // I've found at least one example of BSP29 using comments as headers over each entity, skip these
    while (readComment()) {}


    // If the next byte isn't {, check if we need to flip planes and entities lumps, we might have a bshift BSP
    if (m_file.peek() != '{')
    {
        entLump = m_header.lumps[Planes];
        m_file.seekg(entLump.offset, std::ios::beg);
        while (isspace(m_file.peek()))  // Skip whitepaces
            m_file.get();
        if (m_file.peek() != '{')
            throw std::runtime_error("Unexpected BSP format");
    }


    const int lumpEnd = entLump.offset + entLump.length;

    char c;
    unsigned int i = 0u;
    while (m_file.get(c) && m_file.tellg() < lumpEnd)
    {
        if (c == '{')
        {
            Entity entity = readEntity();
            EntityEntry matchEntry = g_options.firstQuery->testChain(entity, i++);

            if (!matchEntry.matched)
                continue;

            if (!g_options.entries.contains(m_filepath))
                g_options.entries.insert_or_assign(m_filepath, std::vector<EntityEntry>{});

            matchEntry.targetname = entity.contains("targetname") ? entity.at("targetname") : "";
            g_options.entries.at(m_filepath).push_back(matchEntry);
        }
    }
}

std::string Bsp::readToken(const int maxLength)
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

static std::string keyStartsWith(const Entity& entity, const std::string& prefix)
{
    for (const auto& key: entity | std::views::keys)
        if (!key.empty() && key.starts_with(prefix))
            return key;
    return "";
}

static std::string valueStartsWith(const Entity& entity, const std::string& prefix)
{
    for (const auto& [key, value] : entity)
        if (!value.empty() && value.starts_with(prefix))
            return key;
    return "";
}

static bool isValueNumeric(const std::string& value, double& numeric)
{
    std::string valueTrimmed = value;
    if (const auto suffixPos = value.find('#'); suffixPos != std::string::npos)
        valueTrimmed = value.substr(0, suffixPos);
    else if (const auto suffixPos = value.find(' '); suffixPos != std::string::npos)
        valueTrimmed = value.substr(0, suffixPos);

    char* err;
    numeric = std::strtod(valueTrimmed.c_str(), &err);

    if (*err)
        return false;

    return true;
}



Query::Query(const std::string& rawQuery)
{
    parse(rawQuery);
    if (!valid)
        return;
    checkIndexedKey();
    valueIsNumeric = isValueNumeric(value, valueNumeric);
}

void Query::parse(const std::string& rawQuery)
{
    if (size_t pos = rawQuery.find("=="); pos != std::string::npos)
    {
        op = QueryExact;
        key = rawQuery.substr(0, pos);
        value = rawQuery.substr(pos + 2);
        return;
    }
    if (size_t pos = rawQuery.find("<="); pos != std::string::npos)
    {
        op = QueryLessEquals;
        key = rawQuery.substr(0, pos);
        value = rawQuery.substr(pos + 2);
        return;
    }
    if (size_t pos = rawQuery.find(">="); pos != std::string::npos)
    {
        op = QueryGreaterEquals;
        key = rawQuery.substr(0, pos);
        value = rawQuery.substr(pos + 2);
        return;
    }
    if (size_t pos = rawQuery.find("!="); pos != std::string::npos)
    {
        op = QueryNotEquals;
        key = rawQuery.substr(0, pos);
        value = rawQuery.substr(pos + 2);
        return;
    }
    if (size_t pos = rawQuery.find("="); pos != std::string::npos)
    {
        op = QueryEquals;
        key = rawQuery.substr(0, pos);
        value = rawQuery.substr(pos + 1);
        return;
    }
    if (size_t pos = rawQuery.find("<"); pos != std::string::npos)
    {
        op = QueryLess;
        key = rawQuery.substr(0, pos);
        value = rawQuery.substr(pos + 1);
        return;
    }
    if (size_t pos = rawQuery.find(">"); pos != std::string::npos)
    {
        op = QueryGreater;
        key = rawQuery.substr(0, pos);
        value = rawQuery.substr(pos + 1);
        return;
    }
    valid = false;
    key = rawQuery;
}

void Query::checkIndexedKey()
{
    if (key.empty())
        return;

    if (size_t pos = key.find('['), posEnd = key.find(']'); pos != std::string::npos && posEnd != std::string::npos)
    {
        const std::string& rawIndex = key.substr(pos + 1, key.length() - posEnd);
        valueIndex = std::stoi(rawIndex);
        key = key.substr(0, pos);
        elementAccess = true;
    }
}


// TODO: Clean up, make more DRY
EntityEntry Query::testEntity(const Entity& entity, unsigned int index) const
{
    EntityEntry entry{ .index = index, .classname = entity.at("classname") };

    // Nothing to test
    if (key.empty() && value.empty())
        return entry;

    if (elementAccess)
    {
        if (key.empty() || !entity.contains(key))
            return entry;

        auto parts = splitString(entity.at(key));
        if (valueIndex > parts.size())
            return entry;

        std::string needle;
        if (valueIndex < 0)
            needle = parts[parts.size() + (valueIndex % parts.size())];
        else
            needle = parts[valueIndex];

        switch (op)
        {
        case QueryEquals:
        {
            if (needle.starts_with(value))
            {
                entry.key = key;
                entry.value = needle;
                entry.queryMatches = entry.key + '[' + std::to_string(valueIndex) + "]=" + entry.value;
                entry.matched = true;
                return entry;
            }
            break;
        }
        case QueryNotEquals:
        {
            if (needle != value)
            {
                entry.key = key;
                entry.value = '!' + value;
                entry.queryMatches = entry.key + '[' + std::to_string(valueIndex) + "]!=" + value;
                entry.matched = true;
                return entry;
            }
            break;
        }
        case QueryExact:
        {
            if (needle == value)
            {
                entry.key = key;
                entry.value = needle;
                entry.queryMatches = entry.key + '[' + std::to_string(valueIndex) + "]==" + entry.value;
                entry.matched = true;
                return entry;
            }
            break;
        }
        case QueryGreater:
        {
            double needleNum;
            if (valueIsNumeric && isValueNumeric(needle, needleNum) && needleNum > valueNumeric)
            {
                entry.key = key;
                entry.value = needle;
                entry.queryMatches = entry.key + '[' + std::to_string(valueIndex) + "]=" + entry.value;
                entry.matched = true;
                return entry;
            }
            break;
        }
        case QueryLess:
        {
            double needleNum;
            if (valueIsNumeric && isValueNumeric(needle, needleNum) && needleNum < valueNumeric)
            {
                entry.key = key;
                entry.value = needle;
                entry.queryMatches = entry.key + '[' + std::to_string(valueIndex) + "]=" + entry.value;
                entry.matched = true;
                return entry;
            }
            break;
        }
        case QueryGreaterEquals:
        {
            double needleNum;
            if (valueIsNumeric && isValueNumeric(needle, needleNum) && needleNum >= valueNumeric)
            {
                entry.key = key;
                entry.value = needle;
                entry.queryMatches = entry.key + '[' + std::to_string(valueIndex) + "]=" + entry.value;
                entry.matched = true;
                return entry;
            }
            break;
        }
        case QueryLessEquals:
        {
            double needleNum;
            if (valueIsNumeric && isValueNumeric(needle, needleNum) && needleNum <= valueNumeric)
            {
                entry.key = key;
                entry.value = needle;
                entry.queryMatches = entry.key + '[' + std::to_string(valueIndex) + "]=" + entry.value;
                entry.matched = true;
                return entry;
            }
            break;
        }
        }

        return entry;
    }


    if (key == "spawnflags" && valueIsNumeric && entity.contains(key))
    {
        unsigned int valueUInt = static_cast<unsigned int>(valueNumeric);
        if (unsigned int spawnflags = std::atoi(entity.at(key).c_str()); spawnflags > 0)
        {
            switch (op)
            {
            case QueryEquals:
            {
                if (spawnflags & valueUInt)
                {
                    entry.queryMatches = key + '=' + entity.at(key);
                    entry.matched = true;
                    return entry;
                }
                break;
            }
            case QueryNotEquals:
            {
                if ((spawnflags & valueUInt) == 0)
                {
                    entry.queryMatches = key + "!=" + value;
                    entry.matched = true;
                    return entry;
                }
                break;
            }
            case QueryExact:
            {
                if ((spawnflags & valueUInt) == valueUInt)
                {
                    entry.queryMatches = key + '=' + entity.at(key);
                    entry.matched = true;
                    return entry;
                }
                break;
            }
            }
        }

        return entry;
    }


    switch (op)
    {
    case QueryEquals:
    {
        if (!key.empty())
        {
            if (const std::string& needle = keyStartsWith(entity, key); !needle.empty())
            {
                entry.key = needle;

                if (value.empty())
                {
                    entry.queryMatches = entry.key + '=';
                    entry.matched = true;
                    return entry;
                }

                if (entity.at(needle).starts_with(value))
                {
                    entry.value = entity.at(needle);
                    entry.queryMatches = entry.key + '=' + entry.value;
                    entry.matched = true;
                    return entry;
                }
            }

            break;
        }
        else if (!value.empty())
        {
            if (const std::string& needle = valueStartsWith(entity, value); !needle.empty())
            {
                entry.key = needle;
                entry.value = entity.at(needle);
                entry.queryMatches = entry.key + '=' + entry.value;
                entry.matched = true;
                return entry;
            }
        }

        break;
    }
    case QueryNotEquals:
    {
        if (!key.empty())
        {
            if (!entity.contains(key))
                return entry;

            if (!entity.at(key).starts_with(value))
            {
                entry.key = key;
                entry.value = '!' + value;
                entry.queryMatches = entry.key + "!=" + value;
                entry.matched = true;
                return entry;
            }
            break;
        }
        else if (!value.empty())
        {
            if (valueStartsWith(entity, value).empty())
            {
                entry.value = '!' + value;
                entry.queryMatches = "!=" + value;
                entry.matched = true;
                return entry;
            }
        }
        break;
    }
    case QueryExact:
    {
        if (!key.empty())
        {
            if (!entity.contains(key))
                return entry;

            entry.key = key;

            if (value.empty())
            {
                entry.queryMatches = entry.key + '=';
                entry.matched = true;
                return entry;
            }

            if (entity.at(key) == value)
            {
                entry.value = value;
                entry.queryMatches = entry.key + '=' + entry.value;
                entry.matched = true;
                return entry;
            }
        }
        else if (!value.empty())
        {
            for (const auto& [needleKey, needle] : entity)
            {
                if (needle == value)
                {
                    entry.key = needleKey;
                    entry.value = needle;
                    entry.queryMatches = entry.key + '=' + entry.value;
                    entry.matched = true;
                    return entry;
                }
            }
        }

        break;
    }
    case QueryGreater:
    {
        if (value.empty() || (!key.empty() && !entity.contains(key)))
            return entry;


        if (!key.empty() && entity.contains(key))
        {
            entry.key = key;

            double needleNum;
            if (valueIsNumeric && isValueNumeric(entity.at(key), needleNum) && needleNum > valueNumeric)
            {
                entry.value = entity.at(key);
                entry.queryMatches = entry.key + '=' + entry.value;
                entry.matched = true;
                return entry;
            }
        }
        else if (!value.empty())
        {
            for (const auto& [needleKey, needle] : entity)
            {
                double needleNum;
                if (valueIsNumeric && isValueNumeric(needle, needleNum) && needleNum > valueNumeric)
                {
                    entry.key = needleKey;
                    entry.value = needle;
                    entry.queryMatches = entry.key + '=' + entry.value;
                    entry.matched = true;
                    return entry;
                }
            }
        }

        break;
    }
    case QueryLess:
    {
        if (value.empty() || (!key.empty() && !entity.contains(key)))
            return entry;


        if (!key.empty() && entity.contains(key))
        {
            entry.key = key;

            double needleNum;
            if (valueIsNumeric && isValueNumeric(entity.at(key), needleNum) && needleNum < valueNumeric)
            {
                entry.value = entity.at(key);
                entry.queryMatches = entry.key + '=' + entry.value;
                entry.matched = true;
                return entry;
            }
        }
        else if (!value.empty())
        {
            for (const auto& [needleKey, needle] : entity)
            {
                double needleNum;
                if (valueIsNumeric && isValueNumeric(needle, needleNum) && needleNum < valueNumeric)
                {
                    entry.key = needleKey;
                    entry.value = needle;
                    entry.queryMatches = entry.key + '=' + entry.value;
                    entry.matched = true;
                    return entry;
                }
            }
        }

        break;
    }
    case QueryGreaterEquals:
    {
        if (value.empty() || (!key.empty() && !entity.contains(key)))
            return entry;


        if (!key.empty() && entity.contains(key))
        {
            entry.key = key;

            double needleNum;
            if (valueIsNumeric && isValueNumeric(entity.at(key), needleNum) && needleNum >= valueNumeric)
            {
                entry.value = entity.at(key);
                entry.queryMatches = entry.key + '=' + entry.value;
                entry.matched = true;
                return entry;
            }
        }
        else if (!value.empty())
        {
            for (const auto& [needleKey, needle] : entity)
            {
                double needleNum;
                if (valueIsNumeric && isValueNumeric(needle, needleNum) && needleNum >= valueNumeric)
                {
                    entry.key = needleKey;
                    entry.value = needle;
                    entry.queryMatches = entry.key + '=' + entry.value;
                    entry.matched = true;
                    return entry;
                }
            }
        }

        break;
    }
    case QueryLessEquals:
    {
        if (value.empty() || (!key.empty() && !entity.contains(key)))
            return entry;


        if (!key.empty() && entity.contains(key))
        {
            entry.key = key;

            double needleNum;
            if (valueIsNumeric && isValueNumeric(entity.at(key), needleNum) && needleNum <= valueNumeric)
            {
                entry.value = entity.at(key);
                entry.queryMatches = entry.key + '=' + entry.value;
                entry.matched = true;
                return entry;
            }
        }
        else if (!value.empty())
        {
            for (const auto& [needleKey, needle] : entity)
            {
                double needleNum;
                if (valueIsNumeric && isValueNumeric(needle, needleNum) && needleNum <= valueNumeric)
                {
                    entry.key = needleKey;
                    entry.value = needle;
                    entry.queryMatches = entry.key + '=' + entry.value;
                    entry.matched = true;
                    return entry;
                }
            }
        }

        break;
    }
    }

    return entry;
}


EntityEntry Query::testChain(const Entity& entity, unsigned int index)
{
    EntityEntry entry = testEntity(entity, index);

    if (next)
    {
        if (type == QueryAnd)
        {
            if (!entry.matched)
                return entry;

            EntityEntry nextEntry = next->testChain(entity);
            if (nextEntry.matched)
            {
                if (!nextEntry.queryMatches.empty())
                    entry.queryMatches.append(" AND " + nextEntry.queryMatches);
            }
            else
            {
                entry.key = "";
                entry.value = "";
                entry.queryMatches = "";
                entry.matched = false;
            }
        }
        if (!entry.matched && type == QueryOr)
        {
            EntityEntry nextEntry = next->testChain(entity);

            if (nextEntry.matched)
            {
                entry.matched = true;

                if (entry.queryMatches.empty())
                    entry.queryMatches = nextEntry.queryMatches;
                else
                    entry.queryMatches.append(" OR " + nextEntry.queryMatches);
            }
        }
    }

    return entry;
}
