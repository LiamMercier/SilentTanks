import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import QtQuick.Window 2.15

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
            if (!confirmDialog.opened){
                confirmDialog.open()
            }
        }
    }

    Dialog {
        id: confirmDialog
        title: "Confirm Exit"
        modal: true

        x: (root.width - width) / 2
        y: (root.height - height) / 2

        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: {
            calledClose = true
            Client.shutdown_client()
        }
        onRejected: {
            calledClose = false
        }
    }

}
