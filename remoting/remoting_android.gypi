# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'conditions': [
    ['OS=="android"', {
      'targets': [
        {
          'target_name': 'remoting_jni_headers',
          'type': 'none',
          'sources': [
            'android/java/src/org/chromium/chromoting/jni/JniInterface.java',
          ],
          'variables': {
            'jni_gen_package': 'remoting',
          },
          'includes': [ '../build/jni_generator.gypi' ],
        },  # end of target 'remoting_jni_headers'
        {
          'target_name': 'remoting_client_jni',
          'type': 'shared_library',
          'dependencies': [
            'remoting_base',
            'remoting_client',
            'remoting_jni_headers',
            'remoting_protocol',
            '../google_apis/google_apis.gyp:google_apis',
            '../ui/gfx/gfx.gyp:gfx',
          ],
          'sources': [
            'client/jni/android_keymap.cc',
            'client/jni/android_keymap.h',
            'client/jni/chromoting_jni_instance.cc',
            'client/jni/chromoting_jni_instance.h',
            'client/jni/chromoting_jni_onload.cc',
            'client/jni/chromoting_jni_runtime.cc',
            'client/jni/chromoting_jni_runtime.h',
            'client/jni/jni_frame_consumer.cc',
            'client/jni/jni_frame_consumer.h',
          ],
        },  # end of target 'remoting_client_jni'
        {
          'target_name': 'remoting_android_resources',
          'type': 'none',
          'copies': [
            {
              'destination': '<(SHARED_INTERMEDIATE_DIR)/remoting/android/res/drawable',
              'files': [
                'resources/chromoting128.png',
                'resources/icon_host.png',
              ],
            },
          ],
        },  # end of target 'remoting_android_resources'
        {
          'target_name': 'remoting_apk_manifest',
          'type': 'none',
          'conditions': [
            ['enable_cast==1', {
              'sources': [
                'android/cast/AndroidManifest.xml.jinja2'
              ],
            }, { # 'enable_cast != 1'
              'sources': [
                'android/java/AndroidManifest.xml.jinja2',
              ],
            }],
          ],
          'rules': [{
            'rule_name': 'generate_manifest',
            'extension': 'jinja2',
            'inputs': [
              '<(remoting_localize_path)',
              '<(branding_path)',
            ],
            'outputs': [
              '<(SHARED_INTERMEDIATE_DIR)/remoting/android/<(RULE_INPUT_ROOT)',
            ],
            'action': [
              'python', '<(remoting_localize_path)',
              '--variables', '<(branding_path)',
              '--template', '<(RULE_INPUT_PATH)',
              '--locale_output', '<@(_outputs)',
              'en',
            ],
          }],
        },  # end of target 'remoting_apk_manifest'
        {
          'target_name': 'remoting_android_client_java',
          'type': 'none',
          'variables': {
            'java_in_dir': 'android/java',
            'has_java_resources': 1,
            'R_package': 'org.chromium.chromoting',
            'R_package_relpath': 'org/chromium/chromoting',
            'res_extra_dirs': [ '<(SHARED_INTERMEDIATE_DIR)/remoting/android/res' ],
            'res_extra_files': [
              '<!@pymod_do_main(grit_info <@(grit_defines) --outputs "<(SHARED_INTERMEDIATE_DIR)" resources/remoting_strings.grd)',
              '<(SHARED_INTERMEDIATE_DIR)/remoting/android/res/drawable/chromoting128.png',
              '<(SHARED_INTERMEDIATE_DIR)/remoting/android/res/drawable/icon_host.png',
            ],
          },
          'dependencies': [
            'android_support_v4_javalib_no_res',
            '../base/base.gyp:base_java',
            '../ui/android/ui_android.gyp:ui_java',
            'remoting_android_resources',
            '../third_party/android_tools/android_tools.gyp:android_support_v7_appcompat_javalib',
            '../third_party/android_tools/android_tools.gyp:android_support_v7_mediarouter_javalib',
          ],
          'includes': [ '../build/java.gypi' ],
          'conditions' : [
            ['enable_cast==1', {
              'variables': {
                'additional_src_dirs': [
                  'android/cast',
                ],
              },
              'dependencies': [
                'google_play_services_javalib',
              ],
            }],
          ],
        },
        {
          'target_name': 'remoting_apk',
          'type': 'none',
          'dependencies': [
            'remoting_apk_manifest',
            'remoting_client_jni',
            'remoting_android_client_java',
          ],
          'variables': {
            'apk_name': '<!(python <(version_py_path) -f <(branding_path) -t "@APK_FILE_NAME@")',
            'android_app_version_name': '<(version_full)',
            'android_app_version_code': '<!(python tools/android_version.py <(android_app_version_name))',
            'android_manifest_path': '<(SHARED_INTERMEDIATE_DIR)/remoting/android/AndroidManifest.xml',
            'java_in_dir': 'android/apk',
            'native_lib_target': 'libremoting_client_jni',
          },
          'includes': [ '../build/java_apk.gypi' ],
        },  # end of target 'remoting_apk'
        {
          'target_name': 'remoting_test_apk',
          'type': 'none',
          'dependencies': [
            '../base/base.gyp:base_java_test_support',
            'remoting_android_client_java',
          ],
          'variables': {
            'apk_name': 'ChromotingTest',
            'java_in_dir': 'android/javatests',
            'is_test_apk': 1,
          },
          'includes': [ '../build/java_apk.gypi' ],
        },  # end of target 'remoting_test_apk'
        {
          # This jar contains the Android support v4 libary. It does not have
          # any associated resources.
          'target_name': 'android_support_v4_javalib_no_res',
          'type': 'none',
          'variables': {
            'jar_path': '../third_party/android_tools/sdk/extras/android/support/v4/android-support-v4.jar',
          },
          'includes': ['../build/java_prebuilt.gypi'],
        }, # end of target 'android_support_v4_javalib_no_res'
      ], # end of 'targets'
      'conditions': [
        ['enable_cast==1', {
            'targets': [
              {
                # This jar contains the Google Play services library without the
                # resources needed for the library to work. See crbug.com/274697 or
                # ../third_party/android_tools/android_tools.gyp for more info.
                # This target will fail to build unless you have a local version
                # of the Google Play services jar.
                'target_name': 'google_play_services_javalib_no_res',
                'type': 'none',
                'variables': {
                  'jar_path': 'android/google-play-services_lib/libs/google-play-services.jar',
                },
                'includes': ['../build/java_prebuilt.gypi'],

              }, # end of target 'google_play_services_javalib_no_res'
              {
                # This target contains the Google Play services library with the
                # resources needed. It will fail to build unless you have a local
                # version of the Google Play services libary project.
                # TODO(aiguha): Solve issue of needing to use local version. Also,
                # watch crbug.com/274697.
                'target_name': 'google_play_services_javalib',
                'type': 'none',
                'variables': {
                  'java_in_dir': 'android/google-play-services_lib/',
                  'R_package': ['com.google.android.gms'],
                  'R_package_relpath': ['com/google/android/gms'],
                  'has_java_resources': 1,
                  'res_v14_verify_only': 1,
                },
                'dependencies': [
                  'google_play_services_javalib_no_res',
                ],
                'includes': ['../build/java.gypi'],
              }, # end of target 'google_play_services_javalib'
            ], # end of targets
        }],
      ],
    }],  # 'OS=="android"'

    ['OS=="android"', {
      'targets': [
        {
          'target_name': 'remoting_unittests_apk',
          'type': 'none',
          'dependencies': [
            'remoting_unittests',
          ],
          'variables': {
            'test_suite_name': 'remoting_unittests',
          },
          'includes': [ '../build/apk_test.gypi' ],
        },
      ],
    }],  # 'OS=="android"
  ],  # end of 'conditions'
}
