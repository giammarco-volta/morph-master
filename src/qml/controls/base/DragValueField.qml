// src/qml/controls/base/DragValueField.qml
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
     * As in MidiKnob, the value is always calculated relative to
     * the press position and to the value present when dragging began.
     */
    property real dragPixelsForFullRange: 160

    property bool showTitle: true
    property string prefix: ""
    property string suffix: ""

    property real displayMultiplier: 1.0
    property int displayDecimals: 0

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
            ? 12
            : 8

    /*
     * A long press opens a numeric editor. This remains configurable
     * so specialized uses can disable direct entry if necessary.
     */
    property bool directEntryEnabled: true
    property int longPressDragThreshold: 8

    /*
     * Initial movement ignored before numeric editing begins.
     * This prevents small finger-placement movements from changing
     * the value on touch devices.
     */
    property real dragDeadZonePixels:
        layoutClass === UiMetrics.Phone
        || layoutClass === UiMetrics.Tablet
            ? 2
            : 0

    /*
     * This is the same profile-dependent control height used for the
     * other controls on the page. It can still be overridden locally.
     */
    property real fieldHeight:
        UiMetrics.controlHeight(layoutClass)

    readonly property string displayText:
        prefix
        + (value * displayMultiplier).toFixed(
              Math.max(0, displayDecimals))
        + suffix

    signal editingStarted()
    signal editingFinished()

    property bool editingGestureActive: false

    implicitWidth: 72

    implicitHeight:
        (showTitle ? titleLabel.implicitHeight + 4 : 0)
        + fieldHeight

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

    function beginEditing() {
        if (!editingGestureActive) {
            editingGestureActive = true
            editingStarted()
        }
    }

    function endEditing() {
        if (editingGestureActive) {
            editingGestureActive = false
            editingFinished()
        }
    }

    function displayedValue(number) {
        return (number * displayMultiplier).toFixed(
                    Math.max(0, displayDecimals))
    }

    function openDirectEntry() {
        if (!directEntryEnabled || !enabled)
            return

        directEntryField.text = displayedValue(value)
        directEntryPopup.open()

        Qt.callLater(function() {
            directEntryField.forceActiveFocus()
            directEntryField.selectAll()
        })
    }

    function commitDirectEntry() {
        const normalizedText =
            directEntryField.text.trim().replace(",", ".")
        const displayedCandidate = Number(normalizedText)

        if (!isNaN(displayedCandidate)
                && isFinite(displayedCandidate)
                && displayMultiplier !== 0) {
            setValue(displayedCandidate / displayMultiplier)
            directEntryPopup.close()
        }
    }

    function increment() {
        adjustBy(Math.max(1, Math.abs(stepSize)))
    }

    function decrement() {
        adjustBy(-Math.max(1, Math.abs(stepSize)))
    }

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

        color:
            root.enabled
                ? Theme.secondaryText
                : Theme.disabledText

        font.pixelSize: Theme.labelFontSize
        font.bold: true

        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter

        elide: Text.ElideRight
    }

    Rectangle {
        id: valueFrame

        anchors.left: parent.left
        anchors.right: parent.right

        anchors.top:
            root.showTitle
                ? titleLabel.bottom
                : parent.top

        anchors.topMargin:
            root.showTitle ? 4 : 0

        height: root.fieldHeight

        radius: 5

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

        Text {
            anchors.fill: parent
            anchors.leftMargin: 6
            anchors.rightMargin: 6

            text: root.displayText

            color:
                root.enabled
                    ? Theme.text
                    : Theme.disabledText

            font.pixelSize: Theme.controlFontSize
            font.bold: true

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter

            elide: Text.ElideRight
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
                    : Qt.SizeAllCursor

            property real pressX: 0
            property real pressY: 0
            property int pressValue: 0
            property bool hasDragged: false
            property bool directEntryTriggered: false

            onPressed: function(mouse) {
                root.forceActiveFocus()

                pressX = mouse.x
                pressY = mouse.y
                pressValue = root.value
                hasDragged = false
                directEntryTriggered = false

                root.beginEditing()
            }

            onPositionChanged: function(mouse) {
                if (!pressed || directEntryTriggered)
                    return

                const deltaX = mouse.x - pressX
                const deltaY = mouse.y - pressY

                if (Math.sqrt(deltaX * deltaX + deltaY * deltaY)
                        >= root.longPressDragThreshold)
                    hasDragged = true

                root.applyRelativeDrag(
                    deltaX,
                    deltaY,
                    pressValue)
            }

            onPressAndHold: function(mouse) {
                if (!hasDragged && root.directEntryEnabled) {
                    directEntryTriggered = true
                    root.openDirectEntry()
                }
            }

            onReleased: {
                if (!directEntryTriggered)
                    root.endEditing()
            }

            onCanceled: {
                if (!directEntryPopup.opened)
                    root.endEditing()
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

    Popup {
        id: directEntryPopup

        parent: Overlay.overlay
        anchors.centerIn: parent

        width: Math.min(320, parent ? parent.width - 32 : 320)
        height: directEntryColumn.implicitHeight + 32
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        padding: 0

        onClosed: {
            root.endEditing()
            root.forceActiveFocus()
        }

        background: Rectangle {
            color: Theme.panelDark
            border.width: 1
            border.color: Theme.border
            radius: 8
        }

        contentItem: Item {
            Column {
                id: directEntryColumn

                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Label {
                    width: parent.width
                    text: root.title.length > 0
                          ? root.title
                          : qsTr("Enter value")
                    color: Theme.text
                    font.pixelSize: Theme.controlFontSize
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }

                TextField {
                    id: directEntryField

                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    selectByMouse: true
                    inputMethodHints: Qt.ImhFormattedNumbersOnly

                    validator: DoubleValidator {
                        bottom: Math.min(
                                    root.from * root.displayMultiplier,
                                    root.to * root.displayMultiplier)
                        top: Math.max(
                                 root.from * root.displayMultiplier,
                                 root.to * root.displayMultiplier)
                        decimals: Math.max(0, root.displayDecimals)
                        notation: DoubleValidator.StandardNotation
                    }

                    onAccepted: root.commitDirectEntry()
                }

                Label {
                    width: parent.width
                    text: qsTr("Range: %1 – %2")
                          .arg(root.displayedValue(
                                   Math.min(root.from, root.to)))
                          .arg(root.displayedValue(
                                   Math.max(root.from, root.to)))
                    color: Theme.secondaryText
                    font.pixelSize: Theme.labelFontSize
                    horizontalAlignment: Text.AlignHCenter
                }

                Row {
                    width: parent.width
                    spacing: 8

                    Button {
                        width: (parent.width - parent.spacing) / 2
                        text: qsTr("Cancel")
                        onClicked: directEntryPopup.close()
                    }

                    Button {
                        width: (parent.width - parent.spacing) / 2
                        text: qsTr("OK")
                        enabled: directEntryField.acceptableInput
                        onClicked: root.commitDirectEntry()
                    }
                }
            }
        }
    }
}
