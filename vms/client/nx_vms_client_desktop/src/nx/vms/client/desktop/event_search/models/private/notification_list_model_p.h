// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <QtCore/QHash>
#include <QtCore/QScopedPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QTimer>

#include <nx/vms/event/event_fwd.h>
#include <nx/vms/rules/actions/actions_fwd.h>

#include "../notification_list_model.h"
#include "sound_controller.h"

class AudioPlayer;

namespace nx::vms::event { class StringsHelper; }

namespace nx::vms::rules {

class NotificationActionBase;
class NotificationAction;
class RepeatSoundAction;
class ShowOnAlarmLayoutAction;

} // namespace nx::vms::rules

namespace nx::vms::client::desktop {

class NotificationListModel::Private:
    public QObject,
    public WindowContextAware
{
    Q_OBJECT
    using base_type = QObject;

public:
    explicit Private(NotificationListModel* q);
    virtual ~Private() override;

    int maximumCount() const;
    void setMaximumCount(int value);

private:
    void onNotificationActionBase(
        const QSharedPointer<nx::vms::rules::NotificationActionBase>& action,
        const QString& cloudSystemId);

    void onNotificationAction(
        const QSharedPointer<nx::vms::rules::NotificationAction>& action,
        const QString& cloudSystemId);

    void onRepeatSoundAction(
        const QSharedPointer<nx::vms::rules::RepeatSoundAction>& action);

    void onAlarmLayoutAction(
        const QSharedPointer<nx::vms::rules::ShowOnAlarmLayoutAction>& action);

    void removeNotification(const nx::vms::rules::NotificationActionBasePtr& action);

    void addNotification(const vms::event::AbstractActionPtr& action);
    void onNotificationRemoved(const vms::event::AbstractActionPtr& action);

    void onRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last);

    void onProcessNotificationsCacheTimeout();

    void updateCloudItems(const QString& systemId);
    void removeCloudItems(const QString& systemId);

    QString caption(const nx::vms::event::EventParameters& parameters,
        const QnVirtualCameraResourcePtr& camera) const;
    QString description(const nx::vms::event::EventParameters& parameters) const;
    QString tooltip(const vms::event::AbstractActionPtr& action) const;

    QString getPoeOverBudgetDescription(const nx::vms::event::EventParameters& parameters) const;

    QString iconPath(const vms::event::AbstractActionPtr& action) const;
    QString iconPath(
        const nx::vms::rules::NotificationActionPtr& action, const QString& cloudSystemId) const;

    void setupClientAction(
        const nx::vms::rules::NotificationActionPtr& action,
        EventData& eventData);
    void setupAcknowledgeAction(
        const nx::vms::rules::NotificationActionPtr& action,
        const QnResourcePtr& device,
        EventData& eventData);

    void truncateToMaximumCount();

    void removeAllItems(nx::Uuid ruleId);

private:
    NotificationListModel* const q = nullptr;
    int m_maximumCount = NotificationListModel::kDefaultMaximumCount;
    QScopedPointer<vms::event::StringsHelper> m_helper;

    // Used for deduplication of alarm layout tiles.
    QHash<nx::Uuid/*ruleId*/, QHash<nx::Uuid /*sourceId*/, QSet<nx::Uuid /*itemId*/>>> m_uuidHashes;

    SoundController m_soundController;

    QMultiHash<QString /*system id*/, nx::Uuid /*item id*/> m_itemsByCloudSystem;

    QTimer m_notificationsCacheTimer;
    std::list<EventData> m_notificationsCache;
};

} // namespace nx::vms::client::desktop
