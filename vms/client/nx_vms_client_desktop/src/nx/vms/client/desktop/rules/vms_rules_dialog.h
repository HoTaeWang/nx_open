// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/vms/client/desktop/common/dialogs/qml_dialog_wrapper.h>
#include <nx/vms/rules/rules_fwd.h>
#include <ui/dialogs/common/session_aware_dialog.h>
#include <ui/workbench/workbench_context_aware.h>

namespace nx::vms::client::desktop::rules {

class RulesTableModel;

class VmsRulesDialog: public QmlDialogWrapper, public QnSessionAwareDelegate
{
    Q_OBJECT

public:
    explicit VmsRulesDialog(QWidget* parent);
    ~VmsRulesDialog() override;

    void setError(const QString& error);
    void setFilter(const QString& filter);

    Q_INVOKABLE void addRule();
    Q_INVOKABLE void editSchedule(const UuidList& ids);
    Q_INVOKABLE void duplicateRule(nx::Uuid id);
    Q_INVOKABLE void editRule(nx::Uuid id);
    Q_INVOKABLE void deleteRules(const UuidList& ids);
    Q_INVOKABLE void setRulesState(const UuidList& ids, bool isEnabled);
    Q_INVOKABLE void resetToDefaults();
    Q_INVOKABLE void openEventLogDialog();

    bool tryClose(bool force) override;

private:
    QWidget* m_parentWidget{nullptr};
    RulesTableModel* m_rulesTableModel{nullptr};

    void deleteRulesImpl(const UuidList& ids);
    void saveRuleImpl(const std::shared_ptr<vms::rules::Rule>& rule);
    void resetToDefaultsImpl();
};

} // namespace nx::vms::client::desktop::rules
