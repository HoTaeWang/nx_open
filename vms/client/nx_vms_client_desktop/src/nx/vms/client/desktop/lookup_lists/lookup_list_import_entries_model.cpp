// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "lookup_list_import_entries_model.h"

#include <QSet>

#include <QtCore/QString>

#include <nx/vms/client/desktop/application_context.h>
#include <nx/vms/client/desktop/settings/local_settings.h>
#include <ui/dialogs/common/custom_file_dialog.h>
#include <utils/common/hash.h>

#include "lookup_list_model.h"

namespace {

const QString kDoNotImportText = "Do not import";

} // namespace

namespace nx::vms::client::desktop {

struct LookupListImportEntriesModel::Private
{
    using Header = QList<QString>;
    LookupListImportEntriesModel* const q;
    QList<Header> columnHeaders;
    QSet<QString> attributeNames;
    QList<QString> defaultColumnHeader;
    QMap<int, QString> columnIndexToAttribute;
    PreviewRawData previewData;
    LookupListEntriesModel* listEntriesModel = nullptr;

    FixupData fixupData;
    QList<QVariantMap> completelyIncorrectRows;
    QVector<int> importedRows;

    void initDefaultColumnHeaders(const std::vector<QString>& listModelAttributesNames);
    void adjustHeadersToMatchPreview();
    void reset();
    void resetImportData();
    bool fullEntryRequiresFixup(const QVariantMap& entry);
    bool allValuesEmpty(const QVariantMap& entry);
    bool checkColumnPosIsCorrect(int index);
    QVariant updateHeader(int index, Header defaultHeader) const;
};

QVariant LookupListImportEntriesModel::Private::updateHeader(int index, Header defaultHeader) const
{
    if (defaultHeader.size() < 2)
        return defaultHeader; //< Only "Do not import" choice exist in header.

    QString expectedChoice = columnIndexToAttribute[index];
    if (expectedChoice.isEmpty())
        expectedChoice = kDoNotImportText;

    // Set expectedChoice as first choice for column.
    defaultHeader.move(defaultHeader.indexOf(expectedChoice), 0);
    return defaultHeader;
}

void LookupListImportEntriesModel::Private::resetImportData()
{
    fixupData.clear();
    completelyIncorrectRows.clear();
    importedRows.clear();
    emit q->fixupDataChanged();
}

void LookupListImportEntriesModel::Private::reset()
{
    previewData.clear();
    columnHeaders.clear();
    defaultColumnHeader.clear();
    columnIndexToAttribute.clear();
    attributeNames.clear();
    resetImportData();
}

bool LookupListImportEntriesModel::Private::checkColumnPosIsCorrect(int columnPos)
{
    return NX_ASSERT(columnPos >= 0);
}

bool LookupListImportEntriesModel::Private::fullEntryRequiresFixup(const QVariantMap& entry)
{
    for(const auto& attributeName: entry.keys())
    {
        if (listEntriesModel->isValidValue(entry.value(attributeName).toString(),attributeName))
            return false;
    }
    return true;
}

bool LookupListImportEntriesModel::Private::allValuesEmpty(const QVariantMap& entry)
{
    for (const auto& attributeName: entry.keys())
    {
        if (!entry[attributeName].toString().isEmpty())
            return false;
    }
    return true;
}

void LookupListImportEntriesModel::Private::adjustHeadersToMatchPreview()
{
    if (previewData.empty())
        return;

    const int previewColumnCount = previewData.front().size();
    const int currentColumnHeaderCount = columnHeaders.size();
    if (!previewColumnCount || previewColumnCount == currentColumnHeaderCount)
        return;

    // Remove entries that correspond to non-existing columns.
    for (int i = previewColumnCount; i < currentColumnHeaderCount; ++i)
        columnIndexToAttribute.remove(i);

    // Adjust to match the preview column count, filling missing columns with default header.
    columnHeaders.resize(previewColumnCount, defaultColumnHeader);
}

void LookupListImportEntriesModel::Private::initDefaultColumnHeaders(
    const std::vector<QString>& listModelAttributesNames)
{
    reset();

    // Set the column header as string "Do not import" + list of all attribute names.
    defaultColumnHeader.push_back(kDoNotImportText);
    for (const auto& value: listModelAttributesNames)
        defaultColumnHeader.push_back(value);
    attributeNames =
        QSet<QString>(listModelAttributesNames.begin(), listModelAttributesNames.end());

    // "Do not import" is by index 0, so skip it.
    for (int attributeNameIndex = 1; attributeNameIndex < defaultColumnHeader.size();
         attributeNameIndex++)
    {
        Header curColumnHeader = defaultColumnHeader;
        // Set current attribute name as first choice for current column.
        curColumnHeader.move(curColumnHeader.indexOf(defaultColumnHeader[attributeNameIndex]), 0);
        columnHeaders.push_back(curColumnHeader);
    }

    // Set default association of column index to attribute name.
    for (int attributeNamesIndex = 0; attributeNamesIndex < (int) listModelAttributesNames.size();
        ++attributeNamesIndex)
    {
        columnIndexToAttribute[attributeNamesIndex] =
            listModelAttributesNames[attributeNamesIndex];
    }
}

LookupListImportEntriesModel::LookupListImportEntriesModel(QAbstractTableModel* parent):
    base_type(parent),
    d(new Private{.q = this})
{
}

LookupListImportEntriesModel::~LookupListImportEntriesModel()
{
}

QVariant LookupListImportEntriesModel::headerData(
    int section, Qt::Orientation orientation, int role) const
{
    if (!NX_ASSERT(orientation == Qt::Orientation::Horizontal))
        return {};

    if (role != Qt::DisplayRole || section >= d->columnHeaders.size())
        return {};

    return d->updateHeader(section, d->columnHeaders[section]);
}

QMap<int, QString> LookupListImportEntriesModel::columnIndexToAttribute()
{
    return d->columnIndexToAttribute;
}

void LookupListImportEntriesModel::headerIndexChanged(int index, const QString& headerName)
{
    if (!d->attributeNames.contains(headerName))
    {
        // "Do not import" or unknown attribute was chosen.
        d->columnIndexToAttribute.remove(index);
    }
    else
    {
        d->columnIndexToAttribute[index] = headerName;
    }
}

void LookupListImportEntriesModel::reset()
{
    beginResetModel();
    d->reset();
    endResetModel();
    emit rowCountChanged();
}

int LookupListImportEntriesModel::rowCount(const QModelIndex& /*parent*/) const
{
    return d->previewData.size();
}

int LookupListImportEntriesModel::columnCount(const QModelIndex& /*parent*/) const
{
    return d->previewData.size() ? d->previewData.front().size() : 0;
}

QString LookupListImportEntriesModel::doNotImportText() const
{
    return kDoNotImportText;
}

bool LookupListImportEntriesModel::hasIndex(
    int row, int column, const QModelIndex& /*parent*/) const
{
    return row >= 0 && column >= 0 && row < d->previewData.size()
        && column < (int) d->previewData[row].size();
}

QVariant LookupListImportEntriesModel::data(const QModelIndex& index, int role) const
{
    if (role != Qt::DisplayRole || !hasIndex(index.row(), index.column()))
        return {};

    return d->previewData[index.row()][index.column()];
}

void LookupListImportEntriesModel::setRawData(const PreviewRawData& rawData, bool resetHeader)
{
    if (!NX_ASSERT(d->listEntriesModel))
        return;

    beginResetModel();

    if (resetHeader)
        d->initDefaultColumnHeaders(d->listEntriesModel->listModel()->rawData().attributeNames);

    d->previewData = rawData;

    d->adjustHeadersToMatchPreview();

    endResetModel();
    emit rowCountChanged();
}

LookupListEntriesModel* LookupListImportEntriesModel::lookupListEntriesModel()
{
    return d->listEntriesModel;
}

void LookupListImportEntriesModel::setLookupListEntriesModel(
    LookupListEntriesModel* lookupListEntriesModel)
{
    if (lookupListEntriesModel == d->listEntriesModel)
        return;

    if (d->listEntriesModel)
        d->listEntriesModel->disconnect(this);

    d->listEntriesModel = lookupListEntriesModel;

    if (d->listEntriesModel)
    {
        connect(d->listEntriesModel,
            &LookupListEntriesModel::listModelChanged,
            this,
            [this](LookupListModel* listModel)
            {
                if (listModel)
                {
                    d->initDefaultColumnHeaders(listModel->rawData().attributeNames);
                    emit headerDataChanged(Qt::Horizontal, 0, d->columnHeaders.size());
                }
            });
    }
    reset();

    emit lookupListEntriesModelChanged();
}

bool LookupListImportEntriesModel::fixupRequired()
{
    return !d->fixupData.empty();
}

void LookupListImportEntriesModel::applyFix()
{
    if (d->fixupData.empty())
        return; //< No fixup is required or all fix entries are set to "Any" value.

    // Fix existing rows.
    for (const auto& attributeName: d->fixupData.keys())
    {
        const auto fixupEntry = d->fixupData[attributeName].value<FixupEntry>();
        for (const auto& [pos, incorectWord]: fixupEntry.positionToIncorrectWord)
        {
            const auto posToChange = d->listEntriesModel->index(pos.first, pos.second);
            if (!NX_ASSERT(posToChange.isValid()))
                return;

            const auto rawValueByPos =
                d->listEntriesModel
                ->data(posToChange, LookupListEntriesModel::DataRole::RawValueRole)
                .toString();
            if (!NX_ASSERT(rawValueByPos.isEmpty(), "Data on change position must be empty"))
                return;

            d->listEntriesModel->setData(
                posToChange, fixupEntry.incorrectToCorrectWord[incorectWord]);
        }
    }

    // Add rows for complete incorrect rows.
    for (auto fixedRow: d->completelyIncorrectRows)
    {
        for (const auto& attributeName: fixedRow.keys())
        {
            const int columnPos = d->listEntriesModel->columnPosOfAttribute(attributeName);
            if (!d->checkColumnPosIsCorrect(columnPos))
                return;

            const auto incorrectValue = fixedRow.value(attributeName).toString();
            const auto fixupEntry = d->fixupData[attributeName].value<FixupEntry>();
            fixedRow[attributeName] = fixupEntry.incorrectToCorrectWord[incorrectValue];
        }
        // User didn't provide fixes for this row, so as a result all values are empty.
        // Skip it.
        if (!d->allValuesEmpty(fixedRow))
        {
            d->importedRows.push_back(d->listEntriesModel->rowCount());
            d->listEntriesModel->addEntry(fixedRow);
        }
    }
}

void LookupListImportEntriesModel::addLookupListEntry(QVariantMap& entry)
{
    if (entry.isEmpty() || d->allValuesEmpty(entry))
        return;

    // If all columns are invalid need to add this row separately after fixup,
    // since empty rows are prohibited in LookupListEntriesModel.
    const bool rowIsCompletlyIncorrect = d->fullEntryRequiresFixup(entry);

    // Entry will be added as new row to the LookupListEntryModel.
    const int rowInModel = lookupListEntriesModel()->rowCount({});

    for (const auto& attributeName: entry.keys())
    {
        const auto value = entry.value(attributeName).toString();
        if (d->listEntriesModel->isValidValue(value, attributeName))
            continue;

        auto fixupEntry = d->fixupData[attributeName].value<FixupEntry>();
        if (!rowIsCompletlyIncorrect)
        {
            // Incorrect column will be set to empty value, to fix it later.
            entry[attributeName] = {};

            const int columnPos = d->listEntriesModel->columnPosOfAttribute(attributeName);
            if (!d->checkColumnPosIsCorrect(columnPos))
                continue;

            fixupEntry.positionToIncorrectWord[{rowInModel, columnPos}] = value;
        }

        // By default setting empty fix.
        fixupEntry.incorrectToCorrectWord[value] = QString();

        d->fixupData[attributeName] = QVariant::fromValue(fixupEntry);
        emit fixupDataChanged();
    }

    if (rowIsCompletlyIncorrect)
    {
        d->completelyIncorrectRows.push_back(entry);
    }
    else
    {
        d->importedRows.push_back(rowInModel);
        lookupListEntriesModel()->addEntry(entry);
    }
}

void LookupListImportEntriesModel::addFixForWord(
    const QString& attributeName, const QString& incorrectWord, const QString& fix)
{
    auto fixupEntry = d->fixupData[attributeName].value<FixupEntry>();
    fixupEntry.incorrectToCorrectWord[incorrectWord] = fix;
    d->fixupData[attributeName] = QVariant::fromValue(fixupEntry);
}

LookupListImportEntriesModel::FixupData LookupListImportEntriesModel::fixupData()
{
    return d->fixupData;
}

void LookupListImportEntriesModel::revertImport()
{
    if(!d->listEntriesModel)
        return;

    if (d->importedRows.empty())
        return;

    d->listEntriesModel->deleteEntries(d->importedRows);
    d->resetImportData();
}

bool LookupListImportEntriesModel::hasImportedRows()
{
    return !d->importedRows.empty();
}

} // namespace nx::vms::client::desktop
