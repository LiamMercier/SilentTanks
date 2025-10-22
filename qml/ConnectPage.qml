import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

Rectangle {
    width: parent.width
    height: parent.height

    color: "#1b1c1d"

    ColumnLayout {
        id: connectColumn

        property bool addButtonVisible: false

        anchors.centerIn: parent
        width: parent.width * 0.8
        height: parent.height
        spacing: 8

        Item {
            id: connectTopSpacer

            Layout.preferredHeight: connectColumn.height * 0.05
            Layout.maximumHeight: connectColumn.height * 0.05
        }

        Text {
            Layout.alignment: Qt.AlignHCenter

            text: "Servers"
            font.pointSize: 20
            color: "#f2f2f2"
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        // TODO: list view of server's we have saved from disk.

        Button {
            id: addButton

            visible: !connectColumn.addButtonVisible

            Layout.alignment: Qt.AlignHCenter

            text: "Add Server"

            contentItem: Text {
                text: addButton.text
                font: addButton.font
                color: "#eaeaea"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                color: "#323436"
                radius: 2
            }

            onClicked: {
                connectColumn.addButtonVisible = true
                return
            }
        }

        RowLayout {
            id: dnsConnectRow
            visible: connectColumn.addButtonVisible

            Layout.alignment: Qt.AlignHCenter
            spacing: 12

            TextField {
                id: addressField

                Layout.alignment: Qt.AlignHCenter

                placeholderText: "Server Domain"
                color: "#f2f2f2"
                Layout.preferredWidth: 175
                Layout.maximumWidth: 175

                background: Rectangle {
                    color: "#2a2c2e"
                    radius: 2
                }

                onAccepted: {

                }
            }

            TextField {
                id: portField

                Layout.alignment: Qt.AlignHCenter

                placeholderText: "Port (empty for default)"
                color: "#f2f2f2"
                Layout.preferredWidth: 175
                Layout.maximumWidth: 175

                background: Rectangle {
                    color: "#2a2c2e"
                    radius: 2
                }

                onAccepted: {

                }
            }
        }

        Text {
            Layout.alignment: Qt.AlignHCenter

            visible: connectColumn.addButtonVisible

            text: "or"
            font.pointSize: 16
            color: "#f2f2f2"
        }

        TextField {
            id: identityField

            Layout.alignment: Qt.AlignHCenter

            visible: connectColumn.addButtonVisible

            placeholderText: "Server Identity String"
            color: "#f2f2f2"
            // Above two buttons plus spacing
            Layout.preferredWidth: 362
            Layout.maximumWidth: 362

            background: Rectangle {
                color: "#2a2c2e"
                radius: 2
            }

            onAccepted: {

            }
        }

        RowLayout {
            id: acceptServerRow

            visible: connectColumn.addButtonVisible

            Layout.alignment: Qt.AlignHCenter

            spacing: 20

            Button {
                id: saveButton

                Layout.alignment: Qt.AlignHCenter

                text: "Save"

                contentItem: Text {
                    text: saveButton.text
                    font: saveButton.font
                    color: "#eaeaea"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: "#323436"
                    radius: 2
                }

                onClicked: {
                    if (connectColumn.addButtonVisible)
                        // If done, try to save the server details.

                        // Save using the address and port if no identity
                        // was given.
                        if (identityField.text == "")
                        {
                            var res = addressField.text + ":" + portField.text
                            Client.save_server_domain(res)
                        }
                        // Otherwise try to save using the identity string.
                        else
                        {
                            Client.save_server_identity(identityField.text)
                        }

                        // Clean up for next time.
                        addressField.text = ""
                        portField.text = ""
                        identityField.text = ""
                        connectColumn.addButtonVisible = false
                        return
                }
            }

            Button {
                id: cancelButton

                Layout.alignment: Qt.AlignHCenter

                text: "Cancel"

                contentItem: Text {
                    text: cancelButton.text
                    font: cancelButton.font
                    color: "#eaeaea"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: "#323436"
                    radius: 2
                }

                onClicked: {
                    // Close and clean up for next time.
                    addressField.text = ""
                    portField.text = ""
                    identityField.text = ""
                    connectColumn.addButtonVisible = false
                    return
                }
            }
        }

        Item
        {
            id: connectBottomSpacer

            Layout.preferredHeight: connectColumn.height * 0.05
            Layout.maximumHeight: connectColumn.height * 0.05
        }
    }
}
