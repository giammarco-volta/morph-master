import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster

LabeledControl {
    id: root

    property int currentRange: 2

    Component {
        id: rangePanel

        Rectangle {
            id: panel

            property int currentRange: 2
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
                currentRange = p.currentRange
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

                columns: 5
                rowSpacing: 8
                columnSpacing: 8

                Repeater {
                    model: 25

                    ActionButton {
                        text: String(index)

                        Layout.preferredWidth: 56
                        Layout.preferredHeight: 48

                        selected: panel.currentRange === index

                        onClicked: {
                            panel.currentRange = index
                            if (panel.commit)
                                panel.commit(panel.currentRange)
                            closeModalPanel()
                        }
                    }
                }
            }
        }
    }

    ActionButton {
        Layout.fillWidth: true
        Layout.preferredHeight: Theme.controlHeight
        implicitHeight: Theme.controlHeight

        text: root.currentRange === 1
              ? "1 semitone"
              : root.currentRange + " semitones"
        opensPopup: true

        onClicked: openModalPanel(rangePanel, {
            currentRange: root.currentRange,
            commit: function(value) { root.currentRange = value }
        })
    }
}
