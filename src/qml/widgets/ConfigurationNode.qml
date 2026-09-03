import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import MorphMaster

// Full-size central icon revision: 2026-07-17-v4

Rectangle {
    id: root

    property string title: "Node"
    property string iconKind: "surface"
    property string primaryText: ""
    property string secondaryText: ""
    property string tertiaryText: ""
    property bool configured: true

    /* Used only by the central MORPHMASTER node. */
    property bool showMorphOutputs: false
    property var morphOutputNames: []
    property var morphOutputTrackLists: []

    readonly property color effectiveIconColor:
        configured ? Theme.accent : Theme.disabledText

    color: Theme.panelDark
    border.color: configured ? Theme.border : Qt.darker(Theme.border, 1.15)
    border.width: 1
    radius: 7

    implicitWidth: 230
    implicitHeight: 190

    component NodeIcon: Canvas {
        id: iconCanvas

        property color iconColor: root.effectiveIconColor
        property string iconKind: root.iconKind

        onIconColorChanged: requestPaint()
        onIconKindChanged: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        onPaint: {
            const ctx = getContext("2d")
            const sx = width / 76
            const sy = height / 64

            ctx.reset()
            ctx.scale(sx, sy)
            ctx.strokeStyle = iconColor
            ctx.fillStyle = iconColor
            ctx.lineWidth = 2
            ctx.lineCap = "round"
            ctx.lineJoin = "round"

            if (iconKind === "input") {
                /* Compact MIDI keyboard. */
                ctx.strokeRect(7, 15, 62, 35)

                const whiteKeyWidth = 62 / 8
                for (let i = 1; i < 8; ++i) {
                    const x = 7 + i * whiteKeyWidth
                    ctx.beginPath()
                    ctx.moveTo(x, 15)
                    ctx.lineTo(x, 50)
                    ctx.stroke()
                }

                const blackKeys = [1, 2, 4, 5, 6]
                for (let i = 0; i < blackKeys.length; ++i) {
                    const x = 7 + blackKeys[i] * whiteKeyWidth - 2.5
                    ctx.fillRect(x, 15, 5, 20)
                }

                ctx.beginPath()
                ctx.arc(14, 9, 2, 0, Math.PI * 2)
                ctx.fill()
                ctx.beginPath()
                ctx.arc(22, 9, 2, 0, Math.PI * 2)
                ctx.fill()
            } else if (iconKind === "surface") {
                /* Stylised MorphMaster surface. */
                ctx.strokeRect(10, 8, 56, 48)

                ctx.beginPath()
                ctx.moveTo(10, 32)
                ctx.lineTo(66, 32)
                ctx.moveTo(38, 8)
                ctx.lineTo(38, 56)
                ctx.stroke()

                ctx.beginPath()
                ctx.moveTo(15, 48)
                ctx.bezierCurveTo(27, 47, 31, 17, 43, 16)
                ctx.bezierCurveTo(51, 15, 57, 24, 61, 29)
                ctx.stroke()

                const dots = [
                    [22, 42],
                    [37, 25],
                    [52, 20]
                ]

                for (let i = 0; i < dots.length; ++i) {
                    ctx.beginPath()
                    ctx.arc(dots[i][0], dots[i][1], 2.6,
                            0, Math.PI * 2)
                    ctx.fill()
                }
            } else {
                /* Remote instrument / sound module. */
                ctx.strokeRect(8, 11, 60, 42)

                ctx.beginPath()
                ctx.arc(20, 32, 9, 0, Math.PI * 2)
                ctx.stroke()
                ctx.beginPath()
                ctx.arc(56, 32, 9, 0, Math.PI * 2)
                ctx.stroke()

                ctx.beginPath()
                ctx.arc(20, 32, 3, 0, Math.PI * 2)
                ctx.fill()
                ctx.beginPath()
                ctx.arc(56, 32, 3, 0, Math.PI * 2)
                ctx.fill()

                ctx.beginPath()
                ctx.moveTo(31, 19)
                ctx.lineTo(45, 19)
                ctx.stroke()

                const controls = [34, 38, 42]
                for (let i = 0; i < controls.length; ++i) {
                    ctx.beginPath()
                    ctx.arc(controls[i], 26, 1.3,
                            0, Math.PI * 2)
                    ctx.fill()
                }
            }
        }
    }

    Loader {
        anchors.fill: parent
        sourceComponent: root.showMorphOutputs
                         ? morphOutputsComponent
                         : standardComponent
    }

    component StandardTextLabel: Label {
        Layout.fillWidth: true

        color: root.configured
               ? Theme.secondaryText
               : Theme.disabledText
        font.pixelSize: Theme.labelFontSize

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideMiddle
    }

    /* Original node geometry, unchanged for MIDI INPUT and MIDI OUTPUT. */
    Component {
        id: standardComponent

        ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sectionPadding
        spacing: 7

        Label {
            Layout.fillWidth: true

            text: root.title
            color: root.configured ? Theme.text : Theme.disabledText
            font.pixelSize: Theme.labelFontSize
            font.bold: true

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        NodeIcon {
            Layout.preferredWidth: 76
            Layout.preferredHeight: 64
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            Layout.fillWidth: true

            text: root.primaryText
            color: root.configured ? Theme.text : Theme.disabledText
            font.pixelSize: Theme.controlFontSize
            font.bold: true

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideMiddle
        }

        StandardTextLabel {
            visible: text.length > 0
            text: root.secondaryText
        }

        StandardTextLabel {
            visible: text.length > 0
            text: root.tertiaryText
        }

        Item {
            Layout.fillHeight: true
        }
        }
    }

    /*
     * MORPHMASTER keeps exactly the same outer size. The eight Morph Outputs
     * occupy a compact column on the right; the previous icon and text remain
     * in the remaining internal space on the left.
     */
    Component {
        id: morphOutputsComponent

        ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.sectionPadding
        spacing: 5

        Label {
            Layout.fillWidth: true

            text: root.title
            color: root.configured ? Theme.text : Theme.disabledText
            font.pixelSize: Theme.labelFontSize
            font.bold: true

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 4

                NodeIcon {
                    Layout.preferredWidth: 76
                    Layout.preferredHeight: 64
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    Layout.fillWidth: true

                    text: root.primaryText
                    color: root.configured ? Theme.text : Theme.disabledText
                    font.pixelSize: Theme.controlFontSize
                    font.bold: true
                    fontSizeMode: Text.Fit
                    minimumPixelSize: 8

                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideMiddle
                }

                StandardTextLabel {
                    visible: text.length > 0
                    text: root.secondaryText
                    fontSizeMode: Text.Fit
                    minimumPixelSize: 7
                }

                StandardTextLabel {
                    visible: text.length > 0
                    text: root.tertiaryText
                    fontSizeMode: Text.Fit
                    minimumPixelSize: 7
                }

                Item {
                    Layout.fillHeight: true
                }
            }

            ColumnLayout {
                Layout.preferredWidth: Math.min(116, root.width * 0.46)
                Layout.fillHeight: true
                spacing: 2

                Repeater {
                    model: 8

                    delegate: Rectangle {
                        id: outputBox

                        required property int index

                        readonly property var assignedTracks:
                            root.morphOutputTrackLists
                            && index < root.morphOutputTrackLists.length
                                ? root.morphOutputTrackLists[index]
                                : []

                        readonly property bool active:
                            assignedTracks && assignedTracks.length > 0

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredHeight: 16

                        color: active ? "#14D8B85A" : "transparent"
                        border.color: active ? Theme.accent : Theme.border
                        border.width: active ? 1.4 : 1
                        radius: 3

                        Label {
                            anchors.fill: parent
                            anchors.leftMargin: 3
                            anchors.rightMargin: 3

                            text: root.morphOutputNames
                                  && outputBox.index < root.morphOutputNames.length
                                      ? root.morphOutputNames[outputBox.index]
                                      : "Morph Output"

                            color: outputBox.active
                                   ? Theme.text
                                   : Theme.secondaryText
                            font.pixelSize: 10
                            font.bold: outputBox.active
                            fontSizeMode: Text.Fit
                            minimumPixelSize: 7

                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
        }
    }
}
