/*
    SPDX-FileCopyrightText: 2019 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1 as QQC2
import QtLocation 5.11 as QtLocation
import QtPositioning 5.11
import org.kde.kirigami 2.17 as Kirigami
import org.kde.itinerary 1.0
import "." as App

Kirigami.Page {
    id: root
    property variant coordinate: QtPositioning.coordinate(0, 0)

    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0

   actions.main: Kirigami.Action {
        icon.name: "crosshairs"
        text: i18n("Pick Location")
        onTriggered: {
            coordinate = map.center;
            applicationWindow().pageStack.goBack();
        }
    }

    QtLocation.Plugin {
        id: mapPlugin
        required.mapping: QtLocation.Plugin.AnyMappingFeatures
        preferred: ["osm"]
    }

    QtLocation.Map {
        id: map
        anchors.fill: parent
        center: root.coordinate
        plugin: mapPlugin

        QtLocation.MapQuickItem {
            coordinate: map.center
            anchorPoint { x: icon.width / 2; y: icon.height / 2 }
            sourceItem: Kirigami.Icon {
                id: icon
                source: "crosshairs"
                width: height
                height: Kirigami.Units.iconSizes.large
                color: Kirigami.Theme.negativeTextColor
            }
        }
    }
}
