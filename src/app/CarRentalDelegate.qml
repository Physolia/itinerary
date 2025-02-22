/*
    SPDX-FileCopyrightText: 2018 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1 as QQC2
import org.kde.kirigami 2.17 as Kirigami
import org.kde.itinerary 1.0
import "." as App

App.TimelineDelegate {
    id: root

    headerIconSource: "qrc:///images/car.svg"
    headerItem: RowLayout {
        QQC2.Label {
            text: root.rangeType == TimelineElement.RangeEnd ?
                i18n("Rental Car Drop-off") :
                i18n("Rental Car Pick-up")
            color: root.headerTextColor
            Layout.fillWidth: true
        }
        QQC2.Label {
            text: root.rangeType ==  TimelineElement.RangeEnd ?
                Localizer.formatTime(reservation, "dropoffTime") :
                Localizer.formatTime(reservation, "pickupTime")
            color: root.headerTextColor
        }
    }

    contentItem: Column {
        id: topLayout
        spacing: Kirigami.Units.smallSpacing

        QQC2.Label {
            text: reservation.pickupLocation.name
            visible: root.rangeType != TimelineElement.RangeEnd
        }
        QQC2.Label {
            visible: text !== ""
            width: topLayout.width
            text: Localizer.formatAddressWithContext(reservation.pickupLocation.address,
                                                     reservation.dropoffLocation.address,
                                                     Settings.homeCountryIsoCode)
        }
        QQC2.Label {
            text: i18n("Drop-off: %1", Localizer.formatDateTime(reservation, "dropoffTime"))
            visible: root.rangeType != TimelineElement.RangeEnd
        }
        QQC2.Label {
            text: reservation.dropoffLocation.name
            visible: root.rangeType != TimelineElement.RangeBegin
        }
        QQC2.Label {
            visible: text !== ""
            width: topLayout.width
            text: Localizer.formatAddressWithContext(reservation.dropoffLocation.address,
                                                     reservation.pickupLocation.address,
                                                     Settings.homeCountryIsoCode)
        }
    }

    onClicked: showDetailsPage(carRentalDetailsPage, root.batchId)
}
