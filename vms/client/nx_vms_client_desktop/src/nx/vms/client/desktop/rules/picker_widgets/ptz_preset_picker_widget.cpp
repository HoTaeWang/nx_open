// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "ptz_preset_picker_widget.h"

#include <core/ptz/abstract_ptz_controller.h>
#include <core/ptz/activity_ptz_controller.h>
#include <core/ptz/fallback_ptz_controller.h>
#include <core/ptz/preset_ptz_controller.h>
#include <core/ptz/ptz_controller_pool.h>
#include <core/resource/camera_resource.h>
#include <core/resource_management/resource_pool.h>
#include <nx/vms/client/desktop/system_context.h>
#include <nx/vms/rules/action_builder_fields/target_device_field.h>
#include <nx/vms/rules/utils/field.h>
#include <ui/fisheye/fisheye_ptz_controller.h>

namespace nx::vms::client::desktop::rules {

PtzPresetPicker::PtzPresetPicker(
    vms::rules::PtzPresetField* field,
    SystemContext* context,
    ParamsWidget* parent)
    :
    DropdownTextPickerWidgetBase<vms::rules::PtzPresetField>(field, context, parent)
{
}

void PtzPresetPicker::updateUi()
{
    DropdownTextPickerWidgetBase<vms::rules::PtzPresetField>::updateUi();

    const auto cameraField =
        getActionField<vms::rules::TargetDeviceField>(vms::rules::utils::kDeviceIdFieldName);

    if (!NX_ASSERT(cameraField))
        return;

    m_presets.clear();
    m_ptzController.reset();

    auto camera = resourcePool()->getResourceById<QnVirtualCameraResource>(cameraField->id());
    if (!camera)
        return;

    auto fisheyeController =
        QSharedPointer<QnPresetPtzController>::create(QSharedPointer<QnFisheyePtzController>(
            new QnFisheyePtzController(camera), &QObject::deleteLater));

    auto systemContext = SystemContext::fromResource(camera);
    auto ptzPool = systemContext->ptzControllerPool();
    if (QnPtzControllerPtr serverController = ptzPool->controller(camera))
    {
        serverController.reset(new QnActivityPtzController(
            QnActivityPtzController::Client,
            serverController));
        m_ptzController.reset(new QnFallbackPtzController(fisheyeController, serverController));
    }
    else
    {
        m_ptzController = fisheyeController;
    }

    connect(
        m_ptzController.data(),
        &QnAbstractPtzController::changed,
        this,
        &PtzPresetPicker::updateComboBox);

    m_ptzController->getPresets(&m_presets);

    updateComboBox();
}

void PtzPresetPicker::onActivated()
{
    m_field->setValue(m_comboBox->currentData().toString());

    DropdownTextPickerWidgetBase<vms::rules::PtzPresetField>::onActivated();
}

void PtzPresetPicker::updateComboBox()
{
    m_comboBox->clear();
    for (const auto& preset: m_presets)
        m_comboBox->addItem(preset.name, preset.id);

    m_comboBox->setCurrentIndex(m_comboBox->findData(m_field->value()));
}

} // namespace nx::vms::client::desktop::rules
