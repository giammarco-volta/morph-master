import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster
import NaadaLab.Ui as SharedUi

FormSection {
    id: root

    title: "Presets"

    readonly property bool hasSelectedPreset:
        SettingsController.currentPresetIndex >= 0
        && SettingsController.currentPresetIndex
           < SettingsController.presetNames.length
        && SettingsController.currentPresetName.length > 0

    function currentPresetNameForSaveAs() {
        return SettingsController.currentPresetName.trim()
    }

    Component {
        id: presetSelectionPanel

        SearchableSelectionPopup { }
    }

    Component {
        id: presetNotesPanelComponent

        Rectangle {
            id: notesPanel

            width: Math.min(720, ApplicationWindow.window.width - 40)
            height: Math.min(500, ApplicationWindow.window.height - 40)
            radius: 10

            color: Theme.background
            border.color: Theme.border
            border.width: 1

            function init(properties) {
                notesInput.text = properties.initialNotes || ""

                Qt.callLater(function() {
                    notesInput.forceActiveFocus()
                    notesInput.cursorPosition = notesInput.text.length
                })
            }

            function saveNotes() {
                SettingsController.currentPresetNotes = notesInput.text

                if (SettingsController.saveCurrentPreset())
                    ApplicationWindow.window.closeModalPanel()
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    spacing: 8

                    Label {
                        text: "Edit preset notes"
                        color: Theme.text
                        font.pixelSize: 17
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    SharedUi.ActionButton {
                        text: "Cancel"
                        Layout.preferredWidth: 92

                        onClicked: {
                            ApplicationWindow.window.closeModalPanel()
                        }
                    }

                    SharedUi.ActionButton {
                        text: "Save"
                        Layout.preferredWidth: 92

                        onClicked: {
                            notesPanel.saveNotes()
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    radius: 5
                    color: Theme.panel
                    border.color:
                        notesInput.activeFocus
                            ? Theme.accent
                            : Theme.border
                    border.width: 1

                    ScrollView {
                        id: notesScroll

                        anchors.fill: parent
                        anchors.margins: 1
                        clip: true

                        TextArea {
                            id: notesInput

                            width: notesScroll.availableWidth
                            placeholderText:
                                "Notes about the remote instrument setup..."

                            wrapMode: TextEdit.Wrap
                            selectByMouse: true
                            persistentSelection: true

                            color: Theme.text
                            selectionColor: Theme.accent
                            selectedTextColor: "#111111"
                            placeholderTextColor: Theme.disabledText

                            font.pixelSize: 16
                            background: null
                            padding: 10

                            inputMethodHints:
                                Qt.ImhNoPredictiveText
                        }
                    }
                }
            }
        }
    }

    /*
     * Two-level twelve-part geometry:
     *
     * Left half:
     *   Preset selector 6 columns, spanning the full height.
     *
     * Right half:
     *   Edit preset notes 6 columns
     *   Save 2 | Save As 2 | Delete 2
     */

    Item {
        id: presetRow

        Layout.fillWidth: true
        implicitHeight: presetCombo.implicitHeight

        readonly property real spacing:
            UiMetrics.spacing(
                ApplicationWindow.window
                    ? ApplicationWindow.window.layoutClass
                    : UiMetrics.Desktop)

        readonly property real halfGap:
            spacing / 2

        /*
         * The preset selector occupies the complete left half.
         *
         * The right half uses the otherwise unused label area:
         * a compact notes button above and the three preset actions below.
         */
        readonly property real actionRowGap: 4

        readonly property real lowerButtonHeight:
            Theme.controlHeight - 2

        readonly property real upperButtonHeight:
            Math.max(
                20,
                presetCombo.implicitHeight
                - lowerButtonHeight
                - actionRowGap)

        function controlX(column) {
            return width * column / 12
                   + (column > 0 ? halfGap : 0)
        }

        function controlWidth(column, columnSpan) {
            const endColumn = column + columnSpan
            const leftInset = column > 0 ? halfGap : 0
            const rightInset = endColumn < 12 ? halfGap : 0

            return Math.max(
                0,
                width * columnSpan / 12
                - leftInset
                - rightInset)
        }

        SearchableComboField {
            id: presetCombo

            x: presetRow.controlX(0)
            width: presetRow.controlWidth(0, 6)
            height: implicitHeight

            title: "Preset"
            currentText: SettingsController.currentPresetName
            placeholderText: "Select preset..."
            enabled: SettingsController.presetNames.length > 0

            onClicked: {
                ApplicationWindow.window.openModalPanel(
                    presetSelectionPanel,
                    {
                        panelTitle: "Presets",
                        currentLabel: "Current preset",
                        singularNoun: "preset",
                        pluralNoun: "presets",
                        currentValue: SettingsController.currentPresetName,
                        allItems: SettingsController.presetNames,
                        filterItems: function(text) {
                            return SettingsController.findPresetNames(text)
                        },
                        commit: function(name) {
                            const names = SettingsController.presetNames
                            const index = names.indexOf(name)

                            if (index >= 0)
                                SettingsController.activatePreset(index)
                        }
                    })
            }
        }

        SharedUi.ActionButton {
            x: presetRow.controlX(6)
            y: 0

            width: presetRow.controlWidth(6, 6)
            height: presetRow.upperButtonHeight

            text: "Edit preset notes"
            enabled: root.hasSelectedPreset

            font.pixelSize: Math.max(11, Theme.labelFontSize - 2)

            textVerticalOffset: -4

            onClicked: {
                ApplicationWindow.window.openModalPanel(
                    presetNotesPanelComponent,
                    {
                        initialNotes:
                            SettingsController.currentPresetNotes,
                        closeOnOutsideClick: false
                    })
            }
        }

        SharedUi.ActionButton {
            x: presetRow.controlX(6)
            y: presetRow.height - presetRow.lowerButtonHeight

            width: presetRow.controlWidth(6, 2)
            height: presetRow.lowerButtonHeight

            text: "Save"
            enabled: root.hasSelectedPreset

            onClicked: {
                SettingsController.saveCurrentPreset()
            }
        }

        SharedUi.ActionButton {
            x: presetRow.controlX(8)
            y: presetRow.height - presetRow.lowerButtonHeight

            width: presetRow.controlWidth(8, 2)
            height: presetRow.lowerButtonHeight

            text: "Save As..."

            onClicked: {
                ApplicationWindow.window.openModalPanel(
                    saveAsPanelComponent,
                    {
                        initialPresetName:
                            root.currentPresetNameForSaveAs(),
                        closeOnOutsideClick: false
                    })
            }
        }

        SharedUi.ActionButton {
            x: presetRow.controlX(10)
            y: presetRow.height - presetRow.lowerButtonHeight

            width: presetRow.controlWidth(10, 2)
            height: presetRow.lowerButtonHeight

            text: "Delete"
            enabled: root.hasSelectedPreset

            onClicked: {
                SettingsController.deleteCurrentPreset()
            }
        }
    }

    Component {
        id: saveAsPanelComponent

        Rectangle {
            id: panelRoot

            property string initialPresetName: ""

            function init(properties) {
                initialPresetName =
                    properties.initialPresetName || ""

                Qt.callLater(function() {
                    presetNameInput.forceActiveFocus()
                    presetNameInput.cursorPosition =
                        presetNameInput.text.length
                })
            }

            width:
                Math.min(
                    430,
                    ApplicationWindow.window.width - 40)
            height: 150
            radius: 10

            color: Theme.background
            border.color: Theme.border
            border.width: 1

            function savePreset() {
                const name = presetNameInput.text.trim()

                if (name.length === 0)
                    return

                SettingsController.saveCurrentPresetAs(name)
                ApplicationWindow.window.closeModalPanel()
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    spacing: 8

                    Label {
                        text: "Save preset as"
                        color: Theme.text
                        font.pixelSize: 17
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    SharedUi.ActionButton {
                        text: "Cancel"
                        Layout.preferredWidth: 92

                        onClicked: {
                            ApplicationWindow.window.closeModalPanel()
                        }
                    }

                    SharedUi.ActionButton {
                        text: "Save"
                        Layout.preferredWidth: 92

                        enabled:
                            presetNameInput.text.trim().length > 0

                        onClicked: {
                            panelRoot.savePreset()
                        }
                    }
                }

                Rectangle {
                    id: inputFrame

                    Layout.fillWidth: true
                    Layout.preferredHeight: 42

                    radius: 5
                    color: Theme.panel
                    border.color:
                        presetNameInput.activeFocus
                            ? Theme.accent
                            : Theme.border
                    border.width: 1

                    TextInput {
                        id: presetNameInput

                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10

                        text: panelRoot.initialPresetName
                        verticalAlignment: TextInput.AlignVCenter

                        color: Theme.text
                        selectionColor: Theme.accent
                        selectedTextColor: "#111111"

                        font.pixelSize: 16
                        clip: true
                        selectByMouse: true
                        activeFocusOnPress: true

                        inputMethodHints:
                            Qt.ImhNoPredictiveText

                        onAccepted: {
                            panelRoot.savePreset()
                        }

                        Component.onCompleted: {
                            forceActiveFocus()

                            if (Qt.platform.os !== "android")
                                selectAll()
                        }
                    }

                    Text {
                        visible:
                            presetNameInput.text.length === 0
                            && !presetNameInput.activeFocus

                        anchors.verticalCenter:
                            parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 10

                        text: "Preset name"
                        color: Theme.disabledText
                        font.pixelSize: 16
                    }
                }
            }
        }
    }

}