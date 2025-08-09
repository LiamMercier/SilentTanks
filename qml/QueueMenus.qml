import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import GUICommon 1.0

Item {
    id: rootQueueMenu
    implicitWidth: 400
    implicitHeight: 250

    // Hold the selected mode, possibly a no-op.
    property int selectedMode: Client.selected_mode

    // Start of queue buttons, lay them out in
    // a row for the user.
    RowLayout {
        anchors.fill: parent
        spacing: 6

        ListModel {
            id: modeModel
        }

        Component.onCompleted:
        {
            modeModel.append({label:"Casual 1v1", mode: QueueType.ClassicTwoPlayer})
            modeModel.append({label:"Ranked 1v1", mode: QueueType.RankedTwoPlayer})
        }

        ComboBox {
            id: modeCombo
            model: modeModel
            textRole: "label"
            Layout.preferredWidth: 140

            onCurrentIndexChanged:
            {
                var newMode = modeModel.get(currentIndex).mode

                if (Client.selected_mode !== newMode)
                {
                    // Write mode change to C++ client.
                    Client.set_selected_mode(newMode)
                }
            }
        }

    }
}
