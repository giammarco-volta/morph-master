// src/qml/widgets/SingleTrackEditor.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster
import NaadaLab.Ui as SharedUi

ColumnLayout {
    id: root

    property int trackNumber: 1

    signal morphOutputEditRequested(int morphOutputIndex)

    readonly property var track:
        SettingsController ? SettingsController.track(root.trackNumber) : null

    readonly property bool trackAssigned:
        morphOutputCombo.currentIndex > 0

    /*
     * Morph Output values follow the surface order used by SettingsController:
     * 0 Loud, 1 High and Loud, 2 High, 3 High and Soft,
     * 4 Soft, 5 Low and Soft, 6 Low, 7 Low and Loud, 8 None.
     */

    readonly property int layoutClass:
        ApplicationWindow.window
            ? ApplicationWindow.window.layoutClass
            : UiMetrics.Desktop

    readonly property int rowSpacing:
        UiMetrics.spacing(layoutClass)

    readonly property int horizontalMargin: 8

    property bool updatingFromModel: true
    property bool applyingMorphOutputSelection: false

    property var morphOutputValues: [
        8, // None
        0, // Loud
        1, // High-Loud
        2, // High
        3, // High-Soft
        4, // Soft
        5, // Low-Soft
        6, // Low
        7  // Low-Loud
    ]

    readonly property var footageNames: [
        "16'",
        "8'",
        "5 1/3'",
        "4'",
        "2 2/3'",
        "2'",
        "1 3/5'",
        "1 1/3'",
        "1 1/7'",
        "1'"
    ]

    readonly property var generalMidiProgramNames: [
        "001 Acoustic Grand Piano",
        "002 Bright Acoustic Piano",
        "003 Electric Grand Piano",
        "004 Honky-tonk Piano",
        "005 Electric Piano 1",
        "006 Electric Piano 2",
        "007 Harpsichord",
        "008 Clavi",
        "009 Celesta",
        "010 Glockenspiel",
        "011 Music Box",
        "012 Vibraphone",
        "013 Marimba",
        "014 Xylophone",
        "015 Tubular Bells",
        "016 Dulcimer",
        "017 Drawbar Organ",
        "018 Percussive Organ",
        "019 Rock Organ",
        "020 Church Organ",
        "021 Reed Organ",
        "022 Accordion",
        "023 Harmonica",
        "024 Tango Accordion",
        "025 Acoustic Guitar (nylon)",
        "026 Acoustic Guitar (steel)",
        "027 Electric Guitar (jazz)",
        "028 Electric Guitar (clean)",
        "029 Electric Guitar (muted)",
        "030 Overdriven Guitar",
        "031 Distortion Guitar",
        "032 Guitar Harmonics",
        "033 Acoustic Bass",
        "034 Electric Bass (finger)",
        "035 Electric Bass (pick)",
        "036 Fretless Bass",
        "037 Slap Bass 1",
        "038 Slap Bass 2",
        "039 Synth Bass 1",
        "040 Synth Bass 2",
        "041 Violin",
        "042 Viola",
        "043 Cello",
        "044 Contrabass",
        "045 Tremolo Strings",
        "046 Pizzicato Strings",
        "047 Orchestral Harp",
        "048 Timpani",
        "049 String Ensemble 1",
        "050 String Ensemble 2",
        "051 Synth Strings 1",
        "052 Synth Strings 2",
        "053 Choir Aahs",
        "054 Voice Oohs",
        "055 Synth Voice",
        "056 Orchestra Hit",
        "057 Trumpet",
        "058 Trombone",
        "059 Tuba",
        "060 Muted Trumpet",
        "061 French Horn",
        "062 Brass Section",
        "063 Synth Brass 1",
        "064 Synth Brass 2",
        "065 Soprano Sax",
        "066 Alto Sax",
        "067 Tenor Sax",
        "068 Baritone Sax",
        "069 Oboe",
        "070 English Horn",
        "071 Bassoon",
        "072 Clarinet",
        "073 Piccolo",
        "074 Flute",
        "075 Recorder",
        "076 Pan Flute",
        "077 Blown Bottle",
        "078 Shakuhachi",
        "079 Whistle",
        "080 Ocarina",
        "081 Lead 1 (square)",
        "082 Lead 2 (sawtooth)",
        "083 Lead 3 (calliope)",
        "084 Lead 4 (chiff)",
        "085 Lead 5 (charang)",
        "086 Lead 6 (voice)",
        "087 Lead 7 (fifths)",
        "088 Lead 8 (bass + lead)",
        "089 Pad 1 (new age)",
        "090 Pad 2 (warm)",
        "091 Pad 3 (polysynth)",
        "092 Pad 4 (choir)",
        "093 Pad 5 (bowed)",
        "094 Pad 6 (metallic)",
        "095 Pad 7 (halo)",
        "096 Pad 8 (sweep)",
        "097 FX 1 (rain)",
        "098 FX 2 (soundtrack)",
        "099 FX 3 (crystal)",
        "100 FX 4 (atmosphere)",
        "101 FX 5 (brightness)",
        "102 FX 6 (goblins)",
        "103 FX 7 (echoes)",
        "104 FX 8 (sci-fi)",
        "105 Sitar",
        "106 Banjo",
        "107 Shamisen",
        "108 Koto",
        "109 Kalimba",
        "110 Bag Pipe",
        "111 Fiddle",
        "112 Shanai",
        "113 Tinkle Bell",
        "114 Agogo",
        "115 Steel Drums",
        "116 Woodblock",
        "117 Taiko Drum",
        "118 Melodic Tom",
        "119 Synth Drum",
        "120 Reverse Cymbal",
        "121 Guitar Fret Noise",
        "122 Breath Noise",
        "123 Seashore",
        "124 Bird Tweet",
        "125 Telephone Ring",
        "126 Helicopter",
        "127 Applause",
        "128 Gunshot"
    ]

    spacing: rowSpacing

    function morphOutputIndexFromValue(value) {
        for (let i = 0; i < root.morphOutputValues.length; ++i) {
            if (root.morphOutputValues[i] === value)
                return i
        }

        return 0
    }

    function morphOutputValueFromIndex(index) {
        if (index < 0 || index >= root.morphOutputValues.length)
            return root.morphOutputValues[0]

        return root.morphOutputValues[index]
    }

    function applyMorphOutputSelection(comboIndex) {
        if (!root.track)
            return

        const newMorphOutputValue =
            root.morphOutputValueFromIndex(comboIndex)

        root.applyingMorphOutputSelection = true
        root.updatingFromModel = true

        if (root.track.morphOutput !== newMorphOutputValue)
            root.track.morphOutput = newMorphOutputValue

        root.updatingFromModel = false
        root.applyingMorphOutputSelection = false
        root.syncFromModel()
    }

    function syncFromModel() {
        if (!root.track)
            return

        root.updatingFromModel = true

        morphOutputCombo.currentIndex =
            root.morphOutputIndexFromValue(root.track.morphOutput)

        bankMsbField.value = root.track.bankMSB
        bankLsbField.value = root.track.bankLSB
        manualProgramCombo.currentIndex = root.track.programNumber

        footageCombo.currentIndex = root.track.footage
        detuneOffsetField.value = root.track.detuneOffset
        detuneSpreadField.value = root.track.detuneSpread

        volumeKnob.value = root.track.volume
        panKnob.value = root.track.pan
        reverbKnob.value = root.track.reverb
        chorusKnob.value = root.track.chorus
        toneKnob.value = root.track.tone
        timbreKnob.value = root.track.timbre

        root.updatingFromModel = false
    }

    Component.onCompleted: {
        Qt.callLater(root.syncFromModel)
    }

    onTrackChanged: {
        Qt.callLater(root.syncFromModel)
    }

    Connections {
        target: root.track
        ignoreUnknownSignals: true

        function onMorphOutputChanged() {
            if (!root.applyingMorphOutputSelection)
                root.syncFromModel()
        }

        function onInstrumentProgramDisplayNameChanged() {
            root.syncFromModel()
        }

        function onBankMSBChanged() {
            root.syncFromModel()
        }

        function onBankLSBChanged() {
            root.syncFromModel()
        }

        function onProgramNumberChanged() {
            root.syncFromModel()
        }

        function onFootageChanged() {
            root.syncFromModel()
        }

        function onDetuneOffsetChanged() {
            root.syncFromModel()
        }

        function onDetuneSpreadChanged() {
            root.syncFromModel()
        }

        function onVolumeChanged() {
            root.syncFromModel()
        }

        function onPanChanged() {
            root.syncFromModel()
        }

        function onReverbChanged() {
            root.syncFromModel()
        }

        function onChorusChanged() {
            root.syncFromModel()
        }

        function onToneChanged() {
            root.syncFromModel()
        }

        function onTimbreChanged() {
            root.syncFromModel()
        }
    }

    Connections {
        target: SettingsController
        ignoreUnknownSignals: true

        function onUseInstrumentDefinitionChanged() {
            root.syncFromModel()
        }
    }

    Component {
        id: instrumentProgramBrowserComponent
        InstrumentProgramBrowserPopup { }
    }

    // -----------------------------------------------------------------
    // Row 1: Morph Output and per-track detune.
    //
    // Detune Offset is always applied. Detune Spread is the maximum random
    // deviation around the offset and is displayed as a ± value.
    // -----------------------------------------------------------------
    Item {
        id: morphRow

        Layout.fillWidth: true
        implicitHeight: morphOutputCombo.implicitHeight

        readonly property real contentWidth:
            Math.max(0, width - 2 * root.horizontalMargin)

        readonly property real halfGap:
            root.rowSpacing / 2

        function controlX(column) {
            return root.horizontalMargin
                   + contentWidth * column / 12
                   + (column > 0 ? halfGap : 0)
        }

        function controlWidth(column, columnSpan) {
            const endColumn = column + columnSpan
            const leftInset = column > 0 ? halfGap : 0
            const rightInset = endColumn < 12 ? halfGap : 0

            return Math.max(
                        0,
                        contentWidth * columnSpan / 12
                        - leftInset
                        - rightInset)
        }

        SharedUi.LabeledComboBox {
            id: morphOutputCombo

            x: morphRow.controlX(0)
            width: morphRow.controlWidth(0, 4)
            height: implicitHeight

            title: "Assigned Morph Output"
            modelData: [
                "None",
                "Loud",
                "High and Loud",
                "High",
                "High and Soft",
                "Soft",
                "Low and Soft",
                "Low",
                "Low and Loud"
            ]
            popupMinWidth: 190
            longPressEnabled: true

            onActivated: function(index) {
                if (root.updatingFromModel)
                    return

                root.applyMorphOutputSelection(index)
            }

            onLongPressed: function(currentIndex) {
                const morphOutputIndex =
                    root.morphOutputValueFromIndex(currentIndex)

                if (morphOutputIndex >= 0 && morphOutputIndex < 8)
                    root.morphOutputEditRequested(morphOutputIndex)
            }
        }

        DragValueField {
            id: detuneOffsetField

            x: morphRow.controlX(4)
            width: morphRow.controlWidth(4, 4)
            height: implicitHeight

            title: "Detune Offset"
            from: -50
            to: 50
            value: 0
            suffix: " ct"

            enabled: root.trackAssigned
            opacity: enabled ? 1.0 : 0.45

            onValueChanged: {
                if (root.updatingFromModel || !root.track)
                    return

                if (root.track.detuneOffset !== value)
                    root.track.detuneOffset = value
            }
        }

        DragValueField {
            id: detuneSpreadField

            x: morphRow.controlX(8)
            width: morphRow.controlWidth(8, 4)
            height: implicitHeight

            title: "Detune Spread"
            from: 0
            to: 50
            value: 0
            prefix: "±"
            suffix: " ct"

            enabled: root.trackAssigned
            opacity: enabled ? 1.0 : 0.45

            onValueChanged: {
                if (root.updatingFromModel || !root.track)
                    return

                if (root.track.detuneSpread !== value)
                    root.track.detuneSpread = value
            }
        }
    }

    // -----------------------------------------------------------------
    // Row 2: exact twelve-part geometry, shared by both patch modes.
    //
    // Instrument Definition:
    //   Category 4              | Program 6 | Footage 2
    //
    // Manual:
    //   Bank MSB 2 | Bank LSB 2 | Program 6 | Footage 2
    //
    // The Program and Footage boundaries therefore stay fixed when the
    // patch-selection mode changes.
    // -----------------------------------------------------------------
    Item {
        id: patchRow

        Layout.fillWidth: true

        implicitHeight: Math.max(
                            instrumentProgramSelector.implicitHeight,
                            bankMsbField.implicitHeight,
                            bankLsbField.implicitHeight,
                            manualProgramCombo.implicitHeight,
                            footageCombo.implicitHeight)

        readonly property real contentWidth:
            Math.max(0, width - 2 * root.horizontalMargin)

        readonly property real halfGap:
            root.rowSpacing / 2

        function controlX(column) {
            return root.horizontalMargin
                   + contentWidth * column / 12
                   + (column > 0 ? halfGap : 0)
        }

        function controlWidth(column, columnSpan) {
            const endColumn = column + columnSpan
            const leftInset = column > 0 ? halfGap : 0
            const rightInset = endColumn < 12 ? halfGap : 0

            return Math.max(
                        0,
                        contentWidth * columnSpan / 12
                        - leftInset
                        - rightInset)
        }

        DragValueField {
            id: bankMsbField

            visible: !SettingsController.useInstrumentDefinition

            x: patchRow.controlX(0)
            width: patchRow.controlWidth(0, 2)
            height: implicitHeight

            title: "Bank MSB"

            from: 0
            to: 127
            value: 0

            enabled: root.trackAssigned
            opacity: enabled ? 1.0 : 0.45

            onValueChanged: {
                if (root.updatingFromModel ||
                    SettingsController.useInstrumentDefinition ||
                    !root.track) {
                    return
                }

                if (root.track.bankMSB !== value)
                    root.track.bankMSB = value
            }
        }

        DragValueField {
            id: bankLsbField

            visible: !SettingsController.useInstrumentDefinition

            x: patchRow.controlX(2)
            width: patchRow.controlWidth(2, 2)
            height: implicitHeight

            title: "Bank LSB"

            from: 0
            to: 127
            value: 0

            enabled: root.trackAssigned
            opacity: enabled ? 1.0 : 0.45

            onValueChanged: {
                if (root.updatingFromModel ||
                    SettingsController.useInstrumentDefinition ||
                    !root.track) {
                    return
                }

                if (root.track.bankLSB !== value)
                    root.track.bankLSB = value
            }
        }

        SharedUi.LabeledControl {
            id: instrumentProgramSelector

            visible: SettingsController.useInstrumentDefinition

            x: patchRow.controlX(0)
            width: patchRow.controlWidth(0, 10)
            height: implicitHeight

            title: "Program"
            enabled: root.trackAssigned
            opacity: enabled ? 1.0 : 0.45

            ComboBox {
                id: instrumentProgramCombo

                Layout.fillWidth: true
                Layout.preferredHeight: Theme.controlHeight
                implicitHeight: Theme.controlHeight

                enabled: instrumentProgramSelector.enabled
                font.pixelSize: Theme.controlFontSize
                model: [root.track ? root.track.instrumentProgramDisplayName : ""]
                currentIndex: 0

                // Keep the native ComboBox appearance, including its indicator,
                // but route interaction to the searchable modal browser.
                MouseArea {
                    anchors.fill: parent
                    enabled: instrumentProgramCombo.enabled
                    cursorShape: Qt.PointingHandCursor
                    onClicked: ApplicationWindow.window.openModalPanel(
                        instrumentProgramBrowserComponent,
                        { track: root.track, closeOnOutsideClick: true })
                }
            }
        }

        SharedUi.LabeledComboBox {
            id: manualProgramCombo

            visible: !SettingsController.useInstrumentDefinition

            x: patchRow.controlX(4)
            width: patchRow.controlWidth(4, 6)
            height: implicitHeight

            title: "Program"
            modelData: root.generalMidiProgramNames
            popupMinWidth: 300

            enabled: root.trackAssigned
            opacity: enabled ? 1.0 : 0.45

            onCurrentIndexChanged: {
                if (root.updatingFromModel ||
                    SettingsController.useInstrumentDefinition ||
                    !root.track ||
                    currentIndex < 0) {
                    return
                }

                if (root.track.programNumber !== currentIndex)
                    root.track.programNumber = currentIndex
            }
        }

        SharedUi.LabeledComboBox {
            id: footageCombo

            x: patchRow.controlX(10)
            width: patchRow.controlWidth(10, 2)
            height: implicitHeight

            title: "Footage"
            modelData: root.footageNames

            enabled: root.trackAssigned
            opacity: enabled ? 1.0 : 0.45

            onCurrentIndexChanged: {
                if (root.updatingFromModel ||
                    !root.track ||
                    currentIndex < 0) {
                    return
                }

                if (root.track.footage !== currentIndex)
                    root.track.footage = currentIndex
            }
        }
    }

    // -----------------------------------------------------------------
    // Row 3: six direct-edit mixer knobs.
    //
    // Each control occupies exactly two parts of the same twelve-part
    // geometry used by the first two rows.
    // -----------------------------------------------------------------
    Item {
        id: mixerRow

        Layout.fillWidth: true

        implicitHeight: Math.max(
                            volumeKnob.implicitHeight,
                            panKnob.implicitHeight,
                            reverbKnob.implicitHeight,
                            chorusKnob.implicitHeight,
                            toneKnob.implicitHeight,
                            timbreKnob.implicitHeight)

        readonly property real contentWidth:
            Math.max(0, width - 2 * root.horizontalMargin)

        readonly property real halfGap:
            root.rowSpacing / 2

        function controlX(column) {
            return root.horizontalMargin
                   + contentWidth * column / 12
                   + (column > 0 ? halfGap : 0)
        }

        function controlWidth(column, columnSpan) {
            const endColumn = column + columnSpan
            const leftInset = column > 0 ? halfGap : 0
            const rightInset = endColumn < 12 ? halfGap : 0

            return Math.max(
                        0,
                        contentWidth * columnSpan / 12
                        - leftInset
                        - rightInset)
        }

        MidiKnob {
            id: volumeKnob

            x: mixerRow.controlX(0)
            width: mixerRow.controlWidth(0, 2)
            height: implicitHeight

            title: "Volume"
            from: 0
            to: 127
            value: 100

            enabled: root.trackAssigned
            opacity: enabled ? 1.0 : 0.45

            onValueChanged: {
                if (root.updatingFromModel || !root.track)
                    return

                if (root.track.volume !== value)
                    root.track.volume = value
            }
        }

        MidiKnob {
            id: panKnob

            x: mixerRow.controlX(2)
            width: mixerRow.controlWidth(2, 2)
            height: implicitHeight

            title: "Pan"
            from: -64
            to: 63
            value: 0

            enabled: root.trackAssigned
            opacity: enabled ? 1.0 : 0.45

            onValueChanged: {
                if (root.updatingFromModel || !root.track)
                    return

                if (root.track.pan !== value)
                    root.track.pan = value
            }
        }

        MidiKnob {
            id: reverbKnob

            x: mixerRow.controlX(4)
            width: mixerRow.controlWidth(4, 2)
            height: implicitHeight

            title: "Reverb"
            from: 0
            to: 127
            value: 40

            enabled: root.trackAssigned
            opacity: enabled ? 1.0 : 0.45

            onValueChanged: {
                if (root.updatingFromModel || !root.track)
                    return

                if (root.track.reverb !== value)
                    root.track.reverb = value
            }
        }

        MidiKnob {
            id: chorusKnob

            x: mixerRow.controlX(6)
            width: mixerRow.controlWidth(6, 2)
            height: implicitHeight

            title: "Chorus"
            from: 0
            to: 127
            value: 0

            enabled: root.trackAssigned
            opacity: enabled ? 1.0 : 0.45

            onValueChanged: {
                if (root.updatingFromModel || !root.track)
                    return

                if (root.track.chorus !== value)
                    root.track.chorus = value
            }
        }

        MidiKnob {
            id: toneKnob

            x: mixerRow.controlX(8)
            width: mixerRow.controlWidth(8, 2)
            height: implicitHeight

            title: "Tone"
            from: -64
            to: 63
            value: 0

            enabled: root.trackAssigned
            opacity: enabled ? 1.0 : 0.45

            onValueChanged: {
                if (root.updatingFromModel || !root.track)
                    return

                if (root.track.tone !== value)
                    root.track.tone = value
            }
        }

        MidiKnob {
            id: timbreKnob

            x: mixerRow.controlX(10)
            width: mixerRow.controlWidth(10, 2)
            height: implicitHeight

            title: "Timbre"
            from: -64
            to: 63
            value: 0

            enabled: root.trackAssigned
            opacity: enabled ? 1.0 : 0.45

            onValueChanged: {
                if (root.updatingFromModel || !root.track)
                    return

                if (root.track.timbre !== value)
                    root.track.timbre = value
            }
        }
    }
}
