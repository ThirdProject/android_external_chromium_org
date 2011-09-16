# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'renderer',
      'type': 'static_library',
      'dependencies': [
        'common',
        'common_net',
        'chrome_resources',
        'chrome_strings',
        'safe_browsing_proto',
        '../content/content.gyp:content_renderer',
        '../content/content.gyp:content_plugin',
        '../net/net.gyp:net',
        '../ppapi/ppapi_internal.gyp:ppapi_proxy',
        '../printing/printing.gyp:printing',
        '../skia/skia.gyp:skia',
        '../third_party/cld/cld.gyp:cld',
        '../third_party/hunspell/hunspell.gyp:hunspell',
        '../third_party/icu/icu.gyp:icui18n',
        '../third_party/icu/icu.gyp:icuuc',
        '../third_party/npapi/npapi.gyp:npapi',
        '../third_party/smhasher/smhasher.gyp:murmurhash3',
        '../third_party/WebKit/Source/WebKit/chromium/WebKit.gyp:webkit',
        '../ui/gfx/surface/surface.gyp:surface',
        '../webkit/support/webkit_support.gyp:glue',
        '../webkit/support/webkit_support.gyp:webkit_gpu',
        '../webkit/support/webkit_support.gyp:webkit_resources',
      ],
      'include_dirs': [
        '..',
        '../third_party/cld',
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
        'renderer/autofill/autofill_agent.cc',
        'renderer/autofill/autofill_agent.h',
        'renderer/autofill/form_autofill_util.cc',
        'renderer/autofill/form_autofill_util.h',
        'renderer/autofill/form_cache.cc',
        'renderer/autofill/form_cache.h',
        'renderer/autofill/password_autofill_manager.cc',
        'renderer/autofill/password_autofill_manager.h',
        'renderer/automation/automation_renderer_helper.cc',
        'renderer/automation/automation_renderer_helper.h',
        'renderer/automation/dom_automation_controller.cc',
        'renderer/automation/dom_automation_controller.h',
        'renderer/automation/dom_automation_v8_extension.cc',
        'renderer/automation/dom_automation_v8_extension.h',
        'renderer/benchmarking_extension.cc',
        'renderer/benchmarking_extension.h',
        'renderer/extensions/bindings_utils.cc',
        'renderer/extensions/bindings_utils.h',
        'renderer/extensions/chrome_app_bindings.cc',
        'renderer/extensions/chrome_app_bindings.h',
        'renderer/extensions/chrome_webstore_bindings.cc',
        'renderer/extensions/chrome_webstore_bindings.h',
        'renderer/extensions/event_bindings.cc',
        'renderer/extensions/event_bindings.h',
        'renderer/extensions/extension_base.cc',
        'renderer/extensions/extension_base.h',
        'renderer/extensions/extension_dispatcher.cc',
        'renderer/extensions/extension_dispatcher.h',
        'renderer/extensions/extension_groups.h',
        'renderer/extensions/extension_helper.cc',
        'renderer/extensions/extension_helper.h',
        'renderer/extensions/extension_process_bindings.cc',
        'renderer/extensions/extension_process_bindings.h',
        'renderer/extensions/extension_resource_request_policy.cc',
        'renderer/extensions/extension_resource_request_policy.h',
        'renderer/extensions/js_only_v8_extensions.cc',
        'renderer/extensions/js_only_v8_extensions.h',
        'renderer/extensions/renderer_extension_bindings.cc',
        'renderer/extensions/renderer_extension_bindings.h',
        'renderer/extensions/user_script_idle_scheduler.cc',
        'renderer/extensions/user_script_idle_scheduler.h',
        'renderer/extensions/user_script_slave.cc',
        'renderer/extensions/user_script_slave.h',
        'renderer/frame_sniffer.cc',
        'renderer/frame_sniffer.h',
        'renderer/loadtimes_extension_bindings.h',
        'renderer/loadtimes_extension_bindings.cc',
        'renderer/net/predictor_queue.cc',
        'renderer/net/predictor_queue.h',
        'renderer/net/renderer_net_predictor.cc',
        'renderer/net/renderer_net_predictor.h',
        'renderer/resources/event_bindings.js',
        'renderer/resources/extension_apitest.js',
        'renderer/resources/extension_process_bindings.js',
        'renderer/resources/greasemonkey_api.js',
        'renderer/resources/json_schema.js',
        'renderer/resources/renderer_extension_bindings.js',
        'renderer/about_handler.cc',
        'renderer/about_handler.h',
        'renderer/blocked_plugin.cc',
        'renderer/blocked_plugin.h',
        'renderer/chrome_content_renderer_client.cc',
        'renderer/chrome_content_renderer_client.h',
        'renderer/chrome_ppapi_interfaces.cc',
        'renderer/chrome_ppapi_interfaces.h',
        'renderer/chrome_ppb_pdf_impl.cc',
        'renderer/chrome_ppb_pdf_impl.h',
        'renderer/chrome_render_process_observer.cc',
        'renderer/chrome_render_process_observer.h',
        'renderer/chrome_render_view_observer.cc',
        'renderer/chrome_render_view_observer.h',
        'renderer/content_settings_observer.cc',
        'renderer/content_settings_observer.h',
        'renderer/external_host_bindings.cc',
        'renderer/external_host_bindings.h',
        'renderer/external_extension.cc',
        'renderer/external_extension.h',
        'renderer/localized_error.cc',
        'renderer/localized_error.h',
        'renderer/page_click_listener.h',
        'renderer/page_click_tracker.cc',
        'renderer/page_click_tracker.h',
        'renderer/page_load_histograms.cc',
        'renderer/page_load_histograms.h',
        'renderer/plugin_uma.cc',
        'renderer/plugin_uma.h',
        'renderer/prerender/prerender_helper.cc',
        'renderer/prerender/prerender_helper.h',
        'renderer/print_web_view_helper.cc',
        'renderer/print_web_view_helper.h',
        'renderer/print_web_view_helper_linux.cc',
        'renderer/print_web_view_helper_mac.mm',
        'renderer/print_web_view_helper_win.cc',
        'renderer/renderer_histogram_snapshots.cc',
        'renderer/renderer_histogram_snapshots.h',
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
        'renderer/search_extension.cc',
        'renderer/search_extension.h',
        'renderer/searchbox.cc',
        'renderer/searchbox.h',
        'renderer/searchbox_extension.cc',
        'renderer/searchbox_extension.h',
        'renderer/security_filter_peer.cc',
        'renderer/security_filter_peer.h',
        'renderer/spellchecker/spellcheck_provider.cc',
        'renderer/spellchecker/spellcheck_provider.h',
        'renderer/spellchecker/spellcheck.cc',
        'renderer/spellchecker/spellcheck.h',
        'renderer/spellchecker/spellcheck_worditerator.cc',
        'renderer/spellchecker/spellcheck_worditerator.h',
        'renderer/translate_helper.cc',
        'renderer/translate_helper.h',
        'renderer/visitedlink_slave.cc',
        'renderer/visitedlink_slave.h',
      ],
      'conditions': [
        ['disable_nacl!=1', {
          'dependencies': [
            'nacl',
          ],
          'sources': [
            'renderer/nacl_desc_wrapper_chrome.cc',
          ],
        }],
        ['safe_browsing==1', {
          'defines': [
            'ENABLE_SAFE_BROWSING',
          ],
        }, {  # safe_browsing==0
          'sources/': [
            ['exclude', '^renderer/safe_browsing/'],
          ],
        }],
        ['OS=="mac"', {
          'dependencies': [
            '../third_party/mach_override/mach_override.gyp:mach_override',
          ],
        }],
        ['toolkit_uses_gtk == 1', {
          'conditions': [
            [ 'linux_use_tcmalloc==1', {
                'dependencies': [
                  '../base/allocator/allocator.gyp:allocator',
                ],
              },
            ],
          ],
          'dependencies': [
            '../build/linux/system.gyp:gtk',
            '../sandbox/sandbox.gyp:sandbox',
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
