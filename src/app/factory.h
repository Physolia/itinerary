/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef FACTORY_H
#define FACTORY_H

#include "qobjectdefs.h"

class QVariant;

/** QML factory functions for gadget types. */
class Factory
{
    Q_GADGET
public:
    Q_INVOKABLE static QVariant makePlace();
};

#endif // FACTORY_H