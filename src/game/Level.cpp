#include "game/Level.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>

namespace
{
    constexpr char levelDirectory[] = "assets/data/level";

    void require(bool condition,const std::string& message)
    {
        if (!condition)
            throw std::runtime_error(message);
    }

    void readQuotedString(std::istream& file,std::string& value)
    {
        file >> std::ws;
        char quote = '\0';
        require(file.get(quote) && quote == '"',"Expected a quoted string in level file");

        value.clear();
        char character = '\0';
        while (file.get(character))
        {
            if (character == '"')
                return;

            if (character == '\\')
            {
                value.push_back(character);
                if (!file.get(character))
                    break;
            }

            value.push_back(character);
        }

        throw std::runtime_error("Unterminated quoted string in level file");
    }

    void decodeEscapes(std::string& text)
    {
        std::string decoded;
        decoded.reserve(text.size());

        for (std::size_t index = 0; index < text.size(); ++index)
        {
            if (text[index] == '\\' && index + 1 < text.size())
            {
                switch (text[index + 1])
                {
                    case 'n':
                        decoded.push_back('\n');
                        ++index;
                        continue;
                    case 'r':
                        decoded.push_back('\r');
                        ++index;
                        continue;
                    case 't':
                        decoded.push_back('\t');
                        ++index;
                        continue;
                    case '\\':
                        decoded.push_back('\\');
                        ++index;
                        continue;
                }
            }

            decoded.push_back(text[index]);
        }

        text = std::move(decoded);
    }
}

std::vector<std::filesystem::path> getLevelFiles()
{
    std::vector<std::filesystem::path> paths;
    const std::filesystem::path directory(levelDirectory);

    if (!std::filesystem::exists(directory))
        return paths;

    for (const auto& entry:std::filesystem::directory_iterator(directory))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".level")
            paths.push_back(entry.path());
    }

    std::sort(paths.begin(),paths.end());
    return paths;
}

LevelData loadLevel(const std::filesystem::path& path)
{
    std::ifstream file(path);
    require(file.is_open(),"Could not open level file: " + path.string());

    LevelData level;
    bool hasPlayerPosition = false;
    bool hasPlayerSize = false;
    bool hasFinish = false;
    std::string name;

    while (file >> name)
    {
        if (!name.empty() && name[0] == '#')
        {
            std::string comment;
            std::getline(file,comment);
            continue;
        }

        if (name == "player_position")
        {
            file >> level.playerPosition.x >> level.playerPosition.y >> level.playerPosition.z;
            hasPlayerPosition = true;
        }
        else if (name == "player_size")
        {
            file >> level.playerSize.x >> level.playerSize.y;
            hasPlayerSize = true;
        }
        else if (name == "instruction")
        {
            LevelInstructionData instruction;
            readQuotedString(file,instruction.text);
            file >> instruction.textPosition.x >> instruction.textPosition.y
                 >> instruction.textSize;
            readQuotedString(file,instruction.buttonText);
            file >> instruction.buttonPosition.x >> instruction.buttonPosition.y
                 >> instruction.buttonSize.x >> instruction.buttonSize.y;
            decodeEscapes(instruction.text);
            level.instructions.push_back(std::move(instruction));
        }
        else if (name == "cube" || name == "finish")
        {
            LevelCubeData cube;
            file >> cube.position.x >> cube.position.y >> cube.position.z
                 >> cube.size.x >> cube.size.y >> cube.size.z;

            if (name == "cube")
                level.cubes.push_back(cube);
            else
            {
                level.finish = cube;
                hasFinish = true;
            }
        }
        else
        {
            throw std::runtime_error("Unknown entry in level file: " + name);
        }

        require(file.good() || file.eof(),"Invalid level entry in: " + path.string());
    }

    require(hasPlayerPosition,"Level is missing player_position: " + path.string());
    require(hasPlayerSize,"Level is missing player_size: " + path.string());
    require(hasFinish,"Level is missing finish: " + path.string());
    require(!level.instructions.empty(),"Level is missing instruction: " + path.string());
    return level;
}