// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "object_lookup_picker_widget.h"

#include <QtCore/QSortFilterProxyModel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>

#include <nx/vms/client/desktop/rules/utils/strings.h>
#include <nx/vms/client/desktop/style/helper.h>
#include <nx/vms/client/desktop/system_context.h>
#include <nx/vms/rules/event_filter_fields/analytics_object_type_field.h>
#include <nx/vms/rules/field_types.h>
#include <nx/vms/rules/utils/field.h>
#include <ui/widgets/common/elided_label.h>

#include "../model_view/lookup_lists_model.h"

namespace nx::vms::client::desktop::rules {

using LookupCheckType = vms::rules::ObjectLookupCheckType;

ObjectLookupPicker::ObjectLookupPicker(
    vms::rules::ObjectLookupField* field,
    SystemContext* context,
    ParamsWidget* parent)
    :
    TitledFieldPickerWidget<vms::rules::ObjectLookupField>(field, context, parent)
{
    setCheckBoxEnabled(false);

    const auto contentLayout = new QVBoxLayout;

    const auto typeLayout = new QHBoxLayout;
    typeLayout->setSpacing(style::Metrics::kDefaultLayoutSpacing.width());

    typeLayout->addWidget(new QWidget);

    auto comboBoxesLayout = new QHBoxLayout;

    m_checkTypeComboBox = new QComboBox;
    m_checkTypeComboBox->addItem(
        tr("Has attributes"),
        QVariant::fromValue(LookupCheckType::hasAttributes));
    m_checkTypeComboBox->addItem(
        tr("Listed"),
        QVariant::fromValue(LookupCheckType::inList));
    m_checkTypeComboBox->addItem(
        tr("Not listed"),
        QVariant::fromValue(LookupCheckType::notInList));

    comboBoxesLayout->addWidget(m_checkTypeComboBox);

    typeLayout->addLayout(comboBoxesLayout);

    typeLayout->setStretch(0, 1);
    typeLayout->setStretch(1, 5);

    contentLayout->addLayout(typeLayout);

    m_stackedWidget = new QStackedWidget;

    {
        auto keywordsWidget = new QWidget;
        auto keywordsLayout = new QHBoxLayout{keywordsWidget};

        auto attributesLabel = new QnElidedLabel;
        attributesLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        attributesLabel->setElideMode(Qt::ElideRight);
        attributesLabel->setText(tr("Attributes"));
        keywordsLayout->addWidget(attributesLabel);

        m_lineEdit = new QLineEdit;
        keywordsLayout->addWidget(m_lineEdit);

        keywordsLayout->setStretch(0, 1);
        keywordsLayout->setStretch(1, 5);

        m_stackedWidget->addWidget(keywordsWidget);
    }

    {
        auto lookupListsWidget = new QWidget;
        auto lookupListsLayout = new QHBoxLayout{lookupListsWidget};

        auto lookupListsLabel = new QnElidedLabel;
        lookupListsLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        lookupListsLabel->setElideMode(Qt::ElideRight);
        lookupListsLabel->setText(tr("From"));
        lookupListsLayout->addWidget(lookupListsLabel);

        m_lookupListComboBox = new QComboBox;

        m_lookupListsModel = new LookupListsModel{context, this};
        auto sortModel = new QSortFilterProxyModel{this};
        sortModel->setSourceModel(m_lookupListsModel);
        sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        sortModel->sort(0);
        m_lookupListComboBox->setModel(sortModel);
        m_lookupListComboBox->setPlaceholderText(Strings::selectString());

        lookupListsLayout->addWidget(m_lookupListComboBox);

        lookupListsLayout->setStretch(0, 1);
        lookupListsLayout->setStretch(1, 5);

        m_stackedWidget->addWidget(lookupListsWidget);
    }

    contentLayout->addWidget(m_stackedWidget);

    m_contentWidget->setLayout(contentLayout);

    connect(
        m_checkTypeComboBox,
        &QComboBox::activated,
        this,
        [this]
        {
            m_field->setCheckType(m_checkTypeComboBox->currentData().value<LookupCheckType>());

            setEdited();
        });

    connect(
        m_lookupListComboBox,
        &QComboBox::activated,
        this,
        [this](int index)
        {
            m_field->setValue(m_lookupListComboBox->itemData(index).value<nx::Uuid>().toString());

            setEdited();
        });

    connect(
        m_lineEdit,
        &QLineEdit::textEdited,
        this,
        [this](const QString& text)
        {
            m_field->setValue(text);

            setEdited();
        });
}

void ObjectLookupPicker::updateUi()
{
    TitledFieldPickerWidget<vms::rules::ObjectLookupField>::updateUi();

    m_checkTypeComboBox->setCurrentIndex(
        m_checkTypeComboBox->findData(QVariant::fromValue(m_field->checkType())));

    if (m_field->checkType() == LookupCheckType::hasAttributes)
    {
        if (nx::Uuid::isUuidString(m_field->value()))
            m_field->setValue({});

        m_stackedWidget->setCurrentIndex(0);
        m_lineEdit->setText(m_field->value());
    }
    else
    {
        if (!nx::Uuid::isUuidString(m_field->value()))
            m_field->setValue({});

        const auto objectTypeField = getEventField<vms::rules::AnalyticsObjectTypeField>(
            vms::rules::utils::kObjectTypeIdFieldName);
        if (NX_ASSERT(
            objectTypeField,
            "%1 field must be provided for the given event",
            vms::rules::utils::kObjectTypeIdFieldName))
        {
            if (m_lookupListsModel->objectTypeId() != objectTypeField->value())
                m_lookupListsModel->setObjectTypeId(objectTypeField->value());
        }

        const auto matches = m_lookupListComboBox->model()->match(
            m_lookupListComboBox->model()->index(0, 0),
            LookupListsModel::LookupListIdRole,
            QVariant::fromValue(nx::Uuid{m_field->value()}),
            /*hits*/ 1,
            Qt::MatchExactly);

        m_lookupListComboBox->setCurrentIndex(matches.size() == 1 ? matches[0].row() : -1);

        m_stackedWidget->setCurrentIndex(1);
    }
}

} // namespace nx::vms::client::desktop::rules
