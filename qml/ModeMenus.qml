import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import GUICommon 1.0

Item {
    id: modeMenuRoot

    property string label: "modes label"
    property var model: null
    property var callback: null

    implicitWidth: triggerButton.implicitWidth
    implicitHeight: triggerButton.implicitHeight

    Button {
        id: triggerButton
        text: modeMenuRoot.label
        font.bold: true
        implicitHeight: 40
        onClicked: {
            popup.x = triggerButton.mapToItem(parent, triggerButton.x, triggerButton.y).x
            popup.y = triggerButton.mapToItem(parent, triggerButton.x, triggerButton.height).y
            popup.width = Math.max(triggerButton.width, 120)
            popup.open()
        }

        background: Rectangle {
            color: "#323436"
            radius: 2
            implicitHeight: triggerButton,implicitHeight
            implicitWidth: 140
        }

        contentItem: Text {
            text: triggerButton.text
            font: triggerButton.font
            color: "#eaeaea"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    Popup {
        id: popup
        modal: false
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

        leftPadding: 7
        rightPadding: 7

        background: Rectangle {
            color: "#202122"
            height: popup.height - 4
            radius: 4
        }

        ColumnLayout {
            id: mainRepeaterColumn
            anchors.fill: parent
            spacing: 4

            Repeater {
                model: modeMenuRoot.model
                delegate: Button {
                    // list model role
                    id: modeButton
                    text: label
                    Layout.alignment: Qt.AlignCenter
                    font.pointSize: 11

                    Layout.fillWidth: true

                    background: Rectangle {
                        radius: 2
                        anchors.fill: parent
                        color: "#3e4042"
                    }

                    contentItem: Text {
                        text: modeButton.text
                        font: modeButton.font
                        color: "#f2f2f2"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        if (modeMenuRoot.callback)
                        {
                            modeMenuRoot.callback(mode)
                        }
                        popup.close()
                    }
                }

            }
        }

    }
}
