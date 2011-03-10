# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'content_common',
      'type': '<(library)',
      'dependencies': [
        '../ipc/ipc.gyp:ipc',
        '../webkit/support/webkit_support.gyp:fileapi',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'common/child_process.cc',
        'common/child_process.h',
        'common/child_process_host.cc',
        'common/child_process_host.h',
        'common/child_process_info.cc',
        'common/child_process_info.h',
        'common/child_process_messages.h',
        'common/child_thread.cc',
        'common/child_thread.h',
        'common/common_param_traits.cc',
        'common/common_param_traits.h',
        'common/content_message_generator.cc',
        'common/content_message_generator.h',
        'common/content_client.cc',
        'common/content_client.h',
        'common/content_constants.cc',
        'common/content_constants.h',
        'common/content_paths.cc',
        'common/content_paths.h',
        'common/content_switches.cc',
        'common/content_switches.h',
        'common/dx_diag_node.cc',
        'common/dx_diag_node.h',
        'common/file_system/file_system_dispatcher.cc',
        'common/file_system/file_system_dispatcher.h',
        'common/file_system/webfilesystem_callback_dispatcher.cc',
        'common/file_system/webfilesystem_callback_dispatcher.h',
        'common/file_system/webfilesystem_impl.cc',
        'common/file_system/webfilesystem_impl.h',
        'common/file_system/webfilewriter_impl.cc',
        'common/file_system/webfilewriter_impl.h',
        'common/file_system_messages.h',
        'common/gpu_info.cc',
        'common/gpu_info.h',
        'common/gpu_messages.h',
        'common/message_router.cc',
        'common/message_router.h',
        'common/notification_details.cc',
        'common/notification_details.h',
        'common/notification_observer.h',
        'common/notification_registrar.cc',
        'common/notification_registrar.h',
        'common/notification_service.cc',
        'common/notification_service.h',
        'common/notification_source.cc',
        'common/notification_source.h',
        'common/notification_type.h',
        'common/p2p_messages.h',
        'common/p2p_sockets.cc',
        'common/p2p_sockets.h',
        'common/property_bag.cc',
        'common/property_bag.h',
        'common/resource_dispatcher.cc',
        'common/resource_dispatcher.h',
        'common/resource_messages.h',
        'common/resource_response.cc',
        'common/resource_response.h',
        'common/socket_stream.h',
        'common/socket_stream_dispatcher.cc',
        'common/socket_stream_dispatcher.h',
        'common/socket_stream_messages.h',
      ],
      'conditions': [
        ['OS=="win"', {
          'msvs_guid': '062E9260-304A-4657-A74C-0D3AA1A0A0A4',
        }],
      ],
    },
  ],
}
