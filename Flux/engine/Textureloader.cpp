/*
* Flux is a free, versatile game engine built for developers of all skill levels.
* Copyright (C) 2026  Zero Point Studio (Idkthisguy)
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Textureloader.h"
#include <iostream>

namespace Flux {
    std::unordered_map<std::string, unsigned int> TextureLoader::cache;
    unsigned int TextureLoader::Load(const std::string& path) {
        auto it = cache.find(path);
        if (it != cache.end()) return it->second;

        stbi_set_flip_vertically_on_load(true);

        int w, h, ch;
        unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 0);
        if (!data) {
            std::cerr << "TextureLoader: failed to load " << path << "\n";
            return 0;
        }

        GLenum fmt = (ch == 4) ? GL_RGBA : (ch == 3) ? GL_RGB : GL_RED;

        unsigned int id;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);

        cache[path] = id;
        return id;
    }

    void TextureLoader::Unload(const std::string& path) {
        auto it = cache.find(path);
        if (it == cache.end()) return;
        glDeleteTextures(1, &it->second);
        cache.erase(it);
    }

}