import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster
import NaadaLab.Ui as SharedUi

SharedUi.LabeledControl {
    id: root

    property int currentChannel: 1
    signal openPanelRequested(var panelComponent, var properties)
    signal closePanelRequested()

    Component {
        id: channelPanel

        Rectangle {
            id: panel

            property int currentChannel: 1
            property var commit

            implicitWidth: content.implicitWidth + 32
            implicitHeight: content.implicitHeight + 32
            width: implicitWidth
            height: implicitHeight

            color: Theme.panel
            radius: 8
            border.color: Theme.border
            border.width: 1

            function init(p) {
                currentChannel = p.currentChannel
                commit = p.commit
            }

            MouseArea {
                anchors.fill: parent
                onClicked: function(mouse) { mouse.accepted = true }
            }

            GridLayout {
                id: content

                anchors.fill: parent
                anchors.margins: 16

                columns: 4
                rowSpacing: 8
                columnSpacing: 8

                Repeater {
                    model: 16

                    SharedUi.ActionButton {
                        text: String(index + 1)

                        Layout.preferredWidth: 64
                        Layout.preferredHeight: 48

                        selected: panel.currentChannel === index + 1

                        onClicked: {
                            panel.currentChannel = index + 1
                            if (panel.commit)
                                panel.commit(panel.currentChannel)
                            root.closePanelRequested()
                        }
                    }
                }
            }
        }
    }

    SharedUi.ActionButton {
        Layout.fillWidth: true
        Layout.preferredHeight: Theme.controlHeight
        implicitHeight: Theme.controlHeight

        text: "Channel " + root.currentChannel
        opensPopup: true

        onClicked: root.openPanelRequested(channelPanel, {
            currentChannel: root.currentChannel,
            commit: function(ch) { root.currentChannel = ch }
        })
    }
}