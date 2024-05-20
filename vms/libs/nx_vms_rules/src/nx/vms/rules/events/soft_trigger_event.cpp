// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "soft_trigger_event.h"

#include "../event_filter_fields/customizable_icon_field.h"
#include "../event_filter_fields/customizable_text_field.h"
#include "../event_filter_fields/source_camera_field.h"
#include "../event_filter_fields/source_user_field.h"
#include "../event_filter_fields/unique_id_field.h"
#include "../strings.h"
#include "../utils/event_details.h"
#include "../utils/field.h"
#include "../utils/type.h"

namespace nx::vms::rules {

SoftTriggerEvent::SoftTriggerEvent(
    std::chrono::microseconds timestamp,
    State state,
    nx::Uuid triggerId,
    nx::Uuid cameraId,
    nx::Uuid userId,
    const QString& name,
    const QString& icon)
    :
    BasicEvent(timestamp, state),
    m_triggerId(triggerId),
    m_cameraId(cameraId),
    m_userId(userId),
    m_triggerName(name),
    m_triggerIcon(icon)
{
}

QString SoftTriggerEvent::uniqueName() const
{
    // All the soft trigger events must be considered as unique events.
    return nx::Uuid::createUuid().toString();
}

QString SoftTriggerEvent::resourceKey() const
{
    return m_cameraId.toSimpleString();
}

QString SoftTriggerEvent::aggregationKey() const
{
    // Soft trigger are aggregated regardless to resource.
    // See eventKey in RuleProcessor::processInstantAction.
    return m_triggerId.toSimpleString();
}

QVariantMap SoftTriggerEvent::details(common::SystemContext* context) const
{
    auto result = BasicEvent::details(context);

    utils::insertIfNotEmpty(result, utils::kCaptionDetailName, caption());
    utils::insertIfNotEmpty(result, utils::kDetailingDetailName, detailing());
    utils::insertIfNotEmpty(result, utils::kExtendedCaptionDetailName, extendedCaption(context));
    utils::insertIfNotEmpty(result, utils::kTriggerNameDetailName, trigger());
    utils::insertIfValid(result, utils::kUserIdDetailName, QVariant::fromValue(m_userId));
    result.insert(utils::kEmailTemplatePathDetailName, manifest().emailTemplatePath);

    utils::insertLevel(result, nx::vms::event::Level::common);
    utils::insertIcon(result, nx::vms::rules::Icon::softTrigger);
    utils::insertIfNotEmpty(result, utils::kCustomIconDetailName, triggerIcon());
    utils::insertClientAction(result, nx::vms::rules::ClientAction::previewCameraOnTime);

    return result;
}

QString SoftTriggerEvent::trigger() const
{
    return m_triggerName.isEmpty() ? tr("Trigger Name") : m_triggerName;
}

QString SoftTriggerEvent::caption() const
{
    return QString("%1 %2").arg(manifest().displayName).arg(trigger());
}

QString SoftTriggerEvent::detailing() const
{
    return tr("Trigger: %1").arg(trigger());
}

QString SoftTriggerEvent::extendedCaption(common::SystemContext* context) const
{
    return tr("Soft Trigger %1 at %2")
        .arg(trigger())
        .arg(Strings::resource(context, cameraId(), Qn::RI_WithUrl));
}

const ItemDescriptor& SoftTriggerEvent::manifest()
{
    static const auto kDescriptor = ItemDescriptor{
        .id = utils::type<SoftTriggerEvent>(),
        .displayName = NX_DYNAMIC_TRANSLATABLE(tr("Soft Trigger")),
        .flags = {ItemFlag::instant, ItemFlag::prolonged},
        .fields = {
            makeFieldDescriptor<UniqueIdField>("triggerId",
                NX_DYNAMIC_TRANSLATABLE(tr("Invisible"))),
            makeFieldDescriptor<SourceCameraField>(
                utils::kCameraIdFieldName,
                Strings::occursAt(),
                {},
                ResourceFilterFieldProperties{
                    .acceptAll = true,
                    .allowEmptySelection = true
                }.toVariantMap()),
            makeFieldDescriptor<SourceUserField>(
                utils::kUserIdFieldName,
                NX_DYNAMIC_TRANSLATABLE(tr("By")),
                {},
                ResourceFilterFieldProperties{
                    .acceptAll = false,
                    .ids = {},
                    .allowEmptySelection = false,
                    .validationPolicy = kUserInputValidationPolicy
                }.toVariantMap()),
            makeFieldDescriptor<CustomizableTextField>("triggerName",
                NX_DYNAMIC_TRANSLATABLE(tr("Name"))),
            makeFieldDescriptor<CustomizableIconField>("triggerIcon",
                NX_DYNAMIC_TRANSLATABLE(tr("Icon"))),
        },
        .resources = {
            {utils::kCameraIdFieldName, {ResourceType::device, Qn::ViewContentPermission}},
            {utils::kUserIdFieldName, {ResourceType::user}}},
        .emailTemplatePath = ":/email_templates/software_trigger.mustache"
    };
    return kDescriptor;
}

} // namespace nx::vms::rules
