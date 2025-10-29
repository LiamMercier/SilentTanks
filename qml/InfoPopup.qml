import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import GUICommon 1.0

Window
{
    id: infoPopup

    width: 350
    height: 150

    // Prevent resizing.
    minimumWidth: width
    maximumWidth: width
    minimumHeight: height
    maximumHeight: height

    // Stop the main application until response occurs.
    // modality: Qt.ApplicationModal
    modality: Qt.WindowModal

    // Flags to setup a popup style environment.
    flags: Qt.Dialog | Qt.WindowTitleHint | Qt.WindowSystemMenuHint

    // Alias for other files.
    property alias popupTitle: titleText.text
    property alias popupText: bodyText.text

    // Tell our popup queue in C++ that we closed a popup.
    onClosing: {
        Client.notify_popup_closed()
    }

    Rectangle {
        anchors.fill: parent
        color: "#202122"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            Label {
                id: titleText
                text: ""
                font.bold: true
                color: "#ffffff"

                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap

                Layout.fillWidth: true
                palette: infoPopup.palette
            }

            Label {
                id: bodyText
                text: ""
                color: "#f2f2f2"

                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap

                Layout.fillWidth: true
                palette: infoPopup.palette
            }

            Button {
                id: popupOkButton
                text: qsTr("Ok")
                Layout.alignment: Qt.AlignHCenter
                onClicked: infoPopup.close()

                background: Rectangle {
                    implicitWidth: 80
                    implicitHeight: 32
                    radius: 4
                    color: popupOkButton.down ? "#1b1c1d" :
                            popupOkButton.hovered ? "#3e4042" : "#323436"
                }

                contentItem: Text {
                    text: popupOkButton.text
                    color: "#f2f2f2"

                    horizontalAlignment: Text.AlignHCenter

                    anchors.centerIn: parent
                }
            }
        }
    }
}
