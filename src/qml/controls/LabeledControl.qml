import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster

ColumnLayout {
    id: root

    property string title: ""
    property bool showTitle: title.length > 0

    default property alias content: contentLayout.data

    Layout.fillWidth: true
    spacing: Theme.spacing

    Label {
        text: root.title
        color: Theme.secondaryText
        font.pixelSize: Theme.labelFontSize
        font.bold: true
        visible: root.showTitle
        Layout.preferredHeight: root.showTitle ? implicitHeight : 0
    }

    ColumnLayout {
        id: contentLayout
        Layout.fillWidth: true
        spacing: 0
    }
}