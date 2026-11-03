import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster
import NaadaLab.Ui as SharedUi

Rectangle {
    id: root

    property string title: "Curve"

    property real x1: 36
    property int y1: 0
    property real x2: 84
    property int y2: 100
    property int ccCurveIndex: 0
    objectName: "curveEditor" + ccCurveIndex
    readonly property bool compactControls: showTitle && ApplicationWindow.window && ApplicationWindow.window.height <= 650
    readonly property int editorFieldHeight: compactControls ? 26 : 30

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

        x1Selector.value = Math.round(root.x1 * 1000)
        y1Selector.value = root.y1
        x2Selector.value = Math.round(root.x2 * 1000)
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
        const cx1 = Math.max(root.minX, Math.min(newX1, root.maxX - 1))
        const cx2 = Math.max(cx1 + 1, Math.min(newX2, root.maxX))

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
                rowSpacing: root.compactControls ? 3 : 5

                Layout.minimumWidth: 224
                Layout.preferredWidth: 224
                Layout.maximumWidth: 224
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
                    stepSize: 1000
                    displayMultiplier: 0.001
                    displayDecimals: 2
                    fieldHeight: root.editorFieldHeight
                    dragPixelsForFullRange: 180

                    from: root.minX * 1000
                    to: Math.floor(Math.max(root.minX, root.x2 - 1) * 1000)
                    value: Math.round(root.x1 * 1000)

                    Layout.fillWidth: false
                    Layout.minimumWidth: 82
                    Layout.preferredWidth: 82
                    Layout.maximumWidth: 82

                    onValueChanged: {
                        if (root.updatingFromModel)
                            return

                        root.applyEditedValues(value / 1000,
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
                    fieldHeight: root.editorFieldHeight
                    dragPixelsForFullRange: 180

                    from: 0
                    to: 100
                    value: root.y1

                    displayMultiplier: 0.01
                    displayDecimals: 2

                    Layout.fillWidth: false
                    Layout.minimumWidth: 82
                    Layout.preferredWidth: 82
                    Layout.maximumWidth: 82

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
                    stepSize: 1000
                    displayMultiplier: 0.001
                    displayDecimals: 2
                    fieldHeight: root.editorFieldHeight
                    dragPixelsForFullRange: 180

                    from: Math.ceil(Math.min(root.maxX, root.x1 + 1) * 1000)
                    to: root.maxX * 1000
                    value: Math.round(root.x2 * 1000)

                    Layout.fillWidth: false
                    Layout.minimumWidth: 82
                    Layout.preferredWidth: 82
                    Layout.maximumWidth: 82

                    onValueChanged: {
                        if (root.updatingFromModel)
                            return

                        root.applyEditedValues(root.x1,
                                               root.y1,
                                               value / 1000,
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
                    fieldHeight: root.editorFieldHeight
                    dragPixelsForFullRange: 180

                    from: 0
                    to: 100
                    value: root.y2

                    displayMultiplier: 0.01
                    displayDecimals: 2

                    Layout.fillWidth: false
                    Layout.minimumWidth: 82
                    Layout.preferredWidth: 82
                    Layout.maximumWidth: 82

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
                Repeater {
                    model: ["Position", "Slope"]
                    delegate: Label {
                        required property string modelData
                        text: modelData
                        color: Theme.secondaryText
                        font.pixelSize: 11
                        horizontalAlignment: Text.AlignHCenter
                        Layout.columnSpan: 2
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.compactControls ? 12 : 14
                    }
                }
                Repeater {
                    model: 2
                    delegate: ComboBox {
                        id: ccChoice
                        required property int index
                        readonly property int slot: root.ccCurveIndex * 2 + index
                        readonly property var choices: {
                            const revision = SettingsController.curveControls
                            return SettingsController.availableCurveControlCcs(slot)
                        }
                        objectName: "curveCcAssignment" + slot
                        Layout.columnSpan: 2
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.editorFieldHeight
                        font.pixelSize: 11
                        leftPadding: 6
                        rightPadding: 26
                        model: choices
                        textRole: "label"
                        valueRole: "cc"
                        currentIndex: {
                            const cc = SettingsController.curveControls[slot].cc
                            for (let i=0; i<choices.length; ++i)
                                if (choices[i].cc === cc) return i
                            return 0
                        }
                        onActivated: SettingsController.setCurveControl(slot,currentValue,0,127)
                        ToolTip.visible: hovered
                        ToolTip.text: index === 0 ? "Position MIDI CC" : "Slope MIDI CC"
                    }
                }
                Repeater {
                    model: 2
                    delegate: ComboBox {
                        required property int index
                        readonly property int slot: root.ccCurveIndex * 2 + index
                        objectName: "curveCcChannel" + slot
                        Layout.columnSpan: 2
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.editorFieldHeight
                        font.pixelSize: 11
                        leftPadding: 6
                        rightPadding: 26
                        model: ["Input ch.", "Ch 1", "Ch 2", "Ch 3", "Ch 4", "Ch 5", "Ch 6", "Ch 7", "Ch 8",
                                "Ch 9", "Ch 10", "Ch 11", "Ch 12", "Ch 13", "Ch 14", "Ch 15", "Ch 16"]
                        currentIndex: SettingsController.curveControls[slot].channel + 1
                        onActivated: SettingsController.setCurveControlChannel(slot,currentIndex - 1)
                        ToolTip.visible: hovered
                        ToolTip.text: "MIDI channel for this curve control. Input ch. follows the note input channel."
                    }
                }
                Repeater {
                    model: 2
                    delegate: SharedUi.DragValueField {
                        required property int index
                        readonly property int slot: root.ccCurveIndex * 2 + index
                        property bool syncing: false
                        objectName: "curveCcValue" + slot
                        Layout.columnSpan: 2
                        Layout.fillWidth: true
                        showTitle: false
                        fieldHeight: root.editorFieldHeight
                        from: 0
                        to: 127000
                        stepSize: 1000
                        displayMultiplier: 0.001
                        displayDecimals: 2
                        function syncValue() {
                            syncing = true
                            value = Math.round(SettingsController.curveControlValues[slot] * 1000)
                            syncing = false
                        }
                        Component.onCompleted: syncValue()
                        onValueChanged: {
                            if (!syncing) {
                                SettingsController.setCurveParameterValue(slot,value / 1000)
                                syncValue()
                            }
                        }
                        Connections {
                            target: SettingsController
                            function onCurveControlValuesChanged() { syncValue() }
                        }
                        onEditingStarted: root.editingStarted()
                        onEditingFinished: root.editingFinished()
                    }
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