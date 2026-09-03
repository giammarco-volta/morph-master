pragma Singleton

import QtQuick

QtObject {
    enum LayoutClass {
        Phone,
        Tablet,
        Desktop
    }

    readonly property int phoneMaxHeight: 480
    readonly property int tabletMaxHeight: 800

    function layoutClassName(layoutClass) {
        switch (layoutClass) {
        case UiMetrics.Phone:
            return "Phone"
        case UiMetrics.Tablet:
            return "Tablet"
        case UiMetrics.Desktop:
            return "Desktop"
        default:
            return "Unknown"
        }
    }

    function spacing(layoutClass) {
        switch (layoutClass) {
        case UiMetrics.Phone:
            return 4
        case UiMetrics.Tablet:
            return 8
        default:
            return 8
        }
    }

    function knobDiameter(layoutClass) {
        switch (layoutClass) {
        case UiMetrics.Phone:
            return 52
        case UiMetrics.Tablet:
            return 64
        default:
            return 64
        }
    }

    function controlHeight(layoutClass) {
        switch (layoutClass) {
        case UiMetrics.Phone:
            return 40
        case UiMetrics.Tablet:
            return 44
        default:
            return 36
        }
    }
}