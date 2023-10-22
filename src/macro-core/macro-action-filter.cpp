#include "macro-action-filter.hpp"
#include "utility.hpp"

Q_DECLARE_METATYPE(advss::FilterSettingButton);

namespace advss {

const std::string MacroActionFilter::id = "filter";

bool MacroActionFilter::_registered = MacroActionFactory::Register(
	MacroActionFilter::id,
	{MacroActionFilter::Create, MacroActionFilterEdit::Create,
	 "AdvSceneSwitcher.action.filter"});

const static std::map<MacroActionFilter::Action, std::string> actionTypes = {
	{MacroActionFilter::Action::ENABLE,
	 "AdvSceneSwitcher.action.filter.type.enable"},
	{MacroActionFilter::Action::DISABLE,
	 "AdvSceneSwitcher.action.filter.type.disable"},
	{MacroActionFilter::Action::TOGGLE,
	 "AdvSceneSwitcher.action.filter.type.toggle"},
	{MacroActionFilter::Action::SETTINGS,
	 "AdvSceneSwitcher.action.filter.type.settings"},
	{MacroActionFilter::Action::SETTINGS_BUTTON,
	 "AdvSceneSwitcher.action.filter.type.pressSettingsButton"},
	{MacroActionFilter::Action::COPY_TRAFO_TO_SETTINGS,
	 "AdvSceneSwitcher.action.filter.type.copyTrafoToSettings"},
};

static std::vector<FilterSettingButton> getFilterButtons(OBSWeakSource source)
{
	auto s = obs_weak_source_get_source(source);
	std::vector<FilterSettingButton> buttons;
	obs_properties_t *sourceProperties = obs_source_properties(s);
	auto it = obs_properties_first(sourceProperties);
	do {
		if (!it || obs_property_get_type(it) != OBS_PROPERTY_BUTTON) {
			continue;
		}
		FilterSettingButton button = {obs_property_name(it),
					      obs_property_description(it)};
		buttons.emplace_back(button);
	} while (obs_property_next(&it));
	obs_source_release(s);
	return buttons;
}

static void pressFilterButton(const FilterSettingButton &button,
			      obs_source_t *source)
{
	obs_properties_t *sourceProperties = obs_source_properties(source);
	obs_property_t *property =
		obs_properties_get(sourceProperties, button.id.c_str());
	if (!obs_property_button_clicked(property, source)) {
		blog(LOG_WARNING,
		     "Failed to press filter settings button '%s' for %s",
		     button.id.c_str(), obs_source_get_name(source));
	}
	obs_properties_destroy(sourceProperties);
}

static void CopyTrafoToSettings(obs_source_t *s_target,
				SceneItemSelection &trafoSrc,
				SceneSelection &trafoScene)
{
	auto sTrafoSIs = trafoSrc.GetSceneItems(trafoScene);
	if (sTrafoSIs.empty()) {
		return;
	}
	auto trafoJSON = GetSceneItemTransform(sTrafoSIs[0]);
	InsertDataToSourceSettings(
		s_target, "transformationInfo",
		obs_data_create_from_json(trafoJSON.c_str()));
}

bool MacroActionFilter::PerformAction()
{
	auto filters = _filter.GetFilters(_source);
	for (const auto &filter : filters) {
		OBSSourceAutoRelease source =
			obs_weak_source_get_source(filter);
		switch (_action) {
		case MacroActionFilter::Action::ENABLE:
			obs_source_set_enabled(source, true);
			break;
		case MacroActionFilter::Action::DISABLE:
			obs_source_set_enabled(source, false);
			break;
		case MacroActionFilter::Action::TOGGLE:
			obs_source_set_enabled(source,
					       !obs_source_enabled(source));
			break;
		case MacroActionFilter::Action::SETTINGS:
			SetSourceSettings(source, _settings);
			break;
		case MacroActionFilter::Action::SETTINGS_BUTTON:
			pressFilterButton(_button, source);
			break;
		case MacroActionFilter::Action::COPY_TRAFO_TO_SETTINGS:
			CopyTrafoToSettings(source, _trafoSrcSource,
					    _trafoSrcScene);
			break;
		default:
			break;
		}
	}
	return true;
}

void MacroActionFilter::LogAction() const
{
	auto it = actionTypes.find(_action);
	if (it != actionTypes.end()) {
		vblog(LOG_INFO,
		      "performed action \"%s\" for filter \"%s\" on source \"%s\"",
		      it->second.c_str(), _filter.ToString().c_str(),
		      _source.ToString(true).c_str());
	} else {
		blog(LOG_WARNING, "ignored unknown filter action %d",
		     static_cast<int>(_action));
	}
}

bool MacroActionFilter::Save(obs_data_t *obj) const
{
	MacroAction::Save(obj);
	_source.Save(obj);
	_filter.Save(obj, "filter");
	_button.Save(obj);
	obs_data_set_int(obj, "action", static_cast<int>(_action));
	_settings.Save(obj, "settings");
	obs_data_set_int(obj, "version", 1);
	_trafoSrcScene.Save(obj, "trafoSelectionScene");
	_trafoSrcSource.Save(obj, "trafoSelectionSource");
	return true;
}

bool MacroActionFilter::Load(obs_data_t *obj)
{
	MacroAction::Load(obj);
	_source.Load(obj);
	_filter.Load(obj, _source, "filter");
	_button.Load(obj);
	// TODO: Remove this fallback in future version
	if (!obs_data_has_user_value(obj, "version")) {
		const auto value = obs_data_get_int(obj, "action");
		if (value == 2) {
			_action = Action::SETTINGS;
		} else {
			_action = static_cast<Action>(value);
		}
	} else {
		_action = static_cast<Action>(obs_data_get_int(obj, "action"));
	}
	_settings.Load(obj, "settings");
	_trafoSrcScene.Load(obj, "trafoSelectionScene");
	_trafoSrcSource.Load(obj, "trafoSelectionSource");
	return true;
}

std::string MacroActionFilter::GetShortDesc() const
{
	if (!_filter.ToString().empty() && !_source.ToString().empty()) {
		return _source.ToString() + " - " + _filter.ToString();
	}
	return "";
}

static inline void populateActionSelection(QComboBox *list)
{
	for (auto entry : actionTypes) {
		list->addItem(obs_module_text(entry.second.c_str()));
	}
}

static inline void populateFilterButtonSelection(QComboBox *list,
						 OBSWeakSource source)
{
	list->clear();
	auto buttons = getFilterButtons(source);
	if (buttons.empty()) {
		list->addItem(obs_module_text(
			"AdvSceneSwitcher.action.source.noSettingsButtons"));
	}

	for (const auto &button : buttons) {
		QVariant value;
		value.setValue(button);
		list->addItem(QString::fromStdString(button.ToString()), value);
	}
}

MacroActionFilterEdit::MacroActionFilterEdit(
	QWidget *parent, std::shared_ptr<MacroActionFilter> entryData)
	: QWidget(parent),
	  _sources(new SourceSelectionWidget(this, QStringList(), true)),
	  _filters(new FilterSelectionWidget(this, _sources, true)),
	  _actions(new QComboBox()),
	  _settingsButtons(new QComboBox()),
	  _trafoSrcScene(
		  new SceneSelectionWidget(window(), true, false, false, true)),
	  _trafoSrcSource(new SceneItemSelectionWidget(parent)),
	  _trafoSrcLabel(new QLabel(obs_module_text(
		  "AdvSceneSwitcher.action.filter.copyTransform.entry.label"))),
	  _getSettings(new QPushButton(obs_module_text(
		  "AdvSceneSwitcher.action.filter.getSettings"))),
	  _settings(new VariableTextEdit(this))
{
	_filters->setSizeAdjustPolicy(QComboBox::AdjustToContents);

	populateActionSelection(_actions);
	auto sources = GetSourcesWithFilterNames();
	sources.sort();
	_sources->SetSourceNameList(sources);

	QWidget::connect(_actions, SIGNAL(currentIndexChanged(int)), this,
			 SLOT(ActionChanged(int)));
	QWidget::connect(_settingsButtons, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(ButtonChanged(int)));
	QWidget::connect(_sources,
			 SIGNAL(SourceChanged(const SourceSelection &)), this,
			 SLOT(SourceChanged(const SourceSelection &)));
	QWidget::connect(_filters,
			 SIGNAL(FilterChanged(const FilterSelection &)), this,
			 SLOT(FilterChanged(const FilterSelection &)));
	QWidget::connect(_getSettings, SIGNAL(clicked()), this,
			 SLOT(GetSettingsClicked()));
	QWidget::connect(_settings, SIGNAL(textChanged()), this,
			 SLOT(SettingsChanged()));
	QWidget::connect(_trafoSrcScene,
			 SIGNAL(SceneChanged(const SceneSelection &)), this,
			 SLOT(TrafoSrcSceneChanged(const SceneSelection &)));
	QWidget::connect(_trafoSrcScene,
			 SIGNAL(SceneChanged(const SceneSelection &)),
			 _trafoSrcSource,
			 SLOT(SceneChanged(const SceneSelection &)));
	QWidget::connect(
		_trafoSrcSource,
		SIGNAL(SceneItemChanged(const SceneItemSelection &)), this,
		SLOT(TrafoSrcSourceChanged(const SceneItemSelection &)));

	QHBoxLayout *entryLayout = new QHBoxLayout;

	std::unordered_map<std::string, QWidget *> widgetPlaceholders = {
		{"{{sources}}", _sources},
		{"{{filters}}", _filters},
		{"{{actions}}", _actions},
		{"{{settings}}", _settings},
		{"{{settingsButtons}}", _settingsButtons},
		{"{{getSettings}}", _getSettings},
	};
	PlaceWidgets(obs_module_text("AdvSceneSwitcher.action.filter.entry"),
		     entryLayout, widgetPlaceholders);

	QHBoxLayout *buttonLayout = new QHBoxLayout;
	buttonLayout->addWidget(_getSettings);
	buttonLayout->addStretch();
	buttonLayout->setContentsMargins(0, 0, 0, 0);

	QHBoxLayout *trafoSelectionLayout = new QHBoxLayout;
	std::unordered_map<std::string, QWidget *>
		widgetPlaceholdersTrafoSelection = {
			{"{{label}}", _trafoSrcLabel},
			{"{{trafoSrcScene}}", _trafoSrcScene},
			{"{{trafoSrcSource}}", _trafoSrcSource},
		};
	PlaceWidgets(
		obs_module_text(
			"AdvSceneSwitcher.action.source.copyTransform.entry"),
		trafoSelectionLayout, widgetPlaceholdersTrafoSelection);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addLayout(entryLayout);
	mainLayout->addWidget(_settings);
	mainLayout->addLayout(buttonLayout);
	mainLayout->addLayout(trafoSelectionLayout);
	setLayout(mainLayout);

	_entryData = entryData;
	UpdateEntryData();
	_loading = false;
}

void MacroActionFilterEdit::UpdateEntryData()
{
	if (!_entryData) {
		return;
	}

	populateFilterButtonSelection(
		_settingsButtons,
		_entryData->_filter.GetFilters(_entryData->_source)[0]);
	_actions->setCurrentIndex(static_cast<int>(_entryData->_action));
	_sources->SetSource(_entryData->_source);
	_filters->SetFilter(_entryData->_source, _entryData->_filter);
	_settings->setPlainText(_entryData->_settings);
	_trafoSrcScene->SetScene(_entryData->_trafoSrcScene);
	_trafoSrcSource->SetSceneItem(_entryData->_trafoSrcSource);
	SetWidgetVisibility();

	adjustSize();
	updateGeometry();
}

void MacroActionFilterEdit::SourceChanged(const SourceSelection &source)
{
	if (_loading || !_entryData) {
		return;
	}

	{
		auto lock = LockContext();
		_entryData->_source = source;
	}
	populateFilterButtonSelection(
		_settingsButtons,
		_entryData->_filter.GetFilters(_entryData->_source)[0]);
}

void MacroActionFilterEdit::FilterChanged(const FilterSelection &filter)
{
	if (_loading || !_entryData) {
		return;
	}

	{
		auto lock = LockContext();
		_entryData->_filter = filter;
	}
	populateFilterButtonSelection(
		_settingsButtons,
		_entryData->_filter.GetFilters(_entryData->_source)[0]);
	emit HeaderInfoChanged(
		QString::fromStdString(_entryData->GetShortDesc()));
}

void MacroActionFilterEdit::ActionChanged(int value)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_action = static_cast<MacroActionFilter::Action>(value);
	SetWidgetVisibility();
}

void MacroActionFilterEdit::ButtonChanged(int idx)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_button = qvariant_cast<FilterSettingButton>(
		_settingsButtons->itemData(idx));
}

void MacroActionFilterEdit::GetSettingsClicked()
{
	if (_loading || !_entryData ||
	    _entryData->_filter.GetFilters(_entryData->_source).empty()) {
		return;
	}

	_settings->setPlainText(FormatJsonString(GetSourceSettings(
		_entryData->_filter.GetFilters(_entryData->_source).at(0))));
}

void MacroActionFilterEdit::SettingsChanged()
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_settings = _settings->toPlainText().toStdString();

	adjustSize();
	updateGeometry();
}

void MacroActionFilterEdit::TrafoSrcSceneChanged(const SceneSelection &s)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_trafoSrcScene = s;
}

void MacroActionFilterEdit::TrafoSrcSourceChanged(const SceneItemSelection &item)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_trafoSrcSource = item;
	adjustSize();
	updateGeometry();
}

void MacroActionFilterEdit::SetWidgetVisibility()
{
	const bool showSettings = _entryData->_action ==
				  MacroActionFilter::Action::SETTINGS;
	const bool showTrafoSrcSelection =
		_entryData->_action ==
		MacroActionFilter::Action::COPY_TRAFO_TO_SETTINGS;
	_settings->setVisible(showSettings);
	_getSettings->setVisible(showSettings);
	_settingsButtons->setVisible(
		_entryData->_action ==
		MacroActionFilter::Action::SETTINGS_BUTTON);
	_trafoSrcLabel->setVisible(showTrafoSrcSelection);
	_trafoSrcScene->setVisible(showTrafoSrcSelection);
	_trafoSrcSource->setVisible(showTrafoSrcSelection);
	adjustSize();
	updateGeometry();
}

bool FilterSettingButton::Save(obs_data_t *obj) const
{
	auto data = obs_data_create();
	obs_data_set_string(data, "id", id.c_str());
	obs_data_set_string(data, "description", description.c_str());
	obs_data_set_obj(obj, "filterSettingButton", data);
	obs_data_release(data);
	return true;
}

bool FilterSettingButton::Load(obs_data_t *obj)
{
	auto data = obs_data_get_obj(obj, "filterSettingButton");
	id = obs_data_get_string(data, "id");
	description = obs_data_get_string(data, "description");
	obs_data_release(data);
	return true;
}

std::string FilterSettingButton::ToString() const
{
	if (id.empty()) {
		return "";
	}
	return "[" + id + "] " + description;
}

} // namespace advss
