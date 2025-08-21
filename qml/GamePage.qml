import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import GUICommon 1.0

Item {
    id: gamePageRoot
    anchors.fill: parent

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Map area (80% of screen)
        Rectangle {
            id: mapArea
            Layout.prefferedWidth: parent.width * 0.8
            Layout.fillHeight: true
            border.width: 1

            // TODO: map canvas.

        }

        Rectangle {
            id: sidePanel
            Layout.prefferedWidth: parent.width * 0.2
            Layout.fillHeight: true
            border.color: "green"
            border.width: 1

            // TODO: side panel.
        }

    }
}
