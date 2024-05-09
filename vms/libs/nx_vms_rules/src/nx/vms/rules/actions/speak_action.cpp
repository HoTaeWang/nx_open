// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "speak_action.h"

#include <nx/utils/qt_helpers.h>
#include <nx/vms/api/data/user_group_data.h>

#include "../action_builder_fields/target_device_field.h"
#include "../action_builder_fields/target_user_field.h"
#include "../action_builder_fields/text_with_fields.h"
#include "../action_builder_fields/volume_field.h"
#include "../utils/field.h"
#include "../utils/type.h"

namespace nx::vms::rules {

const ItemDescriptor& SpeakAction::manifest()
{
    static const auto kDescriptor = ItemDescriptor{
        .id = utils::type<SpeakAction>(),
        .displayName = tr("Speak"),
        .flags = {ItemFlag::instant},
        .executionTargets = {ExecutionTarget::clients, ExecutionTarget::servers},
        .targetServers = TargetServers::resourceOwner,
        .fields = {
            makeFieldDescriptor<TextWithFields>(utils::kTextFieldName, tr("Text")),
            makeFieldDescriptor<TargetDeviceField>(utils::kDeviceIdsFieldName, tr("At Device")),
            makeFieldDescriptor<TargetUserField>(
                utils::kUsersFieldName,
                tr("To users"),
                {},
                ResourceFilterFieldProperties{
                    .acceptAll = false,
                    .ids = nx::utils::toQSet(vms::api::kAllPowerUserGroupIds),
                    .allowEmptySelection = true,
                    .validationPolicy = {}
                }.toVariantMap()),
            makeFieldDescriptor<VolumeField>("volume", tr("Volume")),
            utils::makeIntervalFieldDescriptor(tr("Interval of Action")),
        },
        .resources = {{utils::kDeviceIdsFieldName, {ResourceType::device, {}, {}, FieldFlag::target}}},
    };
    return kDescriptor;
}

} // namespace nx::vms::rules
