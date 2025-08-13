import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

Item {
    width: parent.width
    height: parent.height

    // Ensure client doesn't accidently double click login buttons.
    property bool cooldown: false

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20

        Text {
            text: "Welcome."
            font.pointSize: 20
            Layout.alignment: Qt.AlignHCenter
        }

        TextField {
            id: usernameField
            placeholderText: ""
            width: 300
            Layout.alignment: Qt.AlignHCenter
        }

        TextField {
            id: passwordField
            placeholderText: ""
            echoMode: TextInput.Password
            width: 300
            Layout.alignment: Qt.AlignHCenter

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
                enabled: !cooldown
                onClicked: {
                    cooldown = true
                    Client.login(usernameField.text, passwordField.text)
                    cooldownTimer.start()
                }
            }

            Button {
                text: "Register"
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
