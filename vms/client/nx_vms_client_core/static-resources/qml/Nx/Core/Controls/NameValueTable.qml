// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

import QtQuick
import QtQuick.Layouts

import Nx.Controls
import Nx.Core
import Nx.Core.Controls

Item
{
    id: control

    /**
     * An array of objects with the following properties:
     *      ${nameRole}, default "name" - item's name
     *      ${valuesRole}, default "values" - item's values or a single value
     *      ${colorsRole}, default "colors" [optional] - item's colors or a single color,
     *          if present must have the same length as values
     */
    property var items: []

    property string nameRole: "name"
    property string valuesRole: "values"
    property string colorsRole: "colors"

    // If positive, only maxRowCount rows will be visible.
    property int maxRowCount: 0
    property int maximumLineCount: 2
    property alias rowSpacing: grid.rowSpacing

    property color nameColor: "gray"
    property color valueColor: "white"
    property int valueAlignment: Text.AlignLeft

    property font font
    property font nameFont: control.font
    property font valueFont: control.font

    // Default: 30% to label, 70% to value. This isn't strict proportion and uses only if there
    // is not enough space for labels or values.
    property real labelFraction: 0.3

    property bool interactive: false
    property Menu contextMenu
    readonly property alias hoveredRow: highlight.rowIndex
    readonly property var hoveredItem: hoveredRow >= 0 ? items[hoveredRow] : undefined

    property real tableLineHeight: CoreSettings.iniConfigValue("attributeTableLineHeightFactor")
    property real attributeTableSpacing: CoreSettings.iniConfigValue("attributeTableSpacing")

    property alias leftPadding: grid.leftPadding
    property alias rightPadding: grid.rightPadding
    property alias topPadding: grid.topPadding
    property alias bottomPadding: grid.bottomPadding

    implicitWidth: 400
    implicitHeight: grid.implicitHeight

    function forceLayout()
    {
        grid.forceLayout()
    }

    function update()
    {
        grid.updateColumnWidths()
    }

    Rectangle
    {
        id: highlight

        property int rowIndex: -1

        color: ColorTheme.colors.dark12
        width: control.width
        visible: interactive && (rowIndex >= 0 || control.contextMenu?.opened)
    }

    Grid
    {
        id: grid

        objectName: "dataTable"

        readonly property var sourceData: control.items ?? []

        readonly property var clippedData: control.maxRowCount > 0
            ? sourceData.slice(0, control.maxRowCount)
            : sourceData

        columns: 2
        leftPadding: control.interactive ? 8 : 0
        rightPadding: leftPadding
        width: control.width
        columnSpacing: 8
        rowSpacing: 0
        verticalItemAlignment: Grid.AlignVCenter
        flow: Grid.TopToBottom

        onWidthChanged:
            updateColumnWidths()

        onClippedDataChanged:
        {
            if (control.contextMenu?.opened)
                control.contextMenu.close()

            labelsRepeater.model = clippedData
            valuesRepeater.model = clippedData
            updateColumnWidths()
        }

        Component.onCompleted:
            updateColumnWidths()

        Repeater
        {
            id: labelsRepeater

            delegate: Component
            {
                Text
                {
                    id: cellLabel

                    objectName: "cellLabel"

                    readonly property int rowIndex: index

                    color: control.nameColor
                    font: control.nameFont

                    text: modelData[control.nameRole] || " "
                    textFormat: Text.StyledText

                    maximumLineCount: control.maximumLineCount
                    wrapMode: Text.Wrap
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignLeft

                    lineHeight: control.tableLineHeight

                    topPadding: Math.round(control.attributeTableSpacing / 2)
                    bottomPadding: (control.attributeTableSpacing - topPadding)
                }
            }
        }

        Item
        {
            width: 1 //< Shouldn't push the name column's width
            height: footer.height
            visible: footer.remainder > 0

            Text
            {
                id: footer

                readonly property int remainder:
                    grid.sourceData.length - grid.clippedData.length

                objectName: "footer"
                text: qsTr("+ %n more", "", remainder)
                width: grid.width
                color: nameColor
                lineHeight: tableLineHeight
                font: control.nameFont
                wrapMode: Text.Wrap
            }
        }

        Repeater
        {
            id: valuesRepeater

            delegate: Component
            {
                RowLayout
                {
                    id: cellValues

                    objectName: "cellValues"
                    spacing: 4

                    property var values: modelData[control.valuesRole] ?? []
                    property var colors: modelData[control.colorsRole] ?? []
                    property int lastVisibleIndex: 0
                    readonly property bool shrinked:
                        lastVisibleIndex < values.length - 1 || valueRowRepeater.shrinked

                    Repeater
                    {
                        id: valueRowRepeater

                        objectName: "valueRowRepeater"
                        model: cellValues.values
                        readonly property bool shrinked: count === 1 && itemAt(0).shrinked

                        RowLayout
                        {
                            id: valueItem

                            objectName: "valueItem"
                            spacing: 4
                            visible: index <= cellValues.lastVisibleIndex
                            readonly property bool shrinked:
                                valueText.width < valueText.implicitWidth

                            Rectangle
                            {
                                id: colorRectangle

                                objectName: "colorRectangle"
                                readonly property bool relevant: cellValues.colors[index] ?? false
                                width: relevant ? 16 : 0
                                height: 16
                                visible: relevant
                                color: cellValues.colors[index] ?? "transparent"
                                border.color: ColorTheme.transparent(ColorTheme.colors.light1, 0.1)
                                radius: 1
                            }

                            Text
                            {
                                id: valueText

                                objectName: "valueText"
                                Layout.fillWidth: index === cellValues.lastVisibleIndex

                                color: control.valueColor
                                font: control.valueFont
                                text: modelData +
                                    (index < cellValues.lastVisibleIndex && !cellValues.colors[index]
                                        ? ","
                                        : " ")
                                lineHeight: control.tableLineHeight
                                wrapMode: Text.Wrap
                                elide: index === 0 ? Text.ElideRight : Text.ElideNone
                                maximumLineCount: control.maximumLineCount
                            }
                        }
                    }

                    Text
                    {
                        id: appendix

                        objectName: "appendix"
                        readonly property bool relevant:
                            cellValues.lastVisibleIndex < valueRowRepeater.count - 1
                        width: relevant ? undefined : 0
                        visible: relevant
                        text: `+ ${valueRowRepeater.count - cellValues.lastVisibleIndex - 1}`
                        color: ColorTheme.darker(control.valueColor, 10)
                        font: control.valueFont
                    }

                    onWidthChanged:
                    {
                        if (!valueRowRepeater.count)
                            return

                        var lastIndex = valueRowRepeater.count - 1
                        var sumWidth = 0
                        for (var i = 0; i < valueRowRepeater.count; ++i)
                        {
                            const item = valueRowRepeater.itemAt(i)
                            if (sumWidth > 0)
                                sumWidth += spacing
                            sumWidth += item.implicitWidth
                            if (sumWidth > width)
                            {
                                lastIndex = i - 1
                                break
                            }
                        }

                        // At least one elided value we should show.
                        lastVisibleIndex = Math.max(0, lastIndex)
                    }
                }
            }
        }

        function updateColumnWidths()
        {
            if (grid.width <= 0)
                return

            const labels = children.filter(child => child.objectName === "cellLabel")
            const values = children.filter(child => child.objectName === "cellValues")
            const calculateImplicitWidth = (items) =>
                items.reduce((prevMax, item) => Math.max(prevMax, item.implicitWidth), 0)

            const implicitLabelWidth = calculateImplicitWidth(labels)
            const implicitValuesWidth = calculateImplicitWidth(values)

            const availableWidth =
                grid.width - grid.columnSpacing - grid.leftPadding - grid.rightPadding
            const preferredLabelWidth = availableWidth * control.labelFraction
            const preferredValuesWidth = availableWidth - preferredLabelWidth

            let labelWidth = 0
            const availableLabelWidth = availableWidth - implicitValuesWidth

            // If value is shorter than its preferred space...
            if (implicitValuesWidth < preferredValuesWidth)
            {
                // Label may occupy not less than its preferred space
                // and no more than its available space.
                labelWidth = Math.max(preferredLabelWidth,
                    Math.min(implicitLabelWidth, availableLabelWidth))
            }
            else // If value is longer than its preferred space...
            {
                // Label may occupy not less than its available space
                // and no more than its preferred space.
                labelWidth = Math.max(availableLabelWidth,
                    Math.min(implicitLabelWidth, preferredLabelWidth))
            }

            const valuesWidth = availableWidth - labelWidth

            for (const label of labels)
                label.width = labelWidth

            for (const value of values)
                value.width = valuesWidth
        }
    }

    MouseArea
    {
        id: gridMouseArea

        enabled: control.interactive
        anchors.fill: grid
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onPositionChanged: (mouse) =>
        {
            if (mouse.buttons !== Qt.NoButton)
                return

            const row = grid.childAt(grid.leftPadding, mouse.y)
            if (!row)
                return

            highlight.y = row.y
            highlight.height = row.height
            highlight.rowIndex = row.rowIndex

            const cell = grid.childAt(mouse.x, mouse.y)
            if (cell && cell.objectName === "cellValues" && cell.shrinked)
            {
                toolTip.dataItem = cell
                toolTip.y = cell.y - toolTip.height
            }
            else
            {
                toolTip.hide()
            }
        }

        onExited:
        {
            highlight.rowIndex = -1
            toolTip.hide()
        }

        onClicked:
        {
            if (contextMenu)
                contextMenu.popup()
        }
    }

    ToolTip
    {
        id: toolTip

        property var dataItem: null
        readonly property var values: dataItem?.values ?? []
        readonly property var colors: dataItem?.colors ?? []

        parent: grid
        visible: !!dataItem
        contentWidth: toolTipContent.width

        Row
        {
            id: toolTipContent

            spacing: 4

            Repeater
            {
                id: toolTipRepeater
                model: toolTip.values

                Row
                {
                    spacing: 4

                    Rectangle
                    {
                        readonly property bool relevant: !!toolTip.colors[index]
                        width: 16
                        height: 16
                        anchors.verticalCenter: parent.verticalCenter
                        visible: relevant
                        color: toolTip.colors[index] ?? "transparent"
                        border.color: ColorTheme.transparent(ColorTheme.colors.light1, 0.1)
                        radius: 1
                    }

                    Text
                    {
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData
                            + (index < toolTipRepeater.model.length - 1 && !toolTip.colors[index]
                                ? ","
                                : "")
                        lineHeight: control.tableLineHeight
                    }
                }
            }
        }
    }
}
