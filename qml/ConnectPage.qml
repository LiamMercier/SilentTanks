import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    width: parent.width
    height: parent.height

    color: "#1b1c1d"

    Column {
        anchors.centerIn: parent
        spacing: 20

        Text {
            text: "Connect to server:"
            font.pointSize: 20
            color: "#f2f2f2"
        }

        TextField {
            id: endpointField
            placeholderText: "Enter server address"
            color: "#f2f2f2"
            width: 300

            background: Rectangle {
                color: "#2a2c2e"
                radius: 2
            }

            onAccepted: {
                Client.connect_to_server(endpointField.text)
            }
        }

        Button {
            id: connectButton
            text: "Connect"

            contentItem: Text {
                text: connectButton.text
                font: connectButton.font
                color: "#eaeaea"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                color: "#323436"
                radius: 2
            }

            onClicked: {
                Client.connect_to_server(endpointField.text)
            }
        }
    }
}
