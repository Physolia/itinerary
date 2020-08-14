/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "context.h"
#include "jnisignature.h"
#include "jnitypes.h"

#include <QtAndroid>

using namespace KAndroidExtras;

QAndroidJniObject Context::getPackageName()
{
    return QtAndroid::androidContext().callObjectMethod("getPackageName", Jni::signature<java::lang::String()>());
}
