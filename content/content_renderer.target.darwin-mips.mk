# This file is generated by gyp; do not edit.

include $(CLEAR_VARS)

LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := content_content_renderer_gyp
LOCAL_MODULE_SUFFIX := .a
LOCAL_MODULE_TAGS := optional
gyp_intermediate_dir := $(call local-intermediates-dir)
gyp_shared_intermediate_dir := $(call intermediates-dir-for,GYP,shared)

# Make sure our deps are built first.
GYP_TARGET_DEPENDENCIES := \
	$(call intermediates-dir-for,GYP,content_content_resources_gyp)/content_resources.stamp \
	$(call intermediates-dir-for,GYP,jingle_jingle_glue_gyp)/jingle_glue.stamp \
	$(call intermediates-dir-for,GYP,skia_skia_gyp)/skia.stamp \
	$(call intermediates-dir-for,STATIC_LIBRARIES,skia_skia_library_gyp)/skia_skia_library_gyp.a \
	$(call intermediates-dir-for,GYP,third_party_WebKit_public_blink_gyp)/blink.stamp \
	$(call intermediates-dir-for,GYP,third_party_icu_icui18n_gyp)/icui18n.stamp \
	$(call intermediates-dir-for,GYP,third_party_icu_icuuc_gyp)/icuuc.stamp \
	$(call intermediates-dir-for,GYP,third_party_npapi_npapi_gyp)/npapi.stamp \
	$(call intermediates-dir-for,GYP,third_party_widevine_cdm_widevine_cdm_version_h_gyp)/widevine_cdm_version_h.stamp \
	$(call intermediates-dir-for,GYP,v8_tools_gyp_v8_gyp)/v8.stamp \
	$(call intermediates-dir-for,STATIC_LIBRARIES,webkit_child_webkit_child_gyp)/webkit_child_webkit_child_gyp.a \
	$(call intermediates-dir-for,STATIC_LIBRARIES,webkit_glue_glue_gyp)/webkit_glue_glue_gyp.a \
	$(call intermediates-dir-for,STATIC_LIBRARIES,third_party_libphonenumber_libphonenumber_without_metadata_gyp)/third_party_libphonenumber_libphonenumber_without_metadata_gyp.a

GYP_GENERATED_OUTPUTS :=

# Make sure our deps and generated files are built first.
LOCAL_ADDITIONAL_DEPENDENCIES := $(GYP_TARGET_DEPENDENCIES) $(GYP_GENERATED_OUTPUTS)

LOCAL_CPP_EXTENSION := .cc
LOCAL_GENERATED_SOURCES :=

GYP_COPIED_SOURCE_ORIGIN_DIRS :=

LOCAL_SRC_FILES := \
	content/public/renderer/android_content_detection_prefixes.cc \
	content/public/renderer/content_renderer_client.cc \
	content/public/renderer/document_state.cc \
	content/public/renderer/history_item_serialization.cc \
	content/public/renderer/key_system_info.cc \
	content/public/renderer/navigation_state.cc \
	content/public/renderer/render_frame_observer.cc \
	content/public/renderer/render_process_observer.cc \
	content/public/renderer/render_thread.cc \
	content/public/renderer/render_view_observer.cc \
	content/renderer/accessibility/accessibility_node_serializer.cc \
	content/renderer/accessibility/renderer_accessibility.cc \
	content/renderer/accessibility/renderer_accessibility_complete.cc \
	content/renderer/accessibility/renderer_accessibility_focus_only.cc \
	content/renderer/android/address_detector.cc \
	content/renderer/android/content_detector.cc \
	content/renderer/android/email_detector.cc \
	content/renderer/android/phone_number_detector.cc \
	content/renderer/android/synchronous_compositor_factory.cc \
	content/renderer/browser_plugin/browser_plugin.cc \
	content/renderer/browser_plugin/browser_plugin_backing_store.cc \
	content/renderer/browser_plugin/browser_plugin_bindings.cc \
	content/renderer/browser_plugin/browser_plugin_compositing_helper.cc \
	content/renderer/browser_plugin/browser_plugin_manager_impl.cc \
	content/renderer/browser_plugin/browser_plugin_manager.cc \
	content/renderer/clipboard_utils.cc \
	content/renderer/context_menu_params_builder.cc \
	content/renderer/cursor_utils.cc \
	content/renderer/date_time_suggestion_builder.cc \
	content/renderer/device_orientation/device_motion_event_pump.cc \
	content/renderer/device_orientation/device_orientation_event_pump.cc \
	content/renderer/device_orientation/device_sensor_event_pump.cc \
	content/renderer/devtools/devtools_agent.cc \
	content/renderer/devtools/devtools_agent_filter.cc \
	content/renderer/devtools/devtools_client.cc \
	content/renderer/disambiguation_popup_helper.cc \
	content/renderer/dom_automation_controller.cc \
	content/renderer/dom_storage/dom_storage_cached_area.cc \
	content/renderer/dom_storage/dom_storage_dispatcher.cc \
	content/renderer/dom_storage/webstoragearea_impl.cc \
	content/renderer/dom_storage/webstoragenamespace_impl.cc \
	content/renderer/drop_data_builder.cc \
	content/renderer/external_popup_menu.cc \
	content/renderer/fetchers/alt_error_page_resource_fetcher.cc \
	content/renderer/fetchers/image_resource_fetcher.cc \
	content/renderer/fetchers/multi_resolution_image_resource_fetcher.cc \
	content/renderer/fetchers/resource_fetcher_impl.cc \
	content/renderer/gamepad_shared_memory_reader.cc \
	content/renderer/geolocation_dispatcher.cc \
	content/renderer/gpu/compositor_output_surface.cc \
	content/renderer/gpu/compositor_software_output_device.cc \
	content/renderer/gpu/delegated_compositor_output_surface.cc \
	content/renderer/gpu/gpu_benchmarking_extension.cc \
	content/renderer/gpu/mailbox_output_surface.cc \
	content/renderer/gpu/render_widget_compositor.cc \
	content/renderer/gpu/stream_texture_host_android.cc \
	content/renderer/idle_user_detector.cc \
	content/renderer/image_loading_helper.cc \
	content/renderer/ime_event_guard.cc \
	content/renderer/in_process_renderer_thread.cc \
	content/renderer/input/input_event_filter.cc \
	content/renderer/input/input_handler_manager.cc \
	content/renderer/input/input_handler_proxy.cc \
	content/renderer/input/input_handler_wrapper.cc \
	content/renderer/internal_document_state_data.cc \
	content/renderer/java/java_bridge_channel.cc \
	content/renderer/java/java_bridge_dispatcher.cc \
	content/renderer/load_progress_tracker.cc \
	content/renderer/media/active_loader.cc \
	content/renderer/media/android/audio_decoder_android.cc \
	content/renderer/media/android/media_info_loader.cc \
	content/renderer/media/android/media_source_delegate.cc \
	content/renderer/media/android/proxy_media_keys.cc \
	content/renderer/media/android/renderer_demuxer_android.cc \
	content/renderer/media/android/renderer_media_player_manager.cc \
	content/renderer/media/android/stream_texture_factory_android_impl.cc \
	content/renderer/media/android/stream_texture_factory_android_synchronous_impl.cc \
	content/renderer/media/android/webmediaplayer_android.cc \
	content/renderer/media/audio_decoder.cc \
	content/renderer/media/audio_device_factory.cc \
	content/renderer/media/audio_input_message_filter.cc \
	content/renderer/media/audio_message_filter.cc \
	content/renderer/media/audio_renderer_mixer_manager.cc \
	content/renderer/media/buffered_data_source.cc \
	content/renderer/media/buffered_resource_loader.cc \
	content/renderer/media/cache_util.cc \
	content/renderer/media/crypto/content_decryption_module_factory.cc \
	content/renderer/media/crypto/key_systems.cc \
	content/renderer/media/crypto/key_systems_support_uma.cc \
	content/renderer/media/crypto/proxy_decryptor.cc \
	content/renderer/media/midi_dispatcher.cc \
	content/renderer/media/midi_message_filter.cc \
	content/renderer/media/render_media_log.cc \
	content/renderer/media/renderer_gpu_video_accelerator_factories.cc \
	content/renderer/media/renderer_webaudiodevice_impl.cc \
	content/renderer/media/renderer_webmidiaccessor_impl.cc \
	content/renderer/media/texttrack_impl.cc \
	content/renderer/media/video_capture_impl.cc \
	content/renderer/media/video_capture_impl_manager.cc \
	content/renderer/media/video_capture_message_filter.cc \
	content/renderer/media/video_frame_provider.cc \
	content/renderer/media/webaudiosourceprovider_impl.cc \
	content/renderer/media/webcontentdecryptionmodule_impl.cc \
	content/renderer/media/webcontentdecryptionmodulesession_impl.cc \
	content/renderer/media/webinbandtexttrack_impl.cc \
	content/renderer/media/webmediaplayer_ms.cc \
	content/renderer/media/webmediaplayer_params.cc \
	content/renderer/media/webmediaplayer_util.cc \
	content/renderer/media/webmediasource_impl.cc \
	content/renderer/media/websourcebuffer_impl.cc \
	content/renderer/memory_benchmarking_extension.cc \
	content/renderer/menu_item_builder.cc \
	content/renderer/mhtml_generator.cc \
	content/renderer/mouse_lock_dispatcher.cc \
	content/renderer/paint_aggregator.cc \
	content/renderer/render_frame_impl.cc \
	content/renderer/render_process_impl.cc \
	content/renderer/render_thread_impl.cc \
	content/renderer/render_view_impl.cc \
	content/renderer/render_view_impl_android.cc \
	content/renderer/render_view_impl_params.cc \
	content/renderer/render_view_linux.cc \
	content/renderer/render_view_mouse_lock_dispatcher.cc \
	content/renderer/render_widget.cc \
	content/renderer/render_widget_fullscreen.cc \
	content/renderer/renderer_clipboard_client.cc \
	content/renderer/renderer_date_time_picker.cc \
	content/renderer/renderer_main.cc \
	content/renderer/renderer_main_platform_delegate_android.cc \
	content/renderer/renderer_webapplicationcachehost_impl.cc \
	content/renderer/renderer_webcolorchooser_impl.cc \
	content/renderer/renderer_webcookiejar_impl.cc \
	content/renderer/renderer_webkitplatformsupport_impl.cc \
	content/renderer/resizing_mode_selector.cc \
	content/renderer/sad_plugin.cc \
	content/renderer/savable_resources.cc \
	content/renderer/scoped_clipboard_writer_glue.cc \
	content/renderer/service_worker/embedded_worker_dispatcher.cc \
	content/renderer/service_worker/service_worker_context_client.cc \
	content/renderer/shared_memory_seqlock_reader.cc \
	content/renderer/shared_worker_repository.cc \
	content/renderer/skia_benchmarking_extension.cc \
	content/renderer/speech_recognition_dispatcher.cc \
	content/renderer/stats_collection_controller.cc \
	content/renderer/stats_collection_observer.cc \
	content/renderer/text_input_client_observer.cc \
	content/renderer/v8_value_converter_impl.cc \
	content/renderer/web_preferences.cc \
	content/renderer/web_ui_extension.cc \
	content/renderer/web_ui_extension_data.cc \
	content/renderer/webclipboard_impl.cc \
	content/renderer/webcrypto/webcrypto_impl.cc \
	content/renderer/webcrypto/webcrypto_impl_openssl.cc \
	content/renderer/webcrypto/webcrypto_util.cc \
	content/renderer/webpublicsuffixlist_impl.cc \
	content/renderer/websharedworker_proxy.cc \
	content/renderer/media/webrtc_logging_noop.cc


# Flags passed to both C and C++ files.
MY_CFLAGS_Debug := \
	-fstack-protector \
	--param=ssp-buffer-size=4 \
	 \
	-fno-exceptions \
	-fno-strict-aliasing \
	-Wall \
	-Wno-unused-parameter \
	-Wno-missing-field-initializers \
	-fvisibility=hidden \
	-pipe \
	-fPIC \
	-EL \
	-mhard-float \
	-ffunction-sections \
	-funwind-tables \
	-g \
	-fstack-protector \
	-fno-short-enums \
	-finline-limit=64 \
	-Wa,--noexecstack \
	-U_FORTIFY_SOURCE \
	-Wno-extra \
	-Wno-ignored-qualifiers \
	-Wno-type-limits \
	-Os \
	-g \
	-fomit-frame-pointer \
	-fdata-sections \
	-ffunction-sections

MY_DEFS_Debug := \
	'-DCONTENT_IMPLEMENTATION' \
	'-DV8_DEPRECATION_WARNINGS' \
	'-D_FILE_OFFSET_BITS=64' \
	'-DNO_TCMALLOC' \
	'-DDISABLE_NACL' \
	'-DCHROMIUM_BUILD' \
	'-DUSE_LIBJPEG_TURBO=1' \
	'-DUSE_PROPRIETARY_CODECS' \
	'-DENABLE_CONFIGURATION_POLICY' \
	'-DDISCARDABLE_MEMORY_ALWAYS_SUPPORTED_NATIVELY' \
	'-DSYSTEM_NATIVELY_SIGNALS_MEMORY_PRESSURE' \
	'-DICU_UTIL_DATA_IMPL=ICU_UTIL_DATA_STATIC' \
	'-DUSE_OPENSSL=1' \
	'-DENABLE_EGLIMAGE=1' \
	'-DCLD_VERSION=1' \
	'-DENABLE_PRINTING=1' \
	'-DENABLE_MANAGED_USERS=1' \
	'-DMEDIA_DISABLE_LIBVPX' \
	'-DPOSIX_AVOID_MMAP' \
	'-DSK_ENABLE_INST_COUNT=0' \
	'-DSK_SUPPORT_GPU=1' \
	'-DGR_GL_CUSTOM_SETUP_HEADER="GrGLConfig_chrome.h"' \
	'-DSK_ENABLE_LEGACY_API_ALIASING=1' \
	'-DSK_ATTR_DEPRECATED=SK_NOTHING_ARG1' \
	'-DSK_SUPPORT_LEGACY_COLORTYPE=1' \
	'-DGR_GL_IGNORE_ES3_MSAA=0' \
	'-DSK_SUPPORT_LEGACY_PIXELREF_CONSTRUCTOR=1' \
	'-DSK_BUILD_FOR_ANDROID' \
	'-DSK_USE_POSIX_THREADS' \
	'-DSK_DEFERRED_CANVAS_USES_FACTORIES=1' \
	'-DCHROME_PNG_WRITE_SUPPORT' \
	'-DPNG_USER_CONFIG' \
	'-DCHROME_PNG_READ_PACK_SUPPORT' \
	'-DUSE_SYSTEM_LIBJPEG' \
	'-DU_USING_ICU_NAMESPACE=0' \
	'-DFEATURE_ENABLE_SSL' \
	'-DFEATURE_ENABLE_VOICEMAIL' \
	'-DEXPAT_RELATIVE_PATH' \
	'-DGTEST_RELATIVE_PATH' \
	'-DJSONCPP_RELATIVE_PATH' \
	'-DNO_MAIN_THREAD_WRAPPING' \
	'-DNO_SOUND_SYSTEM' \
	'-DANDROID' \
	'-DPOSIX' \
	'-DI18N_PHONENUMBERS_USE_ICU_REGEXP=1' \
	'-DPROTOBUF_USE_DLLS' \
	'-DGOOGLE_PROTOBUF_NO_RTTI' \
	'-DGOOGLE_PROTOBUF_NO_STATIC_INITIALIZER' \
	'-D__STDC_CONSTANT_MACROS' \
	'-D__STDC_FORMAT_MACROS' \
	'-D__GNU_SOURCE=1' \
	'-DUSE_STLPORT=1' \
	'-D_STLP_USE_PTR_SPECIALIZATIONS=1' \
	'-DCHROME_BUILD_ID=""' \
	'-DDYNAMIC_ANNOTATIONS_ENABLED=1' \
	'-DWTF_USE_DYNAMIC_ANNOTATIONS=1' \
	'-D_DEBUG'


# Include paths placed before CFLAGS/CPPFLAGS
LOCAL_C_INCLUDES_Debug := \
	$(gyp_shared_intermediate_dir)/shim_headers/icui18n/target \
	$(gyp_shared_intermediate_dir)/shim_headers/icuuc/target \
	$(gyp_shared_intermediate_dir)/shim_headers/ashmem/target \
	$(LOCAL_PATH) \
	$(gyp_shared_intermediate_dir) \
	$(LOCAL_PATH)/skia/config \
	$(LOCAL_PATH)/third_party/WebKit/Source \
	$(LOCAL_PATH)/third_party/khronos \
	$(LOCAL_PATH)/gpu \
	$(gyp_shared_intermediate_dir)/content \
	$(LOCAL_PATH)/third_party/skia/src/core \
	$(LOCAL_PATH)/third_party/skia/include/core \
	$(LOCAL_PATH)/third_party/skia/include/effects \
	$(LOCAL_PATH)/third_party/skia/include/pdf \
	$(LOCAL_PATH)/third_party/skia/include/gpu \
	$(LOCAL_PATH)/third_party/skia/include/lazy \
	$(LOCAL_PATH)/third_party/skia/include/pathops \
	$(LOCAL_PATH)/third_party/skia/include/pipe \
	$(LOCAL_PATH)/third_party/skia/include/ports \
	$(LOCAL_PATH)/third_party/skia/include/utils \
	$(LOCAL_PATH)/skia/ext \
	$(LOCAL_PATH)/third_party/WebKit \
	$(LOCAL_PATH)/third_party/libpng \
	$(LOCAL_PATH)/third_party/zlib \
	$(LOCAL_PATH)/third_party/libwebp \
	$(LOCAL_PATH)/third_party/ots/include \
	$(LOCAL_PATH)/third_party/qcms/src \
	$(LOCAL_PATH)/third_party/iccjpeg \
	$(PWD)/external/jpeg \
	$(PWD)/external/icu4c/common \
	$(PWD)/external/icu4c/i18n \
	$(LOCAL_PATH)/third_party/libjingle/overrides \
	$(LOCAL_PATH)/third_party/libjingle/source \
	$(LOCAL_PATH)/testing/gtest/include \
	$(LOCAL_PATH)/third_party \
	$(LOCAL_PATH)/third_party/webrtc \
	$(PWD)/external/expat/lib \
	$(LOCAL_PATH)/third_party/jsoncpp/overrides/include \
	$(LOCAL_PATH)/third_party/jsoncpp/source/include \
	$(LOCAL_PATH)/third_party/npapi \
	$(LOCAL_PATH)/third_party/npapi/bindings \
	$(LOCAL_PATH)/v8/include \
	$(gyp_shared_intermediate_dir)/protoc_out/third_party/libphonenumber \
	$(LOCAL_PATH)/third_party/libphonenumber/src \
	$(gyp_shared_intermediate_dir)/protoc_out \
	$(LOCAL_PATH)/third_party/protobuf \
	$(LOCAL_PATH)/third_party/protobuf/src \
	$(LOCAL_PATH)/third_party/openssl/openssl/include \
	$(PWD)/frameworks/wilhelm/include \
	$(PWD)/bionic \
	$(PWD)/external/stlport/stlport


# Flags passed to only C++ (and not C) files.
LOCAL_CPPFLAGS_Debug := \
	-fno-rtti \
	-fno-threadsafe-statics \
	-fvisibility-inlines-hidden \
	-Wsign-compare \
	-Wno-uninitialized \
	-Wno-error=c++0x-compat \
	-Wno-non-virtual-dtor \
	-Wno-sign-promo


# Flags passed to both C and C++ files.
MY_CFLAGS_Release := \
	-fstack-protector \
	--param=ssp-buffer-size=4 \
	 \
	-fno-exceptions \
	-fno-strict-aliasing \
	-Wall \
	-Wno-unused-parameter \
	-Wno-missing-field-initializers \
	-fvisibility=hidden \
	-pipe \
	-fPIC \
	-EL \
	-mhard-float \
	-ffunction-sections \
	-funwind-tables \
	-g \
	-fstack-protector \
	-fno-short-enums \
	-finline-limit=64 \
	-Wa,--noexecstack \
	-U_FORTIFY_SOURCE \
	-Wno-extra \
	-Wno-ignored-qualifiers \
	-Wno-type-limits \
	-Os \
	-fno-ident \
	-fdata-sections \
	-ffunction-sections \
	-fomit-frame-pointer

MY_DEFS_Release := \
	'-DCONTENT_IMPLEMENTATION' \
	'-DV8_DEPRECATION_WARNINGS' \
	'-D_FILE_OFFSET_BITS=64' \
	'-DNO_TCMALLOC' \
	'-DDISABLE_NACL' \
	'-DCHROMIUM_BUILD' \
	'-DUSE_LIBJPEG_TURBO=1' \
	'-DUSE_PROPRIETARY_CODECS' \
	'-DENABLE_CONFIGURATION_POLICY' \
	'-DDISCARDABLE_MEMORY_ALWAYS_SUPPORTED_NATIVELY' \
	'-DSYSTEM_NATIVELY_SIGNALS_MEMORY_PRESSURE' \
	'-DICU_UTIL_DATA_IMPL=ICU_UTIL_DATA_STATIC' \
	'-DUSE_OPENSSL=1' \
	'-DENABLE_EGLIMAGE=1' \
	'-DCLD_VERSION=1' \
	'-DENABLE_PRINTING=1' \
	'-DENABLE_MANAGED_USERS=1' \
	'-DMEDIA_DISABLE_LIBVPX' \
	'-DPOSIX_AVOID_MMAP' \
	'-DSK_ENABLE_INST_COUNT=0' \
	'-DSK_SUPPORT_GPU=1' \
	'-DGR_GL_CUSTOM_SETUP_HEADER="GrGLConfig_chrome.h"' \
	'-DSK_ENABLE_LEGACY_API_ALIASING=1' \
	'-DSK_ATTR_DEPRECATED=SK_NOTHING_ARG1' \
	'-DSK_SUPPORT_LEGACY_COLORTYPE=1' \
	'-DGR_GL_IGNORE_ES3_MSAA=0' \
	'-DSK_SUPPORT_LEGACY_PIXELREF_CONSTRUCTOR=1' \
	'-DSK_BUILD_FOR_ANDROID' \
	'-DSK_USE_POSIX_THREADS' \
	'-DSK_DEFERRED_CANVAS_USES_FACTORIES=1' \
	'-DCHROME_PNG_WRITE_SUPPORT' \
	'-DPNG_USER_CONFIG' \
	'-DCHROME_PNG_READ_PACK_SUPPORT' \
	'-DUSE_SYSTEM_LIBJPEG' \
	'-DU_USING_ICU_NAMESPACE=0' \
	'-DFEATURE_ENABLE_SSL' \
	'-DFEATURE_ENABLE_VOICEMAIL' \
	'-DEXPAT_RELATIVE_PATH' \
	'-DGTEST_RELATIVE_PATH' \
	'-DJSONCPP_RELATIVE_PATH' \
	'-DNO_MAIN_THREAD_WRAPPING' \
	'-DNO_SOUND_SYSTEM' \
	'-DANDROID' \
	'-DPOSIX' \
	'-DI18N_PHONENUMBERS_USE_ICU_REGEXP=1' \
	'-DPROTOBUF_USE_DLLS' \
	'-DGOOGLE_PROTOBUF_NO_RTTI' \
	'-DGOOGLE_PROTOBUF_NO_STATIC_INITIALIZER' \
	'-D__STDC_CONSTANT_MACROS' \
	'-D__STDC_FORMAT_MACROS' \
	'-D__GNU_SOURCE=1' \
	'-DUSE_STLPORT=1' \
	'-D_STLP_USE_PTR_SPECIALIZATIONS=1' \
	'-DCHROME_BUILD_ID=""' \
	'-DNDEBUG' \
	'-DNVALGRIND' \
	'-DDYNAMIC_ANNOTATIONS_ENABLED=0' \
	'-D_FORTIFY_SOURCE=2'


# Include paths placed before CFLAGS/CPPFLAGS
LOCAL_C_INCLUDES_Release := \
	$(gyp_shared_intermediate_dir)/shim_headers/icui18n/target \
	$(gyp_shared_intermediate_dir)/shim_headers/icuuc/target \
	$(gyp_shared_intermediate_dir)/shim_headers/ashmem/target \
	$(LOCAL_PATH) \
	$(gyp_shared_intermediate_dir) \
	$(LOCAL_PATH)/skia/config \
	$(LOCAL_PATH)/third_party/WebKit/Source \
	$(LOCAL_PATH)/third_party/khronos \
	$(LOCAL_PATH)/gpu \
	$(gyp_shared_intermediate_dir)/content \
	$(LOCAL_PATH)/third_party/skia/src/core \
	$(LOCAL_PATH)/third_party/skia/include/core \
	$(LOCAL_PATH)/third_party/skia/include/effects \
	$(LOCAL_PATH)/third_party/skia/include/pdf \
	$(LOCAL_PATH)/third_party/skia/include/gpu \
	$(LOCAL_PATH)/third_party/skia/include/lazy \
	$(LOCAL_PATH)/third_party/skia/include/pathops \
	$(LOCAL_PATH)/third_party/skia/include/pipe \
	$(LOCAL_PATH)/third_party/skia/include/ports \
	$(LOCAL_PATH)/third_party/skia/include/utils \
	$(LOCAL_PATH)/skia/ext \
	$(LOCAL_PATH)/third_party/WebKit \
	$(LOCAL_PATH)/third_party/libpng \
	$(LOCAL_PATH)/third_party/zlib \
	$(LOCAL_PATH)/third_party/libwebp \
	$(LOCAL_PATH)/third_party/ots/include \
	$(LOCAL_PATH)/third_party/qcms/src \
	$(LOCAL_PATH)/third_party/iccjpeg \
	$(PWD)/external/jpeg \
	$(PWD)/external/icu4c/common \
	$(PWD)/external/icu4c/i18n \
	$(LOCAL_PATH)/third_party/libjingle/overrides \
	$(LOCAL_PATH)/third_party/libjingle/source \
	$(LOCAL_PATH)/testing/gtest/include \
	$(LOCAL_PATH)/third_party \
	$(LOCAL_PATH)/third_party/webrtc \
	$(PWD)/external/expat/lib \
	$(LOCAL_PATH)/third_party/jsoncpp/overrides/include \
	$(LOCAL_PATH)/third_party/jsoncpp/source/include \
	$(LOCAL_PATH)/third_party/npapi \
	$(LOCAL_PATH)/third_party/npapi/bindings \
	$(LOCAL_PATH)/v8/include \
	$(gyp_shared_intermediate_dir)/protoc_out/third_party/libphonenumber \
	$(LOCAL_PATH)/third_party/libphonenumber/src \
	$(gyp_shared_intermediate_dir)/protoc_out \
	$(LOCAL_PATH)/third_party/protobuf \
	$(LOCAL_PATH)/third_party/protobuf/src \
	$(LOCAL_PATH)/third_party/openssl/openssl/include \
	$(PWD)/frameworks/wilhelm/include \
	$(PWD)/bionic \
	$(PWD)/external/stlport/stlport


# Flags passed to only C++ (and not C) files.
LOCAL_CPPFLAGS_Release := \
	-fno-rtti \
	-fno-threadsafe-statics \
	-fvisibility-inlines-hidden \
	-Wsign-compare \
	-Wno-uninitialized \
	-Wno-error=c++0x-compat \
	-Wno-non-virtual-dtor \
	-Wno-sign-promo


LOCAL_CFLAGS := $(MY_CFLAGS_$(GYP_CONFIGURATION)) $(MY_DEFS_$(GYP_CONFIGURATION))
LOCAL_C_INCLUDES := $(GYP_COPIED_SOURCE_ORIGIN_DIRS) $(LOCAL_C_INCLUDES_$(GYP_CONFIGURATION))
LOCAL_CPPFLAGS := $(LOCAL_CPPFLAGS_$(GYP_CONFIGURATION))
### Rules for final target.

LOCAL_LDFLAGS_Debug := \
	-Wl,-z,now \
	-Wl,-z,relro \
	-Wl,-z,noexecstack \
	-fPIC \
	-EL \
	-Wl,--no-keep-memory \
	-nostdlib \
	-Wl,--no-undefined \
	-Wl,--exclude-libs=ALL \
	-Wl,--fatal-warnings \
	-Wl,--gc-sections \
	-Wl,--warn-shared-textrel \
	-Wl,-O1 \
	-Wl,--as-needed


LOCAL_LDFLAGS_Release := \
	-Wl,-z,now \
	-Wl,-z,relro \
	-Wl,-z,noexecstack \
	-fPIC \
	-EL \
	-Wl,--no-keep-memory \
	-nostdlib \
	-Wl,--no-undefined \
	-Wl,--exclude-libs=ALL \
	-Wl,-O1 \
	-Wl,--as-needed \
	-Wl,--gc-sections \
	-Wl,--fatal-warnings \
	-Wl,--warn-shared-textrel


LOCAL_LDFLAGS := $(LOCAL_LDFLAGS_$(GYP_CONFIGURATION))

LOCAL_STATIC_LIBRARIES := \
	cpufeatures \
	skia_skia_library_gyp \
	webkit_child_webkit_child_gyp \
	webkit_glue_glue_gyp \
	third_party_libphonenumber_libphonenumber_without_metadata_gyp

# Enable grouping to fix circular references
LOCAL_GROUP_STATIC_LIBRARIES := true

LOCAL_SHARED_LIBRARIES := \
	libstlport \
	libdl

# Add target alias to "gyp_all_modules" target.
.PHONY: gyp_all_modules
gyp_all_modules: content_content_renderer_gyp

# Alias gyp target name.
.PHONY: content_renderer
content_renderer: content_content_renderer_gyp

include $(BUILD_STATIC_LIBRARY)
