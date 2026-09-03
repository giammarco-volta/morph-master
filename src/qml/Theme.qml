pragma Singleton

import QtQuick

QtObject {

    // =========================
    // Backgrounds
    // =========================

    readonly property color background:        "#202020"
    readonly property color panel:             "#2A2A2A"
    readonly property color panelDark:         "#252525"
    readonly property color border:            "#505050"

    // =========================
    // Text
    // =========================

    readonly property color text:              "#F0F0F0"
    readonly property color secondaryText:     "#B8B8B8"
    readonly property color disabledText:      "#707070"

    // =========================
    // NaadaLab accent
    // =========================

    readonly property color accent:            "#D8B85A"
    readonly property color accentPressed:     "#C6A74A"

    // =========================
    // Status
    // =========================

    readonly property color warning:           "#D08030"
    readonly property color error:             "#D05050"
    readonly property color success:           "#50A060"
	
    // =========================
    // Sizes
    // =========================

	readonly property int labelFontSize: 14
	readonly property int controlFontSize: 16

	readonly property int spacing: 6
	readonly property int pageSpacing: 16

	readonly property int pageMargin: 24

    readonly property int controlHeight: 36
    readonly property int buttonHeight: 36
    readonly property int sectionPadding: 14
}