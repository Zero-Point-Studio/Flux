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

#pragma once
#include "imgui.h"
#include "./mechanics/Scenenode.h"
#include <vector>
#include <string>
#include <memory>
#include <filesystem>
#include <unordered_map>

namespace Flux {

    class Model;

    class Heiarchy {
    public:
        std::vector<SceneNode> nodes;
        int                    selectedIndex = -1;

        void renderHeiarchy(const std::filesystem::path& activeProjectPath);

        void AddModel(const std::string& path, const std::string& name = "");
        void AddLight(NodeType type, const std::string& name = "");

    private:
        int  renamingIndex   = -1;
        char renameBuffer[128] = {};

        std::unordered_map<std::string, std::shared_ptr<Model>> modelRegistry;
        std::shared_ptr<Model> GetOrLoadModel(const std::string& path);

        void DrawNode(int index);
    };

}