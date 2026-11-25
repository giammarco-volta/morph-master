// src/qml/Main.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

import MorphMaster
import NaadaLab.Ui as SharedUi

ApplicationWindow {
    id: root
    // The window clear colour is not part of contentItem.grabToImage().
    // Include an opaque background in the scene that is exported.
    Rectangle {
        anchors.fill: parent
        z: -1000
        color: SharedUi.Theme.background
    }
    SharedUi.LicensePanel {
        id: licensePanel
        anchors.fill: parent
        z: 10000
        license: root.licensePreviewState ? previewLicense : LicenseAccess
        appName: "Morphora"
        showBadge: false
    }
    // Desktop previews never call or modify the real licensing backend.
    property string licensePreviewState: ""
    function previewLicenseState(state) {
        if (!viewportSimulationEnabled)
            return
        licensePreviewState = state
        licensePanel.expanded = false
    }
    QtObject {
        id: previewLicense
        readonly property bool enabled: true
        readonly property bool allowed: root.licensePreviewState !== "expired"
        readonly property var status: ({
            state: root.licensePreviewState,
            configured: false, canBuy: false, busy: false,
            price: "€9.99",
            message: "Windows preview — purchase and restore actions are disabled.",
            trialEndsAt: root.licensePreviewState === "trial" ? Date.now() / 1000 + 3 * 86400 : 0
        })
        signal accessGranted()
        function activate() {}
        function restore() {}
        function buy() {}
        function unlockReview(code) {}
        function copySupportReference() { return false }
    }
	color: SharedUi.Theme.background
    // Prepare the splash before exposing the first application frame.
    visible: false
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
            width = 640
            height = 360

            minimumWidth = 640
            maximumWidth = 640
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

    Shortcut {
        sequence: "Ctrl+Alt+1"
        enabled: root.viewportSimulationEnabled
        onActivated: root.viewportProfile = Main.PhoneLandscape
    }
    Shortcut {
        sequence: "Ctrl+Alt+2"
        enabled: root.viewportSimulationEnabled
        onActivated: root.viewportProfile = Main.TabletLandscape
    }
    Shortcut {
        sequence: "Ctrl+Alt+4"
        enabled: root.viewportSimulationEnabled
        onActivated: root.previewLicenseState("purchased")
    }
    Shortcut {
        sequence: "Ctrl+Alt+5"
        enabled: root.viewportSimulationEnabled
        onActivated: root.previewLicenseState("trial")
    }
    Shortcut {
        sequence: "Ctrl+Alt+6"
        enabled: root.viewportSimulationEnabled
        onActivated: root.previewLicenseState("expired")
    }
    Shortcut {
        sequence: "Ctrl+Alt+0"
        enabled: root.viewportSimulationEnabled
        onActivated: root.previewLicenseState("")
    }
    Shortcut {
        sequence: "F8"
        enabled: root.viewportSimulationEnabled && !captureNotice.opened
        onActivated: StoreScreenshot.capture(root.contentItem,
            root.viewportProfile === Main.PhoneLandscape ? "phone"
            : root.viewportProfile === Main.TabletLandscape ? "tablet" : "desktop")
    }
    Connections {
        target: StoreScreenshot
        function onFinished(success, message) {
            captureNotice.title = success ? "Screenshot saved" : "Screenshot not saved"
            captureMessage.text = message
            captureNotice.open()
        }
    }
    Dialog {
        id: captureNotice
        anchors.centerIn: parent
        width: Math.min(560, root.width - 32)
        modal: true
        standardButtons: Dialog.Ok
        Label { id: captureMessage; width: parent.width; wrapMode: Text.WrapAnywhere }
    }

    Component.onCompleted: {
        if (viewportSimulationEnabled)
            applyViewportProfile()
        introduction.open()
        root.visible = true
    }

    SplashScreen {
        id: introduction
    }

    function showIntroduction() {
        introduction.showPresentation()
    }

    Shortcut {
        sequence: "Ctrl+Alt+I"
        enabled: !introduction.visible
        onActivated: root.showIntroduction()
    }

    onViewportProfileChanged: {
        if (viewportSimulationEnabled)
            applyViewportProfile()
    }


    NavigationShell {
        anchors.fill: parent
        licenseVisible: licensePanel.visible
        licenseState: licensePanel.info.state || ""
        licensePending: licensePanel.pending
        onLicenseRequested: licensePanel.expanded = true
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
