# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'webkit_storage_browser',
      'type': '<(component)',
      'variables': { 'enable_wexit_time_destructors': 1, },
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/base.gyp:base_i18n',
        '<(DEPTH)/base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '<(DEPTH)/net/net.gyp:net',
        '<(DEPTH)/sql/sql.gyp:sql',
        '<(DEPTH)/third_party/leveldatabase/leveldatabase.gyp:leveldatabase',
        '<(DEPTH)/third_party/sqlite/sqlite.gyp:sqlite',
        '<(DEPTH)/url/url.gyp:url_lib',
        '<(DEPTH)/webkit/common/webkit_common.gyp:webkit_common',
        '<(DEPTH)/webkit/storage_common.gyp:webkit_storage_common',
      ],
      'defines': ['WEBKIT_STORAGE_BROWSER_IMPLEMENTATION'],
      'sources': [
        'browser/webkit_storage_browser_export.h',
        'browser/appcache/appcache.cc',
        'browser/appcache/appcache.h',
        'browser/appcache/appcache_backend_impl.cc',
        'browser/appcache/appcache_backend_impl.h',
        'browser/appcache/appcache_database.cc',
        'browser/appcache/appcache_database.h',
        'browser/appcache/appcache_disk_cache.cc',
        'browser/appcache/appcache_disk_cache.h',
        'browser/appcache/appcache_entry.h',
        'browser/appcache/appcache_executable_handler.h',
        'browser/appcache/appcache_group.cc',
        'browser/appcache/appcache_group.h',
        'browser/appcache/appcache_histograms.cc',
        'browser/appcache/appcache_histograms.h',
        'browser/appcache/appcache_host.cc',
        'browser/appcache/appcache_host.h',
        'browser/appcache/appcache_policy.h',
        'browser/appcache/appcache_quota_client.cc',
        'browser/appcache/appcache_quota_client.h',
        'browser/appcache/appcache_request_handler.cc',
        'browser/appcache/appcache_request_handler.h',
        'browser/appcache/appcache_response.cc',
        'browser/appcache/appcache_response.h',
        'browser/appcache/appcache_service.cc',
        'browser/appcache/appcache_service.h',
        'browser/appcache/appcache_storage.cc',
        'browser/appcache/appcache_storage.h',
        'browser/appcache/appcache_storage_impl.cc',
        'browser/appcache/appcache_storage_impl.h',
        'browser/appcache/appcache_working_set.cc',
        'browser/appcache/appcache_working_set.h',
        'browser/appcache/appcache_update_job.cc',
        'browser/appcache/appcache_update_job.h',
        'browser/appcache/appcache_url_request_job.cc',
        'browser/appcache/appcache_url_request_job.h',
        'browser/appcache/manifest_parser.cc',
        'browser/appcache/manifest_parser.h',
        'browser/blob/blob_data_handle.cc',
        'browser/blob/blob_data_handle.h',
        'browser/blob/blob_storage_context.cc',
        'browser/blob/blob_storage_context.h',
        'browser/blob/blob_url_request_job.cc',
        'browser/blob/blob_url_request_job.h',
        'browser/blob/blob_url_request_job_factory.cc',
        'browser/blob/blob_url_request_job_factory.h',
        'browser/blob/file_stream_reader.cc',
        'browser/blob/file_stream_reader.h',
        'browser/blob/local_file_stream_reader.cc',
        'browser/blob/local_file_stream_reader.h',
        'browser/blob/view_blob_internals_job.cc',
        'browser/blob/view_blob_internals_job.h',
        'browser/database/database_quota_client.cc',
        'browser/database/database_quota_client.h',
        'browser/database/database_tracker.cc',
        'browser/database/database_tracker.h',
        'browser/database/database_util.cc',
        'browser/database/database_util.h',
        'browser/database/databases_table.cc',
        'browser/database/databases_table.h',
        'browser/database/vfs_backend.cc',
        'browser/database/vfs_backend.h',
        'browser/fileapi/async_file_util.h',
        'browser/fileapi/async_file_util_adapter.cc',
        'browser/fileapi/async_file_util_adapter.h',
        'browser/fileapi/copy_or_move_file_validator.h',
        'browser/fileapi/copy_or_move_operation_delegate.cc',
        'browser/fileapi/copy_or_move_operation_delegate.h',
        'browser/fileapi/dragged_file_util.cc',
        'browser/fileapi/dragged_file_util.h',
        'browser/fileapi/external_mount_points.cc',
        'browser/fileapi/external_mount_points.h',
        'browser/fileapi/file_observers.h',
        'browser/fileapi/file_permission_policy.h',
        'browser/fileapi/file_stream_writer.h',
        'browser/fileapi/file_system_backend.h',
        'browser/fileapi/file_system_context.cc',
        'browser/fileapi/file_system_context.h',
        'browser/fileapi/file_system_dir_url_request_job.cc',
        'browser/fileapi/file_system_dir_url_request_job.h',
        'browser/fileapi/file_system_file_stream_reader.cc',
        'browser/fileapi/file_system_file_stream_reader.h',
        'browser/fileapi/file_system_file_util.cc',
        'browser/fileapi/file_system_file_util.h',
        'browser/fileapi/file_system_operation.h',
        'browser/fileapi/file_system_operation_context.cc',
        'browser/fileapi/file_system_operation_context.h',
        'browser/fileapi/file_system_operation_impl.cc',
        'browser/fileapi/file_system_operation_impl.h',
        'browser/fileapi/file_system_operation_runner.cc',
        'browser/fileapi/file_system_operation_runner.h',
        'browser/fileapi/file_system_options.cc',
        'browser/fileapi/file_system_options.h',
        'browser/fileapi/file_system_quota_client.cc',
        'browser/fileapi/file_system_quota_client.h',
        'browser/fileapi/file_system_quota_util.h',
        'browser/fileapi/file_system_url.cc',
        'browser/fileapi/file_system_url.h',
        'browser/fileapi/file_system_url_request_job.cc',
        'browser/fileapi/file_system_url_request_job.h',
        'browser/fileapi/file_system_url_request_job_factory.cc',
        'browser/fileapi/file_system_url_request_job_factory.h',
        'browser/fileapi/file_system_usage_cache.cc',
        'browser/fileapi/file_system_usage_cache.h',
        'browser/fileapi/file_writer_delegate.cc',
        'browser/fileapi/file_writer_delegate.h',
        'browser/fileapi/isolated_context.cc',
        'browser/fileapi/isolated_context.h',
        'browser/fileapi/isolated_file_system_backend.cc',
        'browser/fileapi/isolated_file_system_backend.h',
        'browser/fileapi/local_file_stream_writer.cc',
        'browser/fileapi/local_file_stream_writer.h',
        'browser/fileapi/local_file_util.cc',
        'browser/fileapi/local_file_util.h',
        'browser/fileapi/mount_points.cc',
        'browser/fileapi/mount_points.h',
        'browser/fileapi/native_file_util.cc',
        'browser/fileapi/native_file_util.h',
        'browser/fileapi/obfuscated_file_util.cc',
        'browser/fileapi/obfuscated_file_util.h',
        'browser/fileapi/open_file_system_mode.h',
        'browser/fileapi/plugin_private_file_system_backend.cc',
        'browser/fileapi/plugin_private_file_system_backend.h',
        'browser/fileapi/quota/open_file_handle.cc',
        'browser/fileapi/quota/open_file_handle.h',
        'browser/fileapi/quota/open_file_handle_context.cc',
        'browser/fileapi/quota/open_file_handle_context.h',
        'browser/fileapi/quota/quota_backend_impl.cc',
        'browser/fileapi/quota/quota_backend_impl.h',
        'browser/fileapi/quota/quota_reservation.cc',
        'browser/fileapi/quota/quota_reservation.h',
        'browser/fileapi/quota/quota_reservation_buffer.cc',
        'browser/fileapi/quota/quota_reservation_buffer.h',
        'browser/fileapi/quota/quota_reservation_manager.cc',
        'browser/fileapi/quota/quota_reservation_manager.h',
        'browser/fileapi/recursive_operation_delegate.cc',
        'browser/fileapi/recursive_operation_delegate.h',
        'browser/fileapi/remove_operation_delegate.cc',
        'browser/fileapi/remove_operation_delegate.h',
        'browser/fileapi/sandbox_directory_database.cc',
        'browser/fileapi/sandbox_directory_database.h',
        'browser/fileapi/sandbox_file_stream_writer.cc',
        'browser/fileapi/sandbox_file_stream_writer.h',
        'browser/fileapi/sandbox_file_system_backend.cc',
        'browser/fileapi/sandbox_file_system_backend.h',
        'browser/fileapi/sandbox_file_system_backend_delegate.cc',
        'browser/fileapi/sandbox_file_system_backend_delegate.h',
        'browser/fileapi/sandbox_isolated_origin_database.cc',
        'browser/fileapi/sandbox_isolated_origin_database.h',
        'browser/fileapi/sandbox_origin_database.cc',
        'browser/fileapi/sandbox_origin_database.h',
        'browser/fileapi/sandbox_origin_database_interface.cc',
        'browser/fileapi/sandbox_origin_database_interface.h',
        'browser/fileapi/sandbox_prioritized_origin_database.cc',
        'browser/fileapi/sandbox_prioritized_origin_database.h',
        'browser/fileapi/sandbox_quota_observer.cc',
        'browser/fileapi/sandbox_quota_observer.h',
        'browser/fileapi/task_runner_bound_observer_list.h',
        'browser/fileapi/timed_task_helper.cc',
        'browser/fileapi/timed_task_helper.h',
        'browser/fileapi/transient_file_util.cc',
        'browser/fileapi/transient_file_util.h',
        'browser/quota/quota_callbacks.h',
        'browser/quota/quota_client.h',
        'browser/quota/quota_database.cc',
        'browser/quota/quota_database.h',
        'browser/quota/quota_manager.cc',
        'browser/quota/quota_manager.h',
        'browser/quota/quota_manager_proxy.cc',
        'browser/quota/quota_manager_proxy.h',
        'browser/quota/quota_task.cc',
        'browser/quota/quota_task.h',
        'browser/quota/quota_temporary_storage_evictor.cc',
        'browser/quota/quota_temporary_storage_evictor.h',
        'browser/quota/special_storage_policy.cc',
        'browser/quota/special_storage_policy.h',
        'browser/quota/storage_monitor.cc',
        'browser/quota/storage_monitor.h',
        'browser/quota/storage_observer.cc',
        'browser/quota/storage_observer.h',
        'browser/quota/usage_tracker.cc',
        'browser/quota/usage_tracker.h',
      ],
      # TODO(jschuh): crbug.com/167187 fix size_t to int truncations.
      'msvs_disabled_warnings': [ 4267, ],

      'conditions': [
        ['chrome_multiple_dll!=1', {
          'dependencies': [
            '<(DEPTH)/third_party/WebKit/public/blink.gyp:blink',
          ],
        }],
        ['OS=="android"', {
          'defines': ['APPCACHE_USE_SIMPLE_CACHE'],
          'direct_dependent_settings': {
            'defines': ['APPCACHE_USE_SIMPLE_CACHE'],
          },
        }],
      ],
    },
    {
      'target_name': 'dump_file_system',
      'type': 'executable',
      'sources': [
        'browser/fileapi/dump_file_system.cc',
      ],
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/webkit/storage_common.gyp:webkit_storage_common',
        'webkit_storage_browser',
      ],
    },
  ],
}
