# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'renderer',
      'type': 'static_library',
      'variables': { 'enable_wexit_time_destructors': 1, },
      'dependencies': [
        'common',
        'common_net',
        'chrome_resources.gyp:chrome_resources',
        'chrome_resources.gyp:chrome_strings',
        '../components/components.gyp:visitedlink_renderer',
        '../content/content.gyp:content_renderer',
        '../net/net.gyp:net',
        '../printing/printing.gyp:printing',
        '../skia/skia.gyp:skia',
        '../third_party/hunspell/hunspell.gyp:hunspell',
        '../third_party/icu/icu.gyp:icui18n',
        '../third_party/icu/icu.gyp:icuuc',
        '../third_party/npapi/npapi.gyp:npapi',
        '../third_party/WebKit/Source/WebKit/chromium/WebKit.gyp:webkit',
        '../third_party/widevine/cdm/widevine_cdm.gyp:widevine_cdm_version_h',
        '../ui/surface/surface.gyp:surface',
        '../webkit/gpu/webkit_gpu.gyp:webkit_gpu',
        '../webkit/support/webkit_support.gyp:glue',
        '../webkit/support/webkit_support.gyp:webkit_media',
        '../webkit/support/webkit_support.gyp:webkit_resources',
      ],
      'include_dirs': [
        '..',
        # Needed by chrome_content_renderer_client.cc.
        '<(SHARED_INTERMEDIATE_DIR)',
      ],
      'defines': [
        '<@(nacl_defines)',
      ],
      'direct_dependent_settings': {
        'defines': [
          '<@(nacl_defines)',
        ],
      },
      'sources': [
        'renderer/automation/automation_renderer_helper.cc',
        'renderer/automation/automation_renderer_helper.h',
        'renderer/benchmarking_extension.cc',
        'renderer/benchmarking_extension.h',
        'renderer/extensions/api_definitions_natives.cc',
        'renderer/extensions/api_definitions_natives.h',
        'renderer/extensions/app_bindings.cc',
        'renderer/extensions/app_bindings.h',
        'renderer/extensions/app_runtime_custom_bindings.cc',
        'renderer/extensions/app_runtime_custom_bindings.h',
        'renderer/extensions/app_window_custom_bindings.cc',
        'renderer/extensions/app_window_custom_bindings.h',
        'renderer/extensions/binding_generating_native_handler.cc',
        'renderer/extensions/binding_generating_native_handler.h',
        'renderer/extensions/chrome_v8_context.cc',
        'renderer/extensions/chrome_v8_context.h',
        'renderer/extensions/chrome_v8_context_set.cc',
        'renderer/extensions/chrome_v8_context_set.h',
        'renderer/extensions/chrome_v8_extension.cc',
        'renderer/extensions/chrome_v8_extension.h',
        'renderer/extensions/chrome_v8_extension_handler.cc',
        'renderer/extensions/chrome_v8_extension_handler.h',
        'renderer/extensions/console.cc',
        'renderer/extensions/console.h',
        'renderer/extensions/content_watcher.cc',
        'renderer/extensions/content_watcher.h',
        'renderer/extensions/context_menus_custom_bindings.cc',
        'renderer/extensions/context_menus_custom_bindings.h',
        'renderer/extensions/dispatcher.cc',
        'renderer/extensions/dispatcher.h',
        'renderer/extensions/dom_activity_logger.cc',
        'renderer/extensions/dom_activity_logger.h',
        'renderer/extensions/event_bindings.cc',
        'renderer/extensions/event_bindings.h',
        'renderer/extensions/extension_custom_bindings.cc',
        'renderer/extensions/extension_custom_bindings.h',
        'renderer/extensions/extension_groups.h',
        'renderer/extensions/extension_helper.cc',
        'renderer/extensions/extension_helper.h',
        'renderer/extensions/file_browser_handler_custom_bindings.cc',
        'renderer/extensions/file_browser_handler_custom_bindings.h',
        'renderer/extensions/file_browser_private_custom_bindings.cc',
        'renderer/extensions/file_browser_private_custom_bindings.h',
        'renderer/extensions/file_system_natives.cc',
        'renderer/extensions/file_system_natives.h',
        'renderer/extensions/i18n_custom_bindings.cc',
        'renderer/extensions/i18n_custom_bindings.h',
        'renderer/extensions/media_galleries_custom_bindings.cc',
        'renderer/extensions/media_galleries_custom_bindings.h',
        'renderer/extensions/miscellaneous_bindings.cc',
        'renderer/extensions/miscellaneous_bindings.h',
        'renderer/extensions/module_system.cc',
        'renderer/extensions/module_system.h',
        'renderer/extensions/native_handler.cc',
        'renderer/extensions/native_handler.h',
        'renderer/extensions/object_backed_native_handler.cc',
        'renderer/extensions/object_backed_native_handler.h',
        'renderer/extensions/page_actions_custom_bindings.cc',
        'renderer/extensions/page_actions_custom_bindings.h',
        'renderer/extensions/page_capture_custom_bindings.cc',
        'renderer/extensions/page_capture_custom_bindings.h',
        'renderer/extensions/request_sender.cc',
        'renderer/extensions/request_sender.h',
        'renderer/extensions/resource_request_policy.cc',
        'renderer/extensions/resource_request_policy.h',
        'renderer/extensions/runtime_custom_bindings.cc',
        'renderer/extensions/runtime_custom_bindings.h',
        'renderer/extensions/scoped_persistent.h',
        'renderer/extensions/send_request_natives.cc',
        'renderer/extensions/send_request_natives.h',
        'renderer/extensions/set_icon_natives.cc',
        'renderer/extensions/set_icon_natives.h',
        'renderer/extensions/sync_file_system_custom_bindings.cc',
        'renderer/extensions/sync_file_system_custom_bindings.h',
        'renderer/extensions/tab_finder.cc',
        'renderer/extensions/tab_finder.h',
        'renderer/extensions/tabs_custom_bindings.cc',
        'renderer/extensions/tabs_custom_bindings.h',
        'renderer/extensions/tts_custom_bindings.cc',
        'renderer/extensions/tts_custom_bindings.h',
        'renderer/extensions/user_script_scheduler.cc',
        'renderer/extensions/user_script_scheduler.h',
        'renderer/extensions/user_script_slave.cc',
        'renderer/extensions/user_script_slave.h',
        'renderer/extensions/v8_schema_registry.cc',
        'renderer/extensions/v8_schema_registry.h',
        'renderer/extensions/web_request_custom_bindings.cc',
        'renderer/extensions/web_request_custom_bindings.h',
        'renderer/extensions/webstore_bindings.cc',
        'renderer/extensions/webstore_bindings.h',
        'renderer/frame_sniffer.cc',
        'renderer/frame_sniffer.h',
        'renderer/loadtimes_extension_bindings.cc',
        'renderer/loadtimes_extension_bindings.h',
        'renderer/net/net_error_helper.cc',
        'renderer/net/net_error_helper.h',
        'renderer/net/predictor_queue.cc',
        'renderer/net/predictor_queue.h',
        'renderer/net/renderer_net_predictor.cc',
        'renderer/net/renderer_net_predictor.h',
        'renderer/net_benchmarking_extension.cc',
        'renderer/net_benchmarking_extension.h',
        'renderer/one_click_signin_agent.cc',
        'renderer/one_click_signin_agent.h',
        'renderer/playback_extension.cc',
        'renderer/playback_extension.h',
        'renderer/resource_bundle_source_map.cc',
        'renderer/resource_bundle_source_map.h',
        'renderer/resources/extensions/app_custom_bindings.js',
        'renderer/resources/extensions/app_window_custom_bindings.js',
        'renderer/resources/extensions/binding.js',
        'renderer/resources/extensions/browser_action_custom_bindings.js',
        'renderer/resources/extensions/content_watcher.js',
        'renderer/resources/extensions/context_menus_custom_bindings.js',
        'renderer/resources/extensions/declarative_content_custom_bindings.js',
        'renderer/resources/extensions/declarative_webrequest_custom_bindings.js',
        'renderer/resources/extensions/event.js',
        'renderer/resources/extensions/experimental.offscreenTabs_custom_bindings.js',
        'renderer/resources/extensions/extension_custom_bindings.js',
        'renderer/resources/extensions/file_browser_handler_custom_bindings.js',
        'renderer/resources/extensions/file_browser_private_custom_bindings.js',
        'renderer/resources/extensions/file_system_custom_bindings.js',
        'renderer/resources/extensions/greasemonkey_api.js',
        'renderer/resources/extensions/input.ime_custom_bindings.js',
        'renderer/resources/extensions/json.js',
        'renderer/resources/extensions/json_schema.js',
        'renderer/resources/extensions/last_error.js',
        'renderer/resources/extensions/miscellaneous_bindings.js',
        'renderer/resources/extensions/notifications_custom_bindings.js',
        'renderer/resources/extensions/omnibox_custom_bindings.js',
        'renderer/resources/extensions/page_action_custom_bindings.js',
        'renderer/resources/extensions/page_actions_custom_bindings.js',
        'renderer/resources/extensions/page_capture_custom_bindings.js',
        'renderer/resources/extensions/platform_app.js',
        'renderer/resources/extensions/runtime_custom_bindings.js',
        'renderer/resources/extensions/send_request.js',
        'renderer/resources/extensions/set_icon.js',
        'renderer/resources/extensions/system_indicator_custom_bindings.js',
        'renderer/resources/extensions/test_custom_bindings.js',
        'renderer/resources/extensions/tts_custom_bindings.js',
        'renderer/resources/extensions/tts_engine_custom_bindings.js',
        'renderer/resources/extensions/types_custom_bindings.js',
        'renderer/resources/extensions/utils.js',
        'renderer/resources/extensions/web_request_custom_bindings.js',
        'renderer/resources/extensions/web_view.js',
        'renderer/resources/extensions/web_view_experimental.js',
        'renderer/chrome_content_renderer_client.cc',
        'renderer/chrome_content_renderer_client.h',
        'renderer/chrome_render_process_observer.cc',
        'renderer/chrome_render_process_observer.h',
        'renderer/chrome_render_view_observer.cc',
        'renderer/chrome_render_view_observer.h',
        'renderer/content_settings_observer.cc',
        'renderer/content_settings_observer.h',
        'renderer/custom_menu_commands.h',
        'renderer/external_host_bindings.cc',
        'renderer/external_host_bindings.h',
        'renderer/external_extension.cc',
        'renderer/external_extension.h',
        'renderer/page_load_histograms.cc',
        'renderer/page_load_histograms.h',
        'renderer/pepper/chrome_ppapi_interfaces.cc',
        'renderer/pepper/chrome_ppapi_interfaces.h',
        'renderer/pepper/chrome_renderer_pepper_host_factory.cc',
        'renderer/pepper/chrome_renderer_pepper_host_factory.h',
        'renderer/pepper/pepper_flash_font_file_host.cc',
        'renderer/pepper/pepper_flash_font_file_host.h',
        'renderer/pepper/pepper_flash_fullscreen_host.cc',
        'renderer/pepper/pepper_flash_fullscreen_host.h',
        'renderer/pepper/pepper_flash_menu_host.cc',
        'renderer/pepper/pepper_flash_menu_host.h',
        'renderer/pepper/pepper_flash_renderer_host.cc',
        'renderer/pepper/pepper_flash_renderer_host.h',
        'renderer/pepper/pepper_helper.cc',
        'renderer/pepper/pepper_helper.h',
        'renderer/pepper/pepper_shared_memory_message_filter.cc',
        'renderer/pepper/pepper_shared_memory_message_filter.h',
        'renderer/pepper/ppb_nacl_private_impl.cc',
        'renderer/pepper/ppb_nacl_private_impl.h',
        'renderer/pepper/ppb_pdf_impl.cc',
        'renderer/pepper/ppb_pdf_impl.h',
        'renderer/plugins/plugin_placeholder.cc',
        'renderer/plugins/plugin_placeholder.h',
        'renderer/plugins/plugin_uma.cc',
        'renderer/plugins/plugin_uma.h',
        'renderer/prerender/prerender_dispatcher.cc',
        'renderer/prerender/prerender_dispatcher.h',
        'renderer/prerender/prerender_extra_data.cc',
        'renderer/prerender/prerender_extra_data.h',
        'renderer/prerender/prerender_helper.cc',
        'renderer/prerender/prerender_helper.h',
        'renderer/prerender/prerender_webmediaplayer.cc',
        'renderer/prerender/prerender_webmediaplayer.h',
        'renderer/prerender/prerenderer_client.cc',
        'renderer/prerender/prerenderer_client.h',
        'renderer/printing/print_web_view_helper.cc',
        'renderer/printing/print_web_view_helper.h',
        'renderer/printing/print_web_view_helper_linux.cc',
        'renderer/printing/print_web_view_helper_mac.mm',
        'renderer/printing/print_web_view_helper_win.cc',
        'renderer/safe_browsing/feature_extractor_clock.cc',
        'renderer/safe_browsing/feature_extractor_clock.h',
        'renderer/safe_browsing/features.cc',
        'renderer/safe_browsing/features.h',
        'renderer/safe_browsing/malware_dom_details.cc',
        'renderer/safe_browsing/malware_dom_details.h',
        'renderer/safe_browsing/murmurhash3_util.cc',
        'renderer/safe_browsing/murmurhash3_util.h',
        'renderer/safe_browsing/phishing_classifier.cc',
        'renderer/safe_browsing/phishing_classifier.h',
        'renderer/safe_browsing/phishing_classifier_delegate.cc',
        'renderer/safe_browsing/phishing_classifier_delegate.h',
        'renderer/safe_browsing/phishing_dom_feature_extractor.cc',
        'renderer/safe_browsing/phishing_dom_feature_extractor.h',
        'renderer/safe_browsing/phishing_term_feature_extractor.cc',
        'renderer/safe_browsing/phishing_term_feature_extractor.h',
        'renderer/safe_browsing/phishing_thumbnailer.cc',
        'renderer/safe_browsing/phishing_thumbnailer.h',
        'renderer/safe_browsing/phishing_url_feature_extractor.cc',
        'renderer/safe_browsing/phishing_url_feature_extractor.h',
        'renderer/safe_browsing/scorer.cc',
        'renderer/safe_browsing/scorer.h',
        'renderer/searchbox/searchbox.cc',
        'renderer/searchbox/searchbox.h',
        'renderer/searchbox/searchbox_extension.cc',
        'renderer/searchbox/searchbox_extension.h',
        'renderer/security_filter_peer.cc',
        'renderer/security_filter_peer.h',
        'renderer/spellchecker/cocoa_spelling_engine_mac.cc',
        'renderer/spellchecker/cocoa_spelling_engine_mac.h',
        'renderer/spellchecker/custom_dictionary_engine.cc',
        'renderer/spellchecker/custom_dictionary_engine.h',
        'renderer/spellchecker/hunspell_engine.cc',
        'renderer/spellchecker/hunspell_engine.h',
        'renderer/spellchecker/spellcheck_provider.cc',
        'renderer/spellchecker/spellcheck_provider.h',
        'renderer/spellchecker/spellcheck.cc',
        'renderer/spellchecker/spellcheck.h',
        'renderer/spellchecker/spellcheck_language.cc',
        'renderer/spellchecker/spellcheck_language.h',
        'renderer/spellchecker/spellcheck_worditerator.cc',
        'renderer/spellchecker/spellcheck_worditerator.h',
        'renderer/spellchecker/spelling_engine.h',
        'renderer/static_v8_external_string_resource.cc',
        'renderer/static_v8_external_string_resource.h',
        'renderer/translate_helper.cc',
        'renderer/translate_helper.h',
        'renderer/webview_color_overlay.cc',
        'renderer/webview_color_overlay.h',

        # TODO(joi): Move to components/autofill.gypi once last
        # remaining dependencies are broken.
        '../components/autofill/renderer/autofill_agent.cc',
        '../components/autofill/renderer/autofill_agent.h',
        '../components/autofill/renderer/form_autofill_util.cc',
        '../components/autofill/renderer/form_autofill_util.h',
        '../components/autofill/renderer/form_cache.cc',
        '../components/autofill/renderer/form_cache.h',
        '../components/autofill/renderer/page_click_listener.h',
        '../components/autofill/renderer/page_click_tracker.cc',
        '../components/autofill/renderer/page_click_tracker.h',
        '../components/autofill/renderer/password_autofill_manager.cc',
        '../components/autofill/renderer/password_autofill_manager.h',
        '../components/autofill/renderer/password_generation_manager.cc',
        '../components/autofill/renderer/password_generation_manager.h',
      ],
      'conditions': [
        ['disable_nacl!=1', {
          'dependencies': [
            'nacl',
          ],
        }],
        ['enable_plugins==1', {
          'dependencies': [
            '../ppapi/ppapi_internal.gyp:ppapi_host',
            '../ppapi/ppapi_internal.gyp:ppapi_proxy',
            '../ppapi/ppapi_internal.gyp:ppapi_ipc',
            '../ppapi/ppapi_internal.gyp:ppapi_shared',
          ],
        }, {  # enable_plugins==0
          'sources/': [
            ['exclude', '^renderer/pepper/'],
          ],
        }],
        ['safe_browsing==1', {
          'defines': [
            'FULL_SAFE_BROWSING',
          ],
          'dependencies': [
            'safe_browsing_proto',
            '../third_party/smhasher/smhasher.gyp:murmurhash3',
          ],
        }, {  # safe_browsing==0 || safe_browsing==2
          'sources/': [
            ['exclude', '^renderer/safe_browsing/'],
          ],
        }],
        ['OS=="mac"', {
          'dependencies': [
            '../third_party/mach_override/mach_override.gyp:mach_override',
          ],
        }],
        ['enable_language_detection==1', {
          'dependencies': [
            '../third_party/cld/cld.gyp:cld',
          ],
          'include_dirs': [
            '../third_party/cld',
          ],
        }],
        ['toolkit_uses_gtk == 1', {
          'dependencies': [
            '../build/linux/system.gyp:gtk',
            '../sandbox/sandbox.gyp:sandbox',
          ],
        }],
	['enable_automation==0', {
	  'sources/': [
            ['exclude', '^renderer/automation/']
	  ]
	}],
        ['OS=="android"', {
          'sources!': [
            'renderer/prerender/prerender_webmediaplayer.cc',
            'renderer/prerender/prerender_webmediaplayer.h',
            'renderer/printing/print_web_view_helper.cc',
          ],
          'defines': [
            'ENABLE_MOBILE_YOUTUBE_PLUGIN',
          ],
        }],
        ['OS=="win"', {
          'include_dirs': [
            '<(DEPTH)/third_party/wtl/include',
          ],
          'conditions': [
            ['win_use_allocator_shim==1', {
              'dependencies': [
                '<(allocator_target)',
              ],
              'export_dependent_settings': [
                '<(allocator_target)',
              ],
            }],
          ],
        }],
      ],
    },
  ],
}
