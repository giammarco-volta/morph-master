import QtQuick
import QtQuick.Controls

import MorphMaster

Item {
    id: root
    clip: true

    signal openManualRequested()

    Rectangle {
        anchors.fill: parent
        color: "#202020"
    }

    Flickable {
        id: flick

        anchors.fill: parent
        anchors.margins: 16

        clip: true
        boundsBehavior: Flickable.StopAtBounds

        contentWidth: width
        contentHeight: aboutText.implicitHeight

        Text {
            id: aboutText

            width: flick.width

            text: SettingsController.aboutHtml
            textFormat: Text.RichText
            wrapMode: Text.WordWrap

            color: "#E8E8E8"
            linkColor: "#64B5F6"

            font.pixelSize: 15

            onLinkActivated: function(link) {
                if (link === "morphmaster:user-manual") {
                    root.openManualRequested()
                    return
                }

                Qt.openUrlExternally(link)
            }
        }

        ScrollBar.vertical: ScrollBar {}
    }
}