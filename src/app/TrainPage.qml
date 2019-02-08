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
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1 as QQC2
import org.kde.kirigami 2.4 as Kirigami
import org.kde.kitinerary 1.0
import org.kde.itinerary 1.0
import "." as App

App.DetailsPage {
    id: root
    title: i18n("Train Ticket")
    property var arrival: _liveDataManager.arrival(batchId)
    property var departure: _liveDataManager.departure(batchId)

    Component {
        id: alternativePage
        App.TrainJourneyQueryPage {
            batchId: root.batchId
        }
    }

    Component {
        id: alternativeAction
        Kirigami.Action {
            text: i18n("Alternatives")
            iconName: "clock"
            onTriggered: {
                applicationWindow().pageStack.push(alternativePage);
            }
        }
    }

    Component.onCompleted: {
        actions.contextualActions.push(alternativeAction.createObject(root));
    }

    Kirigami.FormLayout {
        width: root.width

        QQC2.Label {
            Kirigami.FormData.isSection: true
            text: reservationFor.trainName + " " + reservationFor.trainNumber
            horizontalAlignment: Qt.AlignHCenter
            font.bold: true
        }

        // ticket barcode
        App.TicketTokenDelegate {
            Kirigami.FormData.isSection: true
            resIds: _reservationManager.reservationsForBatch(root.batchId)
        }

        // departure data
        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Departure")
        }
        RowLayout {
            Kirigami.FormData.label: i18n("Time:")
            QQC2.Label {
                text: Localizer.formatDateTime(reservationFor, "departureTime")
            }
            QQC2.Label {
                text: (departure.departureDelay >= 0 ? "+" : "") + departure.departureDelay
                color: (departure.departureDelay > 1) ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.positiveTextColor
                visible: departure.hasExpectedDepartureTime
            }
        }
        QQC2.Label {
            Kirigami.FormData.label: i18n("Station:")
            text: reservationFor.departureStation.name
        }
        App.PlaceDelegate {
            place: reservationFor.departureStation
        }
        RowLayout {
            Kirigami.FormData.label: i18n("Platform:")
            QQC2.Label {
                text: departure.hasExpectedPlatform ? departure.expectedPlatform : reservationFor.departurePlatform
                color: departure.platformChanged ? Kirigami.Theme.negativeTextColor :
                    departure.hasExpectedPlatform ? Kirigami.Theme.positiveTextColor :
                    Kirigami.Theme.textColor;
            }
            QQC2.Label {
                text: i18n("(was: %1)", reservationFor.departurePlatform)
                visible: departure.platformChanged && reservationFor.departurePlatform != ""
            }
        }

        // arrival data
        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Arrival")
        }
        RowLayout {
            Kirigami.FormData.label: i18n("Arrival time:")
            QQC2.Label {
                text: Localizer.formatDateTime(reservationFor, "arrivalTime")
            }
            QQC2.Label {
                text: (arrival.arrivalDelay >= 0 ? "+" : "") + arrival.arrivalDelay
                color: (arrival.arrivalDelay > 1) ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.positiveTextColor
                visible: arrival.hasExpectedArrivalTime
            }
        }
        QQC2.Label {
            Kirigami.FormData.label: i18n("Station:")
            text: reservationFor.arrivalStation.name
        }
        App.PlaceDelegate {
            place: reservationFor.arrivalStation
        }
        RowLayout {
            Kirigami.FormData.label: i18n("Platform:")
            QQC2.Label {
                text: arrival.hasExpectedPlatform ? arrival.expectedPlatform : reservationFor.arrivalPlatform
                color: arrival.platformChanged ? Kirigami.Theme.negativeTextColor :
                    arrival.hasExpectedPlatform ? Kirigami.Theme.positiveTextColor :
                    Kirigami.Theme.textColor;
            }
            QQC2.Label {
                text: i18n("(was: %1)", reservationFor.arrivalPlatform)
                visible: arrival.platformChanged && reservationFor.arrivalPlatform != ""
            }
        }

        // seat reservation
        Kirigami.Separator {
            Kirigami.FormData.label: i18n("Seat")
            Kirigami.FormData.isSection: true
            visible: reservation.reservedTicket.ticketedSeat.seatNumber != "" || reservation.reservedTicket.ticketedSeat.seatSection != ""
        }
        QQC2.Label {
            Kirigami.FormData.label: i18n("Coach:")
            text: reservation.reservedTicket.ticketedSeat.seatSection
            visible: reservation.reservedTicket.ticketedSeat.seatSection != ""
        }
        QQC2.Label {
            Kirigami.FormData.label: i18n("Seat:")
            text: reservation.reservedTicket.ticketedSeat.seatNumber
            visible: reservation.reservedTicket.ticketedSeat.seatNumber != ""
        }

        // booking details
        Kirigami.Separator {
            Kirigami.FormData.label: i18n("Booking")
            Kirigami.FormData.isSection: true
        }
        QQC2.Label {
            Kirigami.FormData.label: i18n("Reference:")
            text: reservation.reservationNumber
        }
        QQC2.Label {
            Kirigami.FormData.label: i18n("Under name:")
            text: reservation.underName.name
        }
    }
}
