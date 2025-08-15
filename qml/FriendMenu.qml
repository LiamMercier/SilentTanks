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

    ColumnLayout {
        id: menuColumn
        spacing: 0
        anchors.fill: parent
        anchors.margins: 4

        Button {
            id: messageButton
            text: "Message"
            Layout.fillWidth: true

            onClicked: {
                messageFriend(friendUsername)
                popup.close()
            }
        }

        Button {
            id: unfriendButton
            text: "Remove"
            Layout.fillWidth: true

            onClicked: {
                Client.unfriend_user(friendID)
                popup.close()
            }
        }


    }
}
