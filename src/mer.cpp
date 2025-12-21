#include <vector>
#include <filesystem>
#include "logging.h"
#include "mer.h"
#include "utils.h"


namespace fs = std::filesystem;
using namespace Styling;

Options g_options{};


void printUsage()
{
#ifdef _WIN32
    std::cout << "Usage: mer.exe <search query> [options...]\n\n";
#else
    std::cout << "Usage: mer <search query> [options...]\n\n";
#endif
    std::cout
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
        << "  --version            print application version\n\n"

        << style(italic) << "Example:\n" << style()
        << "> " << style(brightBlack) << "mer" << style() << " --mod valve -c monster_gman -v argument\n"
        << "Half-Life/valve/maps/c1a0.bsp: [\n  monster_gman(index 55, targetname 'argumentg')\n]\n"
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
        {
            fs::path shortGlob = entryPath.parent_path().parent_path().parent_path().stem()
                / entryPath.parent_path().parent_path().stem() / entryPath.parent_path().stem() / entryPath.filename();
            globs.push_back(shortGlob);
        }
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
        std::cout << "\r\033[0K" << "Reading " << glob.string();

        try { const Bsp reader{ glob }; }
        catch (const std::runtime_error& e)
        {
            std::cerr << "\r\033[0K" << style(warning) << "WARNING: Could not read " << glob.string() << ". Reason: " << e.what() << style() << std::endl;
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
        throw std::runtime_error("Could not open " + (g_options.steamCommonDir / filepath).string());
    }

    m_file.read(reinterpret_cast<char*>(&m_header), sizeof(BspHeader));

    if (m_header.version != 30)
        throw std::runtime_error("Unexpected BSP version: " + std::to_string(m_header.version));

    parse();
    m_file.close();
}

void Bsp::parse()
{
    BspLump& entLump = m_header.lumps[LumpIndex::ENTITIES];

    m_file.seekg(entLump.offset, std::ios::beg);
    int lumpEnd = entLump.offset + entLump.length;

    char c;
    int i = 0;
    while (m_file.get(c) && m_file.tellg() < lumpEnd)
    {
        if (c == '{')
        {
            bool test = m_filepath.filename() == "classic.bsp";
            int index = i++;
            Entity entity = readEntity();

            if (!g_options.classnames.empty())
                if (!g_options.matchInList(entity.at("classname"), g_options.classnames))
                    continue;

            if (!g_options.keys.empty())
            {
                bool skip = true;
                for (const auto& [key, value] : entity)
                {
                    if (g_options.matchInList(key, g_options.keys))
                    {
                        skip = false;
                        break;
                    }
                }
                if (skip)
                    continue;
            }

            if (!g_options.values.empty())
            {
                bool skip = true;
                for (const auto& [key, value] : entity)
                {
                    if (g_options.matchInList(value, g_options.values))
                    {
                        skip = false;
                        break;
                    }
                }
                if (skip)
                    continue;
            }

            if (g_options.flags > 0)
            {
                int spawnflags = 0;
                if (entity.contains("spawnflags"))
                    spawnflags = std::stoi(entity.at("spawnflags"));
                int matches = spawnflags & g_options.flags;
                if (matches == 0 || (!g_options.flagsOr && matches != g_options.flags))
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
    size_t start = m_file.tellg();

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
                size_t offset = m_file.tellg();
                std::vector<unsigned char> rawEntBuffer;
                rawEntBuffer.resize(offset - start);
                m_file.seekg(start);
                m_file.read(reinterpret_cast<char*>(&rawEntBuffer[0]), offset - start);

                throw std::runtime_error("Unexpected entity data near \"" + std::string(rawEntBuffer.begin(), rawEntBuffer.end()) + "\"");
            }
        }
    }

    return entity;
}
