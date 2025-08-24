import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import GUICommon 1.0

Item {
    id: boardViewRoot

    // Display 10 tiles left/right based on screen width, let height follow.
    property real cellTileSize: width / 10

    // Zoom amount for scroll wheel.
    property real scale: 1

    property real camX: 0
    property real camY: 0

    // Record selected cell.
    property int selectedCellX: -1
    property int selectedCellY: -1

    function clampCamera() {
        var maxX = Math.max(0, mapWidth * cellTileSize * scale - width)
        var maxY = Math.max(0, mapHeight * cellTileSize * scale - height)

        // Set the camera to either the camera's new position, or max
        // permitted location of the game board.
        camX = Math.max(-cellTileSize, Math.min(camX, maxX + cellTileSize))
        camY = Math.max(-cellTileSize, Math.min(camY, maxY + cellTileSize))
    }

    property int mapWidth: GameManager.map_width
    property int mapHeight: GameManager.map_height

    onMapWidthChanged: viewCanvas.requestPaint()
    onMapHeightChanged: viewCanvas.requestPaint()
    onCellTileSizeChanged: viewCanvas.requestPaint()
    onScaleChanged: viewCanvas.requestPaint()

    Connections {
        target: GameManager
        function onView_changed() {
            viewCanvas.requestPaint()
        }
    }

    // Main render area.
    Canvas {
        id: viewCanvas
        anchors.fill: parent

        onPaint: {
            var cntx = getContext("2d")
            cntx.reset()
            cntx.clearRect(0, 0, width, height)

            var scaledTile = cellTileSize * boardViewRoot.scale

            var startX = Math.floor(camX / scaledTile)
            var startY = Math.floor(camY / scaledTile)
            var cols = Math.ceil(width / scaledTile) + 1
            var rows = Math.ceil(height / scaledTile) + 1

            // Go through each element and draw it.
            for (var y = startY; y < startY + rows; y++)
            {
                if (y < 0 || y >= mapHeight)
                {
                    continue
                }

                for (var x = startX; x < startX + cols; x++)
                {
                    if (x < 0 || x >= mapWidth)
                    {
                        continue
                    }

                    // Grab QVariantMap for this cell.
                    var cell = GameManager.cell_at(x, y)
                    var type = cell.type
                    var occupant = cell.occupant
                    var visible = cell.visible

                    var px = Math.round(x * scaledTile - camX)
                    var py = Math.round(y * scaledTile - camY)

                    cntx.fillStyle = boardViewRoot.colorForType(type)

                    if (!visible)
                    {
                        cntx.globalAlpha = 0.35
                    }

                    else
                    {
                        cntx.globalAlpha = 1.0
                    }

                    cntx.fillRect(px, py, scaledTile, scaledTile)

                    // Temporary, draw outline. TODO: remove this.
                    cntx.strokeStyle = "rgba(0, 0, 0, 1)"
                    cntx.strokeRect(px + 0.5, py + 0.5, scaledTile - 1, scaledTile - 1)
                }
            }

            // Draw outline for selected cell.
            if (boardViewRoot.selectedCellX >= 0 && boardViewRoot.selectedCellY >= 0)
            {
                var sx = Math.round(boardViewRoot.selectedCellX * scaledTile - camX)
                var sy = Math.round(boardViewRoot.selectedCellY * scaledTile - camY)

                cntx.lineWidth = 2
                cntx.strokeStyle = "#ff8800"
                cntx.strokeRect(sx + 1, sy + 1, scaledTile - 2, scaledTile - 2)
            }
        }

        onWidthChanged: viewCanvas.requestPaint()
        onHeightChanged: viewCanvas.requestPaint()

    }

    // Interaction area.
    MouseArea {
        id: inputArea

        anchors.fill: parent
        drag.target: null
        property int lastX: 0
        property int lastY: 0

        property int pressX: 0
        property int pressY: 0
        property int pressButton: 0

        property bool isPanning: false

        // minimum pixels to start drag, squared.
        property int dragThresholdSquared: 25

        // convert (mx, my) to cell coordinates.
        function mouseToCell(mx, my) {
            var worldX = (mx + boardViewRoot.camX) / boardViewRoot.scale
            var worldY = (my + boardViewRoot.camY) / boardViewRoot.scale

            // Integer clipped cell indices.
            var cx = Math.floor(worldX / boardViewRoot.cellTileSize)
            var cy = Math.floor(worldY / boardViewRoot.cellTileSize)

            return { x : cx, y: cy }
        }

        onPressed: function(mouse) {
            lastX = mouse.x
            lastY = mouse.y
            pressX = mouse.x
            pressY = mouse.y
            pressButton = mouse.button
            isPanning = false

        }

        onPositionChanged: function(mouse) {
            // Handle dragging with thresholding.
            if (mouse.buttons & Qt.LeftButton)
            {
                if (!isPanning) {
                    var dx = mouse.x - pressX
                    var dy = mouse.y - pressY
                    var dist = (dx * dx) + (dy * dy)
                    if (dist >= dragThresholdSquared) {
                        isPanning = true
                        cursorShape = Qt.ClosedHandCursor

                        lastX = mouse.x
                        lastY = mouse.y
                    }
                    // Ignore position change, too small so far.
                    else {
                        return
                    }
                }

                // If panning is true, we start doing work.

                var dx2 = mouse.x - lastX
                var dy2 = mouse.y - lastY
                lastX = mouse.x
                lastY = mouse.y

                boardViewRoot.camX -= dx2
                boardViewRoot.camY -= dy2
                boardViewRoot.clampCamera()
                viewCanvas.requestPaint()
                return
            }
        }

        onReleased: function(mouse) {
            if (pressButton === Qt.LeftButton)
            {
                // Reset cursor if needed.
                if (isPanning)
                {
                    isPanning = false
                    cursorShape = Qt.ArrowCursor
                }
                // Otherwise, handle selection.
                else
                {
                    var cell = mouseToCell(mouse.x, mouse.y)

                    if (cell.x >= 0 && cell.x < boardViewRoot.mapWidth
                        && cell.y >= 0 && cell.y < boardViewRoot.mapHeight)
                    {
                        boardViewRoot.selectedCellX = cell.x
                        boardViewRoot.selectedCellY = cell.y
                        console.log(boardViewRoot.selectedCellX, boardViewRoot.selectedCellY)
                        viewCanvas.requestPaint()
                    }
                }
            }

            pressButton = 0

        }

        onWheel: function(wheel){
            var oldScale = boardViewRoot.scale
            if (wheel.angleDelta.y > 0) {
                boardViewRoot.scale *= 1.1
            }
            else {
                boardViewRoot.scale /= 1.1
            }

            // Clamp zoom.
            boardViewRoot.scale = Math.max(0.25, Math.min(4.0, boardViewRoot.scale))

            // Pan to cursor.
            var worldX = (wheel.x + boardViewRoot.camX) / oldScale
            var worldY = (wheel.y + boardViewRoot.camY) / oldScale
            boardViewRoot.camX = worldX * boardViewRoot.scale - wheel.x
            boardViewRoot.camY = worldY * boardViewRoot.scale - wheel.y
            boardViewRoot.clampCamera()
            viewCanvas.requestPaint()
        }
    }

    function colorForType(t) {
        switch(t) {
            case 0: return "#333";
            case 1: return "#9f9";
            case 2: return "#49a";
            default: return "#666";
        }
    }


}
