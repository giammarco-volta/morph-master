import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster

Item {
    id: root

    property int morphOutputId: 8
    property string title: "Unassigned Output Tracks"
    property string specificName: ""
    property bool editingSpecificName: false
    property var trackNumbers: []
    property bool outputPanel: morphOutputId >= 0 && morphOutputId < 8
    property bool compactTrackTiles: false
    property bool fitEightCompactTilesPerRow: false
    property color panelTint: "transparent"
    property bool muted: false
    property bool solo: false
    property bool longPressHighlight: false
    property bool longPressTriggered: false

    readonly property real trackTileHeight:
        root.compactTrackTiles ? 28 : 34

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

    signal trackEditRequested(int trackNumber)
    signal curveEditRequested(int morphOutputId)
    signal trackDropped(int trackNumber, int morphOutputId)
    signal muteToggled(bool muted)
    signal soloToggled(bool solo)
    signal stateCycleRequested()
    signal specificNameEdited(string name)

    onSpecificNameChanged: {
        if (!root.editingSpecificName)
            outputNameEditor.text = root.specificName
    }

    Rectangle {
        id: panel
        anchors.fill: parent

        radius: 8
        color: root.panelTint.a > 0
               ? root.panelTint
               : Qt.lighter(Theme.background, 1.08)

        border.color: dropArea.containsDrag || root.longPressHighlight
                      ? Theme.accent
                      : Theme.border
        border.width: dropArea.containsDrag || root.longPressHighlight ? 2 : 1

        clip: true
    }

    ColumnLayout {
        parent: panel
        anchors.fill: parent
        anchors.margins: root.compactTrackTiles ? 4 : 8
        anchors.bottomMargin: root.outputPanel
                              ? root.trackTileHeight
                                + (root.compactTrackTiles ? 7 : 12)
                              : (root.compactTrackTiles ? 4 : 8)
        spacing: root.compactTrackTiles ? 3 : 7

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: root.compactTrackTiles ? 20 : 26

            Text {
                id: genericOutputName

                anchors.left: parent.left
                anchors.right: root.outputPanel && root.trackNumbers.length > 0
                               ? outputNameEditor.left : parent.right
                anchors.rightMargin: root.outputPanel && root.trackNumbers.length > 0 ? 8 : 0
                anchors.verticalCenter: parent.verticalCenter
                text: root.title + root.stateSuffix
                color: root.outputPanel && root.trackNumbers.length > 0
                       ? root.stateColor
                       : Theme.text
                font.pixelSize: root.outputPanel && root.trackNumbers.length > 0
                                ? (root.compactTrackTiles ? 10 : 14)
                                : (root.compactTrackTiles ? 10 : 13)
                font.bold: true
                horizontalAlignment: root.outputPanel && root.trackNumbers.length > 0
                                     ? Text.AlignLeft : Text.AlignHCenter
                elide: Text.ElideRight

                MouseArea {
                    anchors.fill: parent
                    enabled: root.outputPanel && root.trackNumbers.length > 0
                             && !root.editingSpecificName
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
                visible: root.outputPanel && root.trackNumbers.length > 0
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(parent.width * 0.56, Math.max(70, implicitWidth))
                height: root.compactTrackTiles ? 20 : 25
                text: root.specificName
                readOnly: !root.editingSpecificName
                selectByMouse: true
                maximumLength: 40
                horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter
            topPadding: 0
            bottomPadding: 0
                font.pixelSize: root.compactTrackTiles ? 9 : 12
                background: Rectangle {
                    color: root.editingSpecificName ? Qt.lighter(Theme.background, 1.18) : "transparent"
                    border.color: root.editingSpecificName ? Theme.accent : "transparent"
                    radius: 4
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

        Flow {
            id: tileFlow

            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: root.compactTrackTiles ? 3 : 6

            Repeater {
                model: root.trackNumbers

                delegate: DraggableTrackTile {
                    required property var modelData
                    trackNumber: Number(modelData)
                    compact: root.compactTrackTiles
                    compactWidth: root.fitEightCompactTilesPerRow
                                  ? Math.max(26,
                                      (tileFlow.width - 7 * tileFlow.spacing) / 8)
                                  : 26
                    compactHeight: root.trackTileHeight

                    onEditRequested: function(trackNumber) {
                        if (root.outputPanel)
                            root.trackEditRequested(trackNumber)
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: !root.outputPanel && root.trackNumbers.length > 0

            text: qsTr("Drag tracks to the desired Morph Outputs")
            color: Theme.secondaryText
            opacity: 0.7
            font.pixelSize: root.compactTrackTiles ? 9 : 11
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Text {
            Layout.fillWidth: true
            visible: root.trackNumbers.length === 0
            text: root.outputPanel
                  ? qsTr("No output tracks")
                  : qsTr("All output tracks are assigned")
            color: Theme.secondaryText
            font.pixelSize: 11
            horizontalAlignment: Text.AlignHCenter
        }
    }

    Row {
        parent: panel
        visible: root.outputPanel
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 5
        anchors.bottomMargin: 5
        spacing: root.compactTrackTiles ? 4 : 6
        z: 3

        Rectangle {
            width: root.trackTileHeight
            height: root.trackTileHeight
            radius: root.compactTrackTiles ? 4 : 5
            color: root.muted ? root.muteColor : Qt.lighter(Theme.background, 1.18)
            border.color: root.muted ? root.muteColor : Theme.border
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: "M"
                color: Theme.text
                font.pixelSize: root.compactTrackTiles ? 12 : 14
                font.bold: true
            }

            TapHandler {
                onTapped: root.muteToggled(!root.muted)
            }

            ToolTip.visible: muteHover.hovered
            ToolTip.text: qsTr("Mute this Morph Output")

            HoverHandler {
                id: muteHover
            }
        }

        Rectangle {
            width: root.trackTileHeight
            height: root.trackTileHeight
            radius: root.compactTrackTiles ? 4 : 5
            color: root.solo ? root.soloColor : Qt.lighter(Theme.background, 1.18)
            border.color: root.solo ? root.soloColor : Theme.border
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: "S"
                color: root.solo ? Theme.background : Theme.text
                font.pixelSize: root.compactTrackTiles ? 12 : 14
                font.bold: true
            }

            TapHandler {
                onTapped: root.soloToggled(!root.solo)
            }

            ToolTip.visible: soloHover.hovered
            ToolTip.text: qsTr("Solo this Morph Output")

            HoverHandler {
                id: soloHover
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
        enabled: root.outputPanel && root.trackNumbers.length > 0 && !root.editingSpecificName
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

    DropArea {
        id: dropArea

        parent: panel
        anchors.fill: parent
        keys: ["morphmaster-track"]

        onDropped: function(drop) {
            const source = drop.source
            if (!source || source.trackNumber === undefined)
                return

            root.trackDropped(Number(source.trackNumber), root.morphOutputId)
            drop.acceptProposedAction()
        }
    }
}
