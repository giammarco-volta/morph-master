import QtQuick
import QtQuick.Controls

Popup {
    id: root

    parent: Overlay.overlay
    x: 0
    y: 0
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    padding: 0
    margins: 0
    z: 20000
    modal: true
    dim: false
    focus: true
    closePolicy: Popup.CloseOnEscape
    // The first frame must fully cover the underlying page.
    enter: null
    exit: null

    property bool presentationMode: false
    // Exposed so the component can also be previewed outside the application.
    property url imageSource: "qrc:/MorphoraSplash.png"

    function showPresentation() {
        autoClose.stop()
        presentationMode = true
        open()
    }

    function startAutoCloseWhenReady() {
        if (opened && !presentationMode
                && (artwork.status === Image.Ready || artwork.status === Image.Error))
            autoClose.restart()
    }

    onOpened: startAutoCloseWhenReady()
    onClosed: {
        autoClose.stop()
        presentationMode = false
    }

    background: Rectangle { color: "#08142b" }

    contentItem: Item {
        Image {
            id: artwork
            anchors.fill: parent
            source: root.imageSource
            fillMode: Image.PreserveAspectFit
            asynchronous: false
            smooth: true
            onStatusChanged: root.startAutoCloseWhenReady()
        }

        Label {
            anchors.centerIn: parent
            visible: artwork.status === Image.Error
            text: "Morphora"
            color: "#edc35a"
            font.pixelSize: Math.min(64, root.width / 8)
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: Math.max(8, root.height * 0.018)
            visible: root.presentationMode
            text: qsTr("Tap or click to begin")
            color: "#b9cae8"
            font.pixelSize: Math.max(12, Math.min(18, root.height * 0.022))
        }

        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
            onWheel: function(wheel) { wheel.accepted = true }
        }
    }

    Timer {
        id: autoClose
        interval: 2000
        repeat: false
        onTriggered: root.close()
    }
}
