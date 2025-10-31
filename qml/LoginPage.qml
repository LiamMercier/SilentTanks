import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

Rectangle {
    width: parent.width
    height: parent.height

    color: "#1b1c1d"

    // Ensure client doesn't accidently double click login buttons.
    property bool cooldown: false

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20

        Text {
            text: "Welcome."
            font.pointSize: 20
            color: "#f2f2f2"
            Layout.alignment: Qt.AlignHCenter
        }

        TextField {
            id: usernameField
            placeholderText: "username"
            Layout.preferredWidth: 250
            Layout.maximumWidth: 250
            Layout.alignment: Qt.AlignHCenter

            placeholderTextColor: "#a2a3a4"
            color: "#f2f2f2"

            background: Rectangle {
                color: "#2a2c2e"
                radius: 2
                width: parent.width
            }
        }

        TextField {
            id: passwordField
            placeholderText: "password"
            echoMode: TextInput.Password

            Layout.preferredWidth: 250
            Layout.maximumWidth: 250
            Layout.alignment: Qt.AlignHCenter

            placeholderTextColor: "#a2a3a4"
            color: "#eaeaea"

            background: Rectangle {
                color: "#2a2c2e"
                radius: 2
                width: parent.width
            }

            // Simulate pressing login by default if we press enter
            // on the password field.
            onAccepted: {
                if (!cooldown) {
                    loginButton.clicked()
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 12

            // Button with time between presses.
            Button {
                id: loginButton
                text: "Login"

                background: Rectangle {
                    color: loginButton.down ? "#202122" :
                            loginButton.hovered ? "#3e4042" : "#323436"
                    radius: 2
                }

                contentItem: Text {
                    text: loginButton.text
                    font: loginButton.font
                    color: "#eaeaea"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                enabled: !cooldown
                onClicked: {
                    cooldown = true
                    Client.login(usernameField.text, passwordField.text)
                    cooldownTimer.start()
                }
            }

            Button {
                id: registerButton
                text: "Register"

                background: Rectangle {
                    color: registerButton.down ? "#202122" :
                            registerButton.hovered ? "#3e4042" : "#323436"
                    radius: 2
                }

                contentItem: Text {
                    text: registerButton.text
                    font: registerButton.font
                    color: "#eaeaea"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    cooldown = true
                    Client.register_account(usernameField.text, passwordField.text)
                    cooldownTimer.start()
                }
            }
        }

        // Button timer logic.
        Timer {
            id: cooldownTimer
            interval: 300
            repeat: false
            onTriggered: {
                cooldown = false
            }
        }

    }
}
