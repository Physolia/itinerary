/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.17 as Kirigami
import org.kde.kirigamiaddons.formcard 1.0 as FormCard
import org.kde.i18n.localeData 1.0
import org.kde.kitinerary 1.0
import org.kde.itinerary 1.0
import "." as App

App.EditorPage {
    id: root

    title: i18n("Edit Hotel Reservation")

    isValidInput: checkinEdit.hasValue && checkoutEdit.hasValue && hotelName.text !== "" && checkinEdit.value < checkoutEdit.value

    function apply(reservation) {
        let hotel = address.save(reservation.reservationFor);
        if (hotelName.text) {
            hotel.name = hotelName.text;
        }
        hotel = contactEdit.save(hotel);
        let newRes = reservation;
        newRes.reservationFor = hotel;

        if (checkinEdit.isModified)
            newRes = Util.setDateTimePreserveTimezone(newRes, "checkinTime", checkinEdit.value);
        if (checkoutEdit.isModified)
            newRes = Util.setDateTimePreserveTimezone(newRes, "checkoutTime", checkoutEdit.value);

        bookingEdit.apply(newRes);
        return newRes;
    }

    ColumnLayout {
        spacing: 0

        App.CardPageTitle {
            emojiIcon: "🏨"
            text: i18n("Hotel")
        }

        FormCard.FormHeader {
            title: i18nc("@title:group", "Accommodation")
        }

        FormCard.FormCard {
            FormCard.FormTextFieldDelegate {
                id: hotelName
                label: i18nc("hotel name", "Name")
                text: reservation.reservationFor.name
                status: Kirigami.MessageType.Error
                statusMessage: text === "" ? i18n("Name must not be empty.") : ""
            }

            FormCard.FormDelegateSeparator {}

            App.FormPlaceEditorDelegate {
                id: address
                place: {
                    if (root.batchId || !root.reservation.reservationFor.address.isEmpty || root.reservation.reservationFor.geo.isValid)
                        return reservation.reservationFor

                    const HOUR = 60 * 60 * 1000;
                    const DAY = 24 * HOUR;
                    let dt = reservation.checkinTime;
                    dt.setTime(dt.getTime() - (dt.getHours() * HOUR) + DAY);
                    return cityAtTime(dt);
                }
            }
        }

        FormCard.FormCard {
            Layout.topMargin: Kirigami.Units.largeSpacing

            App.FormDateTimeEditDelegate {
                id: checkinEdit
                text: i18nc("hotel checkin", "Check-in")
                obj: reservation
                propertyName: "checkinTime"
                status: Kirigami.MessageType.Error
                statusMessage: checkinEdit.hasValue ? '' : i18n("Check-in time has to be set.")
            }
            FormCard.FormDelegateSeparator {}
            App.FormDateTimeEditDelegate {
                id: checkoutEdit
                text: i18nc("hotel checkout", "Check-out")
                obj: reservation
                propertyName: "checkoutTime"
                initialValue: {
                    let d = new Date(checkinEdit.value);
                    d.setDate(d.getDate() + 1);
                    d.setHours(12);
                    return d;
                }
                status: Kirigami.MessageType.Error
                statusMessage: {
                    if (!checkoutEdit.hasValue)
                        return i18n("Check-out time has to be set.")
                    if (checkinEdit.hasValue && checkoutEdit.value < checkinEdit.value)
                        return i18n("Check-out time has to be after the check-in time.")
                    return '';
                }
            }
        }

        App.ContactEditorCard {
            id: contactEdit
            contact: reservation.reservationFor
        }

        App.BookingEditorCard {
            id: bookingEdit
            item: reservation
            defaultCurrency: Country.fromAlpha2(address.currentCountry).currencyCode
        }
    }
}
