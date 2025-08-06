import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    width: parent.width
    height: parent.height

    Column {
        anchors.centerIn: parent
        spacing: 20

        Text {
            text: "Connect to server:"
            font.pointSize: 20
        }

        TextField {
            id: endpointField
            placeholderText: "Enter server address"
            width: 300
        }

        Button {
            text: "Connect"
            onClicked: {
                Client.connect_to_server(endpointField.text)
            }
        }
    }
}
