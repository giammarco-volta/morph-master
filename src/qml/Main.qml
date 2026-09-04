// src/qml/Main.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

import MorphMaster
import NaadaLab.Ui as SharedUi

ApplicationWindow {
    id: root
	color: SharedUi.Theme.background
    visible: true
    title: "Morphora"
	font.family: "Arial"

    Material.theme: Material.Dark
    Material.accent: Material.Amber

    readonly property int layoutClass:
        SharedUi.UiMetrics.layoutClassForHeight(contentItem.height)

    readonly property string layoutClassName: SharedUi.UiMetrics.layoutClassName(layoutClass)

    readonly property real viewportWidth: contentItem.width
    readonly property real viewportHeight: contentItem.height

	
    enum ViewportProfile {
        Desktop,
        PhoneLandscape,
        TabletLandscape
    }

    property int viewportProfile: Main.TabletLandscape

    function cycleViewportProfile() {
        if (!viewportSimulationEnabled)
            return

        viewportProfile = (viewportProfile + 1) % 3
    }

    function applyViewportProfile() {
        // Prima rimuove gli eventuali vincoli del profilo precedente.
        minimumWidth = 0
        minimumHeight = 0
        maximumWidth = 16777215
        maximumHeight = 16777215

        switch (viewportProfile) {
        case Main.PhoneLandscape:
            width = 800
            height = 360

            minimumWidth = 800
            maximumWidth = 800
            minimumHeight = 360
            maximumHeight = 360
            break

        case Main.TabletLandscape:
            width = 1280
            height = 720

            minimumWidth = 1280
            maximumWidth = 1280
            minimumHeight = 720
            maximumHeight = 720
            break

        case Main.Desktop:
        default:
            minimumWidth = 640
            minimumHeight = 360

            width = 1400
            height = 900
            break
        }
    }

    readonly property bool viewportSimulationEnabled:
        Qt.platform.os === "windows"

    Component.onCompleted: {
        if (viewportSimulationEnabled)
            applyViewportProfile()
    }

    onViewportProfileChanged: {
        if (viewportSimulationEnabled)
            applyViewportProfile()
    }


    NavigationShell {
        anchors.fill: parent
    }

    SharedUi.ModalPanel {
        id: modalPanel
    }

    function openModalPanel(component, properties) {
        modalPanel.show(component, properties)
    }

    function closeModalPanel() {
        modalPanel.close()
    }

    Rectangle {
        id: noTracksBanner

        visible: false
        z: 10000

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 48

        width: Math.min(parent.width - 32, 620)
        height: 54
        radius: 8

        color: "#E0303030"
        border.color: "#90FFFFFF"
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: "MIDI input received, but no tracks are assigned to a Morph Output. Open Morph Output Assignment or Tracks, and assign at least one track."
            color: "white"
            font.pixelSize: 14
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            width: parent.width - 32
        }

        Timer {
            id: noTracksBannerTimer
            interval: 5500
            repeat: false

            onTriggered: {
                noTracksBanner.visible = false
            }
        }
    }

    Connections {
        target: SettingsController

        function onMidiInputReceivedWithNoAssignedTracks() {
            noTracksBanner.visible = true
            noTracksBannerTimer.restart()
        }
    }


    onLayoutClassChanged: {
        console.log(
            "Layout class:",
            layoutClassName,
            "viewport:",
            viewportWidth,
            "x",
            viewportHeight
        )
    }
}