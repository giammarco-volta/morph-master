import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster

Rectangle {
    id: root
    color: Theme.panel
    opacity: 1.0
    clip: true

    property string title: ""
    default property alias content: contentLayout.data

    Layout.fillWidth: true
    implicitHeight: mainLayout.implicitHeight + 2 * Theme.sectionPadding

    radius: 8
    border.color: Theme.border
    border.width: 1

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: Theme.sectionPadding
        spacing: Theme.pageSpacing

        Label {
            text: root.title
            color: Theme.accent
            font.pixelSize: Theme.labelFontSize
            font.bold: true
            visible: root.title.length > 0
            Layout.preferredHeight: visible ? implicitHeight : 0
        }

        ColumnLayout {
            id: contentLayout
            Layout.fillWidth: true
            spacing: Theme.pageSpacing
        }
    }
}