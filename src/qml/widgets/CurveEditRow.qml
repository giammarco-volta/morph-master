import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster
import NaadaLab.Ui as SharedUi

Rectangle {
    id: root

    property string title: "Curve"

    property int x1: 36
    property int y1: 0
    property int x2: 84
    property int y2: 100

    property int minX: 0
    property int maxX: 127

    property bool noteMode: false
    property bool showTitle: true
    property bool fillAvailableHeight: false

    // Morph Outputs represented by the solid and dashed curves.
    property int primaryMorphOutputId: -1
    property int complementaryMorphOutputId: -1

    signal curveEdited()
    signal editingStarted()
    signal editingFinished()

    color: Theme.panel
    border.color: Theme.border
    border.width: 1
    radius: 3

    implicitHeight: mainLayout.implicitHeight + 10

    property bool updatingFromModel: false

    function clamp(value, lo, hi) {
        return Math.max(lo, Math.min(hi, Math.round(value)))
    }

    function syncControlsFromModel() {
        root.updatingFromModel = true

        x1Selector.value = root.x1
        y1Selector.value = root.y1
        x2Selector.value = root.x2
        y2Selector.value = root.y2

        graph.x1 = root.x1
        graph.y1 = root.y1
        graph.x2 = root.x2
        graph.y2 = root.y2
        graph.minX = root.minX
        graph.maxX = root.maxX
        graph.noteMode = root.noteMode

        root.updatingFromModel = false
    }

    function applyEditedValues(newX1, newY1, newX2, newY2) {
        const cx1 = root.clamp(newX1, root.minX, root.maxX - 1)
        const cx2 = root.clamp(newX2, cx1 + 1, root.maxX)

        root.x1 = cx1
        root.y1 = root.clamp(newY1, 0, 100)
        root.x2 = cx2
        root.y2 = root.clamp(newY2, 0, 100)

        root.syncControlsFromModel()
        root.curveEdited()
    }

    ColumnLayout {
        id: mainLayout

        anchors.fill: parent
        anchors.margins: 5
        spacing: 5

        Label {
            text: root.title

            visible: root.showTitle

            color: Theme.text
            font.pixelSize: Theme.controlFontSize
            font.bold: true

            Layout.fillWidth: true
            Layout.preferredHeight: visible ? 20 : 0
            Layout.minimumHeight: visible ? 20 : 0
            Layout.maximumHeight: visible ? 20 : 0

            verticalAlignment: Text.AlignVCenter
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: root.fillAvailableHeight

            spacing: 8

            GridLayout {
                columns: 4
                columnSpacing: 5
                rowSpacing: 5

                Layout.minimumWidth: 190
                Layout.preferredWidth: 190
                Layout.maximumWidth: 190
                Layout.alignment: Qt.AlignVCenter

                Text {
                    text: "x1"
                    color: Theme.text
                    font.pixelSize: Theme.controlFontSize
                    Layout.preferredWidth: 18
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                }

                SharedUi.DragValueField {
                    id: x1Selector

                    title: ""
                    showTitle: false
                    dragPixelsForFullRange: 180

                    from: root.minX
                    to: Math.max(root.minX, root.x2 - 1)
                    value: root.x1

                    Layout.fillWidth: false
                    Layout.minimumWidth: 65
                    Layout.preferredWidth: 65
                    Layout.maximumWidth: 65

                    onValueChanged: {
                        if (root.updatingFromModel)
                            return

                        root.applyEditedValues(value,
                                               root.y1,
                                               root.x2,
                                               root.y2)
                    }

                    onEditingStarted: root.editingStarted()
                    onEditingFinished: root.editingFinished()
                }

                Text {
                    text: "y1"
                    color: Theme.text
                    font.pixelSize: Theme.controlFontSize
                    Layout.preferredWidth: 18
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                }

                SharedUi.DragValueField {
                    id: y1Selector

                    title: ""
                    showTitle: false
                    dragPixelsForFullRange: 180

                    from: 0
                    to: 100
                    value: root.y1

                    displayMultiplier: 0.01
                    displayDecimals: 2

                    Layout.fillWidth: false
                    Layout.minimumWidth: 65
                    Layout.preferredWidth: 65
                    Layout.maximumWidth: 65

                    onValueChanged: {
                        if (root.updatingFromModel)
                            return

                        root.applyEditedValues(root.x1,
                                               value,
                                               root.x2,
                                               root.y2)
                    }

                    onEditingStarted: root.editingStarted()
                    onEditingFinished: root.editingFinished()
                }

                Text {
                    text: "x2"
                    color: Theme.text
                    font.pixelSize: Theme.controlFontSize
                    Layout.preferredWidth: 18
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                }

                SharedUi.DragValueField {
                    id: x2Selector

                    title: ""
                    showTitle: false
                    dragPixelsForFullRange: 180

                    from: Math.min(root.maxX, root.x1 + 1)
                    to: root.maxX
                    value: root.x2

                    Layout.fillWidth: false
                    Layout.minimumWidth: 65
                    Layout.preferredWidth: 65
                    Layout.maximumWidth: 65

                    onValueChanged: {
                        if (root.updatingFromModel)
                            return

                        root.applyEditedValues(root.x1,
                                               root.y1,
                                               value,
                                               root.y2)
                    }

                    onEditingStarted: root.editingStarted()
                    onEditingFinished: root.editingFinished()
                }

                Text {
                    text: "y2"
                    color: Theme.text
                    font.pixelSize: Theme.controlFontSize
                    Layout.preferredWidth: 18
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                }

                SharedUi.DragValueField {
                    id: y2Selector

                    title: ""
                    showTitle: false
                    dragPixelsForFullRange: 180

                    from: 0
                    to: 100
                    value: root.y2

                    displayMultiplier: 0.01
                    displayDecimals: 2

                    Layout.fillWidth: false
                    Layout.minimumWidth: 65
                    Layout.preferredWidth: 65
                    Layout.maximumWidth: 65

                    onValueChanged: {
                        if (root.updatingFromModel)
                            return

                        root.applyEditedValues(root.x1,
                                               root.y1,
                                               root.x2,
                                               value)
                    }

                    onEditingStarted: root.editingStarted()
                    onEditingFinished: root.editingFinished()
                }
            }

            CurveGraph {
                id: graph

                Layout.fillWidth: true
                Layout.fillHeight: root.fillAvailableHeight
                Layout.minimumWidth: 240
                Layout.preferredHeight: 135

                primaryMorphOutputId: root.primaryMorphOutputId
                complementaryMorphOutputId: root.complementaryMorphOutputId

                onCurveEdited: {
                    if (root.updatingFromModel)
                        return

                    root.applyEditedValues(graph.x1,
                                           graph.y1,
                                           graph.x2,
                                           graph.y2)
                }

                onEditingStarted: root.editingStarted()
                onEditingFinished: root.editingFinished()
            }
        }
    }

    onX1Changed: syncControlsFromModel()
    onY1Changed: syncControlsFromModel()
    onX2Changed: syncControlsFromModel()
    onY2Changed: syncControlsFromModel()
    onMinXChanged: syncControlsFromModel()
    onMaxXChanged: syncControlsFromModel()
    onNoteModeChanged: syncControlsFromModel()

    Component.onCompleted: {
        syncControlsFromModel()
    }
}