# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'common',
      'type': 'static_library',
      'variables': {
        'chrome_common_target': 1,
        'enable_wexit_time_destructors': 1,
      },
      'include_dirs': [
          '..',
          '<(SHARED_INTERMEDIATE_DIR)',  # Needed by chrome_content_client.cc.
        ],
      'direct_dependent_settings': {
        'include_dirs': [
          '..',
        ],
      },
      'dependencies': [
        # TODO(gregoryd): chrome_resources and chrome_strings could be
        #  shared with the 64-bit target, but it does not work due to a gyp
        # issue.
        'common_net',
        'common_version',
        'metrics_proto',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/base.gyp:base_i18n',
        '<(DEPTH)/base/base.gyp:base_prefs',
        '<(DEPTH)/base/base.gyp:base_static',
        '<(DEPTH)/build/temp_gyp/googleurl.gyp:googleurl',
        '<(DEPTH)/chrome/chrome_resources.gyp:chrome_resources',
        '<(DEPTH)/chrome/chrome_resources.gyp:chrome_strings',
        '<(DEPTH)/chrome/chrome_resources.gyp:theme_resources',
        '<(DEPTH)/chrome/common_constants.gyp:common_constants',
        '<(DEPTH)/content/content.gyp:content_common',
        '<(DEPTH)/net/net.gyp:net',
        '<(DEPTH)/skia/skia.gyp:skia',
        '<(DEPTH)/third_party/bzip2/bzip2.gyp:bzip2',
        '<(DEPTH)/third_party/icu/icu.gyp:icui18n',
        '<(DEPTH)/third_party/icu/icu.gyp:icuuc',
        '<(DEPTH)/third_party/libxml/libxml.gyp:libxml',
        '<(DEPTH)/third_party/mt19937ar/mt19937ar.gyp:mt19937ar',
        '<(DEPTH)/third_party/sqlite/sqlite.gyp:sqlite',
        '<(DEPTH)/third_party/zlib/zlib.gyp:minizip',
        '<(DEPTH)/third_party/zlib/zlib.gyp:zlib',
        '<(DEPTH)/ui/ui.gyp:ui_resources',
        '<(DEPTH)/webkit/support/webkit_support.gyp:user_agent',
      ],
      'sources': [
        '../extensions/common/constants.cc',
        '../extensions/common/constants.h',
        '../extensions/common/error_utils.cc',
        '../extensions/common/error_utils.h',
        '../extensions/common/url_pattern.cc',
        '../extensions/common/url_pattern.h',
        '../extensions/common/url_pattern_set.cc',
        '../extensions/common/url_pattern_set.h',
        'common/all_messages.h',
        'common/attrition_experiments.h',
        'common/auto_start_linux.cc',
        'common/auto_start_linux.h',
        'common/autofill/web_element_descriptor.cc',
        'common/autofill/web_element_descriptor.h',
        'common/autofill_messages.h',
        'common/automation_constants.cc',
        'common/automation_constants.h',
        'common/automation_events.cc',
        'common/automation_events.h',
        'common/automation_id.cc',
        'common/automation_id.h',
        'common/automation_messages.cc',
        'common/automation_messages.h',
        'common/automation_messages_internal.h',
        'common/badge_util.cc',
        'common/badge_util.h',
        'common/bzip2_error_handler.cc',
        'common/cancelable_task_tracker.cc',
        'common/cancelable_task_tracker.h',
        'common/child_process_logging.h',
        'common/child_process_logging_mac.mm',
        'common/child_process_logging_posix.cc',
        'common/child_process_logging_win.cc',
        'common/chrome_content_client.cc',
        'common/chrome_content_client.h',
        'common/chrome_content_client_ios.mm',
        'common/chrome_notification_types.h',
        'common/chrome_result_codes.h',
        'common/chrome_sandbox_type_mac.h',
        'common/chrome_utility_messages.h',
        'common/chrome_version_info.cc',
        'common/chrome_version_info_android.cc',
        'common/chrome_version_info_chromeos.cc',
        'common/chrome_version_info_posix.cc',
        'common/chrome_version_info_mac.mm',
        'common/chrome_version_info_win.cc',
        'common/chrome_version_info.h',
        'common/cloud_print/cloud_print_class_mac.h',
        'common/cloud_print/cloud_print_class_mac.mm',
        'common/cloud_print/cloud_print_constants.cc',
        'common/cloud_print/cloud_print_constants.h',
        'common/cloud_print/cloud_print_helpers.cc',
        'common/cloud_print/cloud_print_helpers.h',
        'common/cloud_print/cloud_print_proxy_info.cc',
        'common/cloud_print/cloud_print_proxy_info.h',
        'common/common_message_generator.cc',
        'common/common_message_generator.h',
        'common/common_param_traits.cc',
        'common/common_param_traits.h',
        'common/common_param_traits_macros.h',
        'common/content_settings.cc',
        'common/content_settings.h',
        'common/content_settings_helper.cc',
        'common/content_settings_helper.h',
        'common/content_settings_pattern.cc',
        'common/content_settings_pattern.h',
        'common/content_settings_pattern_parser.cc',
        'common/content_settings_pattern_parser.h',
        'common/content_settings_types.h',
        'common/crash_keys.cc',
        'common/crash_keys.h',
        'common/custom_handlers/protocol_handler.cc',
        'common/custom_handlers/protocol_handler.h',
        'common/descriptors_android.h',
        'common/extensions/api/extension_api.cc',
        'common/extensions/api/extension_api.h',
        'common/extensions/api/extension_api_stub.cc',
        'common/extensions/api/input_ime/input_components_handler.cc',
        'common/extensions/api/input_ime/input_components_handler.h',
        'common/extensions/api/omnibox/omnibox_handler.cc',
        'common/extensions/api/omnibox/omnibox_handler.h',
        'common/extensions/api/speech/tts_engine_manifest_handler.cc',
        'common/extensions/api/speech/tts_engine_manifest_handler.h',
        'common/extensions/command.cc',
        'common/extensions/command.h',
        'common/extensions/csp_validator.cc',
        'common/extensions/csp_validator.h',
        'common/extensions/draggable_region.cc',
        'common/extensions/draggable_region.h',
        'common/extensions/event_filter.cc',
        'common/extensions/event_filter.h',
        'common/extensions/event_filtering_info.cc',
        'common/extensions/event_filtering_info.h',
        'common/extensions/event_matcher.cc',
        'common/extensions/event_matcher.h',
        'common/extensions/extension.cc',
        'common/extensions/extension.h',
        'common/extensions/extension_constants.cc',
        'common/extensions/extension_constants.h',
        'common/extensions/extension_file_util.cc',
        'common/extensions/extension_file_util.h',
        'common/extensions/extension_icon_set.cc',
        'common/extensions/extension_icon_set.h',
        'common/extensions/extension_l10n_util.cc',
        'common/extensions/extension_l10n_util.h',
        'common/extensions/extension_localization_peer.cc',
        'common/extensions/extension_localization_peer.h',
        'common/extensions/extension_manifest_constants.cc',
        'common/extensions/extension_manifest_constants.h',
        'common/extensions/extension_messages.cc',
        'common/extensions/extension_messages.h',
        'common/extensions/extension_process_policy.cc',
        'common/extensions/extension_process_policy.h',
        'common/extensions/extension_resource.cc',
        'common/extensions/extension_resource.h',
        'common/extensions/extension_set.cc',
        'common/extensions/extension_set.h',
        'common/extensions/feature_switch.cc',
        'common/extensions/feature_switch.h',
        'common/extensions/features/base_feature_provider.cc',
        'common/extensions/features/base_feature_provider.h',
        'common/extensions/features/complex_feature.cc',
        'common/extensions/features/complex_feature.h',
        'common/extensions/features/feature.cc',
        'common/extensions/features/feature.h',
        'common/extensions/features/feature_provider.h',
        'common/extensions/features/manifest_feature.cc',
        'common/extensions/features/manifest_feature.h',
        'common/extensions/features/permission_feature.cc',
        'common/extensions/features/permission_feature.h',
        'common/extensions/features/simple_feature.cc',
        'common/extensions/features/simple_feature.h',
        'common/extensions/manifest.cc',
        'common/extensions/manifest.h',
        'common/extensions/manifest_handler.cc',
        'common/extensions/manifest_handler.h',
        'common/extensions/manifest_url_handler.cc',
        'common/extensions/manifest_url_handler.h',
        'common/extensions/matcher/regex_set_matcher.cc',
        'common/extensions/matcher/regex_set_matcher.h',
        'common/extensions/matcher/string_pattern.cc',
        'common/extensions/matcher/string_pattern.h',
        'common/extensions/matcher/substring_set_matcher.cc',
        'common/extensions/matcher/substring_set_matcher.h',
        'common/extensions/matcher/url_matcher.cc',
        'common/extensions/matcher/url_matcher.h',
        'common/extensions/matcher/url_matcher_constants.cc',
        'common/extensions/matcher/url_matcher_constants.h',
        'common/extensions/matcher/url_matcher_factory.cc',
        'common/extensions/matcher/url_matcher_factory.h',
        'common/extensions/matcher/url_matcher_helpers.cc',
        'common/extensions/matcher/url_matcher_helpers.h',
        'common/extensions/message_bundle.cc',
        'common/extensions/message_bundle.h',
        'common/extensions/permissions/api_permission.cc',
        'common/extensions/permissions/api_permission.h',
        'common/extensions/permissions/api_permission_set.cc',
        'common/extensions/permissions/api_permission_set.h',
        'common/extensions/permissions/bluetooth_device_permission.cc',
        'common/extensions/permissions/bluetooth_device_permission.h',
        'common/extensions/permissions/bluetooth_device_permission_data.cc',
        'common/extensions/permissions/bluetooth_device_permission_data.h',
        'common/extensions/permissions/permission_message.cc',
        'common/extensions/permissions/permission_message.h',
        'common/extensions/permissions/permission_set.cc',
        'common/extensions/permissions/permission_set.h',
        'common/extensions/permissions/permissions_info.cc',
        'common/extensions/permissions/permissions_info.h',
        'common/extensions/permissions/set_disjunction_permission.h',
        'common/extensions/permissions/socket_permission.cc',
        'common/extensions/permissions/socket_permission.h',
        'common/extensions/permissions/socket_permission_data.cc',
        'common/extensions/permissions/socket_permission_data.h',
        'common/extensions/permissions/usb_device_permission.cc',
        'common/extensions/permissions/usb_device_permission.h',
        'common/extensions/permissions/usb_device_permission_data.cc',
        'common/extensions/permissions/usb_device_permission_data.h',
        'common/extensions/request_media_access_permission_helper.cc',
        'common/extensions/request_media_access_permission_helper.h',
        'common/extensions/unpacker.cc',
        'common/extensions/unpacker.h',
        'common/extensions/update_manifest.cc',
        'common/extensions/update_manifest.h',
        'common/extensions/user_script.cc',
        'common/extensions/user_script.h',
        'common/extensions/value_counter.cc',
        'common/extensions/value_counter.h',
        'common/extensions/web_intents_handler.cc',
        'common/extensions/web_intents_handler.h',
        'common/external_ipc_fuzzer.h',
        'common/external_ipc_fuzzer.cc',
        'common/form_data.cc',
        'common/form_data.h',
        'common/form_data_predictions.cc',
        'common/form_data_predictions.h',
        'common/form_field_data.cc',
        'common/form_field_data.h',
        'common/form_field_data_predictions.cc',
        'common/form_field_data_predictions.h',
        'common/icon_with_badge_image_source.cc',
        'common/icon_with_badge_image_source.h',
        'common/instant_types.cc',
        'common/instant_types.h',
        'common/json_schema_constants.cc',
        'common/json_schema_constants.h',
        'common/json_schema_validator.cc',
        'common/json_schema_validator.h',
        'common/jstemplate_builder.cc',
        'common/jstemplate_builder.h',
        'common/localized_error.cc',
        'common/localized_error.h',
        'common/logging_chrome.cc',
        'common/logging_chrome.h',
        'common/mac/app_mode_common.h',
        'common/mac/app_mode_common.mm',
        'common/mac/cfbundle_blocker.h',
        'common/mac/cfbundle_blocker.mm',
        'common/mac/launchd.h',
        'common/mac/launchd.mm',
        'common/mac/nscoder_util.h',
        'common/mac/nscoder_util.mm',
        'common/mac/objc_method_swizzle.h',
        'common/mac/objc_method_swizzle.mm',
        'common/mac/objc_zombie.h',
        'common/mac/objc_zombie.mm',
        'common/metrics/entropy_provider.cc',
        'common/metrics/entropy_provider.h',
        'common/metrics/metrics_log_base.cc',
        'common/metrics/metrics_log_base.h',
        'common/metrics/metrics_log_manager.cc',
        'common/metrics/metrics_log_manager.h',
        'common/metrics/metrics_service_base.cc',
        'common/metrics/metrics_service_base.h',
        'common/metrics/metrics_util.cc',
        'common/metrics/metrics_util.h',
        'common/metrics/variations/variations_util.cc',
        'common/metrics/variations/variations_util.h',
        'common/multi_process_lock.h',
        'common/multi_process_lock_linux.cc',
        'common/multi_process_lock_mac.cc',
        'common/multi_process_lock_win.cc',
        'common/nacl_cmd_line.cc',
        'common/nacl_cmd_line.h',
        'common/nacl_messages.cc',
        'common/nacl_messages.h',
        'common/nacl_types.cc',
        'common/nacl_types.h',
        'common/password_form_fill_data.cc',
        'common/password_form_fill_data.h',
        'common/password_generation_util.cc',
        'common/password_generation_util.h',
        'common/pepper_flash.cc',
        'common/pepper_flash.h',
        'common/pref_names_util.cc',
        'common/pref_names_util.h',
        'common/print_messages.cc',
        'common/print_messages.h',
        'common/profiling.cc',
        'common/profiling.h',
        'common/ref_counted_util.h',
        'common/render_messages.cc',
        'common/render_messages.h',
        'common/safe_browsing/safebrowsing_messages.h',
        'common/search_provider.h',
        'common/search_types.h',
        'common/service_messages.h',
        'common/service_process_util.cc',
        'common/service_process_util.h',
        'common/service_process_util_linux.cc',
        'common/service_process_util_mac.mm',
        'common/service_process_util_posix.cc',
        'common/service_process_util_posix.h',
        'common/service_process_util_win.cc',
        'common/spellcheck_common.cc',
        'common/spellcheck_common.h',
        'common/spellcheck_messages.h',
        'common/spellcheck_result.h',
        'common/startup_metric_utils.cc',
        'common/startup_metric_utils.h',
        'common/switch_utils.cc',
        'common/switch_utils.h',
        'common/thumbnail_score.cc',
        'common/thumbnail_score.h',
        'common/time_format.cc',
        'common/time_format.h',
        'common/url_constants.cc',
        'common/url_constants.h',
        'common/view_type.cc',
        'common/view_type.h',
        'common/visitedlink_common.cc',
        'common/visitedlink_common.h',
        'common/web_apps.cc',
        'common/web_apps.h',
        'common/web_resource/web_resource_unpacker.cc',
        'common/web_resource/web_resource_unpacker.h',
        'common/worker_thread_ticker.cc',
        'common/worker_thread_ticker.h',
        'common/zip.cc',  # Requires zlib directly.
        'common/zip.h',
        'common/zip_internal.cc',
        'common/zip_internal.h',
        'common/zip_reader.cc',
        'common/zip_reader.h',
      ],
      'conditions': [
        ['OS != "ios"', {
          'dependencies': [
            '<(DEPTH)/chrome/app/policy/cloud_policy_codegen.gyp:policy',
            '<(DEPTH)/chrome/common/extensions/api/api.gyp:api',
            '<(DEPTH)/ipc/ipc.gyp:ipc',
            '<(DEPTH)/printing/printing.gyp:printing',
            '<(DEPTH)/third_party/adobe/flash/flash_player.gyp:flapper_version_h',
            '<(DEPTH)/third_party/re2/re2.gyp:re2',
            '<(DEPTH)/third_party/widevine/cdm/widevine_cdm.gyp:widevine_cdm_version_h',
            '<(DEPTH)/webkit/support/webkit_support.gyp:glue',
          ],
        }, {  # OS == ios
          'sources/': [
            ['exclude', '^common/automation_'],
            ['exclude', '^common/child_process_'],
            ['exclude', '^common/chrome_content_client\\.cc$'],
            ['exclude', '^common/chrome_version_info_posix\\.cc$'],
            ['exclude', '^common/common_message_generator\\.cc$'],
            ['exclude', '^common/common_param_traits'],
            ['exclude', '^common/custom_handlers/'],
            ['exclude', '^common/extensions/'],
            ['exclude', '^common/external_ipc_fuzzer\\.'],
            ['exclude', '^common/logging_chrome\\.'],
            ['exclude', '^common/multi_process_'],
            ['exclude', '^common/nacl_'],
            ['exclude', '^common/pepper_flash\\.'],
            ['exclude', '^common/profiling\\.'],
            ['exclude', '^common/service_process_util_'],
            ['exclude', '^common/spellcheck_'],
            ['exclude', '^common/web_apps\\.'],
            # TODO(ios): Include files here as they are made to work; once
            # everything is online, remove everything below here and just
            # use the exclusions above.
            ['exclude', '\\.(cc|mm)$'],
            ['include', '_ios\\.(cc|mm)$'],
            ['include', '(^|/)ios/'],
            ['include', '^common/chrome_version_info\\.cc$'],
            ['include', '^common/zip'],
          ],
        }],
        ['OS=="android"', {
          'sources/': [
            ['exclude', '^common/chrome_version_info_posix.cc'],
            ['exclude', '^common/service_'],
          ],
          'dependencies!': [
            '<(DEPTH)/chrome/app/policy/cloud_policy_codegen.gyp:policy',
          ],
        }],
        ['OS=="win"', {
          'include_dirs': [
            '<(DEPTH)/third_party/wtl/include',
          ]
        }],
        ['toolkit_uses_gtk == 1', {
          'dependencies': [
            '../build/linux/system.gyp:gtk',
          ],
          'export_dependent_settings': [
            '../third_party/sqlite/sqlite.gyp:sqlite',
          ],
          'link_settings': {
            'libraries': [
              '-lX11',
              '-lXrender',
              '-lXss',
              '-lXext',
            ],
          },
        }],
        ['OS=="linux" and selinux==1', {
          'dependencies': [
            '../build/linux/system.gyp:selinux',
          ],
        }],
        ['chromeos==1', {
          'sources!': [
            'common/chrome_version_info_linux.cc',
          ],
        }],
        ['OS=="mac"', {
          'dependencies': [
            '../third_party/mach_override/mach_override.gyp:mach_override',
          ],
          'include_dirs': [
            '../third_party/GTM',
          ],
          'sources!': [
            'common/child_process_logging_posix.cc',
            'common/chrome_version_info_posix.cc',
          ],
        }],
        ['enable_extensions==1', {
          'sources!': [
            'common/extensions/api/extension_api_stub.cc',
          ],
          'dependencies': [
            '../device/device.gyp:device_usb',
          ],
        }, {  # enable_extensions == 0
          'sources/': [
            ['exclude', '^common/extensions/api/'],
            ['include', 'common/extensions/api/extension_api_stub.cc'],
          ],
        }],
        ['remoting==1', {
          'dependencies': [
            '../remoting/remoting.gyp:remoting_client_plugin',
          ],
        }],
      ],
      'target_conditions': [
        ['OS == "ios"', {
          'sources/': [
            # Pull in specific Mac files for iOS (which have been filtered out
            # by file name rules).
            ['include', '^common/chrome_version_info_mac\\.mm$'],
            ['include', '^common/mac/nscoder_util\\.'],
          ],
        }],
      ],
      'export_dependent_settings': [
        '../base/base.gyp:base',
        'metrics_proto',
      ],
    },
    {
      'target_name': 'common_version',
      'type': 'none',
      'conditions': [
        ['os_posix == 1 and OS != "mac" and OS != "ios"', {
          'direct_dependent_settings': {
            'include_dirs': [
              '<(SHARED_INTERMEDIATE_DIR)',
            ],
          },
          # Because posix_version generates a header, we must set the
          # hard_dependency flag.
          'hard_dependency': 1,
          'actions': [
            {
              'action_name': 'posix_version',
              'variables': {
                'lastchange_path':
                  '<(DEPTH)/build/util/LASTCHANGE',
                'version_py_path': 'tools/build/version.py',
                'version_path': 'VERSION',
                'template_input_path': 'common/chrome_version_info_posix.h.version',
              },
              'conditions': [
                [ 'branding == "Chrome"', {
                  'variables': {
                     'branding_path':
                       'app/theme/google_chrome/BRANDING',
                  },
                }, { # else branding!="Chrome"
                  'variables': {
                     'branding_path':
                       'app/theme/chromium/BRANDING',
                  },
                }],
              ],
              'inputs': [
                '<(template_input_path)',
                '<(version_path)',
                '<(branding_path)',
                '<(lastchange_path)',
              ],
              'outputs': [
                '<(SHARED_INTERMEDIATE_DIR)/chrome/common/chrome_version_info_posix.h',
              ],
              'action': [
                'python',
                '<(version_py_path)',
                '-f', '<(version_path)',
                '-f', '<(branding_path)',
                '-f', '<(lastchange_path)',
                '<(template_input_path)',
                '<@(_outputs)',
              ],
              'message': 'Generating version information',
            },
          ],
        }],
      ],
    },
    {
      'target_name': 'common_net',
      'type': 'static_library',
      'sources': [
        'common/net/net_resource_provider.cc',
        'common/net/net_resource_provider.h',
        'common/net/predictor_common.h',
        'common/net/url_util.cc',
        'common/net/url_util.h',
        'common/net/x509_certificate_model.cc',
        'common/net/x509_certificate_model_nss.cc',
        'common/net/x509_certificate_model_openssl.cc',
        'common/net/x509_certificate_model.h',
      ],
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/chrome/chrome_resources.gyp:chrome_resources',
        '<(DEPTH)/chrome/chrome_resources.gyp:chrome_strings',
        '<(DEPTH)/crypto/crypto.gyp:crypto',
        '<(DEPTH)/net/net.gyp:net_resources',
        '<(DEPTH)/net/net.gyp:net',
        '<(DEPTH)/third_party/icu/icu.gyp:icui18n',
        '<(DEPTH)/third_party/icu/icu.gyp:icuuc',
      ],
      'conditions': [
        ['OS != "ios"', {
          'dependencies': [
            '<(DEPTH)/gpu/gpu.gyp:gpu_ipc',
          ],
        }, {  # OS == ios
          'sources!': [
            'common/net/net_resource_provider.cc',
            'common/net/x509_certificate_model.cc',
          ],
        }],
        ['os_posix == 1 and OS != "mac" and OS != "ios" and OS != "android"', {
            'dependencies': [
              '../build/linux/system.gyp:ssl',
            ],
          },
        ],
        ['os_posix != 1 or OS == "mac" or OS == "ios"', {
            'sources!': [
              'common/net/x509_certificate_model_nss.cc',
              'common/net/x509_certificate_model_openssl.cc',
            ],
          },
        ],
        ['OS == "android"', {
            'dependencies': [
              '../third_party/openssl/openssl.gyp:openssl',
            ],
          },
        ],
        ['use_openssl==1', {
            'sources!': [
              'common/net/x509_certificate_model_nss.cc',
            ],
          },
          {  # else !use_openssl: remove the unneeded files
            'sources!': [
              'common/net/x509_certificate_model_openssl.cc',
            ],
          },
        ],
      ],
    },
    {
      # Protobuf compiler / generator for the safebrowsing client
      # model proto and the client-side detection (csd) request
      # protocol buffer.
      'target_name': 'safe_browsing_proto',
      'type': 'static_library',
      'sources': [
        'common/safe_browsing/client_model.proto',
        'common/safe_browsing/csd.proto'
      ],
      'variables': {
        'proto_in_dir': 'common/safe_browsing',
        'proto_out_dir': 'chrome/common/safe_browsing',
      },
      'includes': [ '../build/protoc.gypi' ],
    },
    {
      # Protobuf compiler / generator for UMA (User Metrics Analysis).
      'target_name': 'metrics_proto',
      'type': 'static_library',
      'sources': [
        'common/metrics/proto/chrome_experiments.proto',
        'common/metrics/proto/chrome_user_metrics_extension.proto',
        'common/metrics/proto/histogram_event.proto',
        'common/metrics/proto/omnibox_event.proto',
        'common/metrics/proto/profiler_event.proto',
        'common/metrics/proto/system_profile.proto',
        'common/metrics/proto/user_action_event.proto',
      ],
      'variables': {
        'proto_in_dir': 'common/metrics/proto',
        'proto_out_dir': 'chrome/common/metrics/proto',
      },
      'includes': [ '../build/protoc.gypi' ],
    },
  ],
}
