import QtQuick
import QtQuick.Controls

import MorphMaster

Item {
    id: root
    clip: true

    property var manualBlocks: SettingsController.userManualBlocks
    property int expectedImageCount: countImages(manualBlocks)
    property int loadedImageCount: 0
    property bool initialDelayDone: false
    property bool forceShowManual: false

    property bool minimumDelayDone: false
    property bool layoutSettled: false

    readonly property bool manualReady:
        forceShowManual || (minimumDelayDone && layoutSettled)

    function scheduleLayoutSettling() {
        layoutSettled = false
        layoutSettleTimer.restart()
    }

    Component.onCompleted: {
        minimumDelayTimer.start()
        maxLoadingTimer.start()
        layoutSettleTimer.start()
    }

    function countImages(blocks) {
        let count = 0

        for (let i = 0; i < blocks.length; ++i) {
            if (blocks[i].type === "image")
                ++count
        }

        return count
    }

    onManualReadyChanged: {
        if (manualReady && scrollView.contentItem)
            scrollView.contentItem.contentY = 0
    }

    Timer {
        id: minimumDelayTimer
        interval: 300
        repeat: false

        onTriggered: {
            root.minimumDelayDone = true
        }
    }

    Timer {
        id: layoutSettleTimer
        interval: 700
        repeat: false

        onTriggered: {
            root.layoutSettled = true
        }
    }

    Timer {
        id: maxLoadingTimer
        interval: 6000
        repeat: false

        onTriggered: {
            root.forceShowManual = true
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#202020"

        visible: !root.manualReady
        z: 1000

        Column {
            anchors.centerIn: parent
            spacing: 12

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                running: !root.manualReady
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Loading..."
                color: "#E8E8E8"
                font.pixelSize: 16
            }
        }
    }

    ScrollView {
        id: scrollView

        anchors.fill: parent
        anchors.margins: 16
        clip: true

        Column {
            id: contentColumn

            visible: true
            opacity: root.manualReady ? 1.0 : 0.0

            width: Math.max(1, scrollView.availableWidth)
            spacing: 14

            onImplicitHeightChanged: {
                root.scheduleLayoutSettling()
            }

            Repeater {
                model: root.manualBlocks

                delegate: Item {
                    id: blockRoot

                    width: contentColumn.width

                    height: modelData.type === "image"
                            ? imageBlock.height
                            : manualText.implicitHeight

                    Text {
                        id: manualText

                        visible: modelData.type === "text"

                        width: blockRoot.width

                        text: visible ? modelData.html : ""
                        textFormat: Text.RichText
                        wrapMode: Text.WordWrap

                        color: "#E8E8E8"
                        linkColor: "#D8B85A"

                        font.pixelSize: 15

                        onLinkActivated: function(link) {
                            Qt.openUrlExternally(link)
                        }
                    }

                    Item {
                        id: imageBlock

                        visible: modelData.type === "image"

                        width: blockRoot.width

                        property real naturalWidth: modelData.naturalWidth !== undefined &&
                                                    modelData.naturalWidth > 0
                                                    ? modelData.naturalWidth
                                                    : 1

                        property real naturalHeight: modelData.naturalHeight !== undefined &&
                                                     modelData.naturalHeight > 0
                                                     ? modelData.naturalHeight
                                                     : 1

                        property real displayWidth: Math.min(blockRoot.width, naturalWidth)

                        height: visible
                                ? manualImage.height
                                  + (captionText.visible ? 4 + captionText.implicitHeight : 0)
                                : 1

                        Image {
                            id: manualImage

                            visible: modelData.type === "image"

                            anchors.top: parent.top
                            anchors.horizontalCenter: parent.horizontalCenter

                            width: imageBlock.displayWidth
                            height: Math.round(width * imageBlock.naturalHeight / imageBlock.naturalWidth)

                            source: modelData.type === "image" ? modelData.source : ""
                            asynchronous: true

                            sourceSize.width: Math.ceil(width)
                            sourceSize.height: Math.ceil(height)

                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            cache: true
                        }

                        Text {
                            id: captionText

                            visible: modelData.captionHtml !== undefined &&
                                     modelData.captionHtml.length > 0

                            anchors.top: manualImage.bottom
                            anchors.topMargin: 4
                            anchors.horizontalCenter: manualImage.horizontalCenter

                            width: manualImage.width

                            text: visible ? modelData.captionHtml : ""
                            textFormat: Text.RichText
                            wrapMode: Text.WordWrap

                            color: "#B8B8B8"
                            font.pixelSize: 13
                            font.italic: true
                        }
                    }
                }
            }
        }
    }
}