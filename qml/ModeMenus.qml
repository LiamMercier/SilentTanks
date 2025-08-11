import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import GUICommon 1.0

Item {
    id: modeMenuRoot

    property string label: "modes label"
    property var model: null

    implicitWidth: triggerButton.implicitWidth
    implicitHeight: triggerButton.implicitHeight

    Button {
        id: triggerButton
        text: modeMenuRoot.label
        font.bold: true
        implicitHeight: 36
        onClicked: {
            popup.x = triggerButton.mapToItem(parent, triggerButton.x, triggerButton.y).x
            popup.y = triggerButton.mapToItem(parent, triggerButton.x, triggerButton.height).y
            popup.width = Math.max(triggerButton.width, 120)
            popup.open()
        }

        background: Rectangle {
            radius: 2
            implicitHeight: triggerButton.implicitHeight
            implicitWidth: 140

            border.width: 1
            border.color: "red"
        }
    }

    Popup {
        id: popup
        modal: false
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

        background: Rectangle {
            border.color: "black"
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            //anchors.margins: 8
            spacing: 8

            Repeater {
                model: modeMenuRoot.model
                delegate: Button {
                    // list model role
                    text: label
                    Layout.alignment: Qt.AlignCenter
                    font.pointSize: 12

                    onClicked: {
                        Client.set_selected_mode(mode)
                        popup.close()
                    }

                    background: Rectangle {
                    }
                }

            }
        }

    }
}
