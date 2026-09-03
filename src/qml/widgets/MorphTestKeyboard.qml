import QtQuick

import MorphMaster

Item {
    id: root

    // Desktop/tablet can switch between the fixed 88-key view and a view
    // adapted to the configured range. Phone keeps its paged 28-note view.
    property int displayMinNote: 21
    property int displayMaxNote: 108
    property int activeMinNote: SettingsController.surfaceMinNote
    property int activeMaxNote: SettingsController.surfaceMaxNote
    property bool phoneLayout: false
    property bool adaptiveRange: false
    property int phonePage: 0
    property var activeNotes: []

    signal notePressed(int note, int velocity)
    signal noteReleased(int note)

    readonly property bool configuredRangeIsFull:
        activeMinNote === 0 && activeMaxNote === 127
    readonly property int configuredKeyCount:
        Math.max(1, activeMaxNote - activeMinNote + 1)
    readonly property bool useConfiguredDesktopRange:
        !phoneLayout && adaptiveRange && !configuredRangeIsFull
    readonly property int desktopMinNote:
        useConfiguredDesktopRange ? activeMinNote : displayMinNote
    readonly property int desktopMaxNote:
        useConfiguredDesktopRange ? activeMaxNote : displayMaxNote
    readonly property int desktopRowCount:
        useConfiguredDesktopRange && configuredKeyCount <= 49 ? 1 : 2
    readonly property int desktopWhiteKeyCount:
        whiteCountInRange(desktopMinNote, desktopMaxNote)
    readonly property int desktopRowWhiteKeyCount:
        desktopRowCount === 2
            ? Math.ceil(desktopWhiteKeyCount / 2)
            : desktopWhiteKeyCount
    readonly property bool desktopDuplicatesSplitWhite:
        desktopRowCount === 2 && (desktopWhiteKeyCount % 2) !== 0
    readonly property int desktopSplitWhiteNote:
        desktopRowCount === 2
            ? whiteNoteAtIndex(desktopMinNote,
                               desktopMaxNote,
                               Math.floor(desktopWhiteKeyCount / 2))
            : desktopMaxNote + 1
    readonly property int desktopSecondRowFirstNote:
        desktopRowCount === 2
            ? desktopSplitWhiteNote
            : desktopMaxNote + 1
    readonly property int desktopUpperLastNote:
        desktopRowCount !== 2
            ? desktopMaxNote
            : (desktopDuplicatesSplitWhite
               ? desktopSplitWhiteNote
               : desktopSecondRowFirstNote - 1)
    readonly property int desktopGhostNote:
        desktopDuplicatesSplitWhite ? desktopSplitWhiteNote : -1
    readonly property real desktopBoundaryExtraWhiteSlots:
        desktopRowCount === 2 && isBlackNote(desktopUpperLastNote)
            ? 0.31
            : 0.0
    readonly property real desktopSharedWhiteWidth:
        desktopRowCount === 2
            ? width / Math.max(1.0,
                               desktopRowWhiteKeyCount
                               + desktopBoundaryExtraWhiteSlots)
            : 0.0
    readonly property real desktopTwoRowKeyHeight:
        Math.max(1.0, (height - 4.0) / 2.0)
    readonly property real desktopSingleRowHeight:
        configuredKeyCount >= 49
            ? desktopTwoRowKeyHeight
            : (configuredKeyCount >= 37
               ? Math.min(height, desktopTwoRowKeyHeight * 1.5)
               : height)

    readonly property int phonePageCount: 6
    readonly property int phoneFirstNote: 21 + phonePage * 12
    readonly property int phoneLastNote: phoneFirstNote + 27

    function isBlackNote(note) {
        const pitchClass = ((note % 12) + 12) % 12
        return pitchClass === 1 || pitchClass === 3
                || pitchClass === 6 || pitchClass === 8
                || pitchClass === 10
    }

    function whiteCountInRange(firstNote, lastNote) {
        let count = 0
        for (let note = firstNote; note <= lastNote; ++note) {
            if (!isBlackNote(note))
                ++count
        }
        return Math.max(1, count)
    }

    function whiteNoteAtIndex(firstNote, lastNote, index) {
        let currentIndex = 0
        for (let note = firstNote; note <= lastNote; ++note) {
            if (isBlackNote(note))
                continue
            if (currentIndex === index)
                return note
            ++currentIndex
        }
        return lastNote
    }

    function releaseAllPointers() {
        upperStrip.releaseAllPointers()
        lowerStrip.releaseAllPointers()
        phoneStrip.releaseAllPointers()
    }

    function setPhonePage(page) {
        phoneStrip.releaseAllPointers()
        phonePage = Math.max(0, Math.min(phonePageCount - 1, page))
    }

    function noteName(note) {
        const names = ["C", "C♯", "D", "D♯", "E", "F",
                       "F♯", "G", "G♯", "A", "A♯", "B"]
        const pitchClass = ((note % 12) + 12) % 12
        const octave = Math.floor(note / 12) - 1
        return names[pitchClass] + octave
    }

    component KeyboardStrip: Canvas {
        id: strip

        property int firstNote: 21
        property int lastNote: 64
        property int ghostNote: -1
        property real whiteKeyWidthOverride: 0.0
        property bool feedbackActive: true
        property var pointerNotes: ({})

        function isBlack(note) {
            return root.isBlackNote(note)
        }

        function isActive(note) {
            return note !== ghostNote
                    && note >= root.activeMinNote
                    && note <= root.activeMaxNote
        }

        function isFeedbackPressed(note) {
            if (note === ghostNote)
                return false
            for (let i = 0; i < root.activeNotes.length; ++i) {
                if (Number(root.activeNotes[i].note) === note)
                    return true
            }
            return false
        }

        function isLocallyPressed(note) {
            const keys = Object.keys(pointerNotes)
            for (let i = 0; i < keys.length; ++i) {
                if (Number(pointerNotes[keys[i]]) === note)
                    return true
            }
            return false
        }

        function isPressed(note) {
            return isFeedbackPressed(note) || isLocallyPressed(note)
        }

        function whiteCount() {
            let count = 0
            for (let note = firstNote; note <= lastNote; ++note) {
                if (!isBlack(note))
                    ++count
            }
            return Math.max(1, count)
        }

        function whiteBefore(note) {
            let count = 0
            for (let current = firstNote; current < note; ++current) {
                if (!isBlack(current))
                    ++count
            }
            return count
        }

        function whiteNoteAt(index) {
            let currentIndex = 0
            for (let note = firstNote; note <= lastNote; ++note) {
                if (isBlack(note))
                    continue
                if (currentIndex === index)
                    return note
                ++currentIndex
            }
            return -1
        }

        function effectiveWhiteWidth() {
            return whiteKeyWidthOverride > 0.0
                    ? whiteKeyWidthOverride
                    : width / whiteCount()
        }

        function leadingBlackSlots() {
            return isBlack(firstNote) ? 0.31 : 0.0
        }

        function trailingBlackSlots() {
            return isBlack(lastNote) ? 0.31 : 0.0
        }

        function contentOffsetX() {
            if (whiteKeyWidthOverride <= 0.0)
                return 0.0

            const whiteWidth = effectiveWhiteWidth()
            const occupiedSlots = whiteCount()
                                  + leadingBlackSlots()
                                  + trailingBlackSlots()
            const centeredMargin = Math.max(
                        0.0,
                        (width - occupiedSlots * whiteWidth) / 2.0)
            return centeredMargin + leadingBlackSlots() * whiteWidth
        }

        function noteAt(x, y) {
            if (width <= 0 || height <= 0
                    || x < 0 || x >= width || y < 0 || y >= height)
                return -1

            const whiteWidth = effectiveWhiteWidth()
            const xOffset = contentOffsetX()
            const blackWidth = Math.max(2, whiteWidth * 0.62)
            const blackHeight = height * 0.62

            // Black keys must win the hit test where they overlap white keys.
            if (y >= 0 && y <= blackHeight) {
                for (let note = firstNote; note <= lastNote; ++note) {
                    if (!isBlack(note))
                        continue
                    const boundary = xOffset
                                     + whiteBefore(note) * whiteWidth
                    const blackX = boundary - blackWidth * 0.5
                    if (x >= blackX && x < blackX + blackWidth)
                        return isActive(note) ? note : -1
                }
            }

            const localX = x - xOffset
            const whiteContentWidth = whiteCount() * whiteWidth
            if (localX < 0.0 || localX >= whiteContentWidth)
                return -1

            const whiteIndex = Math.max(
                        0,
                        Math.min(whiteCount() - 1,
                                 Math.floor(localX
                                            / Math.max(0.001, whiteWidth))))
            const note = whiteNoteAt(whiteIndex)
            return note >= 0 && isActive(note) ? note : -1
        }

        function velocityAt(note, y) {
            const keyHeight = isBlack(note) ? height * 0.62 : height
            const normalized = Math.max(0.0,
                                        Math.min(1.0,
                                                 y / Math.max(1.0, keyHeight)))
            return Math.max(1, Math.min(127,
                                        Math.round(1 + normalized * 126)))
        }

        function pointerKey(pointerId) {
            return String(pointerId)
        }

        function pointerCountForNote(note, excludedKey) {
            const keys = Object.keys(pointerNotes)
            let count = 0
            for (let i = 0; i < keys.length; ++i) {
                const key = keys[i]
                if (key === excludedKey)
                    continue
                if (Number(pointerNotes[key]) === note)
                    ++count
            }
            return count
        }

        function beginPointer(pointerId, x, y) {
            const key = pointerKey(pointerId)
            const note = noteAt(x, y)
            if (note < 0)
                return

            const next = Object.assign({}, pointerNotes)
            next[key] = note
            pointerNotes = next

            if (pointerCountForNote(note, key) === 0)
                root.notePressed(note, velocityAt(note, y))

            requestPaint()
        }

        function movePointer(pointerId, x, y) {
            const key = pointerKey(pointerId)
            const oldNote = pointerNotes[key] === undefined
                          ? -1
                          : Number(pointerNotes[key])
            const newNote = noteAt(x, y)

            if (newNote === oldNote)
                return

            const next = Object.assign({}, pointerNotes)
            delete next[key]
            pointerNotes = next

            if (oldNote >= 0 && pointerCountForNote(oldNote, "") === 0)
                root.noteReleased(oldNote)

            if (newNote >= 0) {
                const alreadyHeld = pointerCountForNote(newNote, "") > 0
                const withNewNote = Object.assign({}, pointerNotes)
                withNewNote[key] = newNote
                pointerNotes = withNewNote
                if (!alreadyHeld)
                    root.notePressed(newNote, velocityAt(newNote, y))
            }

            requestPaint()
        }

        function endPointer(pointerId) {
            const key = pointerKey(pointerId)
            if (pointerNotes[key] === undefined)
                return

            const note = Number(pointerNotes[key])
            const next = Object.assign({}, pointerNotes)
            delete next[key]
            pointerNotes = next

            if (pointerCountForNote(note, "") === 0)
                root.noteReleased(note)

            requestPaint()
        }

        function releaseAllPointers() {
            const notes = []
            const keys = Object.keys(pointerNotes)
            for (let i = 0; i < keys.length; ++i) {
                const note = Number(pointerNotes[keys[i]])
                if (notes.indexOf(note) < 0)
                    notes.push(note)
            }

            pointerNotes = ({})
            for (let i = 0; i < notes.length; ++i)
                root.noteReleased(notes[i])
            requestPaint()
        }

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            const whiteWidth = effectiveWhiteWidth()
            const xOffset = contentOffsetX()
            const blackWidth = Math.max(2, whiteWidth * 0.62)
            const blackHeight = height * 0.62

            ctx.strokeStyle = "#252525"
            ctx.lineWidth = 1

            let whiteIndex = 0
            for (let note = firstNote; note <= lastNote; ++note) {
                if (isBlack(note))
                    continue

                const x = xOffset + whiteIndex * whiteWidth
                ctx.fillStyle = isPressed(note)
                                ? "#e53935"
                                : (isActive(note) ? "#f0f0ec" : "#777772")
                ctx.fillRect(x, 0, whiteWidth + 0.5, height)
                ctx.strokeRect(x + 0.5, 0.5,
                               Math.max(0, whiteWidth - 1),
                               Math.max(0, height - 1))
                ++whiteIndex
            }

            for (let note = firstNote; note <= lastNote; ++note) {
                if (!isBlack(note))
                    continue

                const boundary = xOffset + whiteBefore(note) * whiteWidth
                const x = boundary - blackWidth * 0.5
                ctx.fillStyle = isPressed(note)
                                ? Qt.darker("#e53935", 1.35)
                                : (isActive(note) ? "#171717" : "#555555")
                ctx.fillRect(x, 0, blackWidth, blackHeight)
                ctx.strokeRect(x + 0.5, 0.5,
                               Math.max(0, blackWidth - 1),
                               Math.max(0, blackHeight - 1))
            }

            // Sparse C labels, only when there is enough room.
            if (whiteWidth >= 8) {
                ctx.font = Math.max(7, Math.min(10, whiteWidth * 0.55))
                           + "px sans-serif"
                ctx.textAlign = "center"
                ctx.textBaseline = "bottom"
                for (let note = firstNote; note <= lastNote; ++note) {
                    if (((note % 12) + 12) % 12 !== 0)
                        continue
                    const x = xOffset
                              + (whiteBefore(note) + 0.5) * whiteWidth
                    ctx.fillStyle = isPressed(note)
                                    ? "#ffffff"
                                    : (isActive(note) ? "#555555" : "#b8b8b2")
                    ctx.fillText(root.noteName(note), x, height - 3)
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            enabled: strip.feedbackActive
            acceptedButtons: Qt.LeftButton
            preventStealing: true

            onPressed: function(mouse) {
                strip.beginPointer("mouse", mouse.x, mouse.y)
            }

            onPositionChanged: function(mouse) {
                if (pressed)
                    strip.movePointer("mouse", mouse.x, mouse.y)
            }

            onReleased: function(mouse) {
                strip.endPointer("mouse")
            }

            onCanceled: strip.endPointer("mouse")
        }


        MultiPointTouchArea {
            anchors.fill: parent
            enabled: strip.feedbackActive
            minimumTouchPoints: 1
            maximumTouchPoints: 10
            mouseEnabled: false

            onPressed: function(points) {
                for (let i = 0; i < points.length; ++i)
                    strip.beginPointer(points[i].pointId, points[i].x, points[i].y)
            }

            onUpdated: function(points) {
                for (let i = 0; i < points.length; ++i)
                    strip.movePointer(points[i].pointId, points[i].x, points[i].y)
            }

            onReleased: function(points) {
                for (let i = 0; i < points.length; ++i)
                    strip.endPointer(points[i].pointId)
            }

            onCanceled: function(points) {
                for (let i = 0; i < points.length; ++i)
                    strip.endPointer(points[i].pointId)
            }
        }

        Component.onCompleted: requestPaint()
        Component.onDestruction: releaseAllPointers()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onFirstNoteChanged: {
            releaseAllPointers()
            requestPaint()
        }
        onLastNoteChanged: {
            releaseAllPointers()
            requestPaint()
        }
        onGhostNoteChanged: {
            releaseAllPointers()
            requestPaint()
        }
        onWhiteKeyWidthOverrideChanged: requestPaint()
        onVisibleChanged: {
            if (!visible)
                releaseAllPointers()
        }
        onFeedbackActiveChanged: {
            if (!feedbackActive)
                releaseAllPointers()
            else
                requestPaint()
        }

        Connections {
            target: root

            function onActiveMinNoteChanged() {
                strip.releaseAllPointers()
                if (strip.feedbackActive)
                    strip.requestPaint()
            }
            function onActiveMaxNoteChanged() {
                strip.releaseAllPointers()
                if (strip.feedbackActive)
                    strip.requestPaint()
            }
            function onActiveNotesChanged() {
                if (strip.feedbackActive)
                    strip.requestPaint()
            }
        }
    }

    Item {
        id: desktopKeyboard

        anchors.fill: parent
        visible: !root.phoneLayout

        KeyboardStrip {
            id: upperStrip

            x: 0
            y: root.desktopRowCount === 2
               ? 0
               : Math.max(0, (parent.height - height) / 2)
            width: parent.width
            height: root.desktopRowCount === 2
                    ? root.desktopTwoRowKeyHeight
                    : root.desktopSingleRowHeight
            firstNote: root.desktopMinNote
            lastNote: root.desktopRowCount === 2
                      ? root.desktopUpperLastNote
                      : root.desktopMaxNote
            whiteKeyWidthOverride: root.desktopRowCount === 2
                                   ? root.desktopSharedWhiteWidth
                                   : 0.0
            feedbackActive: !root.phoneLayout
        }

        KeyboardStrip {
            id: lowerStrip

            x: 0
            y: upperStrip.height + 4
            width: parent.width
            height: root.desktopTwoRowKeyHeight
            visible: root.desktopRowCount === 2
            firstNote: root.desktopSecondRowFirstNote
            lastNote: root.desktopMaxNote
            ghostNote: root.desktopGhostNote
            whiteKeyWidthOverride: root.desktopSharedWhiteWidth
            feedbackActive: !root.phoneLayout && visible
        }
    }

    KeyboardStrip {
        id: phoneStrip
        anchors.fill: parent
        visible: root.phoneLayout
        firstNote: root.phoneFirstNote
        lastNote: root.phoneLastNote
        feedbackActive: root.phoneLayout
    }
}
