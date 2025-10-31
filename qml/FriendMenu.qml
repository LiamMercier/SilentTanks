import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import GUICommon 1.0

Popup {
    id: popup
    modal: false
    focus:true
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

    implicitWidth: 180

    property string friendUsername: ""
    property string friendID: ""

    signal messageFriend(string username)

    background: Rectangle {
        color: "#202122"
        radius: 4
    }

    ColumnLayout {
        id: menuColumn
        spacing: 4
        anchors.fill: parent
        anchors.margins: 4

        Button {
            id: messageButton
            text: "Message"
            Layout.fillWidth: true

            implicitHeight: 34
            implicitWidth: 148

            background: Rectangle {
                radius: 2
                anchors.fill: parent
                color: "#3e4042"
            }

            contentItem: Text {
                text: messageButton.text
                font: messageButton.font
                color: "#f2f2f2"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: {
                messageFriend(friendUsername)
                popup.close()
            }
        }

        Button {
            id: unfriendButton
            text: "Remove"
            Layout.fillWidth: true

            implicitHeight: 34
            implicitWidth: 148

            background: Rectangle {
                radius: 2
                anchors.fill: parent
                color: "#3e4042"
            }

            contentItem: Text {
                text: unfriendButton.text
                font: unfriendButton.font
                color: "#f2f2f2"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: {
                Client.unfriend_user(friendID)
                popup.close()
            }
        }


    }
}
