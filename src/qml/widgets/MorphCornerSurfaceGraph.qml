import QtQuick

import MorphMaster

Rectangle {
    id: root

    property int morphOutputId: 5
    property string title: "Low and Soft"
    property int surfaceRevision: 0

    readonly property var keyCurve: SettingsController.keyCurve()
    readonly property var velocityCurve: SettingsController.velocityCurve()
    readonly property int minNote: SettingsController.surfaceMinNote
    readonly property int maxNote: SettingsController.surfaceMaxNote

    property int assignmentRevision: 0
    property var feedbackNotes: []
    property var projectionCache: ({ width: -1, height: -1 })

    color: Theme.panelDark
    border.color: Theme.border
    border.width: 1
    radius: 4
    clip: true

    function clamp(value, lo, hi) {
        return Math.max(lo, Math.min(hi, value))
    }

    function profileForOutput(outputId) {
        switch (outputId) {
        case 1: return { invertKey: true, invertVelocity: true }   // High and Loud
        case 3: return { invertKey: true, invertVelocity: false }  // High and Soft
        case 5: return { invertKey: false, invertVelocity: false } // Low and Soft
        case 7: return { invertKey: false, invertVelocity: true }  // Low and Loud
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
        const weight = curve.y1 + (curve.y2 - curve.y1) * t
        return clamp(weight / 100.0, 0.0, 1.0)
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

    function firstTrackForOutput() {
        const unused = root.assignmentRevision

        for (let trackNumber = 1; trackNumber <= 16; ++trackNumber) {
            const controller = SettingsController.track(trackNumber)
            if (controller && controller.morphOutput === root.morphOutputId)
                return trackNumber
        }

        return 0
    }

    function rawProjectPoint(xNorm, yNorm, gain) {
        const relX = clamp(xNorm, 0.0, 1.0) * 1.30 + 1.8
        const relY = clamp(yNorm, 0.0, 1.0) * 1.30 + 2.2
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
        const usableWidth = Math.max(1, sceneCanvas.width)
        const usableHeight = Math.max(1, sceneCanvas.height)

        if (root.projectionCache.width === usableWidth
                && root.projectionCache.height === usableHeight) {
            return root.projectionCache
        }

        const corners = [
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

        const left = 20
        const right = usableWidth - 16
        const top = 8
        const bottom = usableHeight - 26
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

    function morphOutputColor(outputId) {
        switch (outputId) {
        case 0: return Qt.rgba(0.45, 0.90, 0.48, 1.0) // Loud
        case 1: return Qt.rgba(1.00, 0.93, 0.35, 1.0) // High and Loud
        case 2: return Qt.rgba(0.96, 0.75, 0.05, 1.0) // High
        case 3: return Qt.rgba(0.55, 0.40, 0.00, 1.0) // High and Soft
        case 4: return Qt.rgba(0.02, 0.28, 0.12, 1.0) // Soft
        case 5: return Qt.rgba(0.02, 0.10, 0.34, 1.0) // Low and Soft
        case 6: return Qt.rgba(0.08, 0.32, 0.92, 1.0) // Low
        case 7: return Qt.rgba(0.35, 0.72, 1.00, 1.0) // Low and Loud
        default: return Theme.accent
        }
    }

    function schedulePaint() {
        Qt.callLater(function() {
            sceneCanvas.requestPaint()
            noteCanvas.requestPaint()
        })
    }

    function noteOn(note, velocity) {
        for (let i = 0; i < root.feedbackNotes.length; ++i) {
            if (root.feedbackNotes[i].note === note) {
                root.feedbackNotes[i].velocity = velocity
                noteCanvas.requestPaint()
                return
            }
        }

        root.feedbackNotes.push({
            note: note,
            velocity: velocity
        })
        noteCanvas.requestPaint()
    }

    function noteOff(note) {
        let removed = false

        for (let i = root.feedbackNotes.length - 1; i >= 0; --i) {
            if (root.feedbackNotes[i].note === note) {
                root.feedbackNotes.splice(i, 1)
                removed = true
            }
        }

        if (removed)
            noteCanvas.requestPaint()
    }

    Text {
        id: titleLabel
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 22

        text: root.title
        color: Theme.text
        font.pixelSize: 12
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    Canvas {
        id: sceneCanvas
        anchors.top: titleLabel.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 1

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.lineCap = "round"
            ctx.lineJoin = "round"

            const xResolution = 14
            const yResolution = 10
            const wallColumns = 8
            const wallRows = 6

            function point(xNorm, yNorm, gain) {
                return root.projectPoint(xNorm, yNorm, gain)
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

            fillPolygon([point(1, 0, 0), point(1, 1, 0),
                         point(1, 1, 1), point(1, 0, 1)],
                        Qt.rgba(0.10, 0.12, 0.17, 1.0), 0.52)
            fillPolygon([point(0, 1, 0), point(1, 1, 0),
                         point(1, 1, 1), point(0, 1, 1)],
                        Qt.rgba(0.10, 0.12, 0.17, 1.0), 0.42)

            ctx.save()
            ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.11)
            ctx.lineWidth = 1
            ctx.setLineDash([2, 3])

            for (let i = 0; i <= wallColumns; ++i) {
                const t = i / wallColumns
                strokeSegment(point(1, t, 0), point(1, t, 1))
                strokeSegment(point(t, 1, 0), point(t, 1, 1))
                strokeSegment(point(t, 0, 0), point(t, 1, 0))
            }

            for (let i = 0; i <= wallRows; ++i) {
                const t = i / wallRows
                strokeSegment(point(1, 0, t), point(1, 1, t))
                strokeSegment(point(0, 1, t), point(1, 1, t))
                strokeSegment(point(0, t, 0), point(1, t, 0))
            }
            ctx.restore()

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
                        gain: (z00 + z10 + z11 + z01) * 0.25,
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
                            root.morphOutputColor(root.morphOutputId), 1.0)
            }

            ctx.save()
            ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.27)
            ctx.lineWidth = 0.8
            ctx.setLineDash([])

            for (let yi = 0; yi <= yResolution; ++yi) {
                const yNorm = yi / yResolution
                ctx.beginPath()
                for (let xi = 0; xi <= xResolution; ++xi) {
                    const xNorm = xi / xResolution
                    const p = point(xNorm, yNorm,
                                    root.gainAt(root.noteForNorm(xNorm),
                                                root.velocityForNorm(yNorm)))
                    if (xi === 0)
                        ctx.moveTo(p.x, p.y)
                    else
                        ctx.lineTo(p.x, p.y)
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
                    if (yi === 0)
                        ctx.moveTo(p.x, p.y)
                    else
                        ctx.lineTo(p.x, p.y)
                }
                ctx.stroke()
            }
            ctx.restore()

            ctx.save()
            ctx.strokeStyle = Qt.rgba(0.85, 0.88, 0.92, 0.28)
            ctx.lineWidth = 1
            strokeSegment(base00, base10)
            strokeSegment(base10, base11)
            strokeSegment(base11, base01)
            strokeSegment(base01, base00)
            ctx.restore()

            ctx.save()
            ctx.fillStyle = Theme.secondaryText
            ctx.font = "9px sans-serif"
            ctx.textBaseline = "middle"
            ctx.textAlign = "center"
            ctx.fillText("Low", base00.x + 6, base00.y + 15)
            ctx.fillText("High", base10.x, base10.y + 15)
            ctx.textAlign = "right"
            ctx.fillText("Soft", base00.x - 5, base00.y + 1)
            ctx.fillText("Loud", base01.x - 5, base01.y)
            ctx.restore()
        }
    }

    Canvas {
        id: noteCanvas
        anchors.fill: sceneCanvas

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            const trackNumber = root.firstTrackForOutput()
            if (trackNumber <= 0)
                return

            for (let i = 0; i < root.feedbackNotes.length; ++i) {
                const noteData = root.feedbackNotes[i]
                const xNorm = root.maxNote > root.minNote
                            ? root.clamp((noteData.note - root.minNote)
                                         / (root.maxNote - root.minNote),
                                         0.0, 1.0)
                            : 0.0
                const yNorm = root.clamp(noteData.velocity / 127.0, 0.0, 1.0)
                const p = root.projectPoint(
                              xNorm,
                              yNorm,
                              root.gainAt(noteData.note, noteData.velocity))

                ctx.save()
                ctx.fillStyle = "#e53935"
                ctx.strokeStyle = "#ffffff"
                ctx.lineWidth = 1
                ctx.beginPath()
                ctx.arc(p.x, p.y, 7.5, 0, Math.PI * 2)
                ctx.fill()
                ctx.stroke()

                ctx.fillStyle = "#ffffff"
                ctx.font = "bold 9px sans-serif"
                ctx.textAlign = "center"
                ctx.textBaseline = "middle"
                ctx.fillText(String(trackNumber), p.x, p.y + 0.5)
                ctx.restore()
            }
        }
    }

    Connections {
        target: SettingsController
        function onSurfaceKeyboardRangeChanged() {
            root.invalidateProjection()
            root.schedulePaint()
        }
    }

    Connections {
        target: SettingsController.morphOutputStateModel
        function onDataChanged() {
            root.assignmentRevision += 1
            noteCanvas.requestPaint()
        }
        function onModelReset() {
            root.assignmentRevision += 1
            noteCanvas.requestPaint()
        }
    }

    Component.onCompleted: root.schedulePaint()

    onWidthChanged: {
        root.invalidateProjection()
        root.schedulePaint()
    }

    onHeightChanged: {
        root.invalidateProjection()
        root.schedulePaint()
    }

    onMorphOutputIdChanged: root.schedulePaint()

    onSurfaceRevisionChanged: {
        sceneCanvas.requestPaint()
        noteCanvas.requestPaint()
    }
}
