import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

Popup {
    id: detailsPopup
    modal: true
    focus: true

    width: parent ? parent.width * 0.3 : 105
    height: parent ? parent.height * 0.7 : 550

    padding: 0

    background: Rectangle {
        id: backgroundRect
        anchors.fill: parent
        color: "#323436"
        radius: 4
    }

    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property string serverName: ""
    property string serverAddress: ""
    property int serverPort: -1
    property string serverFingerprint: ""

    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Text {
            Layout.fillWidth: true

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter

            text: "Server Details"
            font.pointSize: 18
            color: "#f2f2f2"
        }

        RowLayout {
            spacing: 6

            Text {
                text: "Name:"
                color: "#f2f2f2"
            }

            Text {
                text: detailsPopup.serverName
                color: "#f2f2f2"
            }
        }

        RowLayout {
            spacing: 6

            Text {
                text: "Address:"
                color: "#f2f2f2"
            }

            Text {
                text: detailsPopup.serverAddress
                color: "#f2f2f2"
            }
        }

        RowLayout {
            spacing: 6

            Text {
                text: "Port:"
                color: "#f2f2f2"
            }

            Text {
                text: detailsPopup.serverPort
                color: "#f2f2f2"
            }
        }

        RowLayout {
            spacing: 6

            Text {
                text: "Fingerprint:"
                color: "#f2f2f2"
            }

            Text {
                text: detailsPopup.serverFingerprint
                color: "#f2f2f2"

                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }

        Item {
            Layout.fillHeight: true
        }

        Button {
            text: "Delete"
            onClicked: {
                // TODO: delete server information
                console.log("TODO: delete server information")
                detailsPopup.close()
            }
        }

    }
}
