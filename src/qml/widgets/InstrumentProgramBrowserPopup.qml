import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster

Rectangle {
    id: root

    property var track: null
    property var results: []
    property int totalPrograms: track ? track.instrumentProgramCount : 0

    width: Math.min(760, ApplicationWindow.window ? ApplicationWindow.window.width - 40 : 760)
    height: Math.min(620, ApplicationWindow.window ? ApplicationWindow.window.height - 40 : 620)
    radius: 8
    color: Theme.panel
    border.color: Theme.border
    border.width: 1

    function init(properties) {
        track = properties.track

        nameField.clear()
        programField.clear()

        searchTimer.stop()
        rebuildResults()

        Qt.callLater(function() {
            nameField.forceActiveFocus()
            root.positionCurrentProgram()
        })
    }

    function textFilterIsValid() {
        return nameField.text.trim().length >= 2
    }

    function numericFilterIsValid() {
        return programField.text.length > 0
    }

    function scheduleSearch() {
        if (!track)
            return

        searchTimer.restart()
    }

    function rebuildResults() {
        if (!track) {
            results = []
            return
        }

        const requestedProgram = numericFilterIsValid() ? Number(programField.text) - 1 : -1
        results = track.findInstrumentPrograms(nameField.text, requestedProgram)
    }

    function isCurrentProgram(programData) {
        return track
                && programData.msb === track.bankMSB
                && programData.lsb === track.bankLSB
                && programData.programNumber === track.programNumber
    }

    function positionCurrentProgram() {
        for (let index = 0; index < root.results.length; ++index) {
            if (!root.isCurrentProgram(root.results[index]))
                continue

            resultsView.currentIndex = index
            resultsView.forceLayout()
            resultsView.positionViewAtIndex(index, ListView.Center)
            return
        }
    }

    Timer {
        id: searchTimer
        interval: 40
        repeat: false
        onTriggered: root.rebuildResults()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: "Instrument programs"
                font.pixelSize: 20
                font.bold: true
                Layout.fillWidth: true
            }

            ToolButton {
                text: "×"
                font.pixelSize: 22
                onClicked: ApplicationWindow.window.closeModalPanel()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            TextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: "Search name…"
                selectByMouse: true
                onTextChanged: root.scheduleSearch()

                rightPadding: nameClear.visible ? 34 : 10

                ToolButton {
                    id: nameClear
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    visible: nameField.text.length > 0
                    text: "×"
                    onClicked: nameField.clear()
                }
            }

            TextField {
                id: programField
                Layout.preferredWidth: 150
                placeholderText: "Program #"
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator { bottom: 1; top: 128 }
                selectByMouse: true
                onTextChanged: root.scheduleSearch()

                rightPadding: programClear.visible ? 34 : 10

                ToolButton {
                    id: programClear
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    visible: programField.text.length > 0
                    text: "×"
                    onClicked: programField.clear()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: currentColumn.implicitHeight + 16
            radius: 5
            color: Theme.panelDark
            border.color: Theme.border

            ColumnLayout {
                id: currentColumn
                anchors.fill: parent
                anchors.margins: 8
                spacing: 2

                Label { text: "Current program"; color: Theme.secondaryText; font.pixelSize: 12 }
                Label {
                    Layout.fillWidth: true
                    text: root.track ? root.track.instrumentProgramDisplayName : ""
                    elide: Text.ElideRight
                    font.bold: true
                }
            }
        }

        Label {
            Layout.fillWidth: true
            text: root.results.length + " / " + root.totalPrograms
                  + (root.totalPrograms === 1 ? " program" : " programs")
            color: Theme.secondaryText
        }

        ListView {
            id: resultsView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.results
            spacing: 2
            reuseItems: true

            ScrollBar.vertical: ScrollBar { }

            delegate: ItemDelegate {
                required property var modelData
                width: resultsView.width
                height: 48

                contentItem: RowLayout {
                    spacing: 8

                    Label {
                        text: root.isCurrentProgram(modelData) ? "✓" : ""
                        font.bold: true
                        Layout.preferredWidth: 16
                        Layout.alignment: Qt.AlignTop
                    }

                    Column {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            width: parent.width
                            text: modelData.programName
                            elide: Text.ElideRight
                            font.bold: true
                        }
                        Label {
                            width: parent.width
                            text: (modelData.bankName.length > 0 ? modelData.bankName + " · " : "")
                                  + "MSB " + modelData.msb + " · LSB " + modelData.lsb
                                  + " · Program " + (modelData.programNumber + 1)
                            elide: Text.ElideRight
                            color: Theme.secondaryText
                            font.pixelSize: 12
                        }
                    }
                }

                onClicked: {
                    root.track.selectInstrumentProgram(modelData.msb, modelData.lsb, modelData.programNumber)
                    ApplicationWindow.window.closeModalPanel()
                }
            }

            Label {
                anchors.centerIn: parent
                visible: root.results.length === 0 && !searchTimer.running
                text: "No matching programs"
                color: Theme.secondaryText
            }
        }
    }
}
