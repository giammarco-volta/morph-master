import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MorphMaster
import NaadaLab.Ui as SharedUi

Rectangle {
    id: root
    color: SharedUi.Theme.background
    function syncControlChannel() {
        controlChannelSelector.currentChannel = SettingsController.controlChannel + 1;
    }
    Component.onCompleted: syncControlChannel()
    Connections {
        target: SettingsController
        function onControlBindingChanged() {
            root.syncControlChannel();
        }
    }
    Component {
        id: presetSelectionPanel
        SharedUi.SearchableSelectionPopup {
            onCloseRequested: ApplicationWindow.window.closeModalPanel()
        }
    }
    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12
        Rectangle {
            color: SharedUi.Theme.panel
            radius: 8
            border.color: SharedUi.Theme.border
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: SharedUi.Theme.sectionPadding
                Label {
                    text: qsTr("Preset list")
                    color: SharedUi.Theme.accent
                    font.pixelSize: SharedUi.Theme.labelFontSize
                    font.bold: true
                }
                ListView {
                    id: presetView
                    objectName: "presetListView"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 6
                    model: SettingsController.presetList
                    currentIndex: SettingsController.presetListIndex
                    onCurrentIndexChanged: {
                        if (currentIndex >= 0)
                            positionViewAtIndex(currentIndex, ListView.Contain);
                    }
                    ScrollBar.vertical: ScrollBar {}
                    delegate: RowLayout {
                        id: entry
                        required property int index
                        required property string modelData
                        width: presetView.width
                        height: 48
                        spacing: 4
                        Button {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            text: entry.modelData
                            highlighted: SettingsController.presetListIndex === entry.index
                            onClicked: SettingsController.recallPresetListItem(entry.index)
                            contentItem: Text {
                                text: parent.text
                                color: SharedUi.Theme.text
                                font.pixelSize: 16
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            background: Rectangle {
                                radius: 5
                                color: parent.highlighted ? SharedUi.Theme.selection : SharedUi.Theme.panel
                                border.color: parent.highlighted ? SharedUi.Theme.accent : SharedUi.Theme.border
                            }
                        }
                        ToolButton {
                            text: "..."
                            Accessible.name: qsTr("Edit list entry")
                            onClicked: entryMenu.popup()
                            Menu {
                                id: entryMenu
                                MenuItem {
                                    text: qsTr("Move up")
                                    enabled: entry.index > 0
                                    onTriggered: SettingsController.movePresetListItem(entry.index, entry.index - 1)
                                }
                                MenuItem {
                                    text: qsTr("Move down")
                                    enabled: entry.index < SettingsController.presetList.length - 1
                                    onTriggered: SettingsController.movePresetListItem(entry.index, entry.index + 1)
                                }
                                MenuItem {
                                    text: qsTr("Remove from list")
                                    onTriggered: SettingsController.removeFromPresetList(entry.index)
                                }
                            }
                        }
                    }
                    footer: Button {
                        width: presetView.width
                        height: 48
                        text: "+"
                        font.pixelSize: 28
                        enabled: SettingsController.presetNames.length > 0
                        ToolTip.visible: hovered
                        ToolTip.text: "add preset"
                        Accessible.name: qsTr("Add preset")
                        onClicked: ApplicationWindow.window.openModalPanel(presetSelectionPanel, {
                            panelTitle: qsTr("Add preset"),
                            currentLabel: qsTr("Current preset"),
                            singularNoun: "preset",
                            pluralNoun: "presets",
                            currentValue: SettingsController.currentPresetName,
                            allItems: SettingsController.presetNames,
                            filterItems: function (text) {
                                return SettingsController.findPresetNames(text);
                            },
                            commit: function (name) {
                                SettingsController.addToPresetList(name);
                                Qt.callLater(function () {
                                    presetView.positionViewAtEnd();
                                });
                            }
                        })
                    }
                }
            }
        }
        ScrollView {
            id: controlsScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            contentWidth: availableWidth
            clip: true
            ColumnLayout {
                width: controlsScroll.availableWidth
                spacing: 12
                SharedUi.FormSection {
                    id: controlSettingsPanel
                    objectName: "controlSettingsPanel"
                    title: qsTr("Keyboard and pedal control")
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.alignment: Qt.AlignTop
                    CheckBox {
                        objectName: "midiControlEnabled"
                        text: qsTr("Enable assigned control")
                        checked: SettingsController.controlEnabled
                        onToggled: SettingsController.controlEnabled = checked
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.maximumWidth: 500
                        spacing: SharedUi.Theme.spacing
                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                text: qsTr("Control channel")
                                color: SharedUi.Theme.secondaryText
                                font.pixelSize: SharedUi.Theme.labelFontSize
                                font.bold: true
                            }
                            CheckBox {
                                id: sameAsInputCheck
                                objectName: "presetControlSameAsInput"
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                text: qsTr("Same as Input channel")
                                checked: SettingsController.controlSameAsInput
                                onToggled: SettingsController.controlSameAsInput = checked
                                contentItem: Text {
                                    text: sameAsInputCheck.text
                                    font: sameAsInputCheck.font
                                    color: SharedUi.Theme.text
                                    leftPadding: sameAsInputCheck.indicator.width + sameAsInputCheck.spacing
                                    verticalAlignment: Text.AlignVCenter
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                        SharedUi.MidiChannelSelector {
                            id: controlChannelSelector
                            objectName: "presetControlChannel"
                            title: ""
                            enabled: !SettingsController.controlSameAsInput
                            Layout.fillWidth: true
                            Layout.maximumWidth: 500
                            onOpenPanelRequested: function (panelComponent, properties) {
                                ApplicationWindow.window.openModalPanel(panelComponent, properties);
                            }
                            onClosePanelRequested: ApplicationWindow.window.closeModalPanel()
                            onCurrentChannelChanged: {
                                if (SettingsController.controlChannel !== currentChannel - 1)
                                    SettingsController.controlChannel = currentChannel - 1;
                            }
                        }
                    }
                    GridLayout {
                        Layout.fillWidth: true
                        Layout.maximumWidth: 500
                        columns: controlSettingsPanel.width >= 430 ? 2 : 1
                        columnSpacing: 12
                        rowSpacing: 8
                        Label {
                            text: qsTr("Control")
                            color: SharedUi.Theme.text
                        }
                        SharedUi.LabeledComboBox {
                            objectName: "midiControlSource"
                            Layout.fillWidth: true
                            Layout.maximumWidth: 400
                            modelData: SettingsController.controlSources
                            currentIndex: SettingsController.controlSource
                            onActivated: function (index) {
                                SettingsController.controlSource = index;
                            }
                            Accessible.name: qsTr("MIDI control")
                        }
                        Label {
                            text: qsTr("Action")
                            color: SharedUi.Theme.text
                        }
                        SharedUi.LabeledComboBox {
                            id: controlActionSelector
                            objectName: "midiControlAction"
                            Layout.fillWidth: true
                            Layout.maximumWidth: 400
                            modelData: [qsTr("Next preset"), qsTr("Previous preset")]
                            currentIndex: SettingsController.controlAction
                            onActivated: function (index) {
                                SettingsController.controlAction = index;
                            }
                            Accessible.name: qsTr("Assigned action")
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.maximumWidth: controlActionSelector.x + controlActionSelector.width
                        Label {
                            text: qsTr("Activation threshold")
                            color: SharedUi.Theme.text
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }
                        SpinBox {
                            objectName: "midiControlThreshold"
                            Layout.minimumHeight: SharedUi.Theme.controlHeight
                            Layout.preferredHeight: SharedUi.Theme.controlHeight
                            Layout.maximumHeight: SharedUi.Theme.controlHeight
                            from: 1
                            to: 127
                            editable: true
                            value: SettingsController.controlThreshold
                            onValueModified: SettingsController.controlThreshold = value
                            Accessible.name: qsTr("Activation threshold")
                        }
                    }
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: SharedUi.Theme.secondaryText
                        text: qsTr("One action per gesture. Enabled controls are reserved for Morphora.")
                    }
                }

                SharedUi.FormSection {
                    title: qsTr("Recall preset")
                    Layout.fillWidth: true
                    Button {
                        objectName: "previousListPreset"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 80
                        text: "\u25b2"
                        font.pixelSize: 36
                        background: Rectangle {
                            radius: 8
                            color: parent.down ? SharedUi.Theme.selection : SharedUi.Theme.background
                            border.color: SharedUi.Theme.border
                            opacity: parent.enabled ? 1 : 0.45
                        }
                        enabled: SettingsController.presetList.length > 0 && SettingsController.presetListIndex !== 0
                        Accessible.name: qsTr("Previous preset")
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Previous preset")
                        onClicked: SettingsController.stepPresetList(-1)
                    }
                    Button {
                        objectName: "nextListPreset"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 80
                        text: "\u25bc"
                        font.pixelSize: 36
                        background: Rectangle {
                            radius: 8
                            color: parent.down ? SharedUi.Theme.selection : SharedUi.Theme.background
                            border.color: SharedUi.Theme.border
                            opacity: parent.enabled ? 1 : 0.45
                        }
                        enabled: SettingsController.presetList.length > 0 && SettingsController.presetListIndex < SettingsController.presetList.length - 1
                        Accessible.name: qsTr("Next preset")
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Next preset")
                        onClicked: SettingsController.stepPresetList(1)
                    }
                }
            }
        }
    }
}
