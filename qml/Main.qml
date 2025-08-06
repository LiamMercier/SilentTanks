import QtQuick
import QtQuick.Controls
import QtQuick.Window

import GUICommon 1.0

ApplicationWindow {
    id: root
    visible: true
    width: Screen.width * 0.6
    height: Screen.height * 0.75

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
                default:
                    pageLoader.source = ""
                    break;
            }
        }
    }

    // Declare popups now.
    InfoPopup {
        id: infoPopup;
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
    }

}
