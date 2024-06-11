// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/vms/api/types/event_rule_types.h>
#include <nx/vms/rules/basic_event.h>

#include "../data_macros.h"

namespace nx::vms::rules {

class NX_VMS_RULES_API LdapSyncIssueEvent: public BasicEvent
{
    Q_OBJECT
    Q_CLASSINFO("type", "nx.events.ldapSyncIssue")
public:
    using Reasons = QMap<vms::api::EventReason, int>;

    LdapSyncIssueEvent() = default;
    LdapSyncIssueEvent(
        std::chrono::microseconds timestamp,
        vms::api::EventReason reasonCode,
        std::chrono::seconds syncInterval,
        nx::Uuid serverId);

    virtual QString resourceKey() const override;
    virtual QString uniqueName() const override;
    virtual QVariantMap details(common::SystemContext* context) const override;

    void fillAggregationInfo(const AggregatedEventPtr& aggregatedEvent) override;

    static const ItemDescriptor& manifest();

private:
    FIELD(vms::api::EventReason, reasonCode, setReasonCode)
    FIELD(std::chrono::seconds, syncInterval, setSyncInterval)
    FIELD(nx::Uuid, serverId, setServerId)
    FIELD(Reasons, countByReasons, setCountByReasons)

    QString reasonText() const;
    QString ldapSyncIssueReason(vms::api::EventReason reasonCode) const;
};

} // namespace nx::vms::rules
