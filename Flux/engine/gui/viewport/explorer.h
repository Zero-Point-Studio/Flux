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
#include <fstream>
#include <vector>
#include <string>

namespace Flux {
	enum class fileType { Folder, Script, Text, Model, Texture };

	struct virtualFile
	{
		std::string name;
		fileType type;
		std::filesystem::path path;
		std::vector<virtualFile> children;
	};

	struct creationTask
	{
		std::string defaultName;
		std::string extension;
		bool pending = false;
	};

	class Viewport;

	class Explorer {
	public:
		void renderExplorer(Viewport& viewport);

		bool refreshRequested = false;
		std::filesystem::path refreshPath;
		std::filesystem::path activeFolderPath;
		virtualFile projectRoot = { "Project", fileType::Folder };
		creationTask pendingCreationTask;
		std::filesystem::path pathToDelete = "";

	private:
		void DrawVirtualNodes(virtualFile& file);
		void syncFiles(const std::filesystem::path& path, virtualFile& node);
		void copyTemplateItem(const std::string& folderType,
							  const std::string& templateName,
							  const std::string& targetBaseName,
							  const std::string& ext);
		void createNewFolder(const std::string& name);

		std::filesystem::path resolveUniqueName(
			const std::filesystem::path& parentDir,
			const std::string& baseStem,
			const std::string& ext) const;

		virtualFile* renamingNode = nullptr;
		char         renameBuffer[256] = {};

		bool showNewProjectModal     = false;
		char newProjectNameBuf[256]  = "NewGame";
		std::filesystem::path pendingTemplateRoot;
	};
}