// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/vms/rules/action_builder_fields/target_user_field.h>

#include "resource_picker_widget_base.h"

namespace nx::vms::client::desktop::rules {

class TargetUserPicker: public ResourcePickerWidgetBase<vms::rules::TargetUserField>
{
    Q_OBJECT

public:
    using ResourcePickerWidgetBase<vms::rules::TargetUserField>::ResourcePickerWidgetBase;

protected:
    void onSelectButtonClicked() override;
    void updateUi() override;
};

} // namespace nx::vms::client::desktop::rules
