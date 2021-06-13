#pragma once
#include <QDoubleSpinBox>
#include "macro-action-edit.hpp"

enum class VCamAction {
	STOP,
	START,
};

class MacroActionVCam : public MacroAction {
public:
	bool PerformAction();
	void LogAction();
	bool Save(obs_data_t *obj);
	bool Load(obs_data_t *obj);
	std::string GetId() { return id; };
	static std::shared_ptr<MacroAction> Create()
	{
		return std::make_shared<MacroActionVCam>();
	}

	VCamAction _action = VCamAction::STOP;

private:
	static bool _registered;
	static const std::string id;
};

class MacroActionVCamEdit : public QWidget {
	Q_OBJECT

public:
	MacroActionVCamEdit(
		QWidget *parent,
		std::shared_ptr<MacroActionVCam> entryData = nullptr);
	void UpdateEntryData();
	static QWidget *Create(QWidget *parent,
			       std::shared_ptr<MacroAction> action)
	{
		return new MacroActionVCamEdit(
			parent,
			std::dynamic_pointer_cast<MacroActionVCam>(action));
	}

private slots:
	void ActionChanged(int value);

protected:
	QComboBox *_actions;
	std::shared_ptr<MacroActionVCam> _entryData;

private:
	QHBoxLayout *_mainLayout;
	bool _loading = true;
};
