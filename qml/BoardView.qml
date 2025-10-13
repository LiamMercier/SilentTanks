import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import GUICommon 1.0
import SoundManager 1.0

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

    function cellToScreen(cx, cy) {
        var scaledTile = cellTileSize * scale
        var screenX = Math.round(cx * scaledTile - camX)
        var screenY = Math.round(cy * scaledTile - camY)
        return { x: screenX, y: screenY }
    }

    property int mapWidth: GameManager.map_width
    property int mapHeight: GameManager.map_height

    onMapWidthChanged: viewCanvas.requestPaint()
    onMapHeightChanged: viewCanvas.requestPaint()

    Connections {
        target: GameManager
        function onView_changed() {
            viewCanvas.requestPaint()
        }
    }

    // Cache svg images for rendering pipeline.

    property var tileKeys: ["visible",
                            "fog",
                            "cannon_north",
                            "cannon_north_east",
                            "cannon_east",
                            "cannon_south_east",
                            "cannon_south",
                            "cannon_south_west",
                            "cannon_west",
                            "cannon_north_west",

                            // player 1 tank
                            "tank_0_north",
                            "tank_0_north_east",
                            "tank_0_east",
                            "tank_0_south_east",
                            "tank_0_south",
                            "tank_0_south_west",
                            "tank_0_west",
                            "tank_0_north_west",

                            // player 2 tank
                            "tank_1_north",
                            "tank_1_north_east",
                            "tank_1_east",
                            "tank_1_south_east",
                            "tank_1_south",
                            "tank_1_south_west",
                            "tank_1_west",
                            "tank_1_north_west",

                            // player 3 tank
                            "tank_2_north",
                            "tank_2_north_east",
                            "tank_2_east",
                            "tank_2_south_east",
                            "tank_2_south",
                            "tank_2_south_west",
                            "tank_2_west",
                            "tank_2_north_west",

                            // player 4 tank
                            "tank_3_north",
                            "tank_3_north_east",
                            "tank_3_east",
                            "tank_3_south_east",
                            "tank_3_south",
                            "tank_3_south_west",
                            "tank_3_west",
                            "tank_3_north_west",

                            // player 5 tank
                            "tank_4_north",
                            "tank_4_north_east",
                            "tank_4_east",
                            "tank_4_south_east",
                            "tank_4_south",
                            "tank_4_south_west",
                            "tank_4_west",
                            "tank_4_north_west"
                           ]

    property var tileImageCache: ({})

    // creates images
    function ensureTileImages() {
        var pixelSize = Math.max(1, Math.round(cellTileSize * scale))

        for (var i = 0; i < tileKeys.length; i++)
        {
            var key = tileKeys[i]
            var entry = tileImageCache[key]

            if (!entry || entry.size !== pixelSize) {

                // destroy old image
                if (entry && entry.img) {
                    entry.img.destroy()
                }

                var img = Qt.createQmlObject('import QtQuick 2.15; Image { visible: false; }', boardViewRoot);

                // grab source
                img.width = pixelSize
                img.height = pixelSize
                img.source = "qrc:/svgs/tiles/" + key + ".svg"

                // store into the cache
                tileImageCache[key] = {img: img, size: pixelSize}
            }
        }

        console.log(pixelSize)
    }

    onScaleChanged: {
        ensureTileImages()
        viewCanvas.requestPaint()
    }

    onCellTileSizeChanged: {
        ensureTileImages()
        viewCanvas.requestPaint()
    }

    Component.onCompleted: ensureTileImages()

    // END tile related caching

    // Main render area.
    Canvas {
        id: viewCanvas
        anchors.fill: parent

        onPaint: {
            var cntx = getContext("2d")
            cntx.reset()
            cntx.clearRect(0, 0, width, height)

            cntx.imageSmoothingEnabled = false;

            var scaledTile = cellTileSize * boardViewRoot.scale

            var startX = Math.floor(camX / scaledTile)
            var startY = Math.floor(camY / scaledTile)
            var cols = Math.ceil(width / scaledTile) + 1
            var rows = Math.ceil(height / scaledTile) + 1

            // Prevent subpixel drawing.
            var camXRounded = Math.round(camX)
            var camYRounded = Math.round(camY)
            var tilePx = Math.max(1, Math.round(scaledTile))

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

                    var px = x * tilePx - camXRounded
                    var py = y * tilePx - camYRounded

                    // Draw tile.
                    if (cell.occupant !== 255 || cell.type === 0) {
                        var keys = sourcesForTile(cell)

                        for (var keyIndex = 0; keyIndex < keys.length; keyIndex++)
                        {
                            var imageEntry = tileImageCache[keys[keyIndex]]

                            if (imageEntry && imageEntry.img)
                            {
                                cntx.drawImage(imageEntry.img, px, py, tilePx, tilePx)
                            }
                            else
                            {
                                console.log("failed to load", keys[keyIndex])
                            }
                        }

                    }
                    else {
                        cntx.fillStyle = boardViewRoot.colorForTile(cell)
                        cntx.fillRect(px, py, tilePx, tilePx)
                    }

                    // Draw overlay if in setup.
                    if (GameManager.state === 0
                        && GameManager.valid_placement_tile(x, y))
                    {
                        cntx.fillStyle = "rgba(0, 200, 64, 0.04)"
                        cntx.fillRect(px, py, tilePx, tilePx)
                    }
                }
            }

            // Draw outline for selected cell.
            if (boardViewRoot.selectedCellX >= 0 && boardViewRoot.selectedCellY >= 0)
            {
                var sx = Math.round(boardViewRoot.selectedCellX * scaledTile - camX)
                var sy = Math.round(boardViewRoot.selectedCellY * scaledTile - camY)

                cntx.lineWidth = 2
                cntx.strokeStyle = "#008080"
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
                        viewCanvas.requestPaint()

                        // open popup.
                        showActionsPopup(boardViewRoot.selectedCellX,
                                         boardViewRoot.selectedCellY)
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

    function sourcesForTile(cell) {
        var result = [];

        // If empty
        if (cell.type === 0)
        {
            // If visible
            if (cell.visible)
            {
                result.push("visible")
            }
            else
            {
                result.push("fog")
            }
        }

        // If tank exists, grab tank and barrel.
        if (cell.occupant !== 255)
        {
            // Fetch tank data.
            var tankData = GameManager.get_tank_data(cell.occupant)

            var filename_prefix = "tank_" + tankData.owner + "_"

            // Find the tank orientation and append correct cannon image.
            switch(tankData.tank_direction)
            {
                case 0:
                {
                    result.push(filename_prefix + "north")
                    break
                }
                case 1:
                {
                    result.push(filename_prefix + "north_east")
                    break
                }
                case 2:
                {
                    result.push(filename_prefix + "east")
                    break
                }
                case 3:
                {
                    result.push(filename_prefix + "south_east")
                    break
                }
                case 4:
                {
                    result.push(filename_prefix + "south")
                    break
                }
                case 5:
                {
                    result.push(filename_prefix + "south_west")
                    break
                }
                case 6:
                {
                    result.push(filename_prefix + "west")
                    break
                }
                case 7:
                {
                    result.push(filename_prefix + "north_west")
                    break
                }
            }

            // Find the barrel orientation and append correct cannon image.
            switch(tankData.barrel_direction)
            {
                case 0:
                {
                    result.push("cannon_north")
                    break
                }
                case 1:
                {
                    result.push("cannon_north_east")
                    break
                }
                case 2:
                {
                    result.push("cannon_east")
                    break
                }
                case 3:
                {
                    result.push("cannon_south_east")
                    break
                }
                case 4:
                {
                    result.push("cannon_south")
                    break
                }
                case 5:
                {
                    result.push("cannon_south_west")
                    break
                }
                case 6:
                {
                    result.push("cannon_west")
                    break
                }
                case 7:
                {
                    result.push("cannon_north_west")
                    break
                }
            }
        }

        return result
    }

    function colorForTile(cell) {
        if (cell.occupant === 255)
        {
            switch(cell.type) {
                // Empty.
                case 0:
                {
                    if (cell.visible)
                    {
                        return "#D3D3D3";
                    }
                    else
                    {
                        return "#808080";
                    }
                }
                // Foliage.
                case 1:
                {
                    if (cell.visible)
                    {
                        return "#117C13";
                    }
                    else
                    {
                        return "#284903";
                    }
                }
                // Terrain.
                case 2:
                {
                    if (cell.visible)
                    {
                        return "#555555";
                    }
                    else
                    {
                        return "#2F2F2F";
                    }
                }
                // Should never occur.
                default: return "#666666";
            }
        }
        else
        {
            // Just give occupants some random color for now.
            if (cell.occupant >= 0 && cell.occupant < 2)
            {
                return "#950606"
            }
            else if (cell.occupant >= 2 && cell.occupant < 4)
            {
                return "#111184"
            }
            else
            {
                return "#000000"
            }
        }
    }

    // Hold list of actions given the game state.
    ListModel {
        id: actionsModel
    }

    Popup {
        id: actionPopup
        modal: true
        focus: true

        padding: 0

        background: Rectangle {
            color: "transparent"
        }

        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        property var svgMap: ({
            "rotate_barrel_left" : "qrc:/svgs/buttons/barrel_left",
            "move_forward" : "qrc:/svgs/buttons/tank_forward",
            "rotate_barrel_right" : "qrc:/svgs/buttons/barrel_right",
            "rotate_tank_left" : "qrc:/svgs/buttons/tank_left",
            "rotate_tank_right" : "qrc:/svgs/buttons/tank_right",
            "reload" : "qrc:/svgs/buttons/reload",
            "move_reverse" : "qrc:/svgs/buttons/tank_reverse",
            "fire" : "qrc:/svgs/buttons/fire",
            "" : "",
            "no_op" : ""
        })

        contentItem: Rectangle {
            id: popupContentItemRect
            implicitWidth: boardViewRoot.width * 0.30
            implicitHeight: boardViewRoot.width * 0.30
            color: Qt.rgba(1, 1, 1, 0.25)
            border.width: 1

            Rectangle {
                anchors.fill: parent
                color: "transparent"
                opacity: 1.0

                Image {
                    anchors.fill: parent
                    source: "qrc:/svgs/buttons/turn_background.svg"
                    fillMode: Image.PreserveAspectFit
                }

                GridLayout {
                    id: grid
                    anchors.margins: 0
                    anchors.fill: parent
                    rowSpacing: 0
                    columnSpacing: 0

                    // 3x3 action grid.
                    rows: 3
                    columns: 3

                    Repeater {
                        model: actionsModel
                        delegate: Button {
                            id: actionButton
                            Layout.preferredWidth: grid.width / grid.columns
                            Layout.preferredHeight: grid.height / grid.rows
                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                            text: model.actionText
                            property string svgSource: {
                                var src = ""

                                var cell = GameManager.cell_at(selectedCellX, selectedCellY)
                                var tank_data = GameManager.get_tank_data(cell.occupant)

                                // Handle reload specific icon.
                                if (model.action == "reload" && tank_data.loaded)
                                {
                                    src = "qrc:/svgs/buttons/reload_loaded.svg"
                                }
                                // Handle middle "button" icon
                                else if (model.action == "no_op")
                                {
                                    switch (tank_data.health)
                                    {
                                        case 1:
                                        {
                                            src = "qrc:/svgs/buttons/one_health.svg"
                                            break
                                        }
                                        case 2:
                                        {
                                            src = "qrc:/svgs/buttons/two_health.svg"
                                            break
                                        }
                                        case 3:
                                        {
                                            src = "qrc:/svgs/buttons/three_health.svg"
                                            break
                                        }
                                    }
                                    actionButton.enabled = false
                                }
                                // Handle place directions.
                                else if (model.action == "place")
                                {
                                    src = "qrc:/svgs/buttons/place_dir_" + model.dir + ".svg"
                                }
                                // Otherwise, give the default.
                                else
                                {
                                    src = actionPopup.svgMap[model.action] ? actionPopup.svgMap[model.action] : actionPopup.svgMap["no_op"]
                                }

                                return src
                            }

                            background: Rectangle {
                                color: "transparent"
                            }

                            // Load icon for button based on action type.
                            contentItem: Column {
                                anchors.fill: parent
                                anchors.centerIn: parent

                                Image {
                                    source: svgSource
                                    width: actionButton.width
                                    height: actionButton.height

                                    fillMode: Image.PreserveAspectFit
                                }
                            }

                            onClicked: {
                                switch (model.action)
                                {
                                    case "place":
                                    {
                                        if (selectedCellX >= 0 && selectedCellY >= 0)
                                        {
                                            Client.send_place_tank(selectedCellX,
                                                                   selectedCellY,
                                                                   model.dir)
                                        }
                                        break
                                    }
                                    case "rotate_barrel_left":
                                    {
                                        if (selectedCellX >= 0 && selectedCellY >= 0)
                                        {
                                            Client.send_rotate_barrel(selectedCellX,
                                                                      selectedCellY,
                                                                      1)
                                        }
                                        break
                                    }
                                    case "move_forward":
                                    {
                                        if (selectedCellX >= 0 && selectedCellY >= 0)
                                        {
                                            Client.send_move_tank(selectedCellX,
                                                                selectedCellY,
                                                                0)
                                        }
                                        break
                                    }
                                    case "rotate_barrel_right":
                                    {
                                        if (selectedCellX >= 0 && selectedCellY >= 0)
                                        {
                                            Client.send_rotate_barrel(selectedCellX,
                                                                    selectedCellY,
                                                                    0)
                                        }
                                        break
                                    }
                                    case "rotate_tank_left":
                                    {
                                        if (selectedCellX >= 0 && selectedCellY >= 0)
                                        {
                                            Client.send_rotate_tank(selectedCellX,
                                                                    selectedCellY,
                                                                    1)
                                        }
                                        break
                                    }
                                    case "fire":
                                    {
                                        if (selectedCellX >= 0 && selectedCellY >= 0)
                                        {
                                            Client.send_fire_tank(selectedCellX,
                                                                selectedCellY)
                                        }
                                        break
                                    }
                                    case "rotate_tank_right":
                                    {
                                        if (selectedCellX >= 0 && selectedCellY >= 0)
                                        {
                                            Client.send_rotate_tank(selectedCellX,
                                                                    selectedCellY,
                                                                    0)
                                        }
                                        break
                                    }
                                    case "reload":
                                    {
                                        if (selectedCellX >= 0 && selectedCellY >= 0)
                                        {
                                            Client.send_reload_tank(selectedCellX,
                                                                    selectedCellY)
                                        }
                                        break
                                    }
                                    case "move_reverse":
                                    {
                                        if (selectedCellX >= 0 && selectedCellY >= 0)
                                        {
                                            Client.send_move_tank(selectedCellX,
                                                                selectedCellY,
                                                                1)
                                        }
                                        break
                                    }
                                    case "no_op":
                                    {
                                        console.log("no_op found")
                                        break
                                    }
                                    default:
                                    {
                                        break
                                    }
                                }

                                actionPopup.close()
                            }

                            implicitWidth: popupContentItemRect.width / grid.rows
                            implicitHeight: popupContentItemRect.height / grid.columns
                        }
                    }
                }
            }

        }

        onClosed: {
            selectedCellX = -1
            selectedCellY = -1
            viewCanvas.requestPaint()
        }

    }

    function showActionsPopup(cx, cy) {
        actionsModel.clear()

        // Give no actions for negative tile coords.
        if (cx < 0 || cy < 0)
        {
            selectedCellX = -1
            selectedCellY = -1
            return
        }

        var cell = GameManager.cell_at(cx, cy)
        var state = GameManager.state

        // If in setup.
        if (state === 0)
        {
            // Don't do anything on invalid placement tiles.
            if (!GameManager.valid_placement_tile(cx, cy))
            {
                return
            }

            // Allow place only if not occupied and not terrain.
            if (cell.occupant === 255 && cell.type !== 2)
            {
                actionsModel.append({ action: "place", actionText: "Place Tank", dir: 7 })
                actionsModel.append({ action: "place", actionText: "Place Tank", dir: 0 })
                actionsModel.append({ action: "place", actionText: "Place Tank", dir: 1 })

                actionsModel.append({ action: "place", actionText: "Place Tank", dir: 6 })
                actionsModel.append({ action: "no_op", actionText: "" })
                actionsModel.append({ action: "place", actionText: "Place Tank", dir: 2 })

                actionsModel.append({ action: "place", actionText: "Place Tank", dir: 5 })
                actionsModel.append({ action: "place", actionText: "Place Tank", dir: 4 })
                actionsModel.append({ action: "place", actionText: "Place Tank", dir: 3 })
            }
        }
        // If playing.
        else if (state === 1)
        {
            // Add commands if tank exists.
            if (cell.occupant !== 255)
            {
                // Unless it is not our tank.
                if (GameManager.is_friendly_tank(cell.occupant))
                {
                    // First 3.
                    actionsModel.append({ action: "rotate_barrel_left", actionText: "Rotate\nBarrel\nLeft" })
                    actionsModel.append({ action: "move_forward", actionText: "Move\nForward" })
                    actionsModel.append({ action: "rotate_barrel_right", actionText: "Rotate\nBarrel\nRight" })

                    // Next 3.
                    actionsModel.append({ action: "rotate_tank_left", actionText: "Rotate\nTank\nLeft" })
                    actionsModel.append({ action: "no_op", actionText: "" })
                    actionsModel.append({ action: "rotate_tank_right", actionText: "Rotate\nTank\nRight" })

                    // Bottom 3.
                    actionsModel.append({ action: "reload", actionText: "Reload" })
                    actionsModel.append({ action: "move_reverse", actionText: "Move\nReverse" })
                    actionsModel.append({ action: "fire", actionText: "Fire" })
                }
            }
            // Otherwise, stop.
            else
            {
                selectedCellX = -1
                selectedCellY = -1
                return
            }
        }
        // Otherwise, game is done.
        else
        {
            selectedCellX = -1
            selectedCellY = -1
            return
        }

        // compute screen coordinates for popup.
        var pos = cellToScreen(cx, cy)

        var scaledTile = cellTileSize * scale
        var tilePx = Math.max(1, Math.round(scaledTile))

        var px = pos.x + tilePx
        var py = pos.y - popupContentItemRect.height

        // Clamp to window.
        px = Math.max(4, Math.min(px, boardViewRoot.width - actionPopup.implicitWidth - 4))
        py = Math.max(4, Math.min(py, boardViewRoot.height - actionPopup.implicitHeight - 4))

        actionPopup.x = px
        actionPopup.y = py
        actionPopup.open()
    }


}
