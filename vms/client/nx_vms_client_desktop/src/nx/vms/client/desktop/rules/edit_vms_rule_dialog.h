// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/utils/scoped_connections.h>
#include <nx/vms/client/desktop/common/widgets/editable_label.h>
#include <nx/vms/client/desktop/common/widgets/slide_switch.h>
#include <nx/vms/client/desktop/system_context_aware.h>
#include <nx/vms/rules/rule.h>
#include <ui/dialogs/common/session_aware_dialog.h>

class QLabel;

namespace nx::vms::client::desktop::rules {

class ActionTypePickerWidget;
class EditableTitleWidget;
class EventTypePickerWidget;

class EditVmsRuleDialog:
    public QnSessionAwareButtonBoxDialog
{
    Q_OBJECT

public:
    EditVmsRuleDialog(QWidget* parent = nullptr);

    void setRule(std::shared_ptr<vms::rules::Rule> rule);

    void accept() override;
    void reject() override;

signals:
    void hasChangesChanged();

protected:
    void buttonBoxClicked(QDialogButtonBox::StandardButton button) override;

private:
    QLabel* m_eventLabel{nullptr};
    QLabel* m_actionLabel{nullptr};
    EditableLabel* m_editableLabel{nullptr};
    QPushButton* m_deleteButton{nullptr};
    QWidget* m_contentWidget{nullptr};
    EventTypePickerWidget* m_eventTypePicker{nullptr};
    QWidget* m_eventEditorWidget{nullptr};
    ActionTypePickerWidget* m_actionTypePicker{nullptr};
    QWidget* m_actionEditorWidget{nullptr};
    QPushButton* m_enabledButton{nullptr};
    bool m_hasChanges = false;

    std::shared_ptr<vms::rules::Rule> m_rule;

    nx::utils::ScopedConnections m_scopedConnections;

    void displayComment();
    void displayRule();
    void displayState();
    void displayControls();

    void displayActionEditor();
    void displayEventEditor();

    void onCommentChanged(const QString& comment);
    void onDeleteClicked();
    void onScheduleClicked();
    void onActionTypeChanged(const QString& actionType);
    void onEventTypeChanged(const QString& eventType);
    void onEnabledButtonClicked(bool checked);

    void onEventFilterModified();
    void onActionBuilderModified();

    void setHasChanges(bool hasChanges);
};

} // namespace nx::vms::client::desktop::rules
