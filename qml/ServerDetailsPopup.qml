import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

Popup {
    id: detailsPopup
    modal: true
    focus: true

    width: {
        if (parent)
        {
            if (parent.width > 305)
            {
                return 305
            }
            else
            {
                detailsPopup.close()
                return 0
            }
        }

        return 305
    }
    height: {
        if (parent)
        {
            if (parent.height > 270)
            {
                return 270
            }
            else
            {
                detailsPopup.close()
                return 0
            }
        }

        return 270
    }

    padding: 0

    background: Rectangle {
        id: backgroundRect
        anchors.fill: parent
        color: "#323436"
        radius: 6
    }

    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property string serverName: ""
    property string serverAddress: ""
    property int serverPort: -1
    property string serverFingerprint: ""

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        GridLayout {
            columns: 3
            columnSpacing: 0

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop

            Item {
                id: leftSpacer
                Layout.preferredWidth: closeButton.width + innerSpacer.width
            }

            Text {
                Layout.fillWidth: true

                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                wrapMode: Text.NoWrap
                elide: Text.ElideRight

                text: "Server Details"
                font.pointSize: 18
                color: "#f2f2f2"
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                spacing: 0

                SvgButton {
                    id: closeButton

                    implicitHeight: 24
                    implicitWidth: 24

                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                    unfocusedOpacity: 1.00

                    normalSource: "qrc:/pngs/close_button.png"
                    hoverSource: "qrc:/pngs/close_button_hovered.png"
                    pressedSource: "qrc:/pngs/close_button_pressed.png"

                    toggled: false

                    onClicked: {
                        detailsPopup.close()
                    }
                }

                Item {
                    id: innerSpacer
                    Layout.preferredWidth: 8
                }
            }

        }

        GridLayout {
            columns: 2
            rowSpacing: 8
            columnSpacing: 12
            Layout.fillWidth: true

            Text {
                text: "Name:"
                color: "#f2f2f2"
                font.bold: true
            }

            Text {
                text: detailsPopup.serverName
                color: "#f2f2f2"
            }

            Text {
                text: "Address:"
                color: "#f2f2f2"
                font.bold: true
            }

            Text {
                text: detailsPopup.serverAddress
                color: "#f2f2f2"
            }

            Text {
                text: "Port:"
                color: "#f2f2f2"
                font.bold: true
            }

            Text {
                text: detailsPopup.serverPort
                color: "#f2f2f2"
            }

            Text {
                text: "Fingerprint:"
                color: "#f2f2f2"
                font.bold: true
            }

            Text {
                text: detailsPopup.serverFingerprint
                color: "#f2f2f2"

                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

        }

        SvgButton {
            id: deleteButton

            implicitHeight: 30
            implicitWidth: 80

            Layout.alignment: Qt.AlignHCenter

            buttonText: "Delete"
            fontChoice: "#1f1f1f"
            unfocusedOpacity: 1.00

            normalSource: "qrc:/svgs/buttons/forfeit_button.svg"
            hoverSource: "qrc:/svgs/buttons/forfeit_button_hovered.svg"
            pressedSource: "qrc:/svgs/buttons/forfeit_button_pressed.svg"

            toggled: false

            onClicked: {
                Client.remove_server_identity(serverAddress,
                                              serverPort,
                                              serverFingerprint)
                detailsPopup.close()
            }
        }

    }
}
