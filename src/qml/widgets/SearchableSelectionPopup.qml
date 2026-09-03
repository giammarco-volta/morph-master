import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster

Rectangle {
    id: root

    property string panelTitle: "Select item"
    property string currentLabel: "Current item"
    property string singularNoun: "item"
    property string pluralNoun: "items"
    property string currentValue: ""
    property var allItems: []
    property var results: []
    property var commit
    property var filterItems

    width: Math.min(620, ApplicationWindow.window ? ApplicationWindow.window.width - 40 : 620)
    height: Math.min(620, ApplicationWindow.window ? ApplicationWindow.window.height - 40 : 620)
    radius: 8
    color: Theme.panel
    border.color: Theme.border
    border.width: 1

    function init(properties) {
        panelTitle = properties.panelTitle || panelTitle
        currentLabel = properties.currentLabel || currentLabel
        singularNoun = properties.singularNoun || singularNoun
        pluralNoun = properties.pluralNoun || pluralNoun
        currentValue = properties.currentValue || ""
        allItems = properties.allItems || []
        commit = properties.commit
        filterItems = properties.filterItems

        searchField.clear()
        rebuildResults()
        Qt.callLater(function() {
            searchField.forceActiveFocus()
            root.positionCurrentItem()
        })
    }

    function rebuildResults() {
        if (root.filterItems) {
            results = root.filterItems(searchField.text)
            return
        }

        results = root.allItems
    }

    function positionCurrentItem() {
        const index = root.results.indexOf(root.currentValue)

        if (index < 0)
            return

        resultsView.currentIndex = index
        resultsView.forceLayout()
        resultsView.positionViewAtIndex(index, ListView.Center)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: root.panelTitle
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

        TextField {
            id: searchField

            Layout.fillWidth: true
            placeholderText: "Search name…"
            selectByMouse: true
            inputMethodHints: Qt.ImhNoPredictiveText
            onTextChanged: root.rebuildResults()

            rightPadding: clearButton.visible ? 34 : 10

            ToolButton {
                id: clearButton

                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                visible: searchField.text.length > 0
                text: "×"
                onClicked: searchField.clear()
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

                Label {
                    text: root.currentLabel
                    color: Theme.secondaryText
                    font.pixelSize: 12
                }

                Label {
                    Layout.fillWidth: true
                    text: root.currentValue.length > 0 ? root.currentValue : "None"
                    elide: Text.ElideRight
                    font.bold: true
                }
            }
        }

        Label {
            Layout.fillWidth: true
            text: root.results.length + " / " + root.allItems.length + " "
                  + (root.allItems.length === 1
                     ? root.singularNoun
                     : root.pluralNoun)
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
                required property string modelData

                width: resultsView.width
                height: 44

                contentItem: RowLayout {
                    spacing: 8

                    Label {
                        text: modelData === root.currentValue ? "✓" : ""
                        font.bold: true
                        Layout.preferredWidth: 16
                    }

                    Label {
                        Layout.fillWidth: true
                        text: modelData
                        elide: Text.ElideRight
                        font.bold: modelData === root.currentValue
                    }
                }

                onClicked: {
                    if (root.commit)
                        root.commit(modelData)

                    ApplicationWindow.window.closeModalPanel()
                }
            }

            Label {
                anchors.centerIn: parent
                visible: root.results.length === 0
                text: "No matching " + root.pluralNoun
                color: Theme.secondaryText
            }
        }
    }
}
