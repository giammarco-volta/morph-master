import QtQuick

import MorphMaster

Item {
    id: root

    property int morphOutputId: 6
    property real level: 0.0
    property bool compact: false
    property bool feedbackEnabled: false
    property var feedbackNotes: []

    readonly property bool noteMode:
        morphOutputId === 6 || morphOutputId === 2
    readonly property bool inverted:
        morphOutputId === 2 || morphOutputId === 0
    readonly property var curve:
        noteMode
            ? SettingsController.keyCurve()
            : SettingsController.velocityCurve()
    readonly property int minX:
        noteMode ? SettingsController.surfaceMinNote : 0
    readonly property int maxX:
        noteMode ? SettingsController.surfaceMaxNote : 127

    readonly property real meterLeft: root.compact ? 10 : 12
    readonly property real meterRight: root.compact ? 19 : 23
    readonly property real graphLeft: root.compact ? 36 : 40
    readonly property real meterTickLongLeft: root.compact ? 2 : 3
    readonly property real meterTickShortLeft: root.compact ? 5 : 7

    function clamp(value, lo, hi) {
        return Math.max(lo, Math.min(hi, value))
    }

    function morphOutputColor(outputId) {
        if (!root.feedbackEnabled)
            return Qt.rgba(0.18, 0.18, 0.18, 1.0)

        switch (outputId) {
        case 0: return Qt.rgba(0.45, 0.90, 0.48, 1.0) // Loud
        case 2: return Qt.rgba(0.96, 0.75, 0.05, 1.0) // High
        case 4: return Qt.rgba(0.02, 0.28, 0.12, 1.0) // Soft
        case 6: return Qt.rgba(0.08, 0.32, 0.92, 1.0) // Low
        default: return Theme.accent
        }
    }

    function curveWeightAt(value) {
        if (!root.curve)
            return 0.0

        if (value <= root.curve.x1)
            return root.clamp(root.curve.y1 / 100.0, 0.0, 1.0)

        if (value >= root.curve.x2)
            return root.clamp(root.curve.y2 / 100.0, 0.0, 1.0)

        const span = Math.max(1, root.curve.x2 - root.curve.x1)
        const t = (value - root.curve.x1) / span
        return root.clamp((root.curve.y1
                           + (root.curve.y2 - root.curve.y1) * t) / 100.0,
                          0.0,
                          1.0)
    }

    function gainAt(value) {
        const angle = root.curveWeightAt(value) * Math.PI * 0.5
        return root.inverted ? Math.cos(angle) : Math.sin(angle)
    }

    function graphRect() {
        const left = root.graphLeft
        const top = 10
        const right = root.width - 10
        const bottom = root.height - (root.compact ? 20 : 22)

        return {
            left: left,
            top: top,
            right: right,
            bottom: bottom,
            width: Math.max(1, right - left),
            height: Math.max(1, bottom - top)
        }
    }

    function requestGraphPaint() {
        Qt.callLater(function() {
            graphCanvas.requestPaint()
            meterCanvas.requestPaint()
            noteCanvas.requestPaint()
        })
    }

    Canvas {
        id: graphCanvas
        anchors.fill: parent

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.lineCap = "round"
            ctx.lineJoin = "round"

            const r = root.graphRect()
            const meterLeft = root.meterLeft
            const meterRight = root.meterRight
            const outputColor = root.morphOutputColor(root.morphOutputId)

            function xFor(value) {
                return r.left
                     + (value - root.minX)
                       / Math.max(1, root.maxX - root.minX)
                       * r.width
            }

            function yFor(gain) {
                return r.bottom - root.clamp(gain, 0.0, 1.0) * r.height
            }

            // Meter track: its top and bottom coincide with gain 1 and 0.
            ctx.fillStyle = Qt.rgba(0.04, 0.05, 0.07, 0.95)
            ctx.fillRect(meterLeft, r.top,
                         meterRight - meterLeft, r.height)
            ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.45)
            ctx.lineWidth = 1
            ctx.strokeRect(meterLeft + 0.5, r.top + 0.5,
                           meterRight - meterLeft - 1,
                           r.height - 1)

            // Meter ticks are outside the track, on its left side.
            ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.55)
            ctx.lineWidth = 1
            for (let i = 0; i <= 8; ++i) {
                const y = r.top + i * r.height / 8
                const tickLeft = i % 2 === 0
                               ? root.meterTickLongLeft
                               : root.meterTickShortLeft
                ctx.beginPath()
                ctx.moveTo(tickLeft, y)
                ctx.lineTo(meterLeft, y)
                ctx.stroke()
            }

            // Graph grid and axes.
            ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.13)
            ctx.lineWidth = 1
            ctx.setLineDash([2, 3])
            for (let i = 0; i <= 4; ++i) {
                const y = r.top + i * r.height / 4
                ctx.beginPath()
                ctx.moveTo(r.left, y)
                ctx.lineTo(r.right, y)
                ctx.stroke()
            }
            for (let i = 0; i <= 4; ++i) {
                const x = r.left + i * r.width / 4
                ctx.beginPath()
                ctx.moveTo(x, r.top)
                ctx.lineTo(x, r.bottom)
                ctx.stroke()
            }
            ctx.setLineDash([])

            ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.65)
            ctx.lineWidth = 1.2
            ctx.beginPath()
            ctx.moveTo(r.left, r.top)
            ctx.lineTo(r.left, r.bottom)
            ctx.lineTo(r.right, r.bottom)
            ctx.stroke()

            // Single equal-power branch for this Morph Output.
            ctx.strokeStyle = outputColor
            ctx.lineWidth = 2.5
            ctx.beginPath()
            const samples = Math.max(40, Math.round(r.width / 3))
            for (let i = 0; i <= samples; ++i) {
                const t = i / samples
                const value = root.minX + t * (root.maxX - root.minX)
                const x = r.left + t * r.width
                const y = yFor(root.gainAt(value))
                if (i === 0)
                    ctx.moveTo(x, y)
                else
                    ctx.lineTo(x, y)
            }
            ctx.stroke()

            // Shared gain labels and input range.
            ctx.fillStyle = Theme.secondaryText
            ctx.font = (root.compact ? "7px" : "9px") + " sans-serif"
            ctx.textBaseline = "middle"
            ctx.textAlign = "right"
            ctx.fillText("1", r.left - 4, r.top)
            ctx.fillText("0", r.left - 4, r.bottom)

            ctx.textBaseline = "top"
            ctx.textAlign = "left"
            ctx.fillText(String(root.minX), r.left, r.bottom + 5)
            ctx.textAlign = "center"
            ctx.fillText(root.noteMode ? "note" : "velocity",
                         (r.left + r.right) * 0.5,
                         r.bottom + 5)
            ctx.textAlign = "right"
            ctx.fillText(String(root.maxX), r.right, r.bottom + 5)
        }
    }

    Canvas {
        id: meterCanvas
        anchors.fill: parent

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            const r = root.graphRect()
            const meterLeft = root.meterLeft
            const meterRight = root.meterRight
            const effectiveLevel = root.clamp(root.level, 0.0, 1.0)

            if (effectiveLevel > 0.0) {
                const top = r.bottom - effectiveLevel * r.height
                ctx.fillStyle = "#e53935"
                ctx.fillRect(meterLeft + 2,
                             top,
                             meterRight - meterLeft - 4,
                             r.bottom - top - 1)
            }

            ctx.fillStyle = Theme.secondaryText
            ctx.font = (root.compact ? "7px" : "9px") + " sans-serif"
            ctx.textAlign = "center"
            ctx.textBaseline = "top"
            ctx.fillText(effectiveLevel.toFixed(2),
                         (meterLeft + meterRight) * 0.5,
                         r.bottom + 5)
        }
    }

    Canvas {
        id: noteCanvas
        anchors.fill: parent

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            if (!root.feedbackEnabled)
                return

            const r = root.graphRect()

            function xFor(value) {
                return r.left
                     + (value - root.minX)
                       / Math.max(1, root.maxX - root.minX)
                       * r.width
            }

            function yFor(gain) {
                return r.bottom - root.clamp(gain, 0.0, 1.0) * r.height
            }

            ctx.fillStyle = "#e53935"
            const radius = root.compact ? 2.8 : 4.5

            for (let i = 0; i < root.feedbackNotes.length; ++i) {
                const noteData = root.feedbackNotes[i]
                const inputValue = root.noteMode
                                 ? Number(noteData.note)
                                 : Number(noteData.velocity)

                if (inputValue < root.minX || inputValue > root.maxX)
                    continue

                ctx.beginPath()
                ctx.arc(xFor(inputValue),
                        yFor(root.gainAt(inputValue)),
                        radius,
                        0,
                        Math.PI * 2)
                ctx.fill()
            }
        }
    }

    Connections {
        target: root.curve
        ignoreUnknownSignals: true

        function onRangeChanged() { root.requestGraphPaint() }
        function onX1Changed() { root.requestGraphPaint() }
        function onY1Changed() { root.requestGraphPaint() }
        function onX2Changed() { root.requestGraphPaint() }
        function onY2Changed() { root.requestGraphPaint() }
    }

    Connections {
        target: SettingsController
        function onSurfaceKeyboardRangeChanged() { root.requestGraphPaint() }
    }

    Component.onCompleted: root.requestGraphPaint()
    onWidthChanged: root.requestGraphPaint()
    onHeightChanged: root.requestGraphPaint()
    onMorphOutputIdChanged: root.requestGraphPaint()
    onLevelChanged: meterCanvas.requestPaint()
    onFeedbackNotesChanged: {
        if (root.feedbackEnabled)
            noteCanvas.requestPaint()
    }
    onFeedbackEnabledChanged: noteCanvas.requestPaint()
}
