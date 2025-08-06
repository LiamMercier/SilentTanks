import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import GUICommon 1.0

Window
{
    id: infoPopup
    // TODO: decide on sizes
    width: 300
    height: 150

    // Stop the main application until response occurs.
    modality: Qt.ApplicationModal

    // Flags to setup a popup style environment.
    flags: Qt.Dialog | Qt.WindowTitleHint | Qt.WindowSystemMenuHint

    // Alias for other files.
    property alias popupTitle: titleText.text
    property alias popupText: bodyText.text

    // Tell our popup queue in C++ that we closed a popup.
    onClosing: {
        Client.notify_popup_closed()
    }

    // TODO: make this look good.
    Rectangle {
        anchors.fill: parent
        color: "white"
        border.color: "#444"
        radius: 8

        Column {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            Text {
                id: titleText
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            Text {
                id: bodyText
                wrapMode: Text.WordWrap
            }

            Button {
                text: qsTr("Ok")
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: infoPopup.close()
            }
        }
    }
}
