import QtQuick

import MorphMaster
import NaadaLab.Ui as SharedUi

SharedUi.AboutView {
    id: root

    signal openManualRequested()

    html: SettingsController.aboutHtml

    onLinkActivated: function(link) {
        if (link === "morphmaster:user-manual") {
            root.openManualRequested()
            return
        }

        Qt.openUrlExternally(link)
    }
}
