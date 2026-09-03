import QtQuick
import QtQuick.Controls

import MorphMaster

Rectangle {
    id: root

    property int morphOutputId: 0
    property string title: "Morph Output"
    property string specificName: ""
    property bool editingSpecificName: false
    property var trackNumbers: []
    property color panelTint: "transparent"
    property real inputLevel: 0.0
    property real level: 0.0
    property int levelHoldMs: 120
    property bool compact: false
    property bool muted: false
    property bool solo: false
    property var feedbackNotes: []
    property bool longPressHighlight: false
    property bool longPressTriggered: false

    signal curveEditRequested(int morphOutputId)
    signal stateCycleRequested()
    signal specificNameEdited(string name)

    onSpecificNameChanged: {
        if (!root.editingSpecificName)
            outputNameEditor.text = root.specificName
    }

    readonly property bool cornerOutput:
        morphOutputId === 1 || morphOutputId === 3
        || morphOutputId === 5 || morphOutputId === 7
    readonly property bool hasTracks: trackNumbers.length > 0
    readonly property real headerHeight: compact ? 15 : 21
    readonly property color muteColor: "#A66A3F"
    readonly property color soloColor: "#35CFE0"
    readonly property color stateColor:
        root.solo ? root.soloColor
                  : root.muted ? root.muteColor
                               : Theme.accent
    readonly property string stateSuffix:
        root.solo ? " · S"
                  : root.muted ? " · M"
                               : ""
    readonly property real prevailingGainThreshold: 0.80//0.7071067811865476
    readonly property bool prevailingGain:
        root.hasTracks && root.level > root.prevailingGainThreshold
    readonly property color loudSoftBorderColor:
        Qt.rgba(0.35, 1.00, 0.45, 1.0)
    readonly property color highBorderColor:
        Qt.rgba(1.00, 0.93, 0.35, 1.0)
    readonly property color lowBorderColor:
        Qt.rgba(0.35, 0.72, 1.00, 1.0)

    readonly property color morphOutputColor: {
        switch (root.morphOutputId) {
        case 0: // Loud
        case 4: // Soft
            return root.loudSoftBorderColor

        case 1: // High and Loud
        case 2: // High
        case 3: // High and Soft
            return root.highBorderColor

        case 5: // Low and Soft
        case 6: // Low
        case 7: // Low and Loud
            return root.lowBorderColor

        default:
            return Theme.accent
        }
    }

    function updateDisplayedLevel() {
        const incoming = root.hasTracks
                       ? Math.max(0.0, Math.min(1.0, root.inputLevel))
                       : 0.0

        if (incoming > root.level)
            root.level = incoming

        if (incoming > 0.0) {
            levelHoldTimer.restart()
        } else if (!levelHoldTimer.running) {
            root.level = 0.0
        }
    }

    Timer {
        id: levelHoldTimer
        interval: root.levelHoldMs
        repeat: false

        onTriggered: {
            root.level = root.hasTracks
                       ? Math.max(0.0, Math.min(1.0, root.inputLevel))
                       : 0.0
        }
    }

    onInputLevelChanged: root.updateDisplayedLevel()
    onHasTracksChanged: {
        if (!root.hasTracks) {
            levelHoldTimer.stop()
            root.level = 0.0
        } else {
            root.updateDisplayedLevel()
        }
    }


    radius: 8
    color: root.panelTint.a > 0
           ? root.panelTint
           : Qt.lighter(Theme.background, 1.08)
    border.color: root.longPressHighlight
                  ? Theme.accent
                  : root.prevailingGain
                    ? root.morphOutputColor
                    : Theme.border
    border.width: root.longPressHighlight || root.prevailingGain ? 2 : 1
    clip: true

    Item {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: root.compact ? 2 : 4
        anchors.leftMargin: root.compact ? 4 : 7
        anchors.rightMargin: root.compact ? 4 : 7
        height: root.headerHeight
        z: 2

        Text {
            id: genericOutputName

            anchors.left: parent.left
            anchors.right: root.hasTracks ? outputNameEditor.left : parent.right
            anchors.rightMargin: root.hasTracks ? (root.compact ? 3 : 8) : 0
            anchors.verticalCenter: parent.verticalCenter
            text: root.title + root.stateSuffix
            color: root.hasTracks ? root.stateColor : Theme.text
            font.pixelSize: root.hasTracks ? (root.compact ? 9 : 13) : (root.compact ? 8 : 11)
            font.bold: true
            horizontalAlignment: root.hasTracks ? Text.AlignLeft : Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight

            MouseArea {
                anchors.fill: parent
                enabled: root.hasTracks && !root.editingSpecificName
                preventStealing: true
                cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor

                property bool longPressTriggered: false

                onPressed: {
                    longPressTriggered = false
                    root.longPressHighlight = true
                }

                onPressAndHold: {
                    longPressTriggered = true
                }

                onReleased: {
                    root.longPressHighlight = false

                    if (longPressTriggered)
                        root.curveEditRequested(root.morphOutputId)
                }

                onCanceled: {
                    longPressTriggered = false
                    root.longPressHighlight = false
                }

                onClicked: {
                    if (longPressTriggered) {
                        longPressTriggered = false
                        return
                    }

                    root.stateCycleRequested()
                }
            }
        }

        TextField {
            id: outputNameEditor
            visible: root.hasTracks
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width * (
                root.cornerOutput
                    ? (root.compact ? 0.52 : 0.56)
                    : (root.compact ? 0.58 : 0.62)
            )
            height: root.headerHeight
            text: root.specificName
            readOnly: !root.editingSpecificName
            selectByMouse: true
            maximumLength: 40
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter
            topPadding: 0
            bottomPadding: 0
            font.pixelSize: root.compact ? 7 : 10
            background: Rectangle {
                color: root.editingSpecificName ? Qt.lighter(Theme.background, 1.18) : "transparent"
                border.color: root.editingSpecificName ? Theme.accent : "transparent"
                radius: 3
            }
            onPressed: function(event) {
                root.editingSpecificName = true
                forceActiveFocus()
                selectAll()
                event.accepted = true
            }
            onAccepted: {
                root.specificNameEdited(text)
                root.editingSpecificName = false
                focus = false
            }
            onActiveFocusChanged: {
                if (!activeFocus && root.editingSpecificName) {
                    root.specificNameEdited(text)
                    root.editingSpecificName = false
                }
            }
            Keys.onEscapePressed: {
                text = root.specificName
                root.editingSpecificName = false
                focus = false
            }
        }
    }

    TapHandler {
        /*
         * Empty Morph Outputs are deliberately non-interactive.  For an
         * active output, highlight immediately on press, matching the
         * behaviour of track tiles; navigate only if the gesture becomes
         * a long press.
         */
        enabled: root.hasTracks && !root.editingSpecificName
        acceptedButtons: Qt.LeftButton

        onLongPressed: {
            root.longPressTriggered = true
        }

        onPressedChanged: {
            if (pressed) {
                root.longPressTriggered = false
                root.longPressHighlight = true
                return
            }

            const shouldOpenEditor = root.longPressTriggered
            root.longPressTriggered = false
            root.longPressHighlight = false

            if (shouldOpenEditor)
                root.curveEditRequested(root.morphOutputId)
        }
    }

    Loader {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.topMargin: root.compact ? 0 : 1
        anchors.leftMargin: root.compact ? 2 : 4
        anchors.rightMargin: root.compact ? 2 : 4
        anchors.bottomMargin: root.compact ? 2 : 4

        sourceComponent: root.cornerOutput
                         ? cornerGraphComponent
                         : curveGraphComponent
    }

    Component {
        id: curveGraphComponent

        MorphMonitorCurveGraph {
            morphOutputId: root.morphOutputId
            level: root.level
            compact: root.compact
            feedbackEnabled: root.hasTracks
            feedbackNotes: root.feedbackNotes
        }
    }

    Component {
        id: cornerGraphComponent

        MorphMonitorCornerGraph {
            morphOutputId: root.morphOutputId
            level: root.level
            compact: root.compact
            feedbackEnabled: root.hasTracks
            feedbackNotes: root.feedbackNotes
        }
    }
}
