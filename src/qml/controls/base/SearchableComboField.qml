import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster

LabeledControl {
    id: root

    property string currentText: ""
    property string placeholderText: "Select..."

    signal clicked()

    ActionButton {
        Layout.fillWidth: true
        Layout.preferredHeight: Theme.controlHeight
        implicitHeight: Theme.controlHeight

        text: root.currentText.length > 0
              ? root.currentText
              : root.placeholderText
        opensPopup: true

        contentItem: Item {
            Text {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.right: indicator.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter

                text: parent.parent.text
                color: root.enabled ? Theme.text : Theme.disabledText
                font: parent.parent.font
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                id: indicator

                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter

                text: "▼"
                color: root.enabled ? Theme.text : Theme.disabledText
                font.pixelSize: Math.max(1, parent.parent.font.pixelSize - 6)
                font.bold: false
            }
        }

        onClicked: root.clicked()
    }
}
