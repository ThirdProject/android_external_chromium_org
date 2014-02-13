// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/cert/x509_util_android.h"

#include "base/android/build_info.h"
#include "base/android/jni_android.h"
#include "base/metrics/histogram.h"
#include "jni/X509Util_jni.h"
#include "net/cert/cert_database.h"

namespace net {

void NotifyKeyChainChanged(JNIEnv* env, jclass clazz) {
  CertDatabase::GetInstance()->OnAndroidKeyChainChanged();
}

void NotifyClientCertificatesChanged(JNIEnv* env, jclass clazz) {
  CertDatabase::GetInstance()->OnAndroidKeyStoreChanged();
}

void RecordCertVerifyCapabilitiesHistogram(JNIEnv* env,
                                           jclass clazz,
                                           jboolean found_system_trust_roots) {
  // Only record the histogram for 4.2 and up. Before 4.2, the platform doesn't
  // return the certificate chain anyway.
  if (base::android::BuildInfo::GetInstance()->sdk_int() >= 17) {
    UMA_HISTOGRAM_BOOLEAN("Net.FoundSystemTrustRootsAndroid",
                          found_system_trust_roots);
  }
}

jobject GetApplicationContext(JNIEnv* env, jclass clazz) {
  return base::android::GetApplicationContext();
}

bool RegisterX509Util(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // net namespace
