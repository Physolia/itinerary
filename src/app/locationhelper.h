/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef LOCATIONHELPER_H
#define LOCATIONHELPER_H

class QString;
class QVariant;

namespace LocationHelper
{
    /** Departing country for location changes, location country otherwise. */
    QString departureCountry(const QVariant &res);
    /** Arrival country for location changes, location country otherwise. */
    QString destinationCountry(const QVariant &res);

    /** ISO 3166-1/2 country or region code for @p loc. */
    QString regionCode(const QVariant &loc);
}

#endif // LOCATIONHELPER_H
