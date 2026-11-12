import QtQuick
import MorphMaster

Item {
    id: root

    property real x1: 36
    property int y1: 0
    property real x2: 84
    property int y2: 100

    property int minX: 0
    property int maxX: 127

    property bool noteMode: false
    property bool showXLabels: true
    property bool showYLabels: true

    // The two equal-power curves have equal visual importance. Each one
    // uses the color associated with its Morph Output.
    property int primaryMorphOutputId: -1
    property int complementaryMorphOutputId: -1
    property int assignmentRevision: 0
    property var feedbackNotes: []

    property real handleRadius: 5
    property real handleHitRadius: 24

    readonly property real yLabelRightMargin:
        root.showYLabels
            ? Math.ceil(yLabelMetrics.advanceWidth) + 18
            : 12

    signal curveEdited()
    signal editingStarted()
    signal editingFinished()

    implicitHeight: 150
    clip: true

    property int dragTarget: 0 // 0 none, 1 first point, 2 second point

    function clamp(value, lo, hi) {
        return Math.max(lo, Math.min(hi, Math.round(value)))
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

    function graphRect() {
        const leftMargin = 12
        const topMargin = root.showXLabels ? 24 : 12
        const rightMargin = root.yLabelRightMargin
        const bottomMargin = 12

        return {
            left: leftMargin,
            top: topMargin,
            right: root.width - rightMargin,
            bottom: root.height - bottomMargin,
            width: Math.max(1, root.width - leftMargin - rightMargin),
            height: Math.max(1, root.height - topMargin - bottomMargin)
        }
    }

    function schedulePaint() {
        Qt.callLater(function() {
            canvas.requestPaint()
        })
    }

    function visibleXMarks() {
        let marks = []

        if (root.noteMode) {
            const baseMarks = [12, 24, 36, 48, 60, 72, 84, 96, 108, 120]

            for (let i = 0; i < baseMarks.length; ++i) {
                const v = baseMarks[i]
                if (v >= root.minX && v <= root.maxX)
                    marks.push(v)
            }
        } else {
            function addIfMissing(v) {
                if (marks.indexOf(v) < 0)
                    marks.push(v)
            }

            addIfMissing(root.minX)

            const baseMarks = [0, 32, 64, 96, 127]

            for (let i = 0; i < baseMarks.length; ++i) {
                const v = baseMarks[i]
                if (v >= root.minX && v <= root.maxX)
                    addIfMissing(v)
            }

            addIfMissing(root.maxX)
        }

        marks.sort(function(a, b) { return a - b })
        return marks
    }

    function noteLabel(value) {
        return "C" + Math.round((value - 12) / 12)
    }

    function curveToPixel(x, y) {
        const r = graphRect()

        const xNorm = (x - root.minX) / Math.max(1, root.maxX - root.minX)
        const yNorm = y / 100.0

        return {
            x: r.left + xNorm * r.width,
            y: r.bottom - yNorm * r.height
        }
    }

    /*
     * PiecewiseCurve stores a linear weight in the 0..100 range.
     * ExpressionCalculator then converts that weight to the actual
     * equal-power gain with sin(weight * pi/2), or cos(...) for the
     * complementary Morph Output.
     */
    function curveWeightAt(x, yA, yB, base) {
        const firstX = root.x1
        const lastX = root.x2
        if (x <= firstX)
            return yA

        if (x >= lastX)
            return yB

        const span = Math.max(1, lastX - firstX)
        const t = (x - firstX) / span

        return yA + (yB - yA) * t
    }

    function gainFromWeight(weight, inverted) {
        const normalizedWeight = Math.max(0, Math.min(1, weight / 100.0))
        const angle = normalizedWeight * Math.PI * 0.5
        const gain = inverted ? Math.cos(angle) : Math.sin(angle)
        return gain * 100.0
    }

    function weightFromGain(gain) {
        const normalizedGain = Math.max(0, Math.min(1, gain / 100.0))
        return Math.asin(normalizedGain) * 2.0 / Math.PI * 100.0
    }

    function displayedCurveValueAt(x, yA, yB, inverted, base) {
        return gainFromWeight(curveWeightAt(x, yA, yB, base), inverted)
    }

    function pixelToCurve(px, py) {
        const r = graphRect()

        const cx = clamp(px, r.left, r.right)
        const cy = clamp(py, r.top, r.bottom)

        const xNorm = (cx - r.left) / Math.max(1, r.width)
        const yNorm = (r.bottom - cy) / Math.max(1, r.height)

        return {
            x: clamp(root.minX + xNorm * (root.maxX - root.minX),
                     root.minX,
                     root.maxX),
            displayedY: Math.max(0, Math.min(100, yNorm * 100.0))
        }
    }

    function hitTest(px, py) {
        let nearestTarget = 0
        let nearestDistanceSquared = Number.POSITIVE_INFINITY

        function testHandle(target, x, y) {
            if (x < root.minX || x > root.maxX)
                return

            const point = root.curveToPixel(x, y)

            const dx = px - point.x
            const dy = py - point.y
            const distanceSquared = dx * dx + dy * dy

            if (distanceSquared < nearestDistanceSquared) {
                nearestDistanceSquared = distanceSquared
                nearestTarget = target
            }
        }

        testHandle(1, root.x1, root.gainFromWeight(root.y1, false))
        testHandle(2, root.x2, root.gainFromWeight(root.y2, false))

        const hitRadiusSquared =
            root.handleHitRadius * root.handleHitRadius

        return nearestDistanceSquared <= hitRadiusSquared
               ? nearestTarget
               : 0
    }

    function updateDrag(px, py) {
        const c = pixelToCurve(px, py)

        if (root.dragTarget === 1) {
            root.x1 = clamp(c.x, root.minX, root.x2 - 1)
            root.y1 = clamp(root.weightFromGain(c.displayedY), 0, 100)
        } else if (root.dragTarget === 2) {
            root.x2 = clamp(c.x, root.x1 + 1, root.maxX)
            root.y2 = clamp(root.weightFromGain(c.displayedY), 0, 100)
        }

        canvas.requestPaint()
        root.curveEdited()
    }

    TextMetrics {
        id: yLabelMetrics

        text: "1.00"

        font.pixelSize: Theme.controlFontSize
        font.family: "sans-serif"
    }

    Canvas {
        id: canvas

        anchors.fill: parent

        onWidthChanged: root.schedulePaint()
        onHeightChanged: root.schedulePaint()

        onPaint: {
            const ctx = getContext("2d")
            const r = root.graphRect()

            ctx.clearRect(0, 0, width, height)

            ctx.fillStyle = Theme.panelDark
            ctx.fillRect(0, 0, width, height)

            ctx.lineWidth = 1
            ctx.setLineDash([])
            ctx.strokeStyle = Theme.border
            ctx.strokeRect(r.left + 0.5,
                           r.top + 0.5,
                           r.width,
                           r.height)

            const xMarks = root.visibleXMarks()
            const yMarks = [0, 25, 50, 75, 100]

            ctx.strokeStyle = "#9a9a9a"
            ctx.lineWidth = 1
            ctx.setLineDash([1, 3])

            for (let i = 0; i < xMarks.length; ++i) {
                const pt = root.curveToPixel(xMarks[i], 0)

                ctx.beginPath()
                ctx.moveTo(pt.x, r.top)
                ctx.lineTo(pt.x, r.bottom)
                ctx.stroke()
            }

            for (let i = 0; i < yMarks.length; ++i) {
                const pt = root.curveToPixel(root.minX, yMarks[i])

                ctx.beginPath()
                ctx.moveTo(r.left, pt.y)
                ctx.lineTo(r.right, pt.y)
                ctx.stroke()
            }

            function drawCurve(yA, yB, inverted, outputId, base) {
                const visibleStart = root.minX
                const visibleEnd = root.maxX
                const sampleCount = Math.max(32, Math.ceil(r.width))

                ctx.setLineDash(base ? [3, 4] : [])
                ctx.strokeStyle = base ? Qt.rgba(1,1,1,0.35) : root.morphOutputColor(outputId)
                ctx.lineWidth = 2.2

                ctx.beginPath()

                for (let i = 0; i <= sampleCount; ++i) {
                    const x = visibleStart
                            + (visibleEnd - visibleStart) * i / sampleCount
                    const y = root.displayedCurveValueAt(
                                  x, yA, yB, inverted, base)
                    const pixel = root.curveToPixel(x, y)

                    if (i === 0)
                        ctx.moveTo(pixel.x, pixel.y)
                    else
                        ctx.lineTo(pixel.x, pixel.y)
                }

                ctx.stroke()
            }

            function drawHandle(x, y) {
                if (x < root.minX || x > root.maxX)
                    return

                const point = root.curveToPixel(x, y)

                ctx.beginPath()
                ctx.arc(
                    point.x,
                    point.y,
                    root.handleRadius,
                    0,
                    Math.PI * 2
                )
                ctx.fill()
                ctx.stroke()
            }

            /*
             * Ritagliamo soltanto le linee della curva.
             */
            ctx.save()

            ctx.beginPath()
            ctx.rect(r.left, r.top, r.width, r.height)
            ctx.clip()


            drawCurve(root.y1, root.y2, false,
                      root.primaryMorphOutputId)
            drawCurve(root.y1, root.y2, true,
                      root.complementaryMorphOutputId)

            ctx.restore()

            /*
             * I pallini vengono disegnati dopo il restore,
             * quindi possono estendersi nei margini del grafico.
             */
            ctx.setLineDash([])
            ctx.fillStyle = "white"
            ctx.strokeStyle = root.morphOutputColor(
                                  root.primaryMorphOutputId)
            ctx.lineWidth = 2

            drawHandle(root.x1, root.gainFromWeight(root.y1, false))
            drawHandle(root.x2, root.gainFromWeight(root.y2, false))

            ctx.fillStyle = Theme.text
            ctx.font = Theme.controlFontSize + "px sans-serif"

            if (root.showXLabels) {
                ctx.textAlign = "center"
                ctx.textBaseline = "bottom"

                for (let i = 0; i < xMarks.length; ++i) {
                    const value = xMarks[i]
                    const pt = root.curveToPixel(value, 127)
                    const text = root.noteMode
                               ? root.noteLabel(value)
                               : String(value)

                    ctx.fillText(text, pt.x, r.top - 4)
                }
            }

            if (root.showYLabels) {
                ctx.textAlign = "right"
                ctx.textBaseline = "middle"

                const labelRight = width - 4

                for (let i = 0; i < yMarks.length; ++i) {
                    const value = yMarks[i]
                    const pt = root.curveToPixel(root.maxX, value)

                    ctx.fillText(
                        (value / 100).toFixed(2),
                        labelRight,
                        pt.y
                    )
                }
            }
        }
    }

    Connections {
        target: SettingsController.morphOutputStateModel

        function onDataChanged() {
            root.assignmentRevision += 1
            canvas.requestPaint()
        }

        function onModelReset() {
            root.assignmentRevision += 1
            canvas.requestPaint()
        }
    }

    // Keep note feedback separate from the curve canvas so incoming MIDI
    // does not redraw the editor's grid and curves on every note event.
    Canvas {
        id: feedbackCanvas
        anchors.fill: parent

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.fillStyle = "#e53935"

            for (let i = 0; i < root.feedbackNotes.length; ++i) {
                const noteData = root.feedbackNotes[i]
                const xValue = Number(root.noteMode ? noteData.note : noteData.velocity)
                if (!isFinite(xValue) || xValue < root.minX || xValue > root.maxX)
                    continue

                for (let curve = 0; curve < 2; ++curve) {
                    const outputId = curve === 0 ? root.primaryMorphOutputId
                                                 : root.complementaryMorphOutputId
                    if (outputId < 0 || (noteData.outputMask !== undefined
                        && !(Number(noteData.outputMask) & (1 << outputId))))
                        continue
                    const gain = root.displayedCurveValueAt(xValue, root.y1, root.y2, curve === 1)
                    const point = root.curveToPixel(xValue, gain)
                    ctx.beginPath()
                    ctx.arc(point.x, point.y, 4.5, 0, Math.PI * 2)
                    ctx.fill()
                }
            }
        }
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        preventStealing: true

        onPressed: function(mouse) {
            root.dragTarget = root.hitTest(mouse.x, mouse.y)

            if (root.dragTarget !== 0) {
                root.editingStarted()
                root.updateDrag(mouse.x, mouse.y)
                mouse.accepted = true
            }
        }

        onPositionChanged: function(mouse) {
            if (root.dragTarget === 0)
                return

            root.updateDrag(mouse.x, mouse.y)
            mouse.accepted = true
        }

        onReleased: function(mouse) {
            const wasEditing = root.dragTarget !== 0
            root.dragTarget = 0

            if (wasEditing)
                root.editingFinished()

            mouse.accepted = true
        }

        onCanceled: {
            const wasEditing = root.dragTarget !== 0
            root.dragTarget = 0

            if (wasEditing)
                root.editingFinished()
        }
    }

    onWidthChanged: root.schedulePaint()
    onHeightChanged: root.schedulePaint()
    onFeedbackNotesChanged: feedbackCanvas.requestPaint()
    onX1Changed: { canvas.requestPaint(); feedbackCanvas.requestPaint() }
    onY1Changed: { canvas.requestPaint(); feedbackCanvas.requestPaint() }
    onX2Changed: { canvas.requestPaint(); feedbackCanvas.requestPaint() }
    onY2Changed: { canvas.requestPaint(); feedbackCanvas.requestPaint() }
    onMinXChanged: { canvas.requestPaint(); feedbackCanvas.requestPaint() }
    onMaxXChanged: { canvas.requestPaint(); feedbackCanvas.requestPaint() }
    onNoteModeChanged: { canvas.requestPaint(); feedbackCanvas.requestPaint() }
    onShowXLabelsChanged: { canvas.requestPaint(); feedbackCanvas.requestPaint() }
    onShowYLabelsChanged: { canvas.requestPaint(); feedbackCanvas.requestPaint() }
    onPrimaryMorphOutputIdChanged: canvas.requestPaint()
    onComplementaryMorphOutputIdChanged: canvas.requestPaint()
    onYLabelRightMarginChanged: { root.schedulePaint(); feedbackCanvas.requestPaint() }

    Component.onCompleted: {
        root.schedulePaint()
    }
}