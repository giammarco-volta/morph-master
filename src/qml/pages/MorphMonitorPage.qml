import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster

Item {
    id: root

    readonly property int layoutClass:
        ApplicationWindow.window
            ? ApplicationWindow.window.layoutClass
            : UiMetrics.Desktop
    readonly property bool phoneLayout: layoutClass === UiMetrics.Phone
    readonly property real pageMargin: phoneLayout ? 3 : 12
    readonly property real gridSpacing:
        phoneLayout ? 3 : UiMetrics.spacing(layoutClass)

    signal curveEditRequested(int morphOutputId)

    // One shared list for the whole Monitor page. The page itself is loaded
    // only while selected, so no MIDI feedback work is performed elsewhere.
    property var feedbackNotes: []
    property var outputLevels: [0.0, 0.0, 0.0, 0.0,
                                0.0, 0.0, 0.0, 0.0]

    readonly property bool monoPlayMode:
        SettingsController.playMode === 1

    function playModeText() {
        return root.monoPlayMode
                ? qsTr("Play Mode: Mono")
                : qsTr("Play Mode: Poly")
    }

    function togglePlayMode() {
        /*
         * Release any notes held on the virtual keyboard before changing
         * the note-handling mode.
         */
        testKeyboard.releaseAllPointers()

        SettingsController.playMode =
            root.monoPlayMode ? 0 : 1
    }

    function applyMonitorFeedback(notes, gains) {
        root.feedbackNotes = notes ? notes : []

        const nextLevels = []
        for (let i = 0; i < 8; ++i) {
            const value = gains && i < gains.length ? Number(gains[i]) : 0.0
            nextLevels.push(Math.max(0.0,
                                     Math.min(1.0,
                                              isNaN(value) ? 0.0 : value)))
        }
        root.outputLevels = nextLevels
    }

    function gridRow(outputId) {
        switch (outputId) {
        case 7: return 0 // Low and Loud
        case 0: return 0 // Loud
        case 1: return 0 // High and Loud
        case 6: return 1 // Low
        case 2: return 1 // High
        case 5: return 2 // Low and Soft
        case 4: return 2 // Soft
        case 3: return 2 // High and Soft
        default: return 0
        }
    }

    function gridColumn(outputId) {
        switch (outputId) {
        case 7: return 0
        case 0: return 1
        case 1: return 2
        case 6: return 0
        case 2: return 2
        case 5: return 0
        case 4: return 1
        case 3: return 2
        default: return 0
        }
    }

    function neutralPanelTint() {
        // Neutral mid-grey used by Morph Outputs with no assigned tracks.
        return Qt.rgba(0.50, 0.50, 0.50, 0.20)
    }

    function panelTint(outputId) {
        switch (outputId) {
        case 0: return Qt.rgba(0.45, 0.90, 0.48, 0.24)
        case 1: return Qt.rgba(1.00, 0.93, 0.35, 0.25)
        case 2: return Qt.rgba(0.96, 0.75, 0.05, 0.25)
        case 3: return Qt.rgba(0.55, 0.40, 0.00, 0.28)
        case 4: return Qt.rgba(0.02, 0.28, 0.12, 0.28)
        case 5: return Qt.rgba(0.02, 0.10, 0.34, 0.30)
        case 6: return Qt.rgba(0.08, 0.32, 0.92, 0.25)
        case 7: return Qt.rgba(0.35, 0.72, 1.00, 0.25)
        default: return "transparent"
        }
    }

    function preferredPanelWidth(outputId) {
        if (root.phoneLayout)
            return 1
        return root.gridColumn(outputId) === 1 ? 190 : 150
    }

    function preferredPanelHeight(outputId) {
        if (root.phoneLayout)
            return 1
        return root.gridRow(outputId) === 1 ? 150 : 125
    }

    function keyboardRangeSummary() {
        const minimum = SettingsController.surfaceMinNote
        const maximum = SettingsController.surfaceMaxNote
        if (minimum === 0 && maximum === 127)
            return qsTr("Full · 0–127")
        return qsTr("%1 keys · %2–%3")
                .arg(maximum - minimum + 1)
                .arg(minimum)
                .arg(maximum)
    }

    Connections {
        target: SettingsController

        function onMonitorFeedbackChanged(notes, gains) {
            root.applyMonitorFeedback(notes, gains)
        }
    }

    Component.onCompleted: {
        root.applyMonitorFeedback(
                    SettingsController.currentMonitorFeedbackNotes(),
                    SettingsController.currentMorphOutputGains())
    }

    GridLayout {
        id: monitorGrid

        anchors.fill: parent
        anchors.margins: root.pageMargin
        columns: 3
        rows: 3
        columnSpacing: root.gridSpacing
        rowSpacing: root.gridSpacing

        Repeater {
            model: SettingsController.morphOutputStateModel

            delegate: Item {
                required property int morphOutputId
                required property string name
                required property string specificName
                required property var assignedTrackIndexes
                required property bool muted
                required property bool solo

                Layout.row: root.gridRow(morphOutputId)
                Layout.column: root.gridColumn(morphOutputId)
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 0
                Layout.minimumHeight: 0
                Layout.preferredWidth: root.preferredPanelWidth(morphOutputId)
                Layout.preferredHeight: root.preferredPanelHeight(morphOutputId)

                MorphMonitorPanel {
                    anchors.fill: parent
                    morphOutputId: parent.morphOutputId
                    title: parent.name
                    specificName: parent.specificName
                    trackNumbers: parent.assignedTrackIndexes
                    panelTint: parent.assignedTrackIndexes.length > 0
                               ? root.panelTint(parent.morphOutputId)
                               : root.neutralPanelTint()
                    compact: root.phoneLayout
                    muted: parent.muted
                    solo: parent.solo
                    feedbackNotes: root.feedbackNotes
                    inputLevel: parent.assignedTrackIndexes.length > 0
                                ? root.outputLevels[parent.morphOutputId]
                                : 0.0

                    onSpecificNameEdited: function(value) {
                        SettingsController.setMorphOutputName(parent.morphOutputId, value)
                    }

                    onStateCycleRequested: {
                        if (parent.solo) {
                            SettingsController.setMorphOutputSolo(
                                        parent.morphOutputId, false)
                        } else if (parent.muted) {
                            SettingsController.setMorphOutputMuted(
                                        parent.morphOutputId, false)
                            SettingsController.setMorphOutputSolo(
                                        parent.morphOutputId, true)
                        } else {
                            SettingsController.setMorphOutputMuted(
                                        parent.morphOutputId, true)
                        }
                    }

                    onCurveEditRequested: function(outputId) {
                        root.curveEditRequested(outputId)
                    }
                }
            }
        }

        Rectangle {
            Layout.row: 1
            Layout.column: 1
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 0
            Layout.minimumHeight: 0
            Layout.preferredWidth: root.phoneLayout ? 1 : 250
            Layout.preferredHeight: root.phoneLayout ? 1 : 170

            radius: 8
            color: Qt.lighter(Theme.background, 1.08)
            border.color: Theme.border
            border.width: 1
            clip: true

            Item {
                id: keyboardHeader

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: root.phoneLayout ? 1 : 4
                anchors.leftMargin: root.phoneLayout ? 2 : 7
                anchors.rightMargin: root.phoneLayout ? 2 : 7
                height: root.phoneLayout ? 18 : 21
                z: 2

                Text {
                    id: keyboardTitle

                    anchors.left: parent.left
                    anchors.right: keyboardRange.left
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter

                    visible: !root.phoneLayout
                    text: root.playModeText()

                    color:
                        playModeMouse.containsMouse
                            ? Theme.accent
                            : Theme.text

                    font.pixelSize: 13
                    font.bold: true

                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight

                    MouseArea {
                        id: playModeMouse

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor

                        onClicked: {
                            root.togglePlayMode()
                        }
                    }
                }

                Text {
                    id: keyboardRange

                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.min(implicitWidth, parent.width * 0.68)

                    visible: !root.phoneLayout
                    text: root.keyboardRangeSummary()
                    color: Theme.secondaryText
                    font.pixelSize: 9
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                Item {
                    anchors.fill: parent
                    visible: root.phoneLayout

                    Text {
                        id: previousKeyboardPage

                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: Math.max(22, Math.min(30, parent.width * 0.16))

                        text: "<"
                        color: Theme.text
                        opacity: testKeyboard.phonePage > 0 ? 1.0 : 0.32
                        font.pixelSize: 12
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter

                        MouseArea {
                            anchors.fill: parent
                            enabled: testKeyboard.phonePage > 0
                            onClicked: testKeyboard.setPhonePage(testKeyboard.phonePage - 1)
                        }
                    }

                    Item {
                        id: phoneKeyboardInfo

                        anchors.left: previousKeyboardPage.right
                        anchors.right: nextKeyboardPage.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: 3
                        anchors.rightMargin: 3

                        Text {
                            id: phonePlayModeTitle

                            anchors.left: parent.left
                            anchors.right: parent.horizontalCenter
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.rightMargin: 3

                            text: root.playModeText()

                            color:
                                phonePlayModeMouse.pressed
                                    ? Theme.accent
                                    : Theme.text

                            font.pixelSize: 9
                            font.bold: true

                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight

                            MouseArea {
                                id: phonePlayModeMouse

                                anchors.fill: parent

                                onClicked: {
                                    root.togglePlayMode()
                                }
                            }
                        }

                        Text {
                            id: phoneKeyboardRange

                            anchors.left: parent.horizontalCenter
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.leftMargin: 3

                            text: qsTr("Range: %1–%2")
                                    .arg(testKeyboard.phoneFirstNote)
                                    .arg(testKeyboard.phoneLastNote)

                            color: Theme.secondaryText

                            font.pixelSize: 9
                            font.bold: true

                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                    }

                    Text {
                        id: nextKeyboardPage

                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: previousKeyboardPage.width

                        text: ">"
                        color: Theme.text
                        opacity: testKeyboard.phonePage < testKeyboard.phonePageCount - 1
                                 ? 1.0 : 0.32
                        font.pixelSize: 12
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter

                        MouseArea {
                            anchors.fill: parent
                            enabled: testKeyboard.phonePage
                                     < testKeyboard.phonePageCount - 1
                            onClicked: testKeyboard.setPhonePage(testKeyboard.phonePage + 1)
                        }
                    }
                }
            }

            MorphTestKeyboard {
                id: testKeyboard
                phoneLayout: root.phoneLayout
                adaptiveRange: true
                activeNotes: root.feedbackNotes
                onNotePressed: function(note, velocity) {
                    SettingsController.testNoteOn(note, velocity)
                }
                onNoteReleased: function(note) {
                    SettingsController.testNoteOff(note)
                }
                anchors.top: keyboardHeader.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.topMargin: root.phoneLayout ? 1 : 2
                anchors.leftMargin: root.phoneLayout ? 3 : 6
                anchors.rightMargin: root.phoneLayout ? 3 : 6
                anchors.bottomMargin: root.phoneLayout ? 3 : 6
            }
        }
    }
}
