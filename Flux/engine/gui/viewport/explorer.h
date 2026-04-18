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
#include "viewport.h"
#include <filesystem>
#include <vector>
#include <string>

namespace Flux {
	enum class fileType { Folder, Script, Text, Model };

	struct virtualFile
	{
		std::string name;
		fileType type;
		std::vector<virtualFile> children;
	};
	class Viewport;

	class Explorer {
		public:
			void renderExplorer(Viewport& viewport);

			std::filesystem::path activeFolderPath;
			virtualFile projectRoot = {"Project", fileType::Folder};
		private:
			void DrawVirtualNodes(virtualFile& file);

	};
}