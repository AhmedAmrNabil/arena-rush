#include "font.hpp"

#include <fstream>
#include <iostream>
#include <texture/texture-utils.hpp>

namespace our {

    void Font::destroy() {
        if (textureAtlas) {
            delete textureAtlas;
            textureAtlas = nullptr;
        }
    }

    bool Font::load(const std::string& jsonPath, const std::string& texturePath) {
        std::ifstream file(jsonPath);
        if (!file.is_open()) {
            std::cerr << "Failed to open font JSON: " << jsonPath << std::endl;
            return false;
        }

        nlohmann::json data;
        try {
            file >> data;
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse font JSON: " << e.what() << std::endl;
            return false;
        }

        // font size and line height
        if (data.contains("info")) {
            fontSize = data["info"].value("size", 32.0f);
        }
        if (data.contains("common")) {
            lineHeight = data["common"].value("lineHeight", 32.0f);
        }

        // Parses the characters (by identified the texture atlas info per character, that is where in the texture is
        // this character exactly, how big is it, and how much to advance the cursor after)
        if (data.contains("chars")) {
            for (const auto& charData : data["chars"]) {
                int id = charData["id"];
                CharacterInfo info;
                info.x = charData["x"];
                info.y = charData["y"];
                info.width = charData["width"];
                info.height = charData["height"];
                info.xoffset = charData.value("xoffset", 0.0f);
                info.yoffset = charData.value("yoffset", 0.0f);
                info.xadvance = charData.value("xadvance", (float)info.width);  // defaults to letter width
                characters[(char)id] = info;
            }
        }

        textureAtlas = our::texture_utils::loadImage(texturePath, false);
        return textureAtlas != nullptr;
    }

}  // namespace our
