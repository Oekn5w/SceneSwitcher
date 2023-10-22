#pragma once
#include "macro-action-edit.hpp"
#include "variable-text-edit.hpp"
#include "source-selection.hpp"
#include "filter-selection.hpp"
#include "scene-selection.hpp"
#include "scene-item-selection.hpp"

#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>

namespace advss {

struct FilterSettingButton {
	bool Save(obs_data_t *obj) const;
	bool Load(obs_data_t *obj);
	std::string ToString() const;

	std::string id = "";
	std::string description = "";
};

class MacroActionFilter : public MacroAction {
public:
	MacroActionFilter(Macro *m) : MacroAction(m) {}
	bool PerformAction();
	void LogAction() const;
	bool Save(obs_data_t *obj) const;
	bool Load(obs_data_t *obj);
	std::string GetShortDesc() const;
	std::string GetId() const { return id; };
	static std::shared_ptr<MacroAction> Create(Macro *m)
	{
		return std::make_shared<MacroActionFilter>(m);
	}

	enum class Action {
		ENABLE,
		DISABLE,
		TOGGLE,
		SETTINGS,
		SETTINGS_BUTTON,
		COPY_TRAFO_TO_SETTINGS,
	};

	SourceSelection _source;
	FilterSelection _filter;
	FilterSettingButton _button;
	Action _action = Action::ENABLE;
	StringVariable _settings = "";
	SceneSelection _trafoSrcScene;
	SceneItemSelection _trafoSrcSource;

private:
	static bool _registered;
	static const std::string id;
};

class MacroActionFilterEdit : public QWidget {
	Q_OBJECT

public:
	MacroActionFilterEdit(
		QWidget *parent,
		std::shared_ptr<MacroActionFilter> entryData = nullptr);
	void UpdateEntryData();
	static QWidget *Create(QWidget *parent,
			       std::shared_ptr<MacroAction> action)
	{
		return new MacroActionFilterEdit(
			parent,
			std::dynamic_pointer_cast<MacroActionFilter>(action));
	}

private slots:
	void SourceChanged(const SourceSelection &);
	void FilterChanged(const FilterSelection &);
	void ActionChanged(int value);
	void ButtonChanged(int idx);
	void GetSettingsClicked();
	void SettingsChanged();
	void TrafoSrcSceneChanged(const SceneSelection &);
	void TrafoSrcSourceChanged(const SceneItemSelection &);
signals:
	void HeaderInfoChanged(const QString &);

protected:
	SourceSelectionWidget *_sources;
	FilterSelectionWidget *_filters;
	QComboBox *_actions;
	QComboBox *_settingsButtons;
	SceneSelectionWidget *_trafoSrcScene;
	SceneItemSelectionWidget *_trafoSrcSource;
	QLabel *_trafoSrcLabel;
	QPushButton *_getSettings;
	VariableTextEdit *_settings;
	std::shared_ptr<MacroActionFilter> _entryData;

private:
	void SetWidgetVisibility();
	bool _loading = true;
};

} // namespace advss
