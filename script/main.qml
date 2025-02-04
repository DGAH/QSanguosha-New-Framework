import QtQuick 2.4
import QtMultimedia 5.4
import Cardirector.Resource 1.0
import "Client"
import "utility.js" as Utility

Item {
    id: root

    ClientSettings {
        id: config
    }

    ImageProvider {
        id: rootImage
        providerId: "root"

        function imagePath(imageId, requestedSize)
        {
            if (imageId.indexOf('.') === -1)
                return "image/" + imageId + ".png";
            else
                return "image/" + imageId;
        }
    }

    ImageProvider {
        id: mogaraImage
        providerId: "mogara"

        function imagePath(imageId, requestedSize)
        {
            return "image/mogara/" + imageId + ".png";
        }
    }

    ImageProvider {
        providerId: "background"

        function imagePath(imageId, requestedSize) {
            // We prefer to using compact pictures as background to save storage space
            // @todo: consider supporting more common image formats
            return "image/background/" + imageId + ".jpg";
        }
    }

    ImageProvider {
        providerId: "tileicon"

        function imagePath(imageId, requestedSize) {
            return "image/tileIcon/" + imageId + ".png"
        }
    }

    Loader {
        id: startSceneLoader
        anchors.fill: parent
    }

    Loader {
        id: dialogLoader
        z: 100
        anchors.fill: parent
        onSourceChanged: startSceneLoader.visible = (source == "");
    }

    Loader {
        id: splashLoader
        anchors.fill: parent
        focus: true
    }

    Component.onCompleted: {
        var skip_splash = false;
        for (var i = 0; i < Qt.application.arguments.length; i++) {
            var schema = "qsanguosha://";
            var arg = Qt.application.arguments[i];
            if (arg.substr(0, schema.length).toLowerCase() === schema) {
                dialogLoader.setSource("Gui/Dialog/StartGameDialog.qml");
                arg = arg.substr(schema.length, arg.indexOf("/", schema.length) - schema.length);
                dialogLoader.item.serverAddress = arg;
                dialogLoader.item.accepted();
                skip_splash = true;
                break;
            }
        }

        if (skip_splash || Qt.application.arguments.contains("--skip-splash")) {
            startSceneLoader.source = "Gui/StartScene.qml";
        } else {
            splashLoader.source = "Gui/Splash.qml";
            splashLoader.item.disappearing.connect(function(){
                startSceneLoader.source = "Gui/StartScene.qml";
            });
            splashLoader.item.disappeared.connect(function(){
                splashLoader.source = "";
            });
        }
    }
}
