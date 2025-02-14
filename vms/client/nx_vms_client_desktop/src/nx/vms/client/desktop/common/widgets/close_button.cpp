// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "close_button.h"

#include <nx/vms/client/core/skin/color_theme.h>
#include <nx/vms/client/core/skin/skin.h>

namespace nx::vms::client::desktop {

static const QColor kLight16Color = "#698796";
static const nx::vms::client::core::SvgIconColorer::IconSubstitutions kIconSubstitutions = {
    {QIcon::Normal, {{kLight16Color, "light16"}}},
    {QIcon::Active, {{kLight16Color, "light17"}}},
    {QIcon::Selected, {{kLight16Color, "light15"}}},
};

CloseButton::CloseButton(QWidget* parent):
    HoverButton(qnSkin->icon("text_buttons/cross_close_20.svg", kIconSubstitutions), parent)
{
    setFixedSize(HoverButton::sizeHint());
}

} // namespace nx::vms::client::desktop
