/*
    Copyright (C) 2019 Nicolas Fella <nicolas.fella@gmx.de>

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

#include "androidlockbackend.h"

#include <QDebug>
#include <QtAndroid>
#include <QAndroidJniObject>

AndroidLockBackend::AndroidLockBackend(QObject *parent)
    : LockBackend(parent)
{
}

AndroidLockBackend::~AndroidLockBackend()
{
}

void AndroidLockBackend::setInhibitionOff()
{
    QAndroidJniObject::callStaticMethod<void>("org.kde.solid.Solid", "setLockInhibitionOff", "(Landroid/app/Activity;)V", QtAndroid::androidActivity().object());
}

void AndroidLockBackend::setInhibitionOn(const QString &explanation)
{
    Q_UNUSED(explanation);
    QAndroidJniObject::callStaticMethod<void>("org.kde.solid.Solid", "setLockInhibitionOn", "(Landroid/app/Activity;)V", QtAndroid::androidActivity().object());
}