import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster

Item {
    id: root

    signal trackEditRequested(int trackNumber)
    signal curveEditRequested(int morphOutputId)

    readonly property bool phoneLayout:
        ApplicationWindow.window
            && ApplicationWindow.window.layoutClass === UiMetrics.Phone

    property int assignmentRevision: 0

    function gridRow(outputId) {
        switch (outputId) {
        case 7: return 0 // Low and Loud
        case 0: return 0 // Loud
        case 1: return 0 // High and Loud
        case 6: return 1 // Low
        case 2: return 1 // High
        case 5: return 2 // Low and Soft
        case 4: return 2 // Soft
        case 3: return 2 // High and Soft
        default: return 0
        }
    }

    function gridColumn(outputId) {
        switch (outputId) {
        case 7: return 0
        case 0: return 1
        case 1: return 2
        case 6: return 0
        case 2: return 2
        case 5: return 0
        case 4: return 1
        case 3: return 2
        default: return 0
        }
    }


    function neutralPanelTint() {
        // Neutral mid-grey used by Morph Outputs with no assigned tracks.
        return Qt.rgba(0.50, 0.50, 0.50, 0.20)
    }

    function panelTint(outputId) {
        switch (outputId) {
        case 0: return Qt.rgba(0.45, 0.90, 0.48, 0.24) // Loud
        case 1: return Qt.rgba(1.00, 0.93, 0.35, 0.25) // High and Loud
        case 2: return Qt.rgba(0.96, 0.75, 0.05, 0.25) // High
        case 3: return Qt.rgba(0.55, 0.40, 0.00, 0.28) // High and Soft
        case 4: return Qt.rgba(0.02, 0.28, 0.12, 0.28) // Soft
        case 5: return Qt.rgba(0.02, 0.10, 0.34, 0.30) // Low and Soft
        case 6: return Qt.rgba(0.08, 0.32, 0.92, 0.25) // Low
        case 7: return Qt.rgba(0.35, 0.72, 1.00, 0.25) // Low and Loud
        default: return "transparent"
        }
    }

    function preferredPanelWidth(outputId) {
        const column = root.gridColumn(outputId)
        return column === 1 ? 180 : 130
    }

    function preferredPanelHeight(outputId) {
        const row = root.gridRow(outputId)
        return row === 1 ? 132 : 98
    }

    function unassignedTrackNumbers() {
        const unused = assignmentRevision
        const result = []

        for (let trackNumber = 1; trackNumber <= 16; ++trackNumber) {
            const controller = SettingsController.track(trackNumber)
            if (controller && controller.morphOutput === 8)
                result.push(trackNumber)
        }

        return result
    }

    function assignTrack(trackNumber, morphOutputId) {
        const controller = SettingsController.track(trackNumber)
        if (controller)
            controller.morphOutput = morphOutputId
    }

    Connections {
        target: SettingsController.morphOutputStateModel

        function onDataChanged(topLeft, bottomRight, roles) {
            // Ignore real-time gain and mute/solo updates. The revision is
            // needed only when track assignments change.
            if (roles && roles.length > 0
                    && roles.indexOf(261) < 0   // AssignedTrackIndexesRole
                    && roles.indexOf(263) < 0    // TrackMaskRole
                    && roles.indexOf(259) < 0)  // SpecificNameRole
                return

            root.assignmentRevision += 1
        }

        function onModelReset() {
            root.assignmentRevision += 1
        }
    }

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        Item {
            width: parent.width
            height: Math.max(parent.height, assignmentGrid.implicitHeight + 24)

            GridLayout {
                id: assignmentGrid

                anchors.fill: parent
                anchors.margins: 12
                columns: 3
                rows: 3
                columnSpacing: UiMetrics.spacing(
                                   ApplicationWindow.window
                                       ? ApplicationWindow.window.layoutClass
                                       : UiMetrics.Desktop)
                rowSpacing: columnSpacing

                Repeater {
                    model: SettingsController.morphOutputStateModel

                    delegate: Item {
                        required property int morphOutputId
                        required property string name
                        required property string specificName
                        required property var assignedTrackIndexes
                        required property bool muted
                        required property bool solo

                        Layout.row: root.gridRow(morphOutputId)
                        Layout.column: root.gridColumn(morphOutputId)
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: 96
                        Layout.minimumHeight: 84
                        Layout.preferredWidth: root.preferredPanelWidth(morphOutputId)
                        Layout.preferredHeight: root.preferredPanelHeight(morphOutputId)

                        MorphOutputAssignmentPanel {
                            anchors.fill: parent
                            morphOutputId: parent.morphOutputId
                            title: parent.name
                            specificName: parent.specificName
                            trackNumbers: parent.assignedTrackIndexes
                            compactTrackTiles: root.phoneLayout
                            panelTint: parent.assignedTrackIndexes.length > 0
                                       ? root.panelTint(parent.morphOutputId)
                                       : root.neutralPanelTint()
                            muted: parent.muted
                            solo: parent.solo

                            onTrackEditRequested: function(trackNumber) {
                                root.trackEditRequested(trackNumber)
                            }

                            onSpecificNameEdited: function(value) {
                                SettingsController.setMorphOutputName(parent.morphOutputId, value)
                            }

                            onCurveEditRequested: function(outputId) {
                                root.curveEditRequested(outputId)
                            }

                            onTrackDropped: function(trackNumber, outputId) {
                                root.assignTrack(trackNumber, outputId)
                            }

                            onMuteToggled: function(value) {
                                if (value && parent.solo) {
                                    SettingsController.setMorphOutputSolo(
                                                parent.morphOutputId, false)
                                }

                                SettingsController.setMorphOutputMuted(
                                            parent.morphOutputId, value)
                            }

                            onSoloToggled: function(value) {
                                if (value && parent.muted) {
                                    SettingsController.setMorphOutputMuted(
                                                parent.morphOutputId, false)
                                }

                                SettingsController.setMorphOutputSolo(
                                            parent.morphOutputId, value)
                            }

                            onStateCycleRequested: {
                                if (parent.solo) {
                                    SettingsController.setMorphOutputSolo(
                                                parent.morphOutputId, false)
                                } else if (parent.muted) {
                                    SettingsController.setMorphOutputMuted(
                                                parent.morphOutputId, false)
                                    SettingsController.setMorphOutputSolo(
                                                parent.morphOutputId, true)
                                } else {
                                    SettingsController.setMorphOutputMuted(
                                                parent.morphOutputId, true)
                                }
                            }
                        }
                    }
                }

                MorphOutputAssignmentPanel {
                    Layout.row: 1
                    Layout.column: 1
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: 130
                    Layout.minimumHeight: 118
                    Layout.preferredWidth: 180
                    Layout.preferredHeight: 132

                    morphOutputId: 8
                    title: "Unassigned Output Tracks"
                    trackNumbers: root.unassignedTrackNumbers()
                    compactTrackTiles: root.phoneLayout
                    fitEightCompactTilesPerRow: root.phoneLayout

                    onTrackEditRequested: function(trackNumber) {
                        root.trackEditRequested(trackNumber)
                    }

                    onTrackDropped: function(trackNumber, outputId) {
                        root.assignTrack(trackNumber, outputId)
                    }
                }
            }
        }
    }
}
