/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef FAKE_JNI_H
#define FAKE_JNI_H

#include <cstdint>

#ifdef Q_OS_ANDROID
#error This is a mock object for use on non-Android!
#endif

typedef uint8_t jboolean;
typedef int8_t jbyte;
typedef uint16_t jchar;
typedef int16_t jshort;
typedef int32_t jint;
typedef int64_t jlong;
typedef float jfloat;
typedef double jdouble;

typedef jint jsize;

typedef void* jobject;
typedef jobject jclass;
typedef jobject jstring;
typedef jobject jarray;
typedef jarray jobjectArray;
typedef jarray jbooleanArray;
typedef jarray jbyteArray;
typedef jarray jcharArray;
typedef jarray jshortArray;
typedef jarray jintArray;
typedef jarray jlongArray;
typedef jarray jfloatArray;
typedef jarray jdoubleArray;

struct JNIEnv
{
    inline bool ExceptionCheck() { return false; }
    inline void ExceptionClear() {}
    inline int GetArrayLength(jobjectArray) { return m_arrayLength; }
    inline jobject GetObjectArrayElement(jobjectArray, int index) { return reinterpret_cast<jobject>(index); }

    inline jbooleanArray NewBooleanArray(jsize) { return nullptr; }
    inline jbyteArray    NewByteArray   (jsize) { return nullptr; }
    inline jcharArray    NewCharArray   (jsize) { return nullptr; }
    inline jshortArray   NewShortArray  (jsize) { return nullptr; }
    inline jintArray     NewIntArray    (jsize) { return nullptr; }
    inline jlongArray    NewLongArray   (jsize) { return nullptr; }
    inline jfloatArray   NewFloatArray  (jsize) { return nullptr; }
    inline jdoubleArray  NewDoubleArray (jsize) { return nullptr; }

    inline jboolean* GetBooleanArrayElements(jbooleanArray, jboolean*) { return new jboolean[m_arrayLength]; }
    inline jbyte*    GetByteArrayElements   (jbyteArray, jboolean*)    { return new jbyte[m_arrayLength]; }
    inline jchar*    GetCharArrayElements   (jcharArray, jboolean*)    { return new jchar[m_arrayLength]; }
    inline jshort*   GetShortArrayElements  (jshortArray, jboolean*)   { return new jshort[m_arrayLength]; }
    inline jint*     GetIntArrayElements    (jintArray, jboolean*)     { return new jint[m_arrayLength]; }
    inline jlong*    GetLongArrayElements   (jlongArray, jboolean*)    { return new jlong[m_arrayLength]; }
    inline jfloat*   GetFloatArrayElements  (jfloatArray, jboolean*)   { return new jfloat[m_arrayLength]; }
    inline jdouble*  GetDoubleArrayElements (jdoubleArray, jboolean*)  { return new jdouble[m_arrayLength]; }

    inline void ReleaseBooleanArrayElements(jbooleanArray, jboolean *data, jint) { delete[] data; }
    inline void ReleaseByteArrayElements   (jbyteArray, jbyte *data, jint)       { delete[] data; }
    inline void ReleaseCharArrayElements   (jcharArray, jchar *data, jint)       { delete[] data; }
    inline void ReleaseShortArrayElements  (jshortArray, jshort *data, jint)     { delete[] data; }
    inline void ReleaseIntArrayElements    (jintArray, jint *data, jint)         { delete[] data; }
    inline void ReleaseLongArrayElements   (jlongArray, jlong *data, jint)       { delete[] data; }
    inline void ReleaseFloatArrayElements  (jfloatArray, jfloat *data, jint)     { delete[] data; }
    inline void ReleaseDoubleArrayElements (jdoubleArray, jdouble *data, jint)   { delete[] data; }

    inline void SetBooleanArrayRegion(jbooleanArray, jsize, jsize, const jboolean*) {}
    inline void SetByteArrayRegion   (jbyteArray, jsize, jsize, const jbyte*)       {}
    inline void SetCharArrayRegion   (jcharArray, jsize, jsize, const jchar*)       {}
    inline void SetShortArrayRegion  (jshortArray, jsize, jsize, const jshort*)     {}
    inline void SetIntArrayRegion    (jintArray, jsize, jsize, const jint*)         {}
    inline void SetLongArrayRegion   (jlongArray, jsize, jsize, const jlong*)       {}
    inline void SetFloatArrayRegion  (jfloatArray, jsize, jsize, const jfloat*)     {}
    inline void SetDoubleArrayRegion (jdoubleArray, jsize, jsize, const jdouble*)   {}

    static int m_arrayLength;
};

#define JNI_COMMIT 1
#define JNI_ABORT  2

#endif
