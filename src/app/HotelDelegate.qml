/*
    Copyright (C) 2018 Volker Krause <vkrause@kde.org>

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1 as QQC2
import org.kde.kirigami 2.4 as Kirigami
import org.kde.itinerary 1.0
import "." as App

App.TimelineDelegate {
    id: root

    header: Rectangle {
        id: headerBackground
        Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
        Kirigami.Theme.inherit: false
        color: Kirigami.Theme.backgroundColor
        implicitHeight: headerLayout.implicitHeight + Kirigami.Units.largeSpacing * 2
        anchors.leftMargin: -root.leftPadding
        anchors.topMargin: -root.topPadding
        anchors.rightMargin: -root.rightPadding

        RowLayout {
            id: headerLayout
            anchors.fill: parent
            anchors.margins: Kirigami.Units.largeSpacing

            QQC2.Label {
                text: qsTr("🏨 %1").arg(reservation.reservationFor.name)
                font.pointSize: Kirigami.Theme.defaultFont.pointSize * root.headerFontScale
                Layout.fillWidth: true
            }
        }
    }

    contentItem: ColumnLayout {
        id: topLayout

        App.PlaceDelegate {
            place: reservation.reservationFor
            Layout.fillWidth: true
        }
        QQC2.Label {
            text: qsTr("Check-in time: %1")
                .arg(Localizer.formatTime(reservation, "checkinTime"))
            color: Kirigami.Theme.textColor
        }
        QQC2.Label {
            text: qsTr("Check-out time: %1")
                .arg(Localizer.formatDateTime(reservation, "checkoutTime"))
            color: Kirigami.Theme.textColor
        }

    }
}
