#include "output.h"

namespace Flux {
	void Output::addLog(const std::string& log) {
        logs.push_back(log);
    }

	void Output::renderOutput() {
		ImGui::Begin("Output");
		if (ImGui::IsWindowHovered()) {
			ImGui::SetWindowFocus();
		}

		if (ImGui::Button("Clear Output")) {
			logs.clear();
		}
		
		for (const std::string& msg : logs) {
            ImGui::TextUnformatted(msg.c_str());
        }

		ImGui::End();
	}
	
}