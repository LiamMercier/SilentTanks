import QtQuick
import QtQuick.Controls

Item {
    width: parent.width
    height: parent.height

    // Ensure client doesn't accidently double click login buttons.
    property bool cooldown: false

    Column {
        anchors.centerIn: parent
        spacing: 20

        Text {
            text: "Login or register"
            font.pointSize: 20
        }

        TextField {
            id: usernameField
            placeholderText: ""
            width: 300
        }

        TextField {
            id: passwordField
            placeholderText: ""
            width: 300
        }

        // Button with time between presses.
        Button {
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
