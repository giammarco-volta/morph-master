import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster
import NaadaLab.Ui as SharedUi

FormSection {
    id: root
    title: "Performance"

    readonly property int layoutClass:
        ApplicationWindow.window
            ? ApplicationWindow.window.layoutClass
            : UiMetrics.Desktop

    readonly property real controlHeight:
        UiMetrics.controlHeight(layoutClass)

    readonly property real rowSpacing:
        UiMetrics.spacing(layoutClass)

    property var keyboardRangeValues: [
        0, // Full
        1, // 88 keys
        2, // 76 keys
        3, // 73 keys
        4, // 61 keys
        5, // 49 keys
        6, // 37 keys
        7  // 25 keys
    ]


    property var playModeValues: [
        0, // PlayMode::Poly
        1  // PlayMode::MonoRetrigVelOff
    ]

    Component {
        id: pitchBendRangePanel

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

                    SharedUi.ActionButton {
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

    component LabeledChoicePair: Item {
        id: pair

        property string title: ""
        property string firstText: ""
        property string secondText: ""
        property int currentIndex: 0

        signal activated(int index)

        readonly property int layoutClass:
            ApplicationWindow.window
                ? ApplicationWindow.window.layoutClass
                : UiMetrics.Desktop

        readonly property real fallbackFieldHeight:
            UiMetrics.controlHeight(layoutClass)

        readonly property real buttonSpacing:
            UiMetrics.spacing(layoutClass)

        implicitWidth: 220
        implicitHeight:
            titleLabel.implicitHeight
            + 4
            + fallbackFieldHeight

        Label {
            id: titleLabel

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            text: pair.title
            color: pair.enabled
                   ? Theme.secondaryText
                   : Theme.disabledText

            font.pixelSize: Theme.labelFontSize
            font.bold: true

            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        SharedUi.ActionButton {
            anchors.left: parent.left
            anchors.top: titleLabel.bottom
            anchors.topMargin: 4
            anchors.bottom: parent.bottom

            width: Math.max(
                       0,
                       (parent.width - pair.buttonSpacing) / 2)

            text: pair.firstText
            selected: pair.currentIndex === 0

            onClicked: pair.activated(0)
        }

        SharedUi.ActionButton {
            anchors.right: parent.right
            anchors.top: titleLabel.bottom
            anchors.topMargin: 4
            anchors.bottom: parent.bottom

            width: Math.max(
                       0,
                       (parent.width - pair.buttonSpacing) / 2)

            text: pair.secondText
            selected: pair.currentIndex === 1

            onClicked: pair.activated(1)
        }
    }

    Component {
        id: instrumentSelectionPanel

        SearchableSelectionPopup { }
    }

    Item {
        id: programRow

        Layout.fillWidth: true

        implicitHeight:
            instrumentCombo.implicitHeight

        readonly property real halfGap:
            root.rowSpacing / 2

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

        LabeledChoicePair {
            id: programMode

            x: programRow.controlX(0)
            width: programRow.controlWidth(0, 8)
            height: instrumentCombo.implicitHeight

            title: "Program Selection"
            firstText: "Instrument Definition"
            secondText: "Manual"

            currentIndex:
                SettingsController.useInstrumentDefinition
                    ? 0
                    : 1

            onActivated: function(index) {
                const useDefinition = index === 0

                if (SettingsController.useInstrumentDefinition
                    !== useDefinition) {
                    SettingsController.useInstrumentDefinition =
                        useDefinition
                }
            }
        }

        SearchableComboField {
            id: instrumentCombo

            x: programRow.controlX(8)
            width: programRow.controlWidth(8, 4)
            height: implicitHeight

            title:
                SettingsController.instrumentDatabaseLoading
                    ? "Loading Instruments..."
                    : "Selected Instrument"

            currentText: SettingsController.knownInstrumentName
            placeholderText: "Select instrument..."

            enabled:
                SettingsController.useInstrumentDefinition
                && !SettingsController.instrumentDatabaseLoading
                && SettingsController.instrumentNames.length > 0

            onClicked: {
                ApplicationWindow.window.openModalPanel(
                    instrumentSelectionPanel,
                    {
                        panelTitle: "Instrument Definitions",
                        currentLabel: "Current instrument",
                        singularNoun: "instrument",
                        pluralNoun: "instruments",
                        currentValue: SettingsController.knownInstrumentName,
                        allItems: SettingsController.instrumentNames,
                        filterItems: function(text) {
                            return SettingsController.findInstrumentNames(text)
                        },
                        commit: function(name) {
                            if (SettingsController.knownInstrumentName !== name)
                                SettingsController.knownInstrumentName = name
                        }
                    })
            }
        }
    }

    Item {
        id: playRow

        Layout.fillWidth: true

        implicitHeight:
            Math.max(pitchBendRangeSelector.implicitHeight,
                     keyboardRangeCombo.implicitHeight)

        readonly property real halfGap:
            root.rowSpacing / 2

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

        SharedUi.LabeledControl {
            id: pitchBendRangeSelector

            x: playRow.controlX(0)
            width: playRow.controlWidth(0, 3)
            height: implicitHeight

            title: "Pitch Bend Range"

            SharedUi.ActionButton {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.controlHeight
                implicitHeight: Theme.controlHeight

                text: SettingsController.pitchBendRange === 1
                      ? "1 semitone"
                      : SettingsController.pitchBendRange + " semitones"
                opensPopup: true

                // This button can be considerably narrower than the other
                // popup buttons, especially in the tablet profile. Reserve a
                // little more visual separation between its long value text
                // and the popup indicator, and place the indicator closer to
                // the right edge.
                contentItem: Item {
                    Text {
                        anchors.centerIn: parent
                        anchors.horizontalCenterOffset: -4

                        text: parent.parent.text
                        color: Theme.text
                        font: parent.parent.font
                    }

                    Text {
                        anchors.right: parent.right
                        anchors.rightMargin: root.layoutClass === UiMetrics.Tablet ? -10 : 3
                        anchors.verticalCenter: parent.verticalCenter

                        text: "▼"
                        color: Theme.text
                        font.pixelSize: Math.max(
                            1,
                            parent.parent.font.pixelSize - 6)
                        font.bold: false
                    }
                }

                onClicked: openModalPanel(pitchBendRangePanel, {
                    currentRange: SettingsController.pitchBendRange,
                    commit: function(value) {
                        SettingsController.pitchBendRange = value
                    }
                })
            }
        }

        LabeledComboBox {
            id: keyboardRangeCombo

            x: playRow.controlX(3)
            width: playRow.controlWidth(3, 3)
            height: implicitHeight

            title: "Keyboard Range"

            modelData: [
                "Full",
                "88 keys",
                "76 keys",
                "73 keys",
                "61 keys",
                "49 keys",
                "37 keys",
                "25 keys"
            ]

            currentIndex: {
                for (let i = 0;
                     i < keyboardRangeValues.length;
                     ++i) {
                    if (keyboardRangeValues[i]
                        === SettingsController.keyboardRangeId) {
                        return i
                    }
                }

                return 0
            }

            onActivated: function(index) {
                if (index < 0
                    || index >= keyboardRangeValues.length) {
                    return
                }

                const value = keyboardRangeValues[index]

                if (SettingsController.keyboardRangeId !== value)
                    SettingsController.keyboardRangeId = value
            }
        }

        LabeledChoicePair {
            id: playModeGroup

            x: playRow.controlX(6)
            width: playRow.controlWidth(6, 6)
            height: keyboardRangeCombo.implicitHeight

            title: "Play Mode"
            firstText: "Poly"
            secondText: "Mono"

            currentIndex: {
                for (let i = 0;
                     i < playModeValues.length;
                     ++i) {
                    if (playModeValues[i]
                        === SettingsController.playMode) {
                        return i
                    }
                }

                return 0
            }

            onActivated: function(index) {
                if (index < 0
                    || index >= playModeValues.length) {
                    return
                }

                const value = playModeValues[index]

                if (SettingsController.playMode !== value)
                    SettingsController.playMode = value
            }
        }
    }
}
