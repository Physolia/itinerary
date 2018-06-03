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

#include "settings.h"

#include <QSettings>

Settings::Settings(QObject *parent)
    : QObject(parent)
{
    QSettings s;
    s.beginGroup(QLatin1String("Settings"));
    m_weatherEnabled = s.value(QLatin1String("WeatherForecastEnabled"), false).toBool();
    // TODO configurable home country
    m_homeCountry = s.value(QLatin1String("HomeCountry"), QStringLiteral("DE")).toString();
}

Settings::~Settings() = default;

bool Settings::weatherForecastEnabled() const
{
    return m_weatherEnabled;
}

void Settings::setWeatherForecastEnabled(bool enabled)
{
    if (m_weatherEnabled == enabled) {
        return;
    }

    m_weatherEnabled = enabled;
    QSettings s;
    s.beginGroup(QLatin1String("Settings"));
    s.setValue(QLatin1String("WeatherForecastEnabled"), enabled);

    emit weatherForecastEnabledChanged(enabled);
}

QString Settings::homeCountryIsoCode() const
{
    return m_homeCountry;
}

void Settings::setHomeCountryIsoCode(const QString& isoCode)
{
    if (m_homeCountry == isoCode) {
        return;
    }

    m_homeCountry = isoCode;
    QSettings s;
    s.beginGroup(QLatin1String("Settings"));
    s.setValue(QLatin1String("HomeCountry"), isoCode);

    emit homeCountryIsoCodeChanged(isoCode);
}
