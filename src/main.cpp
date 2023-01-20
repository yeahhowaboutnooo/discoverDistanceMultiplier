#include <SkyrimScripting/Plugin.h>
#include <toml++/toml.h>

#include "Hooks.h"

std::filesystem::path pluginPath = "Data/SKSE/Plugins/";
std::string_view pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
std::filesystem::path tomlFile = pluginPath / std::format("{}.toml", pluginName);

OnInit {
	toml::table tbl;
	try {
		tbl = toml::parse_file(tomlFile.string());
		auto val = tbl.get("multiplier")->value<float>();
		if (val) {
			Hooks::discoverDistanceMultiplier = *val;
		} else {
			logger::error("No multiplier value in {} !", tomlFile.string());
		}
	} catch (const toml::parse_error& err) {
		logger::error("Could not read toml config file: {}", tomlFile.string());
		logger::error("{}", err.description());
	}

	Hooks::Install();
}
