import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import MorphMaster

Item {
    id: root

    /*
     * Renders an SVG as a pure single-color icon.  The SVG alpha channel
     * is used as a mask, so the original fill/stroke color is irrelevant.
     */
    component TintedSvg: Item {
        id: tintedSvg

        property url source
        property color tintColor: "white"

        Rectangle {
            id: tintSource

            anchors.fill: parent
            color: tintedSvg.tintColor
            visible: false
        }

        Image {
            id: svgMask

            anchors.fill: parent
            source: tintedSvg.source
            fillMode: Image.PreserveAspectFit
            smooth: true
            visible: false
        }

        MultiEffect {
            anchors.fill: parent
            source: tintSource
            maskEnabled: true
            maskSource: svgMask
        }
    }


    /*
     * Gives immediate visual feedback when a potentially heavy page is
     * selected.  The lightweight shell is shown first; the real content is
     * revealed on the following frame and is instantiated asynchronously.
     */
    component ResponsivePageHost: Item {
        id: pageHost

        property bool selected: false
        property bool keepLoaded: true
        property string pageTitle: ""
        property Component pageComponent
        property int revealDelayMs: 24
        property int activationDelayMs: 0
        property bool asynchronousLoading: true

        property bool loadedOnce: false
        property bool revealContent: false
        property bool activationRequested: false

        onSelectedChanged: {
            if (selected) {
                revealContent = false

                if (loadedOnce && keepLoaded) {
                    activationRequested = true
                } else if (activationDelayMs > 0) {
                    activationRequested = false
                    activationTimer.restart()
                } else {
                    activationRequested = true
                }

                revealTimer.restart()
            } else {
                activationTimer.stop()
                revealTimer.stop()
                revealContent = false

                if (!keepLoaded)
                    activationRequested = false
            }
        }

        Timer {
            id: activationTimer
            interval: pageHost.activationDelayMs
            repeat: false
            onTriggered: pageHost.activationRequested = true
        }

        Timer {
            id: revealTimer
            interval: pageHost.revealDelayMs
            repeat: false
            onTriggered: pageHost.revealContent = true
        }

        Loader {
            id: pageLoader

            anchors.fill: parent
            active: pageHost.activationRequested
                    || (pageHost.keepLoaded && pageHost.loadedOnce)
            asynchronous: pageHost.asynchronousLoading
            sourceComponent: pageHost.pageComponent
            visible: pageHost.selected
                     && pageHost.revealContent
                     && status === Loader.Ready

            onLoaded: pageHost.loadedOnce = true
        }

        Rectangle {
            anchors.fill: parent
            visible: pageHost.selected
                     && (!pageHost.revealContent
                         || pageLoader.status !== Loader.Ready)
            color: Theme.background
            z: 2

            Text {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: 16

                text: pageHost.pageTitle
                color: Theme.secondaryText
                font.pixelSize: 15
                font.bold: true
            }

            BusyIndicator {
                anchors.centerIn: parent
                visible: pageLoader.status === Loader.Loading
                running: visible
            }

            Text {
                anchors.centerIn: parent
                visible: pageLoader.status === Loader.Error
                text: qsTr("Unable to load this page")
                color: Theme.text
                font.pixelSize: 14
            }
        }
    }

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
        Math.min(56, height / Math.max(1, totalSectionUnits))

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

        Rectangle {
            id: navigationRail

            Layout.preferredWidth: root.railWidth
            Layout.minimumWidth: root.railWidth
            Layout.maximumWidth: root.railWidth
            Layout.fillHeight: true

            color: Qt.darker(Theme.background, 1.15)

            Rectangle {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.bottom: parent.bottom

                width: 1
                color: "#505050"
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Repeater {
                    model: root.sections

                    delegate: ToolButton {
                        id: sectionButton

                        required property int index
                        required property var modelData

                        readonly property bool selected:
                            root.currentSection
                            === sectionButton.modelData.section

                        Layout.fillWidth: true
                        Layout.preferredHeight:
                            root.sectionButtonHeight
                            * sectionButton.modelData.heightUnits
                        Layout.minimumHeight:
                            root.sectionButtonHeight
                            * sectionButton.modelData.heightUnits
                        Layout.maximumHeight:
                            root.sectionButtonHeight
                            * sectionButton.modelData.heightUnits

                        hoverEnabled: true

                        onPressed: {
                            root.currentSection =
                                sectionButton.modelData.section
                        }

                        onPressAndHold: {
                            if (!DebugBuild
                                    || sectionButton.modelData.section !== "about")
                                return

                            const appWindow = ApplicationWindow.window
                            if (appWindow
                                    && appWindow.viewportSimulationEnabled
                                    && appWindow.cycleViewportProfile) {
                                appWindow.cycleViewportProfile()
                            }
                        }

                        ToolTip.visible: hovered
                        ToolTip.text: modelData.name
                        ToolTip.delay: 400

                        background: Rectangle {
                            color: sectionButton.selected
                                   ? "#3A3A3A"
                                   : "transparent"

                            Rectangle {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom

                                width: 3
                                color: sectionButton.selected
                                       ? "#D8B85A"
                                       : "transparent"
                            }
                        }

                        contentItem: Item {
                            id: iconHost

                            readonly property color iconColor:
                                sectionButton.selected
                                    ? "#D8B85A"
                                    : "#E0E0E0"

                            readonly property string iconKind:
                                sectionButton.modelData.section

                            TintedSvg {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.verticalCenterOffset:
                                    iconHost.iconKind === "configuration"
                                        ? parent.height / 4
                                        : 0

                                width: Math.round(25 * root.sectionIconScale)
                                height: width

                                visible:
                                    iconHost.iconKind === "settings"
                                    || iconHost.iconKind === "configuration"

                                source: "qrc:/svg/gear.svg"
                                tintColor: iconHost.iconColor
                            }

                            Text {
                                anchors.centerIn: parent

                                visible: iconHost.iconKind === "about"

                                text: "?"
                                color: iconHost.iconColor
                                font.pixelSize: Math.round(
                                                    23
                                                    * root.sectionIconScale)
                                font.bold: true
                            }

                            Canvas {
                                id: iconCanvas

                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.verticalCenterOffset:
                                    iconHost.iconKind === "configuration"
                                        ? -parent.height / 4
                                        : 0

                                width: 30
                                height: 30
                                scale: root.sectionIconScale

                                visible:
                                    iconHost.iconKind === "midi"
                                    || iconHost.iconKind === "assignment"
                                    || iconHost.iconKind === "monitor"
                                    || iconHost.iconKind === "configuration"
                                    || iconHost.iconKind === "tracks"
                                    || iconHost.iconKind === "curves"
                                    || iconHost.iconKind === "manual"

                                property color iconColor: iconHost.iconColor
                                property string iconKind: iconHost.iconKind

                                onIconColorChanged: requestPaint()
                                onIconKindChanged: requestPaint()

                                onPaint: {
                                    const ctx = getContext("2d")

                                    ctx.clearRect(0, 0, width, height)
                                    ctx.strokeStyle = iconColor
                                    ctx.fillStyle = iconColor
                                    ctx.lineWidth = 2
                                    ctx.lineCap = "round"
                                    ctx.lineJoin = "round"

                                    if (iconKind === "midi"
                                            || iconKind === "configuration") {
                                        ctx.beginPath()
                                        ctx.arc(15, 15, 11,
                                                0, Math.PI * 2)
                                        ctx.stroke()

                                        const pins = [
                                            [10, 11],
                                            [15, 9],
                                            [20, 11],
                                            [11.5, 17],
                                            [18.5, 17]
                                        ]

                                        for (let i = 0;
                                             i < pins.length;
                                             ++i) {
                                            ctx.beginPath()
                                            ctx.arc(pins[i][0],
                                                    pins[i][1],
                                                    1.5,
                                                    0,
                                                    Math.PI * 2)
                                            ctx.fill()
                                        }
                                    } else if (iconKind === "assignment") {
                                        const boxes = [
                                            [5, 5], [17, 5],
                                            [5, 17], [17, 17]
                                        ]

                                        for (let i = 0; i < boxes.length; ++i)
                                            ctx.strokeRect(boxes[i][0], boxes[i][1], 8, 8)
                                    } else if (iconKind === "monitor") {
                                        ctx.beginPath()
                                        ctx.moveTo(4, 22)
                                        ctx.bezierCurveTo(9, 22,
                                                          11, 9,
                                                          17, 9)
                                        ctx.bezierCurveTo(21, 9,
                                                          23, 17,
                                                          26, 17)
                                        ctx.stroke()

                                        ctx.beginPath()
                                        ctx.arc(18, 11, 2.5,
                                                0, Math.PI * 2)
                                        ctx.fill()
                                    } else if (iconKind === "tracks") {
                                        const sliderX = [7, 15, 23]
                                        const sliderY = [11, 20, 15]

                                        for (let i = 0;
                                             i < sliderX.length;
                                             ++i) {
                                            ctx.beginPath()
                                            ctx.moveTo(sliderX[i], 5)
                                            ctx.lineTo(sliderX[i], 25)
                                            ctx.stroke()

                                            ctx.fillRect(sliderX[i] - 2.5,
                                                         sliderY[i] - 2,
                                                         5,
                                                         4)
                                        }
                                    } else if (iconKind === "curves") {
                                        ctx.beginPath()
                                        ctx.moveTo(5, 8)
                                        ctx.bezierCurveTo(12, 8,
                                                          17, 22,
                                                          25, 22)
                                        ctx.stroke()

                                        ctx.beginPath()
                                        ctx.moveTo(5, 22)
                                        ctx.bezierCurveTo(12, 22,
                                                          17, 8,
                                                          25, 8)
                                        ctx.stroke()
                                    } else if (iconKind === "manual") {
                                        ctx.beginPath()
                                        ctx.moveTo(4, 7)
                                        ctx.lineTo(13.5, 9)
                                        ctx.lineTo(13.5, 24)
                                        ctx.lineTo(4, 22)
                                        ctx.closePath()
                                        ctx.stroke()

                                        ctx.beginPath()
                                        ctx.moveTo(26, 7)
                                        ctx.lineTo(16.5, 9)
                                        ctx.lineTo(16.5, 24)
                                        ctx.lineTo(26, 22)
                                        ctx.closePath()
                                        ctx.stroke()

                                        ctx.beginPath()
                                        ctx.moveTo(15, 9)
                                        ctx.lineTo(15, 24)
                                        ctx.stroke()
                                    }
                                }
                            }

                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
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

                ResponsivePageHost {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    selected: root.currentSection === "monitor"
                    keepLoaded: false
                    pageTitle: qsTr("Morph Monitor")
                    pageComponent: monitorComponent
                }

                ResponsivePageHost {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    selected: root.currentSection === "midi"
                    pageTitle: qsTr("MIDI Settings")
                    pageComponent: midiComponent
                }

                ResponsivePageHost {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    selected: root.currentSection === "settings"
                    pageTitle: qsTr("Settings")
                    pageComponent: settingsComponent
                }

                ResponsivePageHost {
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

                ResponsivePageHost {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    selected: root.currentSection === "curves"
                    pageTitle: qsTr("Morph Curves")
                    pageComponent: curvesComponent
                }

                ResponsivePageHost {
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
