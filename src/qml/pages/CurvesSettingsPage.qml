import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster

Item {
    id: page

    /* Tablet/desktop show both global curves; phone shows one at a time. */
    property bool singleCurveMode: false

    signal curveActivated(int curveIndex)

    /* 0 = Low and High Morph Curves, 1 = Soft and Loud Morph Curves. */
    property int currentCurveIndex: 0

    /*
     * The four 3D surfaces are expensive to rebuild. While a curve is
     * being dragged they keep their last rendered shape; one shared
     * revision is emitted only when the gesture finishes.
     */
    property int activeCurveEditGestures: 0
    property int surfaceRevision: 0

    function requestSurfaceRefresh() {
        if (page.activeCurveEditGestures > 0)
            return

        surfaceRefreshTimer.restart()
    }

    function beginCurveEditGesture() {
        page.activeCurveEditGestures += 1
    }

    function finishCurveEditGesture() {
        page.activeCurveEditGestures =
                Math.max(0, page.activeCurveEditGestures - 1)

        if (page.activeCurveEditGestures === 0)
            surfaceRefreshTimer.restart()
    }

    readonly property var cornerMorphOutputs: [
        { outputId: 5, title: "Low and Soft Morph Surface" },
        { outputId: 7, title: "Low and Loud Morph Surface" },
        { outputId: 3, title: "High and Soft Morph Surface" },
        { outputId: 1, title: "High and Loud Morph Surface" }
    ]

    Timer {
        id: surfaceRefreshTimer
        interval: 0
        repeat: false

        onTriggered: page.surfaceRevision += 1
    }

    function normalizedCurveIndex(index) {
        return Math.max(0, Math.min(1, Math.round(index)))
    }

    function curveAt(index) {
        return index === 1
                ? SettingsController.velocityCurve()
                : SettingsController.keyCurve()
    }

    function curveTitle(index) {
        return index === 1
                ? "Soft and Loud Morph Curves"
                : "Low and High Morph Curves"
    }

    onCurrentCurveIndexChanged: {
        const normalized = normalizedCurveIndex(currentCurveIndex)

        if (normalized !== currentCurveIndex)
            currentCurveIndex = normalized
    }

    component BoundCurveEditor: CurveEditRow {
        id: row

        property int curveIndex: 0

        readonly property var curve:
            page.curveAt(curveIndex)

        primaryMorphOutputId: curveIndex === 0 ? 6 : 4
        complementaryMorphOutputId: curveIndex === 0 ? 2 : 0

        property bool syncingFromController: false

        function syncFromController() {
            if (!row.curve)
                return

            row.syncingFromController = true

            row.title = page.curveTitle(row.curveIndex)
            row.noteMode = row.curve.noteMode
            row.minX = row.curve.minX
            row.maxX = row.curve.maxX

            row.x1 = row.curve.x1
            row.y1 = row.curve.y1
            row.x2 = row.curve.x2
            row.y2 = row.curve.y2

            row.syncingFromController = false
        }

        Component.onCompleted: row.syncFromController()

        onCurveChanged: {
            row.syncFromController()
            page.requestSurfaceRefresh()
        }

        onCurveIndexChanged: {
            row.syncFromController()
            page.requestSurfaceRefresh()
        }

        onEditingStarted: page.beginCurveEditGesture()
        onEditingFinished: page.finishCurveEditGesture()

        onCurveEdited: {
            if (row.syncingFromController || !row.curve)
                return

            page.curveActivated(row.curveIndex)

            if (row.curve.x1 !== row.x1)
                row.curve.x1 = row.x1

            if (row.curve.y1 !== row.y1)
                row.curve.y1 = row.y1

            if (row.curve.x2 !== row.x2)
                row.curve.x2 = row.x2

            if (row.curve.y2 !== row.y2)
                row.curve.y2 = row.y2

            page.requestSurfaceRefresh()
        }

        Connections {
            target: row.curve
            ignoreUnknownSignals: true

            function onRangeChanged() {
                row.syncFromController()
                page.requestSurfaceRefresh()
            }

            function onX1Changed() {
                row.syncFromController()
                page.requestSurfaceRefresh()
            }

            function onY1Changed() {
                row.syncFromController()
                page.requestSurfaceRefresh()
            }

            function onX2Changed() {
                row.syncFromController()
                page.requestSurfaceRefresh()
            }

            function onY2Changed() {
                row.syncFromController()
                page.requestSurfaceRefresh()
            }
        }
    }

    Loader {
        anchors.fill: parent

        sourceComponent: page.singleCurveMode
                         ? singleCurveComponent
                         : bothCurvesComponent
    }

    Component {
        id: bothCurvesComponent

        Item {
            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.pageMargin
                anchors.rightMargin: Theme.pageMargin
                anchors.topMargin: Theme.pageSpacing
                anchors.bottomMargin: Theme.pageMargin
                spacing: Theme.spacing

                Repeater {
                    model: 2

                    delegate: BoundCurveEditor {
                        required property int index

                        curveIndex: index

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 0

                        fillAvailableHeight: true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 145
                    spacing: Theme.spacing

                    Repeater {
                        model: page.cornerMorphOutputs

                        delegate: MorphCornerSurfaceGraph {
                            required property var modelData

                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumWidth: 110
                            Layout.minimumHeight: 140

                            morphOutputId: modelData.outputId
                            title: modelData.title
                            surfaceRevision: page.surfaceRevision
                        }
                    }
                }
            }
        }
    }

    Component {
        id: singleCurveComponent

        Item {
            BoundCurveEditor {
                anchors.fill: parent
                anchors.margins: Theme.pageMargin

                curveIndex: page.currentCurveIndex

                showTitle: false
                fillAvailableHeight: true
            }
        }
    }
}
