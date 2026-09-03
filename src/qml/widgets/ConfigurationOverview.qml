import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster

// Morph Outputs single-MIDI-label revision: 2026-07-17-v3

Rectangle {
    id: root

    function validPortName(name, placeholder) {
        return name !== undefined
                && name !== null
                && name.length > 0
                && name !== placeholder
    }

    function keyboardRangeName(rangeId) {
        const names = [
            "Full keyboard",
            "88 keys",
            "76 keys",
            "73 keys",
            "61 keys",
            "49 keys",
            "37 keys",
            "25 keys"
        ]

        return rangeId >= 0 && rangeId < names.length
                ? names[rangeId]
                : "Keyboard range"
    }

    function collectMorphOutputTracks(revision) {
        const outputs = [[], [], [], [], [], [], [], []]

        /* Keep the argument so the binding is refreshed by the watchers. */
        const currentRevision = revision

        for (let trackNumber = 1; trackNumber <= 16; ++trackNumber) {
            const track = SettingsController.track(trackNumber)

            if (track && track.morphOutput >= 0 && track.morphOutput < 8)
                outputs[track.morphOutput].push(trackNumber)
        }

        return outputs
    }

    function compactTrackList(tracks) {
        if (!tracks || tracks.length === 0)
            return ""

        const parts = []
        let rangeStart = tracks[0]
        let rangeEnd = tracks[0]

        function appendRange(start, end) {
            if (start === end)
                parts.push(String(start))
            else
                parts.push(String(start) + "–" + String(end))
        }

        for (let i = 1; i < tracks.length; ++i) {
            const trackNumber = tracks[i]

            if (trackNumber === rangeEnd + 1) {
                rangeEnd = trackNumber
            } else {
                appendRange(rangeStart, rangeEnd)
                rangeStart = trackNumber
                rangeEnd = trackNumber
            }
        }

        appendRange(rangeStart, rangeEnd)
        return parts.join(", ")
    }

    function hasAssignedMorphOutputs(outputs) {
        if (!outputs)
            return false

        for (let i = 0; i < outputs.length; ++i) {
            if (outputs[i] && outputs[i].length > 0)
                return true
        }

        return false
    }

    readonly property var morphOutputNames: [
        "Loud Morph Output",
        "High and Loud Morph Output",
        "High Morph Output",
        "High and Soft Morph Output",
        "Soft Morph Output",
        "Low and Soft Morph Output",
        "Low Morph Output",
        "Low and Loud Morph Output"
    ]

    readonly property bool inputConfigured:
        validPortName(SettingsController.midiInPort,
                      "Input device")

    readonly property bool outputConfigured:
        validPortName(SettingsController.midiOutPort,
                      "Output device")

    readonly property string inputPortText:
        inputConfigured
            ? SettingsController.midiInPort
            : "No MIDI input selected"

    readonly property string inputChannelText:
        inputConfigured
            ? "Channel " + (SettingsController.midiInChannel + 1)
            : "Select an input device above"

    readonly property string inputArrowChannelText:
        "ch: " + (SettingsController.midiInChannel + 1)

    readonly property string playModeText:
        SettingsController.playMode === 0 ? "Poly" : "Mono"

    readonly property string processingSummary:
        playModeText
        + " · "
        + keyboardRangeName(SettingsController.keyboardRangeId)

    readonly property string selectedInstrumentName:
        SettingsController.knownInstrumentName !== undefined
        && SettingsController.knownInstrumentName !== null
            ? SettingsController.knownInstrumentName.trim()
            : ""

    readonly property string programSelectionText:
        SettingsController.useInstrumentDefinition
            ? selectedInstrumentName.length > 0
                ? "Selected instrument: " + selectedInstrumentName
                : "Selected instrument: --"
            : "Manual Program Selection"

    readonly property string presetText:
        SettingsController.currentPresetName.length > 0
            ? "Preset: " + SettingsController.currentPresetName
            : "No preset selected"

    readonly property string outputPortText:
        outputConfigured
            ? SettingsController.midiOutPort
            : "No MIDI output selected"

    /* Explicitly refreshed whenever one of the 16 track Morph Outputs changes. */
    property int trackAssignmentRevision: 0

    readonly property var morphOutputTracks:
        collectMorphOutputTracks(trackAssignmentRevision)

    readonly property bool hasMorphOutputs:
        hasAssignedMorphOutputs(morphOutputTracks)

    color: Theme.panel
    border.color: Theme.border
    border.width: 1
    radius: 7

    /* Original overview height. */
    implicitHeight: 250

    /*
     * Track Morph Output changes happen inside TrackController objects rather
     * than on SettingsController itself. These lightweight delegates keep
     * the overview's Morph Output assignments live.
     */
    Repeater {
        model: 16

        delegate: Item {
            id: assignmentWatcher

            required property int index

            visible: false
            width: 0
            height: 0

            Connections {
                target: SettingsController.track(
                            assignmentWatcher.index + 1)
                ignoreUnknownSignals: true

                function onMorphOutputChanged() {
                    ++root.trackAssignmentRevision
                }
            }
        }
    }

    component FlowArrow: Item {
        id: arrowRoot

        property bool active: false
        property bool showArrow: true
        property string topText: "MIDI"
        property string bottomText: ""
        property string emptyText: ""

        readonly property color arrowColor:
            active ? Theme.accent : Theme.disabledText

        implicitWidth: 92
        implicitHeight: 82

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: arrowCanvas.top
            anchors.bottomMargin: 3

            visible: arrowRoot.showArrow && text.length > 0
            text: arrowRoot.topText
            color: arrowRoot.arrowColor
            font.pixelSize: 11
            font.bold: true
        }

        Canvas {
            id: arrowCanvas

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter

            visible: arrowRoot.showArrow
            height: 26

            property color arrowColor: arrowRoot.arrowColor

            onArrowColorChanged: requestPaint()
            onWidthChanged: requestPaint()

            onPaint: {
                const ctx = getContext("2d")
                const centerY = height / 2
                const endX = width - 5

                ctx.reset()
                ctx.strokeStyle = arrowColor
                ctx.fillStyle = arrowColor
                ctx.lineWidth = 2
                ctx.lineCap = "round"
                ctx.lineJoin = "round"

                ctx.beginPath()
                ctx.moveTo(4, centerY)
                ctx.lineTo(endX - 9, centerY)
                ctx.stroke()

                ctx.beginPath()
                ctx.moveTo(endX, centerY)
                ctx.lineTo(endX - 10, centerY - 7)
                ctx.lineTo(endX - 10, centerY + 7)
                ctx.closePath()
                ctx.fill()
            }
        }

        Label {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: arrowCanvas.bottom
            anchors.topMargin: 3

            visible: arrowRoot.showArrow && text.length > 0
            text: arrowRoot.bottomText
            color: arrowRoot.arrowColor
            font.pixelSize: 10
            font.bold: true

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignTop
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }

        Label {
            anchors.centerIn: parent
            width: parent.width

            visible: !arrowRoot.showArrow
            text: arrowRoot.emptyText
            color: Theme.disabledText
            font.pixelSize: 11
            font.bold: true

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
        }
    }

    /*
     * Eight fixed lanes occupy exactly the same horizontal slot previously
     * used by the single output FlowArrow. A lane draws an arrow only when
     * the corresponding Morph Output has at least one assigned track.
     */
    component MorphOutputArrowStack: Item {
        id: stackRoot

        property var trackLists: []
        property bool midiOutputConfigured: false

        implicitWidth: 112
        implicitHeight: 190

        Label {
            anchors.top: parent.top
            anchors.topMargin: Theme.sectionPadding + 2
            anchors.horizontalCenter: parent.horizontalCenter

            visible: root.hasMorphOutputs
            text: "MIDI"
            color: stackRoot.midiOutputConfigured
                   ? Theme.accent
                   : Theme.disabledText
            font.pixelSize: 11
            font.bold: true
        }

        Label {
            anchors.centerIn: parent
            width: parent.width

            visible: !root.hasMorphOutputs
            text: "No tracks assigned\nto Morph Outputs"
            color: Theme.disabledText
            font.pixelSize: 10
            font.bold: true

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.topMargin: Theme.sectionPadding + 21
            anchors.bottomMargin: Theme.sectionPadding
            spacing: 2

            Repeater {
                model: 8

                delegate: Item {
                    id: arrowLane

                    required property int index

                    readonly property var assignedTracks:
                        stackRoot.trackLists
                        && index < stackRoot.trackLists.length
                            ? stackRoot.trackLists[index]
                            : []

                    readonly property bool active:
                        assignedTracks && assignedTracks.length > 0

                    readonly property color arrowColor:
                        stackRoot.midiOutputConfigured
                            ? Theme.accent
                            : Theme.disabledText

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredHeight: 16

                    Canvas {
                        id: laneArrowCanvas

                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 2
                        anchors.rightMargin: 2
                        y: Math.max(5, (parent.height - height) / 2 - 1)
                        height: 7

                        visible: arrowLane.active

                        property color arrowColor: arrowLane.arrowColor

                        onArrowColorChanged: requestPaint()
                        onWidthChanged: requestPaint()
                        onVisibleChanged: requestPaint()

                        onPaint: {
                            const ctx = getContext("2d")
                            const centerY = height / 2
                            const endX = width - 2

                            ctx.reset()
                            ctx.strokeStyle = arrowColor
                            ctx.fillStyle = arrowColor
                            ctx.lineWidth = 1.3
                            ctx.lineCap = "round"
                            ctx.lineJoin = "round"

                            ctx.beginPath()
                            ctx.moveTo(1, centerY)
                            ctx.lineTo(endX - 6, centerY)
                            ctx.stroke()

                            ctx.beginPath()
                            ctx.moveTo(endX, centerY)
                            ctx.lineTo(endX - 6, centerY - 3)
                            ctx.lineTo(endX - 6, centerY + 3)
                            ctx.closePath()
                            ctx.fill()
                        }
                    }

                    Label {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: laneArrowCanvas.bottom
                        anchors.topMargin: -1

                        visible: arrowLane.active
                        text: (arrowLane.assignedTracks.length === 1
                               ? "Track "
                               : "Tracks ")
                              + root.compactTrackList(
                                    arrowLane.assignedTracks)
                        color: arrowLane.arrowColor
                        font.pixelSize: 7
                        font.bold: true
                        fontSizeMode: Text.Fit
                        minimumPixelSize: 6

                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignTop
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sectionPadding
        spacing: Theme.spacing

        Label {
            Layout.fillWidth: true

            text: "Configuration Overview"
            color: Theme.text
            font.pixelSize: Theme.controlFontSize
            font.bold: true
        }

        Label {
            Layout.fillWidth: true

            text: "Current MIDI signal path and performance configuration"
            color: Theme.secondaryText
            font.pixelSize: Theme.labelFontSize
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 4
            spacing: Theme.spacing

            ConfigurationNode {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1

                title: "MIDI INPUT"
                iconKind: "input"
                configured: root.inputConfigured
                primaryText: root.inputPortText
                secondaryText: root.inputChannelText
            }

            FlowArrow {
                Layout.preferredWidth: 92
                Layout.minimumWidth: 72
                Layout.maximumWidth: 112
                Layout.alignment: Qt.AlignVCenter

                active: root.inputConfigured
                bottomText: root.inputArrowChannelText
            }

            ConfigurationNode {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1.25

                title: "MORPHMASTER"
                iconKind: "surface"
                configured: true
                primaryText: root.processingSummary
                secondaryText: root.programSelectionText
                tertiaryText: root.presetText

                showMorphOutputs: true
                morphOutputNames: root.morphOutputNames
                morphOutputTrackLists: root.morphOutputTracks
            }

            MorphOutputArrowStack {
                Layout.preferredWidth: 112
                Layout.minimumWidth: 88
                Layout.maximumWidth: 140
                Layout.fillHeight: true

                trackLists: root.morphOutputTracks
                midiOutputConfigured: root.outputConfigured
            }

            ConfigurationNode {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1

                title: "MIDI OUTPUT"
                iconKind: "output"
                configured: root.outputConfigured
                primaryText: root.outputPortText
                secondaryText: root.outputConfigured
                               ? "Generated MIDI messages"
                               : "Select an output device above"
            }
        }
    }
}
