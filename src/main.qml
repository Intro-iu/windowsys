import QtQuick 2.12
import QtQuick.Controls 2.12
import QtWayland.Compositor 1.0

WaylandCompositor {
    id: compositor

    WaylandOutput {
        id: output
        size: Qt.size(800, 600)
        scale: 1
    }

    WaylandSurfaceItem {
        anchors.fill: parent
        surface: compositor.wlShellSurface
    }

    Rectangle {
        anchors.fill: parent
        color: "lightblue"

        Text {
            anchors.centerIn: parent
            text: "Hello, Wayland!"
            font.pixelSize: 40
            color: "white"
        }
    }
}
