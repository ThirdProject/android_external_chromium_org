// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ANDROID_AUTOFILL_AUTOFILL_DIALOG_VIEW_ANDROID_H_
#define CHROME_BROWSER_UI_ANDROID_AUTOFILL_AUTOFILL_DIALOG_VIEW_ANDROID_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "chrome/browser/ui/autofill/autofill_dialog_controller.h"
#include "chrome/browser/ui/autofill/autofill_dialog_view.h"

namespace autofill {

// Android implementation of the Autofill dialog that handles the imperative
// autocomplete API call.
class AutofillDialogViewAndroid : public AutofillDialogView {
 public:
  explicit AutofillDialogViewAndroid(AutofillDialogController* controller);
  virtual ~AutofillDialogViewAndroid();

  // AutofillDialogView implementation:
  virtual void Show() OVERRIDE;
  virtual void Hide() OVERRIDE;
  virtual void UpdateNotificationArea() OVERRIDE;
  virtual void UpdateAccountChooser() OVERRIDE;
  virtual void UpdateButtonStrip() OVERRIDE;
  virtual void UpdateSection(DialogSection section) OVERRIDE;
  virtual void GetUserInput(DialogSection section,
                            DetailOutputMap* output) OVERRIDE;
  virtual string16 GetCvc() OVERRIDE;
  virtual bool UseBillingForShipping() OVERRIDE;
  virtual bool SaveDetailsInWallet() OVERRIDE;
  virtual bool SaveDetailsLocally() OVERRIDE;
  virtual const content::NavigationController* ShowSignIn() OVERRIDE;
  virtual void HideSignIn() OVERRIDE;
  virtual void UpdateProgressBar(double value) OVERRIDE;
  virtual void ModelChanged() OVERRIDE;
  virtual void SubmitForTesting() OVERRIDE;
  virtual void CancelForTesting() OVERRIDE;

  // Java to C++ calls
  void ItemSelected(JNIEnv* env, jobject obj, jint section, jint index);
  void AccountSelected(JNIEnv* env, jobject obj, jint index);
  void EditingStart(JNIEnv* env, jobject obj, jint section);
  void EditingComplete(JNIEnv* env, jobject obj, jint section);
  void EditingCancel(JNIEnv* env, jobject obj, jint section);
  void DialogSubmit(JNIEnv* env, jobject obj);
  void DialogCancel(JNIEnv* env, jobject obj);
  base::android::ScopedJavaLocalRef<jstring> GetLabelForSection(
      JNIEnv* env,
      jobject obj,
      jint section);
  base::android::ScopedJavaLocalRef<jobjectArray> GetListForField(JNIEnv* env,
                                                                  jobject obj,
                                                                  jint field);
  void ContinueAutomaticSignin(JNIEnv* env, jobject obj,
                               jstring account_name, jstring sid, jstring lsid);
  base::android::ScopedJavaLocalRef<jobject> GetIconForField(
      JNIEnv* env,
      jobject obj,
      jint field_id,
      jstring jinput);
  base::android::ScopedJavaLocalRef<jstring> GetPlaceholderForField(
      JNIEnv* env,
      jobject obj,
      jint section,
      jint field_id);

  static bool RegisterAutofillDialogViewAndroid(JNIEnv* env);

 private:
  // Returns the list of available user accounts.
  std::vector<std::string> GetAvailableUserAccounts();

  // Starts an automatic sign-in attempt for a given account.
  bool StartAutomaticSignIn(const std::string& username);

  // The controller that drives this view. Weak pointer, always non-NULL.
  AutofillDialogController* const controller_;

  // The corresponding java object.
  base::android::ScopedJavaGlobalRef<jobject> java_object_;

  DISALLOW_COPY_AND_ASSIGN(AutofillDialogViewAndroid);
};

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_ANDROID_AUTOFILL_AUTOFILL_DIALOG_VIEW_ANDROID_H_
