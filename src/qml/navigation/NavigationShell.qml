import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster
import NaadaLab.Ui as SharedUi

Item {
    id: root
    property bool licenseVisible: false
    property string licenseState: ""
    property bool licensePending: false
    signal licenseRequested()
    readonly property real licenseButtonHeight: licenseVisible ? (phoneLayout ? 44 : 56) : 0
    readonly property int licenseIconState: !licensePending && (licenseState === "purchased" || licenseState === "review") ? 0
        : !licensePending && licenseState === "trial" ? 1 : 2
    readonly property string licenseLabel: licensePending ? qsTr("Payment pending")
        : licenseState === "review" ? qsTr("Review access")
        : licenseIconState === 0 ? qsTr("Full version")
        : licenseIconState === 1 ? qsTr("Trial active") : qsTr("Review license access")

    Component.onCompleted: {
        console.log("[MorphMaster] NavigationShell loaded: assignment, 4-track grid, global morph curves")
    }

    readonly property int layoutClass:
        ApplicationWindow.window
            ? ApplicationWindow.window.layoutClass
            : UiMetrics.Desktop

    readonly property bool phoneLayout:
        layoutClass === UiMetrics.Phone

    readonly property int railWidth: 56
    readonly property int headerHeight: phoneLayout ? 48 : 56
    readonly property int trackCount: 16
    readonly property int tracksPerPage: phoneLayout ? 1 : 4

    property string currentSection: "assignment"
    property string lastPhoneConfigurationSection: "midi"

    /* Index of the first track shown, zero based. */
    property int firstTrackIndex: 0

    /* Used by phone to remember the last displayed global curve. */
    property int curvesPageIndex: 0

    readonly property var curvesPageTitles: [
        "Low/High Morph Curve",
        "Soft/Loud Morph Curve"
    ]

    readonly property var sections:
        phoneLayout
            ? [
                  { section: "assignment", name: "Morph Output Assignment", heightUnits: 1 },
                  { section: "midi", name: "MIDI Settings", heightUnits: 1 },
                  { section: "settings", name: "Settings", heightUnits: 1 },
                  { section: "tracks", name: "Tracks", heightUnits: 1 },
                  { section: "curves", name: "Morph Curves", heightUnits: 1 },
                  { section: "monitor", name: "Morph Monitor", heightUnits: 1 },
                  { section: "manual", name: "Manual", heightUnits: 1 },
                  { section: "about", name: "About", heightUnits: 1 }
              ]
            : [
                  { section: "assignment", name: "Morph Output Assignment", heightUnits: 1 },
                  {
                      section: "configuration",
                      name: "MIDI and Settings",
                      heightUnits: 2
                  },
                  { section: "tracks", name: "Tracks", heightUnits: 1 },
                  { section: "curves", name: "Morph Curves", heightUnits: 1 },
                  { section: "monitor", name: "Morph Monitor", heightUnits: 1 },
                  { section: "manual", name: "Manual", heightUnits: 1 },
                  { section: "about", name: "About", heightUnits: 1 }
              ]

    readonly property int totalSectionUnits: 8

    readonly property real sectionButtonHeight:
        Math.min(56, (height - licenseButtonHeight) / Math.max(1, totalSectionUnits))

    readonly property real sectionIconScale:
        Math.min(1, sectionButtonHeight / 56)

    readonly property bool pagedSection:
        currentSection === "tracks"
        || (phoneLayout && currentSection === "curves")

    readonly property int trackPageIndex:
        Math.floor(firstTrackIndex / tracksPerPage)

    readonly property int trackPageCount:
        Math.ceil(trackCount / tracksPerPage)

    readonly property int currentPageIndex:
        currentSection === "tracks"
            ? trackPageIndex
            : curvesPageIndex

    readonly property int currentPageCount:
        currentSection === "tracks"
            ? trackPageCount
            : curvesPageTitles.length

    readonly property string currentPageTitle: {
        if (currentSection === "tracks") {
            const first = firstTrackIndex + 1
            const last = Math.min(trackCount,
                                  firstTrackIndex + tracksPerPage)

            return tracksPerPage === 1
                    ? "Track " + first
                    : "Tracks " + first + "–" + last
        }

        if (phoneLayout && currentSection === "curves")
            return curvesPageTitles[curvesPageIndex]

        return ""
    }

    readonly property bool previousPageEnabled:
        currentSection === "tracks"
            ? firstTrackIndex > 0
            : curvesPageIndex > 0

    readonly property bool nextPageEnabled:
        currentSection === "tracks"
            ? firstTrackIndex + tracksPerPage < trackCount
            : curvesPageIndex < curvesPageTitles.length - 1

    function sectionStackIndex(section) {
        switch (section) {
        case "assignment":
            return 0
        case "monitor":
            return 1
        case "midi":
            return 2
        case "settings":
            return 3
        case "configuration":
            return 4
        case "tracks":
            return 5
        case "curves":
            return 6
        case "manual":
            return 7
        case "about":
            return 8
        default:
            return 0
        }
    }

    function previousPage() {
        if (currentSection === "tracks") {
            firstTrackIndex = Math.max(0,
                                       firstTrackIndex - tracksPerPage)
        } else if (phoneLayout && currentSection === "curves") {
            curvesPageIndex = Math.max(0, curvesPageIndex - 1)
        }
    }

    function nextPage() {
        if (currentSection === "tracks") {
            firstTrackIndex = Math.min(trackCount - tracksPerPage,
                                       firstTrackIndex + tracksPerPage)
        } else if (phoneLayout && currentSection === "curves") {
            curvesPageIndex = Math.min(curvesPageTitles.length - 1,
                                       curvesPageIndex + 1)
        }
    }

    function openCurveEditorForMorphOutput(morphOutputId) {
        const outputId = Math.round(morphOutputId)

        // Side outputs use only one of the two global curves.  This matters
        // on phone, where one curve is shown at a time.  Corner outputs use
        // both axes, so the last selected curve is preserved.
        if (outputId === 2 || outputId === 6)
            curvesPageIndex = 0
        else if (outputId === 0 || outputId === 4)
            curvesPageIndex = 1

        currentSection = "curves"
    }

    function openTrackEditor(trackNumber) {
        const normalizedTrack = Math.max(
                    1,
                    Math.min(trackCount, Math.round(trackNumber)))
        const trackIndex = normalizedTrack - 1

        firstTrackIndex = Math.floor(trackIndex / tracksPerPage)
                          * tracksPerPage
        currentSection = "tracks"
    }

    function normalizeFirstTrackIndex() {
        const maximum = Math.max(0, trackCount - tracksPerPage)
        const clamped = Math.max(0, Math.min(maximum, firstTrackIndex))
        const aligned = Math.floor(clamped / tracksPerPage)
                        * tracksPerPage

        if (firstTrackIndex !== aligned)
            firstTrackIndex = aligned
    }

    onFirstTrackIndexChanged: normalizeFirstTrackIndex()
    onTracksPerPageChanged: normalizeFirstTrackIndex()

    onCurvesPageIndexChanged: {
        const normalized = Math.max(
                    0,
                    Math.min(curvesPageTitles.length - 1,
                             curvesPageIndex))

        if (curvesPageIndex !== normalized)
            curvesPageIndex = normalized
    }

    onPhoneLayoutChanged: {
        if (phoneLayout) {
            if (currentSection === "configuration")
                currentSection = lastPhoneConfigurationSection
        } else if (currentSection === "midi"
                   || currentSection === "settings") {
            lastPhoneConfigurationSection = currentSection
            currentSection = "configuration"
        }
    }

    Component {
        id: midiComponent

        ScrollView {
            id: midiScroll

            clip: true
            contentWidth: availableWidth

            Item {
                width: midiScroll.availableWidth
                height: midiConfiguration.implicitHeight + 24

                MidiConfiguration {
                    id: midiConfiguration

                    x: 12
                    y: 12
                    width: Math.max(0, parent.width - 24)
                }
            }
        }
    }

    Component {
        id: settingsComponent

        ScrollView {
            id: settingsScroll

            clip: true
            contentWidth: availableWidth

            Item {
                width: settingsScroll.availableWidth
                height: settingsColumn.implicitHeight + 24

                ColumnLayout {
                    id: settingsColumn

                    x: 12
                    y: 12
                    width: Math.max(0, parent.width - 24)

                    spacing: UiMetrics.spacing(root.layoutClass)

                    PerformanceSettings {
                        Layout.fillWidth: true
                    }

                    PresetSelector {
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }

    Component {
        id: monitorComponent

        MorphMonitorPage {
            onCurveEditRequested: function(morphOutputId) {
                root.openCurveEditorForMorphOutput(morphOutputId)
            }
        }
    }

    Component {
        id: combinedConfigurationComponent

        Item {
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: Theme.pageSpacing

                Item {
                    id: configurationControls

                    Layout.fillWidth: true
                    Layout.preferredHeight:
                        Math.max(combinedMidi.implicitHeight,
                                 combinedSettingsColumn.implicitHeight)
                    Layout.minimumHeight: Layout.preferredHeight

                    RowLayout {
                        anchors.fill: parent
                        spacing: Theme.pageSpacing

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            MidiConfiguration {
                                id: combinedMidi
                                anchors.fill: parent
                            }
                        }


                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            ColumnLayout {
                                id: combinedSettingsColumn

                                anchors.fill: parent
                                spacing: UiMetrics.spacing(root.layoutClass)

                                PerformanceSettings {
                                    Layout.fillWidth: true
                                }

                                PresetSelector {
                                    Layout.fillWidth: true
                                }

                                Item {
                                    Layout.fillHeight: true
                                }
                            }
                        }
                    }
                }

                ConfigurationOverview {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 230
                }
            }
        }
    }

    Component {
        id: curvesComponent

        CurvesSettingsPage {
            singleCurveMode: root.phoneLayout
            currentCurveIndex: root.curvesPageIndex

            onCurveActivated: function(curveIndex) {
                root.curvesPageIndex = curveIndex
            }
        }
    }

    Component {
        id: manualComponent

        UserManualPage { }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        SharedUi.NavigationRail {
            id: navigationRail

            Layout.preferredWidth: root.railWidth
            Layout.minimumWidth: root.railWidth
            Layout.maximumWidth: root.railWidth
            Layout.fillHeight: true

            sections: root.sections
            currentSection: root.currentSection
            buttonHeight: root.sectionButtonHeight
            iconScale: root.sectionIconScale

            ToolButton {
                id: licenseButton
                anchors.bottom: parent.bottom
                width: parent.width
                height: root.licenseButtonHeight
                visible: root.licenseVisible
                hoverEnabled: true
                Accessible.name: root.licenseLabel
                ToolTip.visible: hovered || pressed
                ToolTip.text: root.licenseLabel
                ToolTip.delay: 400
                onClicked: root.licenseRequested()
                background: Rectangle {
                    color: licenseButton.down ? SharedUi.Theme.selection : "transparent"
                    Rectangle { width: parent.width; height: 1; color: SharedUi.Theme.border }
                }
                contentItem: Item {
                    Canvas {
                        id: licenseIcon
                        anchors.centerIn: parent
                        width: 28; height: 28
                        property int status: root.licenseIconState
                        onStatusChanged: requestPaint()
                        onPaint: {
                            const ctx = getContext("2d")
                            ctx.reset()
                            ctx.strokeStyle = status === 0 ? "#91d7ae" : status === 1 ? "#f1ce6d" : "#ffb574"
                            ctx.lineWidth = 2
                            ctx.lineCap = "round"
                            ctx.lineJoin = "round"
                            ctx.beginPath()
                            ctx.arc(14, 14, 11, 0, 2 * Math.PI)
                            ctx.stroke()
                            ctx.beginPath()
                            if (status === 0) {
                                ctx.moveTo(8, 14); ctx.lineTo(12, 18); ctx.lineTo(20, 10)
                            } else if (status === 1) {
                                ctx.moveTo(14, 7); ctx.lineTo(14, 14); ctx.lineTo(19, 17)
                            } else {
                                ctx.moveTo(14, 7); ctx.lineTo(14, 15)
                                ctx.moveTo(14, 20); ctx.lineTo(14, 20.5)
                            }
                            ctx.stroke()
                        }
                    }
                }
            }

            onSectionActivated: function(section) {
                root.currentSection = section
            }

            onSectionPressAndHold: function(section) {
                if (!DebugBuild || section !== "about")
                    return

                const appWindow = ApplicationWindow.window
                if (appWindow
                        && appWindow.viewportSimulationEnabled
                        && appWindow.cycleViewportProfile) {
                    appWindow.cycleViewportProfile()
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                id: pageHeader

                Layout.fillWidth: true
                Layout.preferredHeight:
                    root.pagedSection ? root.headerHeight : 0
                Layout.minimumHeight:
                    root.pagedSection ? root.headerHeight : 0
                Layout.maximumHeight:
                    root.pagedSection ? root.headerHeight : 0

                visible: root.pagedSection
                color: Qt.darker(Theme.background, 1.12)

                ToolButton {
                    id: previousButton

                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom

                    width: root.phoneLayout ? 56 : 68
                    text: "\u2039"
                    font.pixelSize: root.phoneLayout ? 32 : 38
                    font.bold: true

                    enabled: root.previousPageEnabled
                    onClicked: root.previousPage()
                }

                Text {
                    anchors.left: previousButton.right
                    anchors.right: nextButton.left
                    anchors.verticalCenter: parent.verticalCenter

                    text: root.currentPageTitle
                    color: "white"
                    font.pixelSize: 17
                    font.bold: true

                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                ToolButton {
                    id: nextButton

                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom

                    width: root.phoneLayout ? 56 : 68
                    text: "\u203A"
                    font.pixelSize: root.phoneLayout ? 32 : 38
                    font.bold: true

                    enabled: root.nextPageEnabled
                    onClicked: root.nextPage()
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom

                    height: 1
                    color: "#505050"
                }
            }

            StackLayout {
                id: sectionStack

                Layout.fillWidth: true
                Layout.fillHeight: true

                currentIndex: root.sectionStackIndex(
                                  root.currentSection)

                MorphOutputAssignmentPage {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    onTrackEditRequested: function(trackNumber) {
                        root.openTrackEditor(trackNumber)
                    }

                    onCurveEditRequested: function(morphOutputId) {
                        root.openCurveEditorForMorphOutput(morphOutputId)
                    }
                }

                SharedUi.ResponsivePageHost {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    selected: root.currentSection === "monitor"
                    keepLoaded: false
                    pageTitle: qsTr("Morph Monitor")
                    pageComponent: monitorComponent
                }

                SharedUi.ResponsivePageHost {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    selected: root.currentSection === "midi"
                    pageTitle: qsTr("MIDI Settings")
                    pageComponent: midiComponent
                }

                SharedUi.ResponsivePageHost {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    selected: root.currentSection === "settings"
                    pageTitle: qsTr("Settings")
                    pageComponent: settingsComponent
                }

                SharedUi.ResponsivePageHost {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    selected: root.currentSection === "configuration"
                    keepLoaded: false
                    pageTitle: qsTr("MIDI and Settings")
                    pageComponent: combinedConfigurationComponent
                    activationDelayMs: 24
                    asynchronousLoading: false
                }

                TracksSettingsPage {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    firstTrack: root.firstTrackIndex + 1
                    visibleTrackCount: root.tracksPerPage

                    onMorphOutputEditRequested: function(morphOutputIndex) {
                        root.currentSection = "assignment"
                    }
                }

                SharedUi.ResponsivePageHost {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    selected: root.currentSection === "curves"
                    pageTitle: qsTr("Morph Curves")
                    pageComponent: curvesComponent
                }

                SharedUi.ResponsivePageHost {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    selected: root.currentSection === "manual"
                    pageTitle: qsTr("Manual")
                    pageComponent: manualComponent
                }

                Item {
                    AboutPage {
                        anchors.fill: parent

                        onOpenManualRequested: {
                            root.currentSection = "manual"
                        }
                    }
                }
            }
        }
    }
}
