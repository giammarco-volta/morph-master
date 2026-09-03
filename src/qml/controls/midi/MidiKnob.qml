// src/qml/controls/midi/MidiKnob.qml
import QtQuick
import QtQuick.Controls

import MorphMaster

Item {
    id: root

    property string title: ""

    property int from: 0
    property int to: 127
    property int value: 0
    property int stepSize: 1

    /*
     * Pointer travel required to cover the entire numeric range.
     * The value is always calculated relative to the press position
     * and to the value present when the drag started.
     */
    property real dragPixelsForFullRange: 160

    readonly property int layoutClass:
        ApplicationWindow.window
            ? ApplicationWindow.window.layoutClass
            : UiMetrics.Desktop

    /*
     * Minimum finger travel for one numeric step.
     *
     * Touch layouts deliberately use a coarser response than desktop.
     * dragPixelsForFullRange is retained for compatibility: when it
     * requests an even coarser response, the coarser value wins.
     */
    property real dragPixelsPerStep:
        layoutClass === UiMetrics.Phone
        || layoutClass === UiMetrics.Tablet
            ? 4
            : 3

    /*
     * Initial movement ignored before numeric editing begins.
     */
    property real dragDeadZonePixels:
        layoutClass === UiMetrics.Phone
        || layoutClass === UiMetrics.Tablet
            ? 2
            : 0

    property real preferredKnobDiameter: 76
    property bool showTitle: true
    property bool showValue: true

    readonly property bool bipolar:
        from < 0 && to > 0

    readonly property real normalizedValue:
        to === from
            ? 0
            : Math.max(
                  0,
                  Math.min(1, (value - from) / (to - from)))

    readonly property real normalizedZero:
        !bipolar || to === from
            ? 0
            : Math.max(
                  0,
                  Math.min(1, (0 - from) / (to - from)))

    readonly property real actualKnobDiameter:
        width > 0
            ? Math.min(preferredKnobDiameter, width)
            : preferredKnobDiameter

    signal editingStarted()
    signal editingFinished()

    implicitWidth: 96
    implicitHeight:
        (showTitle ? titleLabel.implicitHeight + 4 : 0)
        + preferredKnobDiameter

    activeFocusOnTab: true

    function clampAndSnap(candidate) {
        const lower = Math.min(from, to)
        const upper = Math.max(from, to)
        const step = Math.max(1, Math.abs(stepSize))

        const snapped =
            from + Math.round((candidate - from) / step) * step

        return Math.round(
                    Math.max(lower, Math.min(upper, snapped)))
    }

    function setValue(candidate) {
        const newValue = clampAndSnap(candidate)

        if (value !== newValue)
            value = newValue
    }

    function adjustBy(delta) {
        setValue(value + delta)
    }

    function applyRelativeDrag(deltaX, deltaY, initialValue) {
        /*
         * Increase: maximum of upward and rightward travel.
         * Decrease: maximum of downward and leftward travel.
         *
         * No axis is selected or locked. Opposing diagonal components
         * subtract from one another.
         */
        const increasePixels =
            Math.max(0, -deltaY, deltaX)

        const decreasePixels =
            Math.max(0, deltaY, -deltaX)

        const effectivePixels =
            increasePixels - decreasePixels

        const numericRange =
            Math.abs(to - from)

        const step =
            Math.max(1, Math.abs(stepSize))

        if (numericRange <= 0)
            return

        /*
         * Existing dragPixelsForFullRange overrides remain meaningful,
         * but cannot make the control more sensitive than
         * dragPixelsPerStep.
         */
        const numericStepCount =
            numericRange / step

        const fullRangePixelsPerStep =
            dragPixelsForFullRange > 0
            && numericStepCount > 0
                ? dragPixelsForFullRange
                  / numericStepCount
                : 0

        const pixelsPerStep =
            Math.max(
                0.1,
                dragPixelsPerStep,
                fullRangePixelsPerStep)

        const deadZone =
            Math.max(0, dragDeadZonePixels)

        let activePixels = 0

        if (effectivePixels > deadZone)
            activePixels = effectivePixels - deadZone
        else if (effectivePixels < -deadZone)
            activePixels = effectivePixels + deadZone

        /*
         * Use truncation toward zero so every complete pixelsPerStep
         * interval corresponds to exactly one numeric step.
         */
        const draggedSteps =
            activePixels >= 0
                ? Math.floor(activePixels / pixelsPerStep)
                : Math.ceil(activePixels / pixelsPerStep)

        setValue(
            initialValue
            + draggedSteps * step)
    }

    function increment() {
        adjustBy(Math.max(1, Math.abs(stepSize)))
    }

    function decrement() {
        adjustBy(-Math.max(1, Math.abs(stepSize)))
    }

    onValueChanged: arcCanvas.requestPaint()
    onFromChanged: arcCanvas.requestPaint()
    onToChanged: arcCanvas.requestPaint()
    onEnabledChanged: arcCanvas.requestPaint()

    Keys.onUpPressed: function(event) {
        root.increment()
        event.accepted = true
    }

    Keys.onRightPressed: function(event) {
        root.increment()
        event.accepted = true
    }

    Keys.onDownPressed: function(event) {
        root.decrement()
        event.accepted = true
    }

    Keys.onLeftPressed: function(event) {
        root.decrement()
        event.accepted = true
    }

    Label {
        id: titleLabel

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        visible: root.showTitle
        height: visible ? implicitHeight : 0

        text: root.title
        color: root.enabled
               ? Theme.secondaryText
               : Theme.disabledText

        font.pixelSize: Theme.labelFontSize
        font.bold: true

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        elide: Text.ElideRight
    }

    Item {
        id: knobArea

        anchors.top:
            root.showTitle
                ? titleLabel.bottom
                : parent.top

        anchors.topMargin:
            root.showTitle ? 4 : 0

        anchors.horizontalCenter: parent.horizontalCenter

        width: root.actualKnobDiameter
        height: width

        Canvas {
            id: arcCanvas

            anchors.fill: parent

            readonly property real startAngle:
                3 * Math.PI / 4

            readonly property real sweepAngle:
                3 * Math.PI / 2

            readonly property real arcWidth:
                Math.max(5, width * 0.075)

            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            onPaint: {
                const ctx = getContext("2d")

                ctx.clearRect(0, 0, width, height)

                const centerX = width / 2
                const centerY = height / 2
                const radius =
                    Math.max(
                        0,
                        width / 2
                        - arcWidth / 2
                        - 2)

                const endAngle =
                    startAngle + sweepAngle

                ctx.lineWidth = arcWidth
                ctx.lineCap = "round"

                /*
                 * Complete background track.
                 */
                ctx.strokeStyle =
                    root.enabled
                        ? Theme.border
                        : Theme.disabledText

                ctx.beginPath()
                ctx.arc(
                    centerX,
                    centerY,
                    radius,
                    startAngle,
                    endAngle,
                    false)
                ctx.stroke()

                /*
                 * Active value arc.
                 *
                 * Unipolar: from minimum to current value.
                 * Bipolar:  from zero to current value.
                 */
                const valueAngle =
                    startAngle
                    + sweepAngle * root.normalizedValue

                let activeStart = startAngle
                let activeEnd = valueAngle

                if (root.bipolar) {
                    const zeroAngle =
                        startAngle
                        + sweepAngle * root.normalizedZero

                    activeStart =
                        Math.min(zeroAngle, valueAngle)

                    activeEnd =
                        Math.max(zeroAngle, valueAngle)
                }

                if (activeEnd - activeStart > 0.0001) {
                    ctx.strokeStyle =
                        root.enabled
                            ? Theme.accent
                            : Theme.disabledText

                    ctx.beginPath()
                    ctx.arc(
                        centerX,
                        centerY,
                        radius,
                        activeStart,
                        activeEnd,
                        false)
                    ctx.stroke()
                }

                /*
                 * Zero marker for bipolar controls.
                 */
                if (root.bipolar) {
                    const zeroAngle =
                        startAngle
                        + sweepAngle * root.normalizedZero

                    const innerRadius =
                        Math.max(0, radius - arcWidth / 2 - 2)

                    const outerRadius =
                        radius + arcWidth / 2 + 1

                    ctx.strokeStyle =
                        root.enabled
                            ? Theme.secondaryText
                            : Theme.disabledText

                    ctx.lineWidth = 1.5
                    ctx.lineCap = "butt"

                    ctx.beginPath()
                    ctx.moveTo(
                        centerX
                        + Math.cos(zeroAngle) * innerRadius,
                        centerY
                        + Math.sin(zeroAngle) * innerRadius)

                    ctx.lineTo(
                        centerX
                        + Math.cos(zeroAngle) * outerRadius,
                        centerY
                        + Math.sin(zeroAngle) * outerRadius)

                    ctx.stroke()
                }
            }
        }

        Rectangle {
            id: dialBody

            anchors.centerIn: parent

            width: Math.max(0, parent.width - 20)
            height: width
            radius: width / 2

            color:
                interactionArea.pressed
                    ? Theme.panel
                    : Theme.panelDark

            border.width:
                root.activeFocus ? 2 : 1

            border.color:
                root.activeFocus
                    ? Theme.accent
                    : Theme.border
        }

        Text {
            anchors.centerIn: dialBody

            visible: root.showValue

            text: root.value
            color: root.enabled
                   ? Theme.text
                   : Theme.disabledText

            font.pixelSize: Theme.controlFontSize
            font.bold: true

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        MouseArea {
            id: interactionArea

            anchors.fill: parent

            enabled: root.enabled
            hoverEnabled: true
            preventStealing: true

            cursorShape:
                pressed
                    ? Qt.ClosedHandCursor
                    : Qt.OpenHandCursor

            property real pressX: 0
            property real pressY: 0
            property int pressValue: 0

            onPressed: function(mouse) {
                root.forceActiveFocus()

                pressX = mouse.x
                pressY = mouse.y
                pressValue = root.value

                root.editingStarted()
            }

            onPositionChanged: function(mouse) {
                if (!pressed)
                    return

                root.applyRelativeDrag(
                    mouse.x - pressX,
                    mouse.y - pressY,
                    pressValue)
            }

            onReleased: {
                root.editingFinished()
            }

            onCanceled: {
                root.editingFinished()
            }

            onWheel: function(wheel) {
                const vertical = wheel.angleDelta.y
                const horizontal = wheel.angleDelta.x
                const delta =
                    Math.abs(vertical) >= Math.abs(horizontal)
                        ? vertical
                        : horizontal

                if (delta > 0)
                    root.increment()
                else if (delta < 0)
                    root.decrement()

                wheel.accepted = true
            }
        }
    }
}
