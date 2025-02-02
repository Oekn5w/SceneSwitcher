#pragma once
#include <obs.hpp>
#include <string>
#include <regex-config.hpp>

namespace advss {

std::string GetSourceSettings(OBSWeakSource ws);
void SetSourceSettings(obs_source_t *s, const std::string &settings);
void InsertDataToSourceSettings(obs_source_t *s, const char *name,
				const std::string &additionalSettings);
bool CompareSourceSettings(const OBSWeakSource &source,
			   const std::string &settings,
			   const RegexConfig &regex);

} // namespace advss
