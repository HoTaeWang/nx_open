// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "motion_event.h"

#include <nx/utils/metatypes.h>

#include "../event_filter_fields/source_camera_field.h"
#include "../strings.h"
#include "../utils/event_details.h"
#include "../utils/field.h"
#include "../utils/type.h"

namespace nx::vms::rules {

MotionEvent::MotionEvent(std::chrono::microseconds timestamp, State state, nx::Uuid deviceId):
    base_type(timestamp, state),
    m_deviceId(deviceId)
{
}

QString MotionEvent::resourceKey() const
{
    return deviceId().toSimpleString();
}

QVariantMap MotionEvent::details(
    common::SystemContext* context, const nx::vms::api::rules::PropertyMap& aggregatedInfo) const
{
    auto result = base_type::details(context, aggregatedInfo);

    result.insert(utils::kExtendedCaptionDetailName, extendedCaption(context));
    utils::insertLevel(result, nx::vms::event::Level::common);
    utils::insertIcon(result, nx::vms::rules::Icon::motion);
    utils::insertClientAction(result, nx::vms::rules::ClientAction::previewCameraOnTime);

    return result;
}

QString MotionEvent::extendedCaption(common::SystemContext* context) const
{
    const auto resourceName = Strings::resource(context, deviceId(), Qn::RI_WithUrl);
    return tr("Motion on %1").arg(resourceName);
}

const ItemDescriptor& MotionEvent::manifest()
{
    static const auto kDescriptor = ItemDescriptor{
        .id = utils::type<MotionEvent>(),
        .displayName = NX_DYNAMIC_TRANSLATABLE(tr("Motion on Camera")),
        .description = "Triggered when motion is detected on the selected cameras. "
            "Note: recording must be enabled for the rule to function.",
        .flags = ItemFlag::prolonged,
        .fields = {
            utils::makeStateFieldDescriptor(Strings::beginWhen()),
            makeFieldDescriptor<SourceCameraField>(
                utils::kDeviceIdFieldName,
                Strings::occursAt(),
                {},
                ResourceFilterFieldProperties{
                    .acceptAll = true,
                    .allowEmptySelection = true,
                    .validationPolicy = kCameraMotionValidationPolicy
                }.toVariantMap()),
        },
        .resources = {
            {utils::kDeviceIdFieldName, {ResourceType::device, Qn::ViewContentPermission}}},
        .emailTemplateName = "camera_motion.mustache"
    };
    return kDescriptor;
}

} // namespace nx::vms::rules
