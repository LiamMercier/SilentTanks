pragma Singleton
import QtQuick 2.15
import QtMultimedia

QtObject {
    id: soundManager

    property bool muted: false

    // client-state.h must be changed when this is changed.
    property var moveSoundFiles: [
        "NotifyTurn.wav",
        "Move.wav",
        "CannonFiring.wav",
        "OutOfAmmo.wav",
        "Reload.wav",
        "Rotate.wav"
    ]

    // Sounds for moves.
    property var movePlayers: []

    property int playersPerSound: 3

    Component.onCompleted: {
        movePlayers = []

        for (var i = 0; i < moveSoundFiles.length; i++) {
            movePlayers[i] = []

            for (var j = 0; j < playersPerSound; j++) {
                var obj = Qt.createQmlObject('import QtMultimedia; SoundEffect {}', soundManager)

                if (obj) {
                    obj.source = "qrc:/sounds/" + moveSoundFiles[i]
                    obj.volume = 1.0
                    movePlayers[i].push(obj)
                }

                else {
                    console.error("Failed to create SoundEffect for", moveSoundFiles[i])
                }
            }
        }
    }

    function playSound(moveIndex) {
        if (muted) {
            return
        }

        if (moveIndex < 0 || moveIndex >= movePlayers.length)
        {
            console.warn("Invalid audio index", moveIndex)
            return
        }

        var players = movePlayers[moveIndex]

        // find free worker
        for (var i = 0; i < players.length; i++) {
            var p = players[i]

            if (!p.playing) {
                p.play()
                return
            }
        }
    }

    function setMuted(m) {
        muted = m
    }

}
