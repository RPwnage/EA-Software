// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>

#include <jni.h>

namespace eadp
{
namespace foundation
{
namespace Internal
{

static const int32_t kMaxLocalJniRefs = 400;

inline jstring convert(JNIEnv* env, const char8_t* value)
{
    return env->NewStringUTF(value);
}

inline jstring convert(JNIEnv* env, const String& value)
{
    return env->NewStringUTF(value.c_str());
}

inline String convert(JNIEnv* env, jstring value, Allocator& allocator)
{
    String result(allocator.emptyString());
    if (value != nullptr)
    {
        const char* buffer = env->GetStringUTFChars(value, nullptr);
        result = buffer;
        env->ReleaseStringUTFChars(value, buffer);
    }

    return result;
}

inline jobject convert(JNIEnv* env, eadp::foundation::SharedPtr<eadp::foundation::Vector<eadp::foundation::String>> strVector)
{
    if (strVector == nullptr)
    {
        return nullptr;
    }

    jclass listClass = env->FindClass("java/util/List");
    jclass arrayListClass = env->FindClass("java/util/ArrayList");
    jmethodID arrayListConstructor = env->GetMethodID(arrayListClass, "<init>", "(I)V");
    jmethodID listAdd = env->GetMethodID(listClass, "add", "(Ljava/lang/Object;)Z");
    jobject arrayL = env->NewObject(arrayListClass, arrayListConstructor, strVector->capacity());
    int refCounter = 0;
    env->PushLocalFrame(kMaxLocalJniRefs);
    for (auto iter = strVector->begin(); iter != strVector->end(); iter++)
    {
        jobject val = eadp::foundation::Internal::convert(env, iter->c_str());
        env->CallBooleanMethod(arrayL, listAdd, val);
        refCounter++;
        if (refCounter == kMaxLocalJniRefs)
        {
            env->PopLocalFrame(NULL);
            env->PushLocalFrame(kMaxLocalJniRefs);
            refCounter = 0;
        }
    }
    env->PopLocalFrame(NULL);

    return arrayL;
}

template<typename T>
jobject convert(JNIEnv* env, const Vector<T>& value)
{
    jclass listClass = env->FindClass("java/util/List");
    jmethodID addMethod = env->GetMethodID(listClass, "add", "(Ljava/lang/Object;)Z");

    jclass arrayListClass = env->FindClass("java/util/ArrayList");
    jmethodID initMethod = env->GetMethodID(arrayListClass, "<init>", "(I)V");
    jobject list = env->NewObject(arrayListClass, initMethod, value.size());

    typename Vector<T>::const_iterator it;
    const int refsPerLoop = 1;
    int refCounter = refsPerLoop;
    env->PushLocalFrame(kMaxLocalJniRefs);
    for (it = value.begin(); it != value.end(); ++it)
    {
        jobject jValue = convert(env, *it);
        env->CallBooleanMethod(list, addMethod, jValue);

        refCounter += refsPerLoop;
        if (refCounter > kMaxLocalJniRefs)
        {
            env->PopLocalFrame(NULL);
            env->PushLocalFrame(kMaxLocalJniRefs);
            refCounter = refsPerLoop;
        }
    }
    env->PopLocalFrame(NULL);

    return list;
}

template<typename T, typename U>
jobject convert(JNIEnv* env, const Map<T, U>& value)
{
    jclass mapClass = env->FindClass("java/util/Map");
    jmethodID putMethod = env->GetMethodID(mapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

    jclass hashMapClass = env->FindClass("java/util/HashMap");
    jmethodID initMethod = env->GetMethodID(hashMapClass, "<init>", "(I)V");
    jobject map = env->NewObject(hashMapClass, initMethod, value.size());

    typename Map<T, U>::const_iterator it;
    const int refsPerLoop = 2;
    int refCounter = refsPerLoop;
    env->PushLocalFrame(kMaxLocalJniRefs);
    for (it = value.begin(); it != value.end(); ++it)
    {
        jobject key = convert(env, it->first);
        jobject value = convert(env, it->second);

        env->CallObjectMethod(map, putMethod, key, value);

        refCounter += refsPerLoop;
        if (refCounter > kMaxLocalJniRefs)
        {
            env->PopLocalFrame(NULL);
            env->PushLocalFrame(kMaxLocalJniRefs);
            refCounter = refsPerLoop;
        }
    }
    env->PopLocalFrame(NULL);

    return map;
}

template<typename T>
struct ObjectConverter
{
    static T convertObject(JNIEnv* env, jobject value, Allocator& allocator);
};

template<typename T>
T convert(JNIEnv* env, jobject value, Allocator& allocator)
{
    return ObjectConverter<T>::convertObject(env, value, allocator);
}

template<typename T>
struct ObjectConverter<Vector<T> >
{
    static Vector<T> convertObject(JNIEnv* env, jobject value, Allocator& allocator)
    {
        Vector<T> result(allocator.make<Vector<T>>());
        if (value != NULL)
        {
            jclass iteratorClass = env->FindClass("java/util/Iterator");
            jclass listClass = env->FindClass("java/util/List");

            jmethodID iteratorMethod = env->GetMethodID(listClass, "iterator", "()Ljava/util/Iterator;");
            jobject iterator = env->CallObjectMethod(value, iteratorMethod);

            jmethodID hasNextMethod = env->GetMethodID(iteratorClass, "hasNext", "()Z");
            jmethodID nextMethod = env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");

            const int refsPerLoop = 1;
            int refCounter = refsPerLoop;
            env->PushLocalFrame(kMaxLocalJniRefs);
            while (env->CallBooleanMethod(iterator, hasNextMethod))
            {
                jobject jValue = env->CallObjectMethod(iterator, nextMethod);
                result.push_back(convert<T>(env, jValue, allocator));

                refCounter += refsPerLoop;
                if (refCounter > kMaxLocalJniRefs)
                {
                    env->PopLocalFrame(NULL);
                    env->PushLocalFrame(kMaxLocalJniRefs);
                    refCounter = refsPerLoop;
                }
            }
            env->PopLocalFrame(NULL);
        }
        return result;
    }
};

template<>
struct ObjectConverter<String>
{
    static String convertObject(JNIEnv* env, jobject value, Allocator& allocator)
    {
        return convert(env, (jstring)value, allocator);
    }
};

} // namespace Internal
} // namespace foundation
} // namespace eadp

