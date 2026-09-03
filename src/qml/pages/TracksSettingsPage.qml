// src/qml/pages/TracksSettingsPage.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster

Item {
    id: root

    signal morphOutputEditRequested(int morphOutputIndex)

    /* Number of the first visible track, in the range 1..16. */
    property int firstTrack: 1

    /* One on phone, four on tablet/desktop. */
    property int visibleTrackCount: 1

    readonly property int normalizedFirstTrack:
        Math.max(1, Math.min(16, firstTrack))

    readonly property int normalizedVisibleTrackCount:
        Math.max(1, Math.min(16, visibleTrackCount))

    readonly property int actualTrackCount:
        Math.min(normalizedVisibleTrackCount,
                 17 - normalizedFirstTrack)

    readonly property bool singleTrackMode:
        actualTrackCount === 1

    component TrackPanel: Rectangle {
        id: panel

        required property int trackNumber

        color: Theme.panel
        border.color: Theme.border
        border.width: 1
        radius: 3

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 5
            spacing: 5

            Label {
                text: "Track " + panel.trackNumber

                color: Theme.text
                font.pixelSize: Theme.controlFontSize
                font.bold: true

                Layout.fillWidth: true
                Layout.preferredHeight: 20
                Layout.minimumHeight: 20
                Layout.maximumHeight: 20

                verticalAlignment: Text.AlignVCenter
            }

            SingleTrackEditor {
                trackNumber: panel.trackNumber

                Layout.fillWidth: true
                Layout.fillHeight: true

                onMorphOutputEditRequested: function(morphOutputIndex) {
                    root.morphOutputEditRequested(morphOutputIndex)
                }
            }
        }
    }

    Loader {
        anchors.fill: parent

        sourceComponent: root.singleTrackMode
                         ? singleTrackComponent
                         : trackGridComponent
    }

    Component {
        id: singleTrackComponent

        SingleTrackEditor {
            trackNumber: root.normalizedFirstTrack

            onMorphOutputEditRequested: function(morphOutputIndex) {
                root.morphOutputEditRequested(morphOutputIndex)
            }
        }
    }

    Component {
        id: trackGridComponent

        Item {
            GridLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.pageMargin
                anchors.rightMargin: Theme.pageMargin
                anchors.topMargin: Theme.pageSpacing
                anchors.bottomMargin: Theme.pageMargin

                columns: 2
                rows: 2
                columnSpacing: Theme.pageSpacing
                rowSpacing: Theme.pageSpacing

                Repeater {
                    model: root.actualTrackCount

                    delegate: TrackPanel {
                        required property int index

                        trackNumber: root.normalizedFirstTrack + index

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: 0
                        Layout.minimumHeight: 0
                    }
                }
            }
        }
    }
}
