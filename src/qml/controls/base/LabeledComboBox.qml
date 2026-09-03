// src/qml/LabeledComboBox.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster
import NaadaLab.Ui as SharedUi

SharedUi.LabeledControl {
    id: root

    property var modelData: []
    property int popupMinWidth: 0
    property bool longPressEnabled: false

    property alias currentIndex: combo.currentIndex
    property alias currentText: combo.currentText

    /*
     * Emitted without changing the selected value. The TapHandler remains
     * passive during an ordinary tap, so the ComboBox keeps its native
     * click/popup behaviour.
     */
    signal activated(int index)
    signal longPressed(int currentIndex)

    property bool longPressTriggered: false

    Timer {
        id: popupCloseTimer
        interval: 0
        repeat: false

        onTriggered: {
            if (root.longPressTriggered)
                combo.popup.close()

            root.longPressTriggered = false
        }
    }

    ComboBox {
        id: combo

        Layout.fillWidth: true
        Layout.preferredHeight: Theme.controlHeight
        implicitHeight: Theme.controlHeight

        model: root.modelData
        font.pixelSize: Theme.controlFontSize

        popup.width: Math.max(combo.width, root.popupMinWidth)

        onActivated: function(index) {
            root.activated(index)
        }

        /*
         * With TapHandler's default DragThreshold policy the handler uses
         * a passive grab, so an ordinary tap is still handled normally by
         * ComboBox.
         */
        TapHandler {
            enabled: root.longPressEnabled && root.enabled
            acceptedButtons: Qt.LeftButton

            onLongPressed: {
                root.longPressTriggered = true
                combo.popup.close()
                root.longPressed(combo.currentIndex)
            }
        }

        /*
         * Some styles open the popup on release. Close it on the next
         * event-loop turn after a recognized long press.
         */
        onPressedChanged: {
            if (!pressed && root.longPressTriggered)
                popupCloseTimer.restart()
        }
    }
}