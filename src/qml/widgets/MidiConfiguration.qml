import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster
import NaadaLab.Ui as SharedUi

SharedUi.FormSection {
    id: root

    title: "MIDI"

    property bool updatingMidiFromModel: false

    function syncMidiFromModel() {
        root.updatingMidiFromModel = true
        inputChannelSelector.currentChannel =
            SettingsController.midiInChannel + 1
        root.updatingMidiFromModel = false
    }

    Component.onCompleted: root.syncMidiFromModel()

    SharedUi.MidiPortSelector {
        title: "MIDI Input"
        ports: SettingsController.midiInputPorts
        currentPort: SettingsController.midiInPort

        Layout.fillWidth: true

        onPortSelected: function(portName) {
            if (SettingsController.midiInPort !== portName)
                SettingsController.midiInPort = portName
        }

        onRefreshRequested: SettingsController.refreshMidiInPorts()
    }

    /*
     * Input Channel uses 8/12 of the row; the preset-CC protection
     * checkbox uses the remaining 4/12.
     */
    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        SharedUi.MidiChannelSelector {
            id: inputChannelSelector

            title: "Input Channel"
            currentChannel: 1

            onOpenPanelRequested: function(panelComponent, properties) {
                ApplicationWindow.window.openModalPanel(panelComponent, properties)
            }

            onClosePanelRequested: {
                ApplicationWindow.window.closeModalPanel()
            }

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

        SharedUi.ActionButton {
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

    SharedUi.MidiPortSelector {
        title: "MIDI Output"
        ports: SettingsController.midiOutputPorts
        currentPort: SettingsController.midiOutPort

        Layout.fillWidth: true

        onPortSelected: function(portName) {
            if (SettingsController.midiOutPort !== portName)
                SettingsController.midiOutPort = portName
        }

        onRefreshRequested: SettingsController.refreshMidiOutPorts()
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        SharedUi.ActionButton {
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

        SharedUi.ActionButton {
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

                    SharedUi.ActionButton {
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

        function onMidiInChannelChanged() {
            root.syncMidiFromModel()
        }
    }
}