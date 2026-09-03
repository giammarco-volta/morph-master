import QtQuick
import QtQuick.Controls

import MorphMaster

Item {
    id: root

    anchors.fill: parent
    visible: false
    z: 100000

    property var stack: []

    property bool closeOnOutsideClick: true

    function show(component, properties) {
        closeOnOutsideClick = true

        if (properties !== undefined && properties.closeOnOutsideClick !== undefined) {
            closeOnOutsideClick = properties.closeOnOutsideClick
            delete properties.closeOnOutsideClick
        }

        if (stack.length > 0)
            stack[stack.length - 1].visible = false

        const item = component.createObject(panelHost)

        if (!item)
            return

        if (item.init)
            item.init(properties || {})

        item.x = Qt.binding(function() {
            return Math.round((root.width - item.width) / 2)
        })

        item.y = Qt.binding(function() {
            return Math.round((root.height - item.height) / 2)
        })

        item.z = stack.length + 1

        stack.push(item)
        root.visible = true
    }

    function close() {
        if (stack.length === 0) {
            root.visible = false
            return
        }

        const item = stack.pop()
        item.destroy()

        if (stack.length > 0) {
            stack[stack.length - 1].visible = true
        } else {
            root.visible = false
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#80000000"
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (root.closeOnOutsideClick)
                root.close()
        }
    }

    Item {
        id: panelHost
        anchors.fill: parent
        z: 1
    }
}