import QtQuick 2.4
import QtQuick.Layouts 1.1
import Cardirector.Gui 1.0
import Cardirector.Device 1.0
import Cardirector.Resource 1.0
import Sanguosha 1.0
import "RoomElement"
import "../engine.js" as Engine

RoomScene {
    property var dashboardModel: null
    property var photoModel: []
    property int playerNum: 0

    id: roomScene
    anchors.fill: parent

    Image {
        source: config.tableImage
        anchors.fill: parent
        focus: true
    }

    MouseArea {
        anchors.fill: parent
        onPressed: parent.forceActiveFocus();
    }

    ImageProvider {
        id: generalImage
        providerId: "general"

        function imagePath(imageId, requestedSize)
        {
            return "image/general/" + imageId + ".png";
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Device.gu(3)

        RowLayout {
            spacing: 1

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Item {
                    id: roomArea
                    anchors.fill: parent
                    anchors.margins: Device.gu(10)

                    Repeater {
                        id: photos
                        model: photoModel
                        Photo {
                            screenName: modelData.screenName
                            hp: modelData.hp
                            maxHp: modelData.maxHp
                            headGeneral: modelData.headGeneralName
                            deputyGeneral: modelData.deputyGeneralName
                            phase: modelData.phase
                            seat: modelData.seat
                            selectable: true
                            state: "candidate"
                        }
                    }

                    onWidthChanged: arrangePhotos();
                    onHeightChanged: arrangePhotos();

                    InvisibleCardArea {
                        id: drawPile
                        x: parent.width / 2
                        y: parent.height * 0.7
                    }

                    TablePile {
                        id: tablePile
                        width: parent.width * 0.6
                        height: Device.gu(150)
                        x: parent.width * 0.2
                        y: parent.height * 0.5
                    }

                    ProgressBar {
                        id: globalProgressBar
                        width: 400
                        height: 15
                        anchors.centerIn: parent
                        visible: false
                    }
                }
            }

            ColumnLayout {
                spacing: 1
                Layout.fillWidth: false
                Layout.preferredWidth: Device.gu(275)

                LogBox {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                ChatBox {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Device.gu(200)
                }
            }
        }

        Dashboard {
            id: dashboard

            acceptButton.enabled: true
            onAccepted: roomScene.accepted();

            Connections {
                target: roomScene
                onDashboardModelChanged: {
                    var model = dashboardModel[0];
                    dashboard.seatNumber = Qt.binding(function(){return model.seat});
                    dashboard.phase = Qt.binding(function(){return model.phase});
                    dashboard.hp = Qt.binding(function(){return model.hp});
                    dashboard.maxHp = Qt.binding(function(){return model.maxHp});
                    dashboard.headGeneralName = Qt.binding(function(){return model.headGeneralName});
                    dashboard.deputyGeneralName = Qt.binding(function(){return model.deputyGeneralName});
                    dashboard.headGeneralKingdom = dashboard.deputyGeneralKingdom = Qt.binding(function(){return model.kingdom});
                }
            }

            Connections {
                target: dashboard.handcardArea
                onSelectedChanged: {
                    var ids = [];
                    var cards = dashboard.handcardArea.getSelectedCards();
                    for (var i = 0; i < cards.length; i++)
                        ids.push(cards[i].cid);
                    roomScene.cardSelected(ids);
                }
            }
        }
    }

    Loader {
        id: popupBox
        onSourceChanged: {
            if (item === null)
                return;
            item.finished.connect(function(){
                source = "";
            });
            item.widthChanged.connect(function(){
                popupBox.moveToCenter();
            });
            item.heightChanged.connect(function(){
                popupBox.moveToCenter();
            });
            moveToCenter();
        }

        function moveToCenter()
        {
            item.x = Math.round((roomArea.width - item.width) / 2);
            item.y = Math.round(roomArea.height * 0.67 - item.height / 2);
        }
    }

    onChooseGeneralStarted: {
        popupBox.source = "RoomElement/ChooseGeneralBox.qml";
        var box = popupBox.item;
        box.accepted.connect(function(){
            roomScene.chooseGeneralFinished(box.headGeneral, box.deputyGeneral);
        });
        for (var i = 0; i < generals.length; i++)
            box.model.append(generals[i]);
        box.arrangeCards();
    }

    onCardsMoved: {
        var component = Qt.createComponent("RoomElement/CardItem.qml");
        if (component.status !== Component.Ready)
            return;

        var cardItems = [], i;
        for (i = 0; i < moves.length; i++) {
            var move = moves[i];
            var from = getAreaItem(move.from);
            var to = getAreaItem(move.to);
            if (!from || !to)
                continue;
            var items = from.remove(move.cards);
            if (items.length > 0)
                to.add(items);
            to.updateCardPosition(true);
        }
    }

    onCardEnabled: dashboard.handcardArea.enableCards(cardIds);
    onPlayerNumChanged: arrangePhotos();

    function arrangePhotos()
    {
        /*
        Layout:
           col1           col2
        _______________________
        |_2_|______1_______|_0_| row1
        |   |              |   |
        | 4 |    table     | 3 |
        |___|______________|___|
        |      dashboard       |
        ------------------------
        region 5 = 0 + 3, region 6 = 2 + 4, region 7 = 0 + 1 + 2
        */

        var regularSeatIndex = [
            [1],
            [5, 6],
            [5, 1, 6],
            [3, 1, 1, 4],
            [3, 7, 7, 7, 4],
            [5, 5, 1, 1, 6, 6],
            [5, 5, 1, 1, 1, 6, 6],
            [5, 5, 1, 1, 1, 1, 6, 6],
            [3, 3, 7, 7, 7, 7, 7, 4, 4]
        ];
        var seatIndex = regularSeatIndex[playerNum - 2];
        var horizontalBorder = roomArea.height * 0.4;
        var sideWidth = playerNum < 9 ? 0.2 : 0.15;
        var verticalBorders = [roomArea.width * sideWidth, roomArea.width * (1 - sideWidth)];
        var regions = [
            {top: 0, bottom: horizontalBorder, left: verticalBorders[1], right: roomArea.width, players: []},
            {top: 0, bottom: horizontalBorder, left: verticalBorders[0], right: verticalBorders[1], players: []},
            {top: 0, bottom: horizontalBorder, left: 0, right: verticalBorders[0], players: []},
            {top: horizontalBorder, bottom: roomArea.height, left: verticalBorders[1], right: roomArea.width, players: []},
            {top: horizontalBorder, bottom: roomArea.height, left: 0, right: verticalBorders[0], players: []},
            {top: 0, bottom: roomArea.height, left: verticalBorders[1], right: roomArea.width, players: []},
            {top: 0, bottom: roomArea.height, left: 0, right: verticalBorders[0], players: []},
            {top: 0, bottom: horizontalBorder, left: 0, right: roomArea.width, players: []}
        ];

        var roomAreaPadding = Device.gu(10);
        var item, region, i, subindex, x, y, spacing;

        for (i = 0; i < playerNum - 1; i++)
            regions[seatIndex[i]].players.push(i);

        for (i = 0; i < playerNum - 1; i++) {
            item = photos.itemAt(i);
            region = regions[seatIndex[i]];
            subindex = region.players.indexOf(i);

            //Top Area 1 or 7
            if (seatIndex[i] === 1 || seatIndex[i] === 7) {
                if (playerNum === 6 || playerNum === 10) {
                    spacing = ((region.right - region.left) - region.players.length * item.width) / (region.players.length + 1);
                    x = region.right - (item.width + spacing) * (subindex + 1);
                } else {
                    x = region.right - item.width / 2 - (region.right - region.left) / region.players.length / 2 * (subindex * 2 + 1);
                }
            //Left Area 4 or 6, Right Area 3 or 5
            } else {
                x = (region.left + region.right - item.width) / 2;
            }

            //Top Area 1 or 7
            if (seatIndex[i] === 1 || seatIndex[i] === 7) {
                y = (region.top + region.bottom - item.height) / 2;
            } else {
                spacing = ((region.bottom - region.top) - region.players.length * item.height) / (region.players.length + 1);
                //Right Area 3 or 5
                if (seatIndex[i] === 3 || seatIndex[i] === 5) {
                    y = region.bottom - (spacing + item.height) * (subindex + 1);
                //Left Area 4 or 6
                } else {
                    y = region.top + spacing * (subindex + 1) + item.height * subindex;
                }
            }
            item.x = Math.round(x);
            item.y = Math.round(y);
        }
    }

    function showIndicatorLine(from, tos)
    {
        var component = Qt.createComponent("RoomElement/IndicatorLine.qml");
        if (component.status !== Component.Ready)
            return;

        var fromItem = getItemBySeat(from);
        var fromPos = mapFromItem(fromItem, fromItem.width / 2, fromItem.height / 2);

        var end = [];
        for (var i = 0; i < tos.length; i++) {
            if (from === tos[i])
                continue;
            var toItem = getItemBySeat(tos[i]);
            var toPos = mapFromItem(toItem, toItem.width / 2, toItem.height / 2);
            end.push(toPos);
        }

        var color = Engine.kingdomColor[fromItem.userRole];
        var line = component.createObject(root, {start: fromPos, end: end, color: color});
        line.running = true;
    }

    function getItemBySeat(seat)
    {
        if (seat === dashboard.seatNumber)
            return dashboard;
        var i = (seat - dashboard.seatNumber - 1 + playerNum) % playerNum;
        return photos.itemAt(i);
    }

    function getAreaItem(area)
    {
        if (area.type === "drawPile") {
            return drawPile;
        } else if (area.type === "table") {
            return tablePile;
        }

        var photo = getItemBySeat(area.seat);
        if (!photo)
            return null;

        if (area.type === "hand") {
            return photo.handcardArea;
        } else if (area.type === "equip")
            return photo.equipArea;
        else if (area.type === "delayedTrick")
            return photo.delayedTrickArea;

        return null;
    }

    function getSelectedSeats()
    {
        var selected = [];
        for (var i = 0; i < photos.count; i++) {
            var photo = photos.itemAt(i);
            if (photo.state === "selected")
                selected.push(photo.seat);
        }
        return selected;
    }
}
