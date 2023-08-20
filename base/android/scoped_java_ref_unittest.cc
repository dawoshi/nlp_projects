// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/scoped_java_ref.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "gtest/gtest.h"

#define EXPECT_SAME_OBJECT(a, b) \
  EXPECT_TRUE(env->IsSameObject(a.obj(), b.obj()))

namespace base {
namespace android {

namespace {
int g_local_refs = 0;
int g_global_refs = 0;

const JNINativeInterface* g_previous_functions;

jobject NewGlobalRef(JNIEnv* env, jobject obj) {
  ++g_global_refs;
  return g_previous_functions->NewGlobalRef(env, obj);
}

void DeleteGlobalRef(JNIEnv* env, jobject obj) {
  --g_global_refs;
  return g_previous_functions->DeleteGlobalRef(env, obj);
}

jobject NewLocalRef(JNIEnv* env, jobject obj) {
  ++g_local_refs;
  return g_previous_functions->NewLocalRef(env, obj);
}

void DeleteLocalRef(JNIEnv* env, jobject obj) {
  --g_local_refs;
  return g_previous_functions->DeleteLocalRef(env, obj);
}
}  // namespace

class ScopedJavaRefTest : public testing::Test {
 protected:
  void SetUp() override {
    g_local_refs = 0;
    g_global_refs = 0;
    JNIEnv* env = AttachCurrentThread();
    g_previous_functions = env->functions;
    hooked_functions = *g_previous_functions;
    env->functions = &hooked_functions;
    // We inject our own functions in JNINativeInterface so we can keep track
    // of the reference counting ourselves.
    hooked_functions.NewGlobalRef = &NewGlobalRef;
    hooked_functions.DeleteGlobalRef = &DeleteGlobalRef;
    hooked_functions.NewLocalRef = &NewLocalRef;
    hooked_functions.DeleteLocalRef = &DeleteLocalRef;
  }

  void TearDown() override {
    JNIEnv* env = AttachCurrentThread();
    env->functions = g_previous_functions;
  }
  // From JellyBean release, the instance of this struct provided in JNIEnv is
  // read-only, so we deep copy it to allow individual functions to be hooked.
  JNINativeInterface hooked_functions;
};

// The main purpose of this is testing the various conversions compile.
TEST_F(ScopedJavaRefTest, Conversions) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> str = ConvertUTF8ToJavaString(env, "string");
  ScopedJavaGlobalRef<jstring> global(str);

  // Contextual conversions to bool should be allowed.
  EXPECT_TRUE(str);
  EXPECT_FALSE(JavaRef<jobject>());

  // All the types should convert from nullptr, even JavaRef.
  {
    JavaRef<jstring> null_ref(nullptr);
    EXPECT_FALSE(null_ref);
    ScopedJavaLocalRef<jobject> null_local(nullptr);
    EXPECT_FALSE(null_local);
    ScopedJavaGlobalRef<jarray> null_global(nullptr);
    EXPECT_FALSE(null_global);
  }

  // Local and global refs should {copy,move}-{construct,assign}.
  // Moves should leave the source as null.
  {
    ScopedJavaLocalRef<jstring> str2(str);
    EXPECT_SAME_OBJECT(str2, str);
    ScopedJavaLocalRef<jstring> str3(std::move(str2));
    EXPECT_SAME_OBJECT(str3, str);
    EXPECT_FALSE(str2);
    ScopedJavaLocalRef<jstring> str4;
    str4 = str;
    EXPECT_SAME_OBJECT(str4, str);
    ScopedJavaLocalRef<jstring> str5;
    str5 = std::move(str4);
    EXPECT_SAME_OBJECT(str5, str);
    EXPECT_FALSE(str4);
  }
  {
    ScopedJavaGlobalRef<jstring> str2(global);
    EXPECT_SAME_OBJECT(str2, str);
    ScopedJavaGlobalRef<jstring> str3(std::move(str2));
    EXPECT_SAME_OBJECT(str3, str);
    EXPECT_FALSE(str2);
    ScopedJavaGlobalRef<jstring> str4;
    str4 = global;
    EXPECT_SAME_OBJECT(str4, str);
    ScopedJavaGlobalRef<jstring> str5;
    str5 = std::move(str4);
    EXPECT_SAME_OBJECT(str5, str);
    EXPECT_FALSE(str4);
  }

  // As above but going from jstring to jobject.
  {
    ScopedJavaLocalRef<jobject> obj2(str);
    EXPECT_SAME_OBJECT(obj2, str);
    ScopedJavaLocalRef<jobject> obj3(std::move(obj2));
    EXPECT_SAME_OBJECT(obj3, str);
    EXPECT_FALSE(obj2);
    ScopedJavaLocalRef<jobject> obj4;
    obj4 = str;
    EXPECT_SAME_OBJECT(obj4, str);
    ScopedJavaLocalRef<jobject> obj5;
    obj5 = std::move(obj4);
    EXPECT_SAME_OBJECT(obj5, str);
    EXPECT_FALSE(obj4);
  }
  {
    ScopedJavaGlobalRef<jobject> obj2(global);
    EXPECT_SAME_OBJECT(obj2, str);
    ScopedJavaGlobalRef<jobject> obj3(std::move(obj2));
    EXPECT_SAME_OBJECT(obj3, str);
    EXPECT_FALSE(obj2);
    ScopedJavaGlobalRef<jobject> obj4;
    obj4 = global;
    EXPECT_SAME_OBJECT(obj4, str);
    ScopedJavaGlobalRef<jobject> obj5;
    obj5 = std::move(obj4);
    EXPECT_SAME_OBJECT(obj5, str);
    EXPECT_FALSE(obj4);
  }

  // Explicit copy construction or assignment between global<->local is allowed,
  // but not implicit conversions.
  {
    ScopedJavaLocalRef<jstring> new_local(global);
    EXPECT_SAME_OBJECT(new_local, str);
    new_local = global;
    EXPECT_SAME_OBJECT(new_local, str);
    ScopedJavaGlobalRef<jstring> new_global(str);
    EXPECT_SAME_OBJECT(new_global, str);
    new_global = str;
    EXPECT_SAME_OBJECT(new_local, str);
    static_assert(!std::is_convertible<ScopedJavaLocalRef<jobject>,
                                       ScopedJavaGlobalRef<jobject>>::value,
                  "");
    static_assert(!std::is_convertible<ScopedJavaGlobalRef<jobject>,
                                       ScopedJavaLocalRef<jobject>>::value,
                  "");
  }

  // Converting between local/global while also converting to jobject also works
  // because JavaRef<jobject> is the base class.
  {
    ScopedJavaGlobalRef<jobject> global_obj(str);
    ScopedJavaLocalRef<jobject> local_obj(global);
    const JavaRef<jobject>& obj_ref1(str);
    const JavaRef<jobject>& obj_ref2(global);
    EXPECT_SAME_OBJECT(obj_ref1, obj_ref2);
    EXPECT_SAME_OBJECT(global_obj, obj_ref2);
  }
  global.Reset(str);
  const JavaRef<jstring>& str_ref = str;
  EXPECT_EQ("string", ConvertJavaStringToUTF8(str_ref));
  str.Reset();
}

TEST_F(ScopedJavaRefTest, RefCounts) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> str;
  // The ConvertJavaStringToUTF8 below creates a new string that would normally
  // return a local ref. We simulate that by starting the g_local_refs count at
  // 1.
  g_local_refs = 1;
  str.Reset(ConvertUTF8ToJavaString(env, "string"));
  EXPECT_EQ(1, g_local_refs);
  EXPECT_EQ(0, g_global_refs);
  {
    ScopedJavaGlobalRef<jstring> global_str(str);
    ScopedJavaGlobalRef<jobject> global_obj(global_str);
    EXPECT_EQ(1, g_local_refs);
    EXPECT_EQ(2, g_global_refs);

    auto str2 = ScopedJavaLocalRef<jstring>::Adopt(env, str.Release());
    EXPECT_EQ(1, g_local_refs);
    {
      ScopedJavaLocalRef<jstring> str3(str2);
      EXPECT_EQ(2, g_local_refs);
    }
    EXPECT_EQ(1, g_local_refs);
    {
      ScopedJavaLocalRef<jstring> str4((ScopedJavaLocalRef<jstring>(str2)));
      EXPECT_EQ(2, g_local_refs);
    }
    EXPECT_EQ(1, g_local_refs);
    {
      ScopedJavaLocalRef<jstring> str5;
      str5 = ScopedJavaLocalRef<jstring>(str2);
      EXPECT_EQ(2, g_local_refs);
    }
    EXPECT_EQ(1, g_local_refs);
    str2.Reset();
    EXPECT_EQ(0, g_local_refs);
    global_str.Reset();
    EXPECT_EQ(1, g_global_refs);
    ScopedJavaGlobalRef<jobject> global_obj2(global_obj);
    EXPECT_EQ(2, g_global_refs);
  }

  EXPECT_EQ(0, g_local_refs);
  EXPECT_EQ(0, g_global_refs);
}

}  // namespace android
}  // namespace base
