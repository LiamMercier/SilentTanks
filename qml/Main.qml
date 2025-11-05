import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import QtQuick.Window 2.15
import QtQuick.Layouts 2.15

import GUICommon 1.0

ApplicationWindow {
    id: root
    visible: true
    width: Screen.width * 0.8
    height: Screen.height * 0.75
    title: "Silent Tanks"

    // Load our client interface based on the client state.
    Loader {
        id: pageLoader
        anchors.fill: parent
        // Initialize at the connect page.
        source: "ConnectPage.qml"
    }

    Connections {
        target: Client
        function onState_changed(new_state) {
            switch (new_state) {
                case ClientState.ConnectScreen:
                    pageLoader.source = "ConnectPage.qml"
                    break;
                case ClientState.LoginScreen:
                    pageLoader.source = "LoginPage.qml"
                    break;
                case ClientState.Lobby:
                    pageLoader.source = "Lobby.qml"
                    break;
                case ClientState.Playing:
                    pageLoader.source = "GamePage.qml"
                    break;
                case ClientState.Replaying:
                    pageLoader.source = "ReplayPage.qml"
                    break;
                default:
                    pageLoader.source = ""
                    break;
            }
        }
    }

    // Declare popups now.
    InfoPopup {
        id: infoPopup
    }

    // Show popups when C++ code calls us to do so.
    Connections {
        target: Client
        function onShow_popup(type, title, body) {
            var popup
            switch(type) {
                case PopupType.Info:
                    popup = infoPopup;
                    break;
                default:
                    return;
            }

            popup.popupTitle = title;
            popup.popupText = body;
            popup.show();
        }

        function onPlay_sound(type) {
            SoundManager.playSound(type)
            console.log(type)
        }
    }

    // Handle application close.
    property bool calledClose: false

    onClosing:
    function (close) {
        if (!calledClose){
            close.accepted = false
            if (!confirmPopup.opened){
                confirmPopup.open()
            }
        }
    }

    Popup {
        id: confirmPopup
        modal: true

        x: (root.width - width) / 2
        y: (root.height - height) / 2

        implicitWidth: 250
        implicitHeight: 150
        focus: true

        background: Rectangle {
            id: confirmExitBackground
            anchors.fill: parent
            radius: 6
            color: "#323436"
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Text {
                Layout.fillWidth: true

                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                wrapMode: Text.NoWrap
                elide: Text.ElideRight

                text: "Exit Application?"
                font.pointSize: 18
                color: "#f2f2f2"
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                spacing: 12

                SvgButton {
                    id: cancelButton

                    implicitHeight: 30
                    implicitWidth: 80

                    Layout.alignment: Qt.AlignHCenter

                    buttonText: "Cancel"
                    fontChoice: "#1f1f1f"
                    unfocusedOpacity: 1.00

                    normalSource: "qrc:/svgs/buttons/forfeit_button.svg"
                    hoverSource: "qrc:/svgs/buttons/forfeit_button_hovered.svg"
                    pressedSource: "qrc:/svgs/buttons/forfeit_button_pressed.svg"

                    toggled: false

                    onClicked: {
                        calledClose = false
                        confirmPopup.close()
                    }
                }

                SvgButton {
                    id: exitButton

                    implicitHeight: 30
                    implicitWidth: 80

                    Layout.alignment: Qt.AlignHCenter

                    buttonText: "Exit"
                    fontChoice: "#1f1f1f"
                    unfocusedOpacity: 1.00

                    normalSource: "qrc:/svgs/buttons/exit_button.svg"
                    hoverSource: "qrc:/svgs/buttons/exit_button_hovered.svg"
                    pressedSource: "qrc:/svgs/buttons/exit_button_pressed.svg"

                    toggled: false

                    onClicked: {
                        calledClose = true
                        confirmPopup.close()
                        Client.shutdown_client()
                    }
                }
            }
        }
    }

}
