/*
    Copyright (C) 2019 Volker Krause <vkrause@kde.org>

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

#include "statisticsmodel.h"
#include "reservationmanager.h"

#include <KItinerary/LocationUtil>
#include <KItinerary/Place>
#include <KItinerary/Reservation>
#include <KItinerary/SortUtil>

#include <KLocalizedString>

#include <QDebug>

using namespace KItinerary;

StatisticsItem::StatisticsItem() = default;

StatisticsItem::StatisticsItem(const QString &label, const QString &value, StatisticsItem::Trend trend)
    : m_label(label)
    , m_value(value)
    , m_trend(trend)
{
}

StatisticsItem::~StatisticsItem() = default;

StatisticsModel::StatisticsModel(QObject *parent)
    : QObject(parent)
{
    connect(this, &StatisticsModel::setupChanged, this, &StatisticsModel::recompute);
    recompute();
}

StatisticsModel::~StatisticsModel() = default;

ReservationManager* StatisticsModel::reservationManager() const
{
    return m_resMgr;
}

void StatisticsModel::setReservationManager(ReservationManager *resMgr)
{
    if (m_resMgr == resMgr) {
        return;
    }
    m_resMgr = resMgr;
    connect(m_resMgr, &ReservationManager::batchAdded, this, &StatisticsModel::recompute);
    emit setupChanged();
}

void StatisticsModel::setTimeRange(const QDate &begin, const QDate &end)
{
    if (m_begin == begin && end == m_end) {
        return;
    }

    m_begin = begin;
    m_end = end;
    recompute();
}

StatisticsItem StatisticsModel::totalCount() const
{
    return StatisticsItem(i18n("Trips"), QString::number(m_statData[Total][TripCount]), trend(Total, TripCount));
}

StatisticsItem StatisticsModel::totalDistance() const
{
    return StatisticsItem(i18n("Distance"), i18n("%1 km", m_statData[Total][Distance] / 1000), trend(Total, Distance));
}

StatisticsItem StatisticsModel::totalNights() const
{
    return StatisticsItem(i18n("Hotel nights"), QString::number(m_hotelCount), trend(m_hotelCount, m_prevHotelCount));
}

StatisticsItem StatisticsModel::totalCO2() const
{
    return StatisticsItem(i18n("CO₂"), i18n("%1 kg", m_statData[Total][CO2] / 1000.0), trend(Total, CO2));
}

StatisticsItem StatisticsModel::flightCount() const
{
    return StatisticsItem(i18n("Flights"), QString::number(m_statData[Flight][TripCount]), trend(Flight, TripCount));
}

StatisticsItem StatisticsModel::flightDistance() const
{
    return StatisticsItem(i18n("Distance"), i18n("%1 km", m_statData[Flight][Distance] / 1000), trend(Flight, Distance));
}

StatisticsItem StatisticsModel::flightCO2() const
{
    return StatisticsItem(i18n("CO₂"), i18n("%1 kg", m_statData[Flight][CO2] / 1000.0), trend(Flight, CO2));
}

StatisticsItem StatisticsModel::trainCount() const
{
    return StatisticsItem(i18n("Trips"), QString::number(m_statData[Train][TripCount]), trend(Train, TripCount));
}

StatisticsItem StatisticsModel::trainDistance() const
{
    return StatisticsItem(i18n("Distance"), i18n("%1 km", m_statData[Train][Distance] / 1000), trend(Train, Distance));
}

StatisticsItem StatisticsModel::trainCO2() const
{
    return StatisticsItem(i18n("CO₂"), i18n("%1 kg", m_statData[Train][CO2] / 1000.0), trend(Train, CO2));
}

StatisticsItem StatisticsModel::busCount() const
{
    return StatisticsItem(i18n("Trips"), QString::number(m_statData[Bus][TripCount]), trend(Bus, TripCount));
}

StatisticsItem StatisticsModel::busDistance() const
{
    return StatisticsItem(i18n("Distance"), i18n("%1 km", m_statData[Bus][Distance] / 1000), trend(Bus, Distance));
}

StatisticsItem StatisticsModel::busCO2() const
{
    return StatisticsItem(i18n("CO₂"), i18n("%1 kg", m_statData[Bus][CO2] / 1000.0), trend(Bus, CO2));
}

StatisticsItem StatisticsModel::carCount() const
{
    return StatisticsItem(i18n("Trips"), QString::number(m_statData[Car][TripCount]), trend(Car, TripCount));
}

StatisticsItem StatisticsModel::carDistance() const
{
    return StatisticsItem(i18n("Distance"), i18n("%1 km", m_statData[Car][Distance] / 1000), trend(Car, Distance));
}

StatisticsItem StatisticsModel::carCO2() const
{
    return StatisticsItem(i18n("CO₂"), i18n("%1 kg", m_statData[Car][CO2] / 1000.0), trend(Car, CO2));
}

StatisticsModel::AggregateType StatisticsModel::typeForReservation(const QVariant &res) const
{
    if (JsonLd::isA<FlightReservation>(res)) {
        return Flight;
    } else if (JsonLd::isA<TrainReservation>(res)) {
        return Train;
    } else if (JsonLd::isA<BusReservation>(res)) {
        return Bus;
    }
    return Car;
}

static int distance(const QVariant &res)
{
    const auto dep = LocationUtil::departureLocation(res);
    const auto arr = LocationUtil::arrivalLocation(res);
    if (dep.isNull() || arr.isNull()) {
        return 0;
    }
    const auto depGeo = LocationUtil::geo(dep);
    const auto arrGeo = LocationUtil::geo(arr);
    if (!depGeo.isValid() || !arrGeo.isValid()) {
        return 0;
    }
    return std::max(0, LocationUtil::distance(depGeo, arrGeo));
}

// from https://en.wikipedia.org/wiki/Environmental_impact_of_transport
static const int emissionPerKm[] = {
    0,
    285, // flight
    14, // train
    68, // bus
    158, // car
};

int StatisticsModel::co2emission(StatisticsModel::AggregateType type, int distance) const
{
    return distance * emissionPerKm[type];
}

void StatisticsModel::computeStats(const QVariant& res, int (&statData)[AGGREGATE_TYPE_COUNT][STAT_TYPE_COUNT])
{
    const auto type = typeForReservation(res);
    const auto dist = distance(res);
    const auto co2 = co2emission(type, dist / 1000);

    statData[type][TripCount]++;
    statData[type][Distance] += dist;
    statData[type][CO2] += co2;

    statData[Total][TripCount]++;
    statData[Total][Distance] += dist;
    statData[Total][CO2] += co2;
}

void StatisticsModel::recompute()
{
    memset(m_statData, 0, AGGREGATE_TYPE_COUNT * STAT_TYPE_COUNT * sizeof(int));
    memset(m_prevStatData, 0, AGGREGATE_TYPE_COUNT * STAT_TYPE_COUNT * sizeof(int));
    m_hotelCount = 0;
    m_prevHotelCount = 0;

    if (!m_resMgr) {
        return;
    }

    QDate prevStart;
    if (m_begin.isValid() && m_end.isValid()) {
        prevStart = m_begin.addDays(m_end.daysTo(m_begin));
    }

    const auto &batches = m_resMgr->batches();
    for (const auto &batchId : batches) {
        const auto res = m_resMgr->reservation(batchId);
        const auto dt = SortUtil::startDateTime(res);

        bool isPrev = false;
        if (m_end.isValid() && dt.date() > m_end) {
            continue;
        }
        if (prevStart.isValid()) {
            if (dt.date() < prevStart) {
                continue;
            }
            isPrev = dt.date() < m_begin;
        }

        if (LocationUtil::isLocationChange(res)) {
            computeStats(res, isPrev ? m_prevStatData : m_statData);
        } else if (JsonLd::isA<LodgingReservation>(res)) {
            const auto hotel = res.value<LodgingReservation>();
            if (isPrev) {
                m_prevHotelCount += hotel.checkinTime().daysTo(hotel.checkoutTime());
            } else {
                m_hotelCount += hotel.checkinTime().daysTo(hotel.checkoutTime());
            }
        }
    }

    emit changed();
}

StatisticsItem::Trend StatisticsModel::trend(int current, int prev) const
{
    if (!m_begin.isValid() || !m_end.isValid()) {
        return StatisticsItem::TrendUnknown;
    }

    return current < prev ? StatisticsItem::TrendDown : current > prev ? StatisticsItem::TrendUp : StatisticsItem::TrendUnchanged;
}

StatisticsItem::Trend StatisticsModel::trend(StatisticsModel::AggregateType type, StatisticsModel::StatType stat) const
{
    return trend(m_statData[type][stat], m_prevStatData[type][stat]);
}
