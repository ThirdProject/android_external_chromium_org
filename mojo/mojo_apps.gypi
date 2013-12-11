{
  'targets': [
    {
      'target_name': 'mojo_js_lib',
      'type': 'static_library',
      'dependencies': [
        '../base/base.gyp:base',
        '../gin/gin.gyp:gin',
        'mojo_common_lib',
        'mojo_gles2',
        'mojo_gles2_bindings',
        'mojo_native_viewport_bindings',
        'mojo_system',
      ],
      'export_dependent_settings': [
        '../base/base.gyp:base',
        '../gin/gin.gyp:gin',
        'mojo_common_lib',
        'mojo_gles2',
        'mojo_gles2_bindings',
        'mojo_native_viewport_bindings',
        'mojo_system',
      ],
      'sources': [
        'apps/js/mojo_runner_delegate.cc',
        'apps/js/mojo_runner_delegate.h',
        'apps/js/bindings/threading.cc',
        'apps/js/bindings/threading.h',
        'apps/js/bindings/core.cc',
        'apps/js/bindings/core.h',
        'apps/js/bindings/handle.cc',
        'apps/js/bindings/handle.h',
        'apps/js/bindings/support.cc',
        'apps/js/bindings/support.h',
        'apps/js/bindings/waiting_callback.cc',
        'apps/js/bindings/waiting_callback.h',
      ],
    },
    {
      'target_name': 'mojo_js_unittests',
      'type': 'executable',
      'dependencies': [
        '../gin/gin.gyp:gin_test',
        'mojo_js_lib',
        'mojo_run_all_unittests',
        'mojo_sample_service',
      ],
      'sources': [
        'apps/js/test/run_js_tests.cc',
      ],
    },
    {
      'target_name': 'mojo_js',
      'type': 'shared_library',
      'dependencies': [
        'mojo_js_lib',
      ],
      'sources': [
        'apps/js/main.cc',
      ],
    },
  ],
}
