# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'test_support_content',
      'type': 'static_library',
      'defines!': ['CONTENT_IMPLEMENTATION'],
      'dependencies': [
        '../build/temp_gyp/googleurl.gyp:googleurl',
        'content_common',
        '../skia/skia.gyp:skia',
        '../testing/gmock.gyp:gmock',
        '../testing/gtest.gyp:gtest',
        '../third_party/WebKit/Source/WebKit/chromium/WebKit.gyp:webkit',
        '../ui/gfx/surface/surface.gyp:surface',
        '../ui/ui.gyp:ui_test_support',
        '../webkit/support/webkit_support.gyp:appcache',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        # TODO(phajdan.jr): All of those files should live in content/test.
        'browser/download/mock_download_manager.h',
        'browser/download/mock_download_manager_delegate.cc',
        'browser/download/mock_download_manager_delegate.h',
        'browser/geolocation/arbitrator_dependency_factories_for_test.cc',
        'browser/geolocation/arbitrator_dependency_factories_for_test.h',
        'browser/geolocation/fake_access_token_store.cc',
        'browser/geolocation/fake_access_token_store.h',
        'browser/geolocation/mock_location_provider.cc',
        'browser/geolocation/mock_location_provider.h',
        'browser/mock_content_browser_client.cc',
        'browser/mock_content_browser_client.h',
        'browser/mock_resource_context.cc',
        'browser/mock_resource_context.h',
        'browser/renderer_host/dummy_resource_handler.cc',
        'browser/renderer_host/dummy_resource_handler.h',
        'browser/renderer_host/media/mock_media_observer.cc',
        'browser/renderer_host/media/mock_media_observer.h',
        'browser/renderer_host/test_backing_store.cc',
        'browser/renderer_host/test_backing_store.h',
        'browser/renderer_host/test_render_view_host.cc',
        'browser/renderer_host/test_render_view_host.h',
        'browser/renderer_host/mock_render_process_host.cc',
        'browser/renderer_host/mock_render_process_host.h',
        'browser/tab_contents/test_tab_contents.cc',
        'browser/tab_contents/test_tab_contents.h',
        'common/test_url_constants.cc',
        'common/test_url_constants.h',
        'gpu/gpu_idirect3d9_mock_win.cc',
        'gpu/gpu_idirect3d9_mock_win.h',
        'renderer/mock_content_renderer_client.cc',
        'renderer/mock_content_renderer_client.h',
        'renderer/mock_render_process.cc',
        'renderer/mock_render_process.h',
        'test/browser_test.h',
        'test/browser_test_base.cc',
        'test/browser_test_base.h',
        'test/content_test_suite.cc',
        'test/content_test_suite.h',
        'test/render_view_fake_resources_test.cc',
        'test/render_view_fake_resources_test.h',
        'test/test_browser_context.cc',
        'test/test_browser_context.h',
        'test/test_content_client.cc',
        'test/test_content_client.h',
        'test/test_notification_tracker.cc',
        'test/test_notification_tracker.h',
        'test/test_tab_contents_view.cc',
        'test/test_tab_contents_view.h',
        'test/test_url_fetcher_factory.cc',
        'test/test_url_fetcher_factory.h',

        # TODO(phajdan.jr): Those files should be moved to webkit
        # test support target.
        '../webkit/appcache/appcache_test_helper.cc',
        '../webkit/appcache/appcache_test_helper.h',
        '../webkit/quota/mock_special_storage_policy.cc',
        '../webkit/quota/mock_special_storage_policy.h',
      ],
    },
    {
      'target_name': 'content_unittests',
      'type': 'executable',
      'defines!': ['CONTENT_IMPLEMENTATION'],
      'dependencies': [
        'content_browser',
        'content_gpu',
        'content_plugin',
        'content_renderer',
        'test_support_content',
        '../base/base.gyp:test_support_base',
        '../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '../crypto/crypto.gyp:crypto',
        '../gpu/gpu.gyp:gpu_unittest_utils',
        '../ipc/ipc.gyp:test_support_ipc',
        '../media/media.gyp:media_test_support',
        '../net/net.gyp:net_test_support',
        '../skia/skia.gyp:skia',
        '../testing/gmock.gyp:gmock',
        '../testing/gtest.gyp:gtest',
        '../third_party/libjingle/libjingle.gyp:libjingle',
        '../third_party/WebKit/Source/WebKit/chromium/WebKit.gyp:webkit',
        '../ui/gfx/gl/gl.gyp:gl',
        '../ui/ui.gyp:ui',
        '../webkit/support/webkit_support.gyp:glue',
        '../webkit/support/webkit_support.gyp:quota',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'browser/appcache/chrome_appcache_service_unittest.cc',
        'browser/browser_thread_unittest.cc',
        'browser/browser_url_handler_unittest.cc',
        'browser/child_process_security_policy_unittest.cc',
        'browser/debugger/devtools_manager_unittest.cc',
        'browser/device_orientation/provider_unittest.cc',
        'browser/download/base_file_unittest.cc',
        'browser/download/download_file_unittest.cc',
        'browser/download/download_id_unittest.cc',
        'browser/download/download_status_updater_unittest.cc',
        'browser/download/save_package_unittest.cc',
        'browser/geolocation/device_data_provider_unittest.cc',
        'browser/geolocation/geolocation_provider_unittest.cc',
        'browser/geolocation/gps_location_provider_unittest_linux.cc',
        'browser/geolocation/location_arbitrator_unittest.cc',
        'browser/geolocation/network_location_provider_unittest.cc',
        'browser/geolocation/wifi_data_provider_common_unittest.cc',
        'browser/geolocation/wifi_data_provider_linux_unittest.cc',
        'browser/geolocation/wifi_data_provider_unittest_win.cc',
        'browser/geolocation/win7_location_api_unittest_win.cc',
        'browser/geolocation/win7_location_provider_unittest_win.cc',
        'browser/gpu/gpu_blacklist_unittest.cc',
        'browser/in_process_webkit/webkit_context_unittest.cc',
        'browser/in_process_webkit/webkit_thread_unittest.cc',
        'browser/mac/closure_blocks_leopard_compat_unittest.cc',
        'browser/mach_broker_mac_unittest.cc',
        'browser/renderer_host/accelerated_plugin_view_mac_unittest.mm',
        'browser/renderer_host/gtk_key_bindings_handler_unittest.cc',
        'browser/renderer_host/media/audio_input_device_manager_unittest.cc',
        'browser/renderer_host/media/audio_renderer_host_unittest.cc',
        'browser/renderer_host/media/media_stream_dispatcher_host_unittest.cc',
        'browser/renderer_host/media/video_capture_host_unittest.cc',
        'browser/renderer_host/media/video_capture_manager_unittest.cc',
        'browser/renderer_host/render_view_host_unittest.cc',
        'browser/renderer_host/render_widget_host_view_mac_editcommand_helper_unittest.mm',
        'browser/renderer_host/render_widget_host_view_mac_unittest.mm',
        'browser/renderer_host/resource_dispatcher_host_unittest.cc',
        'browser/renderer_host/resource_queue_unittest.cc',
        'browser/resolve_proxy_msg_helper_unittest.cc',
        'browser/site_instance_unittest.cc',
        'browser/speech/endpointer/endpointer_unittest.cc',
        'browser/speech/speech_recognition_request_unittest.cc',
        'browser/speech/speech_recognizer_unittest.cc',
        'browser/ssl/ssl_host_state_unittest.cc',
        'browser/tab_contents/navigation_entry_unittest.cc',
        'browser/tab_contents/tab_contents_delegate_unittest.cc',
        'browser/trace_subscriber_stdio_unittest.cc',
        'common/mac/attributed_string_coder_unittest.mm',
        'common/mac/font_descriptor_unittest.mm',
        'common/gpu/gpu_feature_flags_unittest.cc',
        'common/gpu/gpu_info_unittest.cc',
        'common/hi_res_timer_manager_unittest.cc',
        'common/net/url_fetcher_unittest.cc',
        'common/notification_service_unittest.cc',
        'common/process_watcher_unittest.cc',
        'common/property_bag_unittest.cc',
        'common/resource_dispatcher_unittest.cc',
        'gpu/gpu_info_collector_unittest.cc',
        'gpu/gpu_info_collector_unittest_win.cc',
        'renderer/active_notification_tracker_unittest.cc',
        'renderer/gpu/input_event_filter_unittest.cc',
        'renderer/media/audio_message_filter_unittest.cc',
        'renderer/media/audio_renderer_impl_unittest.cc',
        'renderer/media/capture_video_decoder_unittest.cc',
        'renderer/media/media_stream_dispatcher_unittest.cc',
        'renderer/media/rtc_video_decoder_unittest.cc',
        'renderer/media/video_capture_impl_unittest.cc',
        'renderer/media/video_capture_message_filter_unittest.cc',
        'renderer/paint_aggregator_unittest.cc',
        'renderer/v8_value_converter_impl_unittest.cc',
        'test/run_all_unittests.cc',
      ],
      'conditions': [
        ['use_x11 == 1', {
          'dependencies': [
            '../build/linux/system.gyp:dbus',
            '../dbus/dbus.gyp:dbus_test_support',
          ],
        }],
        ['os_posix!=1', {
          'sources!': [
            # TODO(port): port those unit tests.
            'common/process_watcher_unittest.cc',
          ],
        }],
        ['OS=="win" and win_use_allocator_shim==1', {
          'dependencies': [
            '../base/allocator/allocator.gyp:allocator',
          ],
        }],
        ['chromeos==1', {
          'sources/': [
            ['exclude', '^browser/renderer_host/gtk_key_bindings_handler_unittest.cc'],
          ],
        }],
        ['OS=="mac"', {
          'dependencies': [
            'closure_blocks_leopard_compat',
          ],
        }],
      ],
    },
    {
      'target_name': 'content_browsertests',
      'type': 'executable',
      'defines!': ['CONTENT_IMPLEMENTATION'],
      'dependencies': [
        'content_browser',
        'content_gpu',
        'content_plugin',
        'content_renderer',
        'content_shell_lib',
        'test_support_content',
        '../base/base.gyp:test_support_base',
        '../net/net.gyp:net_test_support',
        '../skia/skia.gyp:skia',
        '../testing/gtest.gyp:gtest',
        '../ui/ui.gyp:ui',
        '../webkit/support/webkit_support.gyp:glue',
      ],
      'include_dirs': [
        '..',
      ],
      'defines': [
        'HAS_OUT_OF_PROC_TEST_RUNNER',
      ],
      'sources': [
        'test/content_browser_test.h',
        'test/content_browser_test.cc',
        'test/content_test_launcher.cc',
        'test/test_launcher.cc',
        'test/test_launcher.h',
      ],
      'conditions': [
        ['OS=="win"', {
          'link_settings': {
            'libraries': [
              '-lcomctl32.lib',
            ],
          },
          'resource_include_dirs': [
            '<(SHARED_INTERMEDIATE_DIR)/webkit',
          ],
          'sources': [
            'shell/resource.h',
            'shell/shell.rc',
            # TODO:  It would be nice to have these pulled in
            # automatically from direct_dependent_settings in
            # their various targets (net.gyp:net_resources, etc.),
            # but that causes errors in other targets when
            # resulting .res files get referenced multiple times.
            '<(SHARED_INTERMEDIATE_DIR)/net/net_resources.rc',
            '<(SHARED_INTERMEDIATE_DIR)/webkit/webkit_chromium_resources.rc',
            '<(SHARED_INTERMEDIATE_DIR)/webkit/webkit_resources.rc',
            '<(SHARED_INTERMEDIATE_DIR)/webkit/webkit_strings_en-US.rc',
          ],
          'dependencies': [
            '<(DEPTH)/net/net.gyp:net_resources',
            '<(DEPTH)/webkit/support/webkit_support.gyp:webkit_resources',
            '<(DEPTH)/webkit/support/webkit_support.gyp:webkit_strings',
            '../sandbox/sandbox.gyp:sandbox',
          ],
          'configurations': {
            'Debug_Base': {
              'msvs_settings': {
                'VCLinkerTool': {
                  'LinkIncremental': '<(msvs_large_module_debug_link_mode)',
                },
              },
            },
          },
        }],
        ['OS=="win" and win_use_allocator_shim==1', {
          'dependencies': [
            '../base/allocator/allocator.gyp:allocator',
          ],
        }],
      ],
    },
  ],
  'conditions': [
    ['target_arch=="arm"', {
      'targets': [
        {
          'target_name': 'omx_video_decode_accelerator_unittest',
          'defines!': ['CONTENT_IMPLEMENTATION'],
          'type': 'executable',
          'dependencies': [
            '../base/base.gyp:base',
            'content',
            '../testing/gtest.gyp:gtest',
          ],
          'include_dirs': [
            '<(DEPTH)/third_party/angle/include',
            '<(DEPTH)/third_party/openmax/il',
          ],
          'sources': [
            'common/gpu/media/omx_video_decode_accelerator_unittest.cc',
          ],
        }
      ],
    },
   ],
  ],
}
