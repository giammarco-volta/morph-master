import QtQuick

import MorphMaster

Item {
    id: root

    property int morphOutputId: 5
    property real level: 0.0
    property bool compact: false
    property bool feedbackEnabled: false
    property var feedbackNotes: []

    readonly property var keyCurve: SettingsController.keyCurve()
    readonly property var velocityCurve: SettingsController.velocityCurve()
    readonly property int minNote: SettingsController.surfaceMinNote
    readonly property int maxNote: SettingsController.surfaceMaxNote

    property var projectionCache: ({ width: -1, height: -1 })

    readonly property real meterTrackLeftX: -0.18
    readonly property real meterTrackRightX: -0.08
    readonly property real meterTickMinorX: -0.23
    readonly property real meterTickMajorX: -0.27

    function clamp(value, lo, hi) {
        return Math.max(lo, Math.min(hi, value))
    }

    function profileForOutput(outputId) {
        switch (outputId) {
        case 1: return { invertKey: true, invertVelocity: true }
        case 3: return { invertKey: true, invertVelocity: false }
        case 5: return { invertKey: false, invertVelocity: false }
        case 7: return { invertKey: false, invertVelocity: true }
        default: return { invertKey: false, invertVelocity: false }
        }
    }

    function curveWeightAt(curve, value) {
        if (!curve)
            return 0.0

        if (value <= curve.x1)
            return clamp(curve.y1 / 100.0, 0.0, 1.0)

        if (value >= curve.x2)
            return clamp(curve.y2 / 100.0, 0.0, 1.0)

        const span = Math.max(1, curve.x2 - curve.x1)
        const t = (value - curve.x1) / span
        return clamp((curve.y1 + (curve.y2 - curve.y1) * t) / 100.0,
                     0.0,
                     1.0)
    }

    function equalPowerGain(weight, inverted) {
        const angle = clamp(weight, 0.0, 1.0) * Math.PI * 0.5
        return inverted ? Math.cos(angle) : Math.sin(angle)
    }

    function gainAt(note, velocity) {
        const profile = profileForOutput(root.morphOutputId)
        const keyGain = equalPowerGain(
                          curveWeightAt(root.keyCurve, note),
                          profile.invertKey)
        const velocityGain = equalPowerGain(
                               curveWeightAt(root.velocityCurve, velocity),
                               profile.invertVelocity)
        return Math.sqrt(Math.max(0.0, keyGain * velocityGain))
    }

    function noteForNorm(xNorm) {
        return root.minNote
             + clamp(xNorm, 0.0, 1.0) * (root.maxNote - root.minNote)
    }

    function velocityForNorm(yNorm) {
        return clamp(yNorm, 0.0, 1.0) * 127.0
    }

    function morphOutputColor(outputId) {
        if (!root.feedbackEnabled)
            return Qt.rgba(0.18, 0.18, 0.18, 1.0)

        switch (outputId) {
        case 1: return Qt.rgba(1.00, 0.93, 0.35, 1.0)
        case 3: return Qt.rgba(0.55, 0.40, 0.00, 1.0)
        case 5: return Qt.rgba(0.02, 0.10, 0.34, 1.0)
        case 7: return Qt.rgba(0.35, 0.72, 1.00, 1.0)
        default: return Theme.accent
        }
    }

    // Coordinates are intentionally not clamped: the gain meter extends
    // beyond x=0 as a continuation of the Forte wall.
    function rawProjectPoint(xNorm, yNorm, gain) {
        const relX = xNorm * 1.30 + 1.8
        const relY = yNorm * 1.30 + 2.2
        const relZ = clamp(gain, 0.0, 1.0) * 0.60 - 1.65

        const depth = relX * 0.60885777
                    + relY * 0.71474607
                    - relZ * 0.34413700
        const cameraX = relX * 0.76124323
                      - relY * 0.64846646
        const cameraY = relX * 0.22316130
                      + relY * 0.26197196
                      + relZ * 0.93891945

        return {
            x: cameraX / depth,
            y: -cameraY / depth,
            depth: depth
        }
    }

    function invalidateProjection() {
        root.projectionCache = ({ width: -1, height: -1 })
    }

    function ensureProjection() {
        const usableWidth = Math.max(1, graphCanvas.width)
        const usableHeight = Math.max(1, graphCanvas.height)

        if (root.projectionCache.width === usableWidth
                && root.projectionCache.height === usableHeight)
            return root.projectionCache

        const corners = [
            rawProjectPoint(root.meterTickMajorX, 1, 0),
            rawProjectPoint(root.meterTickMajorX, 1, 1),
            rawProjectPoint(0, 0, 0), rawProjectPoint(1, 0, 0),
            rawProjectPoint(0, 1, 0), rawProjectPoint(1, 1, 0),
            rawProjectPoint(0, 0, 1), rawProjectPoint(1, 0, 1),
            rawProjectPoint(0, 1, 1), rawProjectPoint(1, 1, 1)
        ]

        let minX = corners[0].x
        let maxX = corners[0].x
        let minY = corners[0].y
        let maxY = corners[0].y

        for (let i = 1; i < corners.length; ++i) {
            minX = Math.min(minX, corners[i].x)
            maxX = Math.max(maxX, corners[i].x)
            minY = Math.min(minY, corners[i].y)
            maxY = Math.max(maxY, corners[i].y)
        }

        const left = 8
        const right = usableWidth - 7
        const top = 5
        const bottom = usableHeight - (root.compact ? 20 : 23)
        const rawWidth = Math.max(0.001, maxX - minX)
        const rawHeight = Math.max(0.001, maxY - minY)
        const scale = Math.max(1,
                               Math.min((right - left) / rawWidth,
                                        (bottom - top) / rawHeight))

        root.projectionCache = {
            width: usableWidth,
            height: usableHeight,
            scale: scale,
            offsetX: (left + right) * 0.5
                     - (minX + maxX) * 0.5 * scale,
            offsetY: (top + bottom) * 0.5
                     - (minY + maxY) * 0.5 * scale
        }
        return root.projectionCache
    }

    function projectPoint(xNorm, yNorm, gain) {
        const raw = rawProjectPoint(xNorm, yNorm, gain)
        const transform = ensureProjection()
        return {
            x: transform.offsetX + raw.x * transform.scale,
            y: transform.offsetY + raw.y * transform.scale,
            depth: raw.depth
        }
    }

    function projectionDepth(xNorm, yNorm, gain) {
        return rawProjectPoint(xNorm, yNorm, gain).depth
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

            const xResolution = 12
            const yResolution = 8

            function point(x, y, z) {
                return root.projectPoint(x, y, z)
            }

            function strokeSegment(a, b) {
                ctx.beginPath()
                ctx.moveTo(a.x, a.y)
                ctx.lineTo(b.x, b.y)
                ctx.stroke()
            }

            function fillPolygon(points, fillStyle, alpha) {
                ctx.save()
                ctx.fillStyle = fillStyle
                ctx.globalAlpha = alpha
                ctx.beginPath()
                ctx.moveTo(points[0].x, points[0].y)
                for (let i = 1; i < points.length; ++i)
                    ctx.lineTo(points[i].x, points[i].y)
                ctx.closePath()
                ctx.fill()
                ctx.restore()
            }

            const base00 = point(0, 0, 0)
            const base10 = point(1, 0, 0)
            const base11 = point(1, 1, 0)
            const base01 = point(0, 1, 0)

            // Background walls. Empty Morph Outputs use a neutral graph.
            const wallColor = root.feedbackEnabled
                            ? Qt.rgba(0.10, 0.12, 0.17, 1.0)
                            : Qt.rgba(0.15, 0.15, 0.15, 1.0)
            fillPolygon([point(1, 0, 0), point(1, 1, 0),
                         point(1, 1, 1), point(1, 0, 1)],
                        wallColor, 0.52)
            fillPolygon([point(0, 1, 0), point(1, 1, 0),
                         point(1, 1, 1), point(0, 1, 1)],
                        wallColor, 0.42)

            ctx.save()
            ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.11)
            ctx.lineWidth = 1
            ctx.setLineDash([2, 3])
            for (let i = 0; i <= 6; ++i) {
                const t = i / 6
                strokeSegment(point(1, t, 0), point(1, t, 1))
                strokeSegment(point(t, 1, 0), point(t, 1, 1))
                strokeSegment(point(t, 0, 0), point(t, 1, 0))
                strokeSegment(point(0, t, 0), point(1, t, 0))
            }
            for (let i = 0; i <= 4; ++i) {
                const t = i / 4
                strokeSegment(point(1, 0, t), point(1, 1, t))
                strokeSegment(point(0, 1, t), point(1, 1, t))
            }
            ctx.restore()

            // Perspective gain meter, separated from the graph by a small gap.
            const meterTrack = [
                point(root.meterTrackLeftX, 1, 0),
                point(root.meterTrackRightX, 1, 0),
                point(root.meterTrackRightX, 1, 1),
                point(root.meterTrackLeftX, 1, 1)
            ]
            fillPolygon(meterTrack, Qt.rgba(0.04, 0.05, 0.07, 0.98), 1.0)

            ctx.save()
            ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.58)
            ctx.lineWidth = 1
            strokeSegment(meterTrack[0], meterTrack[1])
            strokeSegment(meterTrack[1], meterTrack[2])
            strokeSegment(meterTrack[2], meterTrack[3])
            strokeSegment(meterTrack[3], meterTrack[0])

            // Denser ticks, all outside the track on its left side.
            for (let i = 0; i <= 8; ++i) {
                const z = i / 8
                const tickX = i % 2 === 0
                            ? root.meterTickMajorX
                            : root.meterTickMinorX
                strokeSegment(point(tickX, 1, z),
                              point(root.meterTrackLeftX, 1, z))
            }
            ctx.restore()

            // Surface cells, back to front.
            const cells = []
            for (let yi = 0; yi < yResolution; ++yi) {
                const y0 = yi / yResolution
                const y1 = (yi + 1) / yResolution
                for (let xi = 0; xi < xResolution; ++xi) {
                    const x0 = xi / xResolution
                    const x1 = (xi + 1) / xResolution
                    const z00 = root.gainAt(root.noteForNorm(x0), root.velocityForNorm(y0))
                    const z10 = root.gainAt(root.noteForNorm(x1), root.velocityForNorm(y0))
                    const z11 = root.gainAt(root.noteForNorm(x1), root.velocityForNorm(y1))
                    const z01 = root.gainAt(root.noteForNorm(x0), root.velocityForNorm(y1))
                    cells.push({
                        p00: point(x0, y0, z00),
                        p10: point(x1, y0, z10),
                        p11: point(x1, y1, z11),
                        p01: point(x0, y1, z01),
                        depth: (root.projectionDepth(x0, y0, z00)
                                + root.projectionDepth(x1, y0, z10)
                                + root.projectionDepth(x1, y1, z11)
                                + root.projectionDepth(x0, y1, z01)) * 0.25
                    })
                }
            }
            cells.sort(function(a, b) { return b.depth - a.depth })
            for (let i = 0; i < cells.length; ++i) {
                const cell = cells[i]
                fillPolygon([cell.p00, cell.p10, cell.p11, cell.p01],
                            root.morphOutputColor(root.morphOutputId),
                            1.0)
            }

            ctx.save()
            ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.27)
            ctx.lineWidth = 0.75
            for (let yi = 0; yi <= yResolution; ++yi) {
                const yNorm = yi / yResolution
                ctx.beginPath()
                for (let xi = 0; xi <= xResolution; ++xi) {
                    const xNorm = xi / xResolution
                    const p = point(xNorm, yNorm,
                                    root.gainAt(root.noteForNorm(xNorm),
                                                root.velocityForNorm(yNorm)))
                    if (xi === 0) ctx.moveTo(p.x, p.y)
                    else ctx.lineTo(p.x, p.y)
                }
                ctx.stroke()
            }
            for (let xi = 0; xi <= xResolution; ++xi) {
                const xNorm = xi / xResolution
                ctx.beginPath()
                for (let yi = 0; yi <= yResolution; ++yi) {
                    const yNorm = yi / yResolution
                    const p = point(xNorm, yNorm,
                                    root.gainAt(root.noteForNorm(xNorm),
                                                root.velocityForNorm(yNorm)))
                    if (yi === 0) ctx.moveTo(p.x, p.y)
                    else ctx.lineTo(p.x, p.y)
                }
                ctx.stroke()
            }
            ctx.restore()

            ctx.save()
            ctx.strokeStyle = Qt.rgba(0.85, 0.88, 0.92, 0.34)
            ctx.lineWidth = 1
            strokeSegment(base00, base10)
            strokeSegment(base10, base11)
            strokeSegment(base11, base01)
            strokeSegment(base01, base00)
            ctx.restore()

            ctx.save()
            ctx.fillStyle = Theme.secondaryText
            ctx.font = "8px sans-serif"
            ctx.textBaseline = "middle"
            ctx.textAlign = "center"
            ctx.fillText("Low", base00.x + 5, base00.y + 12)
            ctx.fillText("High", base10.x, base10.y + 12)
            ctx.textAlign = "right"
            ctx.fillText("Soft", base00.x - 4, base00.y)
            ctx.fillText("Loud", base01.x + 6, base01.y + 10)
            ctx.restore()
        }
    }

    Canvas {
        id: meterCanvas
        anchors.fill: parent

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            const level = root.clamp(root.level, 0.0, 1.0)
            const bottomLeft = root.projectPoint(root.meterTrackLeftX, 1, 0)
            const bottomRight = root.projectPoint(root.meterTrackRightX, 1, 0)

            if (level > 0.0) {
                const points = [
                    bottomLeft,
                    bottomRight,
                    root.projectPoint(root.meterTrackRightX, 1, level),
                    root.projectPoint(root.meterTrackLeftX, 1, level)
                ]

                ctx.fillStyle = "#e53935"
                ctx.beginPath()
                ctx.moveTo(points[0].x, points[0].y)
                for (let i = 1; i < points.length; ++i)
                    ctx.lineTo(points[i].x, points[i].y)
                ctx.closePath()
                ctx.fill()
            }

            ctx.fillStyle = Theme.secondaryText
            ctx.font = (root.compact ? "7px" : "8px") + " sans-serif"
            ctx.textAlign = "center"
            ctx.textBaseline = "top"
            ctx.fillText(level.toFixed(2),
                         (bottomLeft.x + bottomRight.x) * 0.5,
                         Math.max(bottomLeft.y, bottomRight.y) + 5)
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

            ctx.fillStyle = "#e53935"
            const radius = root.compact ? 2.8 : 4.5

            for (let i = 0; i < root.feedbackNotes.length; ++i) {
                const noteData = root.feedbackNotes[i]
                const note = Number(noteData.note)
                const velocity = Number(noteData.velocity)

                if (note < root.minNote || note > root.maxNote)
                    continue

                const xNorm = root.maxNote > root.minNote
                            ? (note - root.minNote)
                              / (root.maxNote - root.minNote)
                            : 0.0
                const yNorm = root.clamp(velocity / 127.0, 0.0, 1.0)
                const point = root.projectPoint(
                                  xNorm,
                                  yNorm,
                                  root.gainAt(note, velocity))

                ctx.beginPath()
                ctx.arc(point.x, point.y, radius, 0, Math.PI * 2)
                ctx.fill()
            }
        }
    }

    Connections {
        target: root.keyCurve
        ignoreUnknownSignals: true
        function onRangeChanged() { root.requestGraphPaint() }
        function onX1Changed() { root.requestGraphPaint() }
        function onY1Changed() { root.requestGraphPaint() }
        function onX2Changed() { root.requestGraphPaint() }
        function onY2Changed() { root.requestGraphPaint() }
    }

    Connections {
        target: root.velocityCurve
        ignoreUnknownSignals: true
        function onRangeChanged() { root.requestGraphPaint() }
        function onX1Changed() { root.requestGraphPaint() }
        function onY1Changed() { root.requestGraphPaint() }
        function onX2Changed() { root.requestGraphPaint() }
        function onY2Changed() { root.requestGraphPaint() }
    }

    Connections {
        target: SettingsController
        function onSurfaceKeyboardRangeChanged() {
            root.invalidateProjection()
            root.requestGraphPaint()
        }
    }

    Component.onCompleted: root.requestGraphPaint()
    onWidthChanged: {
        root.invalidateProjection()
        root.requestGraphPaint()
    }
    onHeightChanged: {
        root.invalidateProjection()
        root.requestGraphPaint()
    }
    onMorphOutputIdChanged: root.requestGraphPaint()
    onLevelChanged: meterCanvas.requestPaint()
    onFeedbackNotesChanged: {
        if (root.feedbackEnabled)
            noteCanvas.requestPaint()
    }
    onFeedbackEnabledChanged: noteCanvas.requestPaint()
}
