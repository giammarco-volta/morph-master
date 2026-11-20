import QtQuick
import QtQuick.Controls

import MorphMaster
import NaadaLab.Ui as SharedUi

SharedUi.AboutView {
    id: root

    signal openManualRequested()

    html: "<p><a href=\"morphora:introduction\">" + qsTr("Show introduction") + "</a></p>" + SettingsController.aboutHtml

    onLinkActivated: function(link) {
        if (link === "morphmaster:user-manual") {
            root.openManualRequested()
            return
        }

        if (link === "morphora:introduction") {
            const window = root.ApplicationWindow.window
            if (window)
                window.showIntroduction()
            return
        }

        Qt.openUrlExternally(link)
    }
}
