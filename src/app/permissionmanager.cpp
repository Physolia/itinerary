/*
    SPDX-FileCopyrightText: 2022 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "permissionmanager.h"

#include <QDebug>

#ifdef Q_OS_ANDROID
#include <KAndroidExtras/ManifestPermission>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtAndroid>
#else
#include <private/qandroidextras_p.h>
#endif
#endif

#ifdef Q_OS_ANDROID
static QString permissionName(Permission::Permission p)
{
    using namespace KAndroidExtras;
    switch (p) {
        case Permission::ReadCalendar:
            return ManifestPermission::READ_CALENDAR;
        case Permission::WriteCalendar:
            return ManifestPermission::WRITE_CALENDAR;
    }
}
#endif

void PermissionManager::requestPermission(Permission::Permission permission, QJSValue callback)
{
    qDebug() << permission;

#ifdef Q_OS_ANDROID
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (QtAndroid::checkPermission(permissionName(permission)) == QtAndroid::PermissionResult::Granted) {
        callback.call();
        return;
    }

    QtAndroid::requestPermissions({permissionName(permission)}, [permission, callback] (const QtAndroid::PermissionResultMap &result) {
        if (result[permissionName(permission)] == QtAndroid::PermissionResult::Granted) {
            auto cb = callback;
            cb.call();
        }
    });
#else
    // TODO make this properly async
    if (QtAndroidPrivate::checkPermission(permissionName(permission)).result() != QtAndroidPrivate::PermissionResult::Authorized) {
        if (QtAndroidPrivate::requestPermission(permissionName(permission)).result() != QtAndroidPrivate::PermissionResult::Authorized) {
            return;
        }
    }
    callback.call();
#endif
#else
    // non-Android
    callback.call();
#endif
}