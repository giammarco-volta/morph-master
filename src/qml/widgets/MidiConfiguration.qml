import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster

FormSection {
    id: root

    title: "MIDI"


    // Per ora sono placeholder.
    // Più avanti diventeranno liste provenienti dal backend MIDI C++.
    property var midiInputPorts: ["Input device"]
    property var midiOutputPorts: ["Output device"]

    property bool updatingMidiFromModel: false

    function indexOfText(model, text) {
        for (let i = 0; i < model.length; ++i) {
            if (model[i] === text)
                return i
        }

        return -1
    }

    function syncMidiFromModel() {
        root.updatingMidiFromModel = true

        inputCombo.modelData = SettingsController.midiInputPorts
        outputCombo.modelData = SettingsController.midiOutputPorts

        inputCombo.currentIndex =
            root.indexOfText(inputCombo.modelData,
                             SettingsController.midiInPort)

        outputCombo.currentIndex =
            root.indexOfText(outputCombo.modelData,
                             SettingsController.midiOutPort)

        inputChannelSelector.currentChannel =
            SettingsController.midiInChannel + 1

        root.updatingMidiFromModel = false
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        LabeledComboBox {
            id: inputCombo

            title: "MIDI Input"
            modelData: []

            Layout.fillWidth: true
            Layout.preferredWidth: 2

            Component.onCompleted: {
                root.syncMidiFromModel()
            }

            onCurrentIndexChanged: {
                if (root.updatingMidiFromModel)
                    return

                if (currentIndex < 0 || currentIndex >= inputCombo.modelData.length)
                    return

                const portName = inputCombo.modelData[currentIndex]

                if (SettingsController.midiInPort !== portName)
                    SettingsController.midiInPort = portName
            }
        }

        ActionButton {
            text: "Refresh"

            Layout.fillWidth: true
            Layout.preferredWidth: 1
            Layout.preferredHeight: Theme.controlHeight
            Layout.alignment: Qt.AlignBottom

            onClicked: {
                SettingsController.refreshMidiInPorts()
            }
        }
    }

    /*
     * Input Channel uses 8/12 of the row; the preset-CC protection
     * checkbox uses the remaining 4/12.
     */
    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        MidiChannelSelector {
            id: inputChannelSelector

            title: "Input Channel"
            currentChannel: 1

            Layout.fillWidth: true
            Layout.preferredWidth: 2

            onCurrentChannelChanged: {
                if (root.updatingMidiFromModel)
                    return

                const channel0 = currentChannel - 1

                if (SettingsController.midiInChannel !== channel0)
                    SettingsController.midiInChannel = channel0
            }
        }

        ActionButton {
            id: filterPresetCcButton

            text: "Ignore preset CCs"

            selected:
                SettingsController.filterPresetControlChanges

            Layout.fillWidth: true
            Layout.preferredWidth: 1
            Layout.preferredHeight: Theme.controlHeight
            Layout.alignment: Qt.AlignBottom

            ToolTip.visible: hovered
            ToolTip.delay: 400
            ToolTip.text:
                "Ignore incoming Volume, Pan, Expression, Reverb, "
                + "Chorus, Tone and Timbre Control Changes. "
                + "Expression (CC11) is always filtered in Mono mode."

            onClicked: {
                SettingsController.filterPresetControlChanges =
                    !SettingsController.filterPresetControlChanges
            }
        }    
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        LabeledComboBox {
            id: outputCombo

            title: "MIDI Output"
            modelData: []

            Layout.fillWidth: true
            Layout.preferredWidth: 2

            onCurrentIndexChanged: {
                if (root.updatingMidiFromModel)
                    return

                if (currentIndex < 0 || currentIndex >= outputCombo.modelData.length)
                    return

                const portName = outputCombo.modelData[currentIndex]

                if (SettingsController.midiOutPort !== portName)
                    SettingsController.midiOutPort = portName
            }
        }

        ActionButton {
            text: "Refresh"

            Layout.fillWidth: true
            Layout.preferredWidth: 1
            Layout.preferredHeight: Theme.controlHeight
            Layout.alignment: Qt.AlignBottom

            onClicked: {
                SettingsController.refreshMidiOutPorts()
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        ActionButton {
            text: "Channel Soft Reset"
            opensPopup: true

            Layout.fillWidth: true
            Layout.preferredWidth: 2

            ToolTip.visible: hovered
            ToolTip.delay: 400
            ToolTip.text:
                "Send Reset All Controllers and "
                + "All Notes Off to a MIDI channel"

            onClicked: {
                ApplicationWindow.window.openModalPanel(
                    softResetChannelPanel,
                    {})
            }
        }

        ActionButton {
            text: "GM2 Reset"

            Layout.fillWidth: true
            Layout.preferredWidth: 1

            onClicked: {
                SettingsController.sendGM2Reset()
            }
        }
    }

    Component {
        id: softResetChannelPanel

        Rectangle {
            id: panel

            implicitWidth: channelGrid.implicitWidth + 32
            implicitHeight: channelGrid.implicitHeight + 32
            width: implicitWidth
            height: implicitHeight

            color: Theme.panel
            radius: 8
            border.color: Theme.border
            border.width: 1

            MouseArea {
                anchors.fill: parent

                onClicked: function(mouse) {
                    mouse.accepted = true
                }
            }

            GridLayout {
                id: channelGrid

                anchors.fill: parent
                anchors.margins: 16

                columns: 4
                rowSpacing: 8
                columnSpacing: 8

                Repeater {
                    model: 16

                    ActionButton {
                        required property int index

                        text: String(index + 1)

                        Layout.preferredWidth: 64
                        Layout.preferredHeight: 48

                        onClicked: {
                            const channel = index + 1

                            SettingsController.sendSoftReset(channel)

                            ApplicationWindow.window.closeModalPanel()
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: SettingsController
        ignoreUnknownSignals: true

        function onMidiInPortChanged() {
            root.syncMidiFromModel()
        }

        function onMidiOutPortChanged() {
            root.syncMidiFromModel()
        }

        function onMidiInChannelChanged() {
            root.syncMidiFromModel()
        }

        function onMidiInputPortsChanged() {
            root.syncMidiFromModel()
        }

        function onMidiOutputPortsChanged() {
            root.syncMidiFromModel()
        }
    }
}