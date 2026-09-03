import QtQuick
import QtQuick.Controls

import MorphMaster

Rectangle {
    id: root

    required property int trackNumber
    property bool compact: false
    property real compactWidth: 26
    property real compactHeight: 24

    signal editRequested(int trackNumber)

    width: compact ? compactWidth : 58
    height: compact ? compactHeight : 34
    radius: compact ? 3 : 5

    color: dragArea.drag.active ? Qt.lighter(Theme.panel, 1.35) : Qt.lighter(Theme.panel, 1.18)
    border.color: dragArea.pressed ? Theme.accent : Theme.border
    border.width: dragArea.pressed ? 2 : 1

    z: dragArea.drag.active ? 1000 : 0

    Drag.active: dragArea.drag.active
    Drag.source: root
    Drag.hotSpot.x: width / 2
    Drag.hotSpot.y: height / 2
    Drag.keys: ["morphmaster-track"]
    Drag.mimeData: ({ "application/x-morphmaster-track": String(root.trackNumber) })

    Text {
        anchors.centerIn: parent
        text: root.compact ? String(root.trackNumber) : "Track " + root.trackNumber
        color: Theme.text
        font.pixelSize: root.compact ? 9 : 12
        font.bold: true
    }

    MouseArea {
        id: dragArea

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        drag.target: root
        drag.threshold: 8

        property real initialX: 0
        property real initialY: 0

        onPressed: function(mouse) {
            initialX = root.x
            initialY = root.y
            longPressTimer.restart()
        }

        onPositionChanged: {
            if (Math.abs(root.x - initialX) >= drag.threshold
                    || Math.abs(root.y - initialY) >= drag.threshold)
                longPressTimer.stop()
        }

        onReleased: {
            longPressTimer.stop()
            root.Drag.drop()
            root.x = initialX
            root.y = initialY
        }

        onCanceled: {
            longPressTimer.stop()
            root.x = initialX
            root.y = initialY
        }

        Timer {
            id: longPressTimer

            interval: 850
            repeat: false

            onTriggered: {
                if (dragArea.pressed && !dragArea.drag.active)
                    root.editRequested(root.trackNumber)
            }
        }
    }
}
