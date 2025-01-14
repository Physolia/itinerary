/*
    SPDX-FileCopyrightText: 2019 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.17 as Kirigami
import org.kde.kirigamiaddons.formcard 1.0 as FormCard
import org.kde.kitinerary 1.0
import org.kde.kpublictransport 1.0
import org.kde.itinerary 1.0
import "." as App

App.EditorPage {
    id: root
    title: i18n("Edit Train Reservation")

    property var departureStation: reservationFor.departureStation
    property var departureTime: Util.dateTimeStripTimezone(reservationFor, "departureTime")
    property var arrivalStation: reservationFor.arrivalStation
    property var arrivalTime: Util.dateTimeStripTimezone(reservationFor, "arrivalTime")

    function apply(reservation) {
        var trip = reservation.reservationFor;
        trip.departureStation = root.departureStation;
        trip = Util.setDateTimePreserveTimezone(trip, "departureTime", root.departureTime);
        trip.departurePlatform = departurePlatform.text;
        trip.arrivalStation = root.arrivalStation;
        trip = Util.setDateTimePreserveTimezone(trip, "arrivalTime", root.arrivalTime);
        trip.arrivalPlatform = arrivalPlatform.text;

        let ticket = reservation.reservedTicket ?? Factory.makeTicket();
        let seat = ticket.ticketedSeat;
        seat.seatSection = coachEdit.text;
        seat.seatNumber = seatEdit.text;
        seat.seatingType = classEdit.text;
        ticket.ticketedSeat = seat;

        var newRes = reservation;
        newRes.reservationFor = trip;
        newRes.reservedTicket = ticket;
        programMembershipEdit.apply(newRes);
        bookingEdit.apply(newRes);
        return newRes;
    }

    App.IntermediateStopSelector {
        id: boardSheet
        title: i18n("Board Later")
        model: root.controller.journey.intermediateStops
        action: Kirigami.Action {
            text: i18n("Change departure station")
            onTriggered: {
                departureStation = PublicTransport.trainStationFromLocation(root.controller.journey.intermediateStops[boardSheet.currentIndex].stopPoint)
                departureTime = Util.dateTimeStripTimezone(root.controller.journey.intermediateStops[boardSheet.currentIndex], "scheduledDepartureTime");
                boardSheet.close();
            }
        }
    }
    App.IntermediateStopSelector {
        id: alightSheet
        title: i18n("Alight Earlier")
        model: root.controller.journey.intermediateStops
        action: Kirigami.Action {
            text: i18n("Change arrival station")
            onTriggered: {
                arrivalStation = PublicTransport.trainStationFromLocation(root.controller.journey.intermediateStops[alightSheet.currentIndex].stopPoint);
                arrivalTime = Util.dateTimeStripTimezone(root.controller.journey.intermediateStops[alightSheet.currentIndex], "scheduledArrivalTime");
                if (!arrivalTime) {
                    arrivalTime = Util.dateTimeStripTimezone(root.controller.journey.intermediateStops[alightSheet.currentIndex], "scheduledDepartureTime");
                }
                alightSheet.close();
            }
        }
    }

    ColumnLayout {
        spacing: 0

        App.CardPageTitle {
            emojiIcon: "🚅"
            text: if (reservationFor.trainNumber || reservationFor.trainName) {
                return reservationFor.trainName + " " + reservationFor.trainNumber;
            } else {
                return i18n("Train")
            }
        }

        FormCard.FormHeader {
            title: i18nc("train departure", "Departure")
        }

        FormCard.FormCard {
            FormCard.FormTextDelegate {
                text: i18nc("train station", "Station")
                description: root.departureStation.name
            }
            FormCard.FormDelegateSeparator {}
            FormCard.FormTextFieldDelegate {
                id: departurePlatform
                label: i18nc("train platform", "Platform")
                text: reservationFor.departurePlatform
            }
            FormCard.FormDelegateSeparator { above: boardLater }
            FormCard.FormButtonDelegate {
                id: boardLater

                text: i18n("Board later")
                icon.name: "arrow-right"
                visible: root.controller.journey && root.controller.journey.intermediateStops.length > 0 // TODO also check for preceding layovers
                onClicked: boardSheet.open();
            }
        }

        FormCard.FormHeader {
            title: i18nc("train arrival", "Arrival")
        }

        FormCard.FormCard {
            FormCard.FormTextDelegate {
                text: i18nc("train station", "Station")
                description: root.arrivalStation.name
            }
            FormCard.FormDelegateSeparator {}
            FormCard.FormTextFieldDelegate {
                id: arrivalPlatform
                label: i18nc("train platform", "Platform")
                text: reservationFor.arrivalPlatform
            }
            FormCard.FormDelegateSeparator { above: alignEarlier }
            FormCard.FormButtonDelegate {
                id: alignEarlier
                text: i18n("Alight earlier")
                icon.name: "arrow-left"
                visible: root.controller.journey && root.controller.journey.intermediateStops.length > 0 // TODO also check for subsequent layovers
                onClicked: alightSheet.open();
            }
        }

        // TODO the below is per reservation, not per batch, so add a selector for that!

        FormCard.FormHeader {
            title: i18n("Seat")
        }

        FormCard.FormCard {
            FormCard.FormTextFieldDelegate {
                id: coachEdit
                label: i18nc("carriage on a train", "Coach")
                text: reservation.reservedTicket.ticketedSeat.seatSection
            }
            FormCard.FormDelegateSeparator {}
            FormCard.FormTextFieldDelegate {
                id: seatEdit
                label: i18n("Seat")
                text: reservation.reservedTicket.ticketedSeat.seatNumber
            }
            FormCard.FormDelegateSeparator {}
            FormCard.FormTextFieldDelegate {
                id: classEdit
                label: i18nc("seating class on a train", "Class")
                text: reservation.reservedTicket.ticketedSeat.seatingType
            }
        }

        ProgramMembershipEditorCard {
            id: programMembershipEdit
            item: root.reservation
        }

        App.BookingEditorCard {
            id: bookingEdit
            item: root.reservation
        }
    }
}
