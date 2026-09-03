// ActionButton.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster

Button {
    id: root

    property bool selected: false
    property bool opensPopup: false
    property real textVerticalOffset: 0

    implicitHeight: Theme.controlHeight

    /*
     * Button styles may define background insets. Those insets make the
     * visible Rectangle shorter than the control itself even when the
     * Button and a neighbouring ComboBox have the same logical height.
     *
     * ActionButton owns its complete visual appearance, so no style inset
     * or padding is needed.
     */
    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0

    padding: 0

    font.pixelSize: Theme.controlFontSize
    font.bold: root.selected

    background: Rectangle {
        x: 0
        y: 0
        width: root.width
        height: root.height

        radius: 6

        color: root.selected
               ? Theme.accent
               : root.down
                   ? Theme.accentPressed
                   : root.hovered
                       ? Theme.panel
                       : Theme.panelDark

        border.color:
            root.selected
                ? Theme.accent
                : Theme.border

        border.width:
            root.selected ? 2 : 1
    }

    contentItem: Item {
        Text {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: root.textVerticalOffset

            text: root.text

            color:
                root.selected
                    ? "black"
                    : Theme.text

            font: root.font
        }

        Text {
            visible: root.opensPopup

            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter

            text: "▼"

            color:
                root.selected
                    ? "black"
                    : Theme.text

            font.pixelSize:
                Math.max(
                    1,
                    root.font.pixelSize - 6)

            font.bold: false
        }
    }
}
