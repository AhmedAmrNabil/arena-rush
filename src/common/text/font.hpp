#pragma once

#include <json/json.hpp>
#include <string>
#include <texture/texture2d.hpp>
#include <unordered_map>

namespace our {

    struct CharacterInfo {
        int x, y;
        int width, height;
        float xoffset, yoffset;
        float xadvance;
    };

    class Font {
        our::Texture2D* textureAtlas = nullptr;
        std::unordered_map<char, CharacterInfo> characters;
        float fontSize;
        float lineHeight;

    public:
        Font() = default;
        ~Font() = default;

        void destroy();
        bool load(const std::string& jsonPath, const std::string& texturePath);

        const CharacterInfo* getCharacter(char c) const {
            auto it = characters.find(c);
            return (it != characters.end()) ? &it->second : nullptr;
        }

        our::Texture2D* getTexture() const {
            return textureAtlas;
        }
        float getLineHeight() const {
            return lineHeight;
        }
        float getFontSize() const {
            return fontSize;
        }

        Font(const Font&) = delete;
        Font& operator=(const Font&) = delete;
    };

}  // namespace our
