#include "source-settings-helpers.hpp"
#include "log-helper.hpp"
#include "json-helpers.hpp"

namespace advss {

std::string GetSourceSettings(OBSWeakSource ws)
{
	if (!ws) {
		return "";
	}

	std::string settings;
	auto s = obs_weak_source_get_source(ws);
	obs_data_t *data = obs_source_get_settings(s);
	auto json = obs_data_get_json(data);
	if (json) {
		settings = json;
	}
	obs_data_release(data);
	obs_source_release(s);

	return settings;
}

void SetSourceSettings(obs_source_t *s, const std::string &settings)
{
	if (settings.empty()) {
		return;
	}

	obs_data_t *data = obs_data_create_from_json(settings.c_str());
	if (!data) {
		blog(LOG_WARNING, "invalid source settings provided: \n%s",
		     settings.c_str());
		return;
	}
	obs_source_update(s, data);
	obs_data_release(data);
}

void InsertDataToSourceSettings(obs_source_t *s, const char *name,
				const std::string &additionalSettings)
{
	if (additionalSettings.empty()) {
		return;
	}
	obs_data_t *data =
		obs_data_create_from_json(additionalSettings.c_str());
	if (!data) {
		blog(LOG_WARNING,
		     "invalid source additional settings provided: \n%s",
		     additionalSettings.c_str());
		return;
	}
	obs_data_t *settings = obs_source_get_settings(s);
	obs_data_set_obj(settings, name, data);

	obs_source_update(s, settings);
	obs_data_release(data);
	obs_data_release(settings);
}

bool CompareSourceSettings(const OBSWeakSource &source,
			   const std::string &settings,
			   const RegexConfig &regex)
{
	std::string currentSettings = GetSourceSettings(source);
	return MatchJson(currentSettings, settings, regex);
}

} // namespace advss
