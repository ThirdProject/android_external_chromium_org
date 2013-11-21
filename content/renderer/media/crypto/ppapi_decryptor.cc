// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/crypto/ppapi_decryptor.h"

#include <string>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_loop_proxy.h"
#include "content/renderer/pepper/content_decryptor_delegate.h"
#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/data_buffer.h"
#include "media/base/decoder_buffer.h"
#include "media/base/video_decoder_config.h"
#include "media/base/video_frame.h"

namespace content {

scoped_ptr<PpapiDecryptor> PpapiDecryptor::Create(
    const std::string& key_system,
    const scoped_refptr<PepperPluginInstanceImpl>& plugin_instance,
    const media::KeyAddedCB& key_added_cb,
    const media::KeyErrorCB& key_error_cb,
    const media::KeyMessageCB& key_message_cb,
    const media::SetSessionIdCB& set_session_id_cb,
    const base::Closure& destroy_plugin_cb) {
  ContentDecryptorDelegate* plugin_cdm_delegate =
      plugin_instance->GetContentDecryptorDelegate();
  if (!plugin_cdm_delegate) {
    DVLOG(1) << "PpapiDecryptor: plugin cdm delegate creation failed.";
    return scoped_ptr<PpapiDecryptor>();
  }

  plugin_cdm_delegate->Initialize(key_system);

  return scoped_ptr<PpapiDecryptor>(new PpapiDecryptor(plugin_instance,
                                                       plugin_cdm_delegate,
                                                       key_added_cb,
                                                       key_error_cb,
                                                       key_message_cb,
                                                       set_session_id_cb,
                                                       destroy_plugin_cb));
}

PpapiDecryptor::PpapiDecryptor(
    const scoped_refptr<PepperPluginInstanceImpl>& plugin_instance,
    ContentDecryptorDelegate* plugin_cdm_delegate,
    const media::KeyAddedCB& key_added_cb,
    const media::KeyErrorCB& key_error_cb,
    const media::KeyMessageCB& key_message_cb,
    const media::SetSessionIdCB& set_session_id_cb,
    const base::Closure& destroy_plugin_cb)
    : plugin_instance_(plugin_instance),
      plugin_cdm_delegate_(plugin_cdm_delegate),
      key_added_cb_(key_added_cb),
      key_error_cb_(key_error_cb),
      key_message_cb_(key_message_cb),
      set_session_id_cb_(set_session_id_cb),
      destroy_plugin_cb_(destroy_plugin_cb),
      render_loop_proxy_(base::MessageLoopProxy::current()),
      weak_ptr_factory_(this) {
  DCHECK(plugin_instance_.get());
  DCHECK(!key_added_cb_.is_null());
  DCHECK(!key_error_cb_.is_null());
  DCHECK(!key_message_cb_.is_null());
  DCHECK(!set_session_id_cb_.is_null());
  DCHECK(!destroy_plugin_cb_.is_null());

  weak_this_ = weak_ptr_factory_.GetWeakPtr();

  plugin_cdm_delegate_->SetKeyEventCallbacks(
      base::Bind(&PpapiDecryptor::KeyAdded, weak_this_),
      base::Bind(&PpapiDecryptor::KeyError, weak_this_),
      base::Bind(&PpapiDecryptor::KeyMessage, weak_this_),
      base::Bind(&PpapiDecryptor::SetSessionId, weak_this_));
}

PpapiDecryptor::~PpapiDecryptor() {
  plugin_cdm_delegate_ = NULL;
  plugin_instance_ = NULL;
  destroy_plugin_cb_.Run();
}

bool PpapiDecryptor::GenerateKeyRequest(uint32 reference_id,
                                        const std::string& type,
                                        const uint8* init_data,
                                        int init_data_length) {
  DVLOG(2) << "GenerateKeyRequest()";
  DCHECK(render_loop_proxy_->BelongsToCurrentThread());
  DCHECK(plugin_cdm_delegate_);

  if (!plugin_cdm_delegate_->GenerateKeyRequest(
      reference_id, type, init_data, init_data_length)) {
    ReportFailureToCallPlugin(reference_id);
    return false;
  }

  return true;
}

void PpapiDecryptor::AddKey(uint32 reference_id,
                            const uint8* key,
                            int key_length,
                            const uint8* init_data,
                            int init_data_length) {
  DVLOG(2) << "AddKey()";
  DCHECK(render_loop_proxy_->BelongsToCurrentThread());

  if (!plugin_cdm_delegate_->AddKey(
      reference_id, key, key_length, init_data, init_data_length)) {
    ReportFailureToCallPlugin(reference_id);
  }

  if (!new_audio_key_cb_.is_null())
    new_audio_key_cb_.Run();

  if (!new_video_key_cb_.is_null())
    new_video_key_cb_.Run();
}

void PpapiDecryptor::CancelKeyRequest(uint32 reference_id) {
  DVLOG(2) << "CancelKeyRequest()";
  DCHECK(render_loop_proxy_->BelongsToCurrentThread());

  if (!plugin_cdm_delegate_->CancelKeyRequest(reference_id))
    ReportFailureToCallPlugin(reference_id);
}

media::Decryptor* PpapiDecryptor::GetDecryptor() {
#if defined(GOOGLE_TV)
  // Google TV only uses PpapiDecrytor as a MediaKeys and does not need the
  // Decryptor interface of the PpapiDecryptor.
  // Details: If we don't do this GTV will be broken. The reason is that during
  // initialization, MediaSourceDelegate tries to use DecryptingDemuxerStream
  // to decrypt the stream in the renderer process (for ClearKey support).
  // However, for GTV, PpapiDecryptor cannot do decryption at all. By returning
  // NULL, DDS init will fail and we fallback to what GTV used to do.
  return NULL;
#else
  return this;
#endif  // defined(GOOGLE_TV)
}

void PpapiDecryptor::RegisterNewKeyCB(StreamType stream_type,
                                      const NewKeyCB& new_key_cb) {
  if (!render_loop_proxy_->BelongsToCurrentThread()) {
    render_loop_proxy_->PostTask(FROM_HERE, base::Bind(
        &PpapiDecryptor::RegisterNewKeyCB, weak_this_, stream_type,
        new_key_cb));
    return;
  }

  DVLOG(3) << __FUNCTION__ << " - stream_type: " << stream_type;
  switch (stream_type) {
    case kAudio:
      new_audio_key_cb_ = new_key_cb;
      break;
    case kVideo:
      new_video_key_cb_ = new_key_cb;
      break;
    default:
      NOTREACHED();
  }
}

void PpapiDecryptor::Decrypt(
    StreamType stream_type,
    const scoped_refptr<media::DecoderBuffer>& encrypted,
    const DecryptCB& decrypt_cb) {
  if (!render_loop_proxy_->BelongsToCurrentThread()) {
    render_loop_proxy_->PostTask(FROM_HERE, base::Bind(
        &PpapiDecryptor::Decrypt, weak_this_,
        stream_type, encrypted, decrypt_cb));
    return;
  }

  DVLOG(3) << __FUNCTION__ << " - stream_type: " << stream_type;
  if (!plugin_cdm_delegate_->Decrypt(stream_type, encrypted, decrypt_cb))
    decrypt_cb.Run(kError, NULL);
}

void PpapiDecryptor::CancelDecrypt(StreamType stream_type) {
  if (!render_loop_proxy_->BelongsToCurrentThread()) {
    render_loop_proxy_->PostTask(FROM_HERE, base::Bind(
        &PpapiDecryptor::CancelDecrypt, weak_this_, stream_type));
    return;
  }

  DVLOG(1) << __FUNCTION__ << " - stream_type: " << stream_type;
  plugin_cdm_delegate_->CancelDecrypt(stream_type);
}

void PpapiDecryptor::InitializeAudioDecoder(
      const media::AudioDecoderConfig& config,
      const DecoderInitCB& init_cb) {
  if (!render_loop_proxy_->BelongsToCurrentThread()) {
    render_loop_proxy_->PostTask(FROM_HERE, base::Bind(
        &PpapiDecryptor::InitializeAudioDecoder, weak_this_, config, init_cb));
    return;
  }

  DVLOG(2) << __FUNCTION__;
  DCHECK(config.is_encrypted());
  DCHECK(config.IsValidConfig());

  audio_decoder_init_cb_ = init_cb;
  if (!plugin_cdm_delegate_->InitializeAudioDecoder(config, base::Bind(
      &PpapiDecryptor::OnDecoderInitialized, weak_this_, kAudio))) {
    base::ResetAndReturn(&audio_decoder_init_cb_).Run(false);
    return;
  }
}

void PpapiDecryptor::InitializeVideoDecoder(
    const media::VideoDecoderConfig& config,
    const DecoderInitCB& init_cb) {
  if (!render_loop_proxy_->BelongsToCurrentThread()) {
    render_loop_proxy_->PostTask(FROM_HERE, base::Bind(
        &PpapiDecryptor::InitializeVideoDecoder, weak_this_, config, init_cb));
    return;
  }

  DVLOG(2) << __FUNCTION__;
  DCHECK(config.is_encrypted());
  DCHECK(config.IsValidConfig());

  video_decoder_init_cb_ = init_cb;
  if (!plugin_cdm_delegate_->InitializeVideoDecoder(config, base::Bind(
      &PpapiDecryptor::OnDecoderInitialized, weak_this_, kVideo))) {
    base::ResetAndReturn(&video_decoder_init_cb_).Run(false);
    return;
  }
}

void PpapiDecryptor::DecryptAndDecodeAudio(
    const scoped_refptr<media::DecoderBuffer>& encrypted,
    const AudioDecodeCB& audio_decode_cb) {
  if (!render_loop_proxy_->BelongsToCurrentThread()) {
    render_loop_proxy_->PostTask(FROM_HERE, base::Bind(
        &PpapiDecryptor::DecryptAndDecodeAudio, weak_this_,
        encrypted, audio_decode_cb));
    return;
  }

  DVLOG(3) << __FUNCTION__;
  if (!plugin_cdm_delegate_->DecryptAndDecodeAudio(encrypted, audio_decode_cb))
    audio_decode_cb.Run(kError, AudioBuffers());
}

void PpapiDecryptor::DecryptAndDecodeVideo(
    const scoped_refptr<media::DecoderBuffer>& encrypted,
    const VideoDecodeCB& video_decode_cb) {
  if (!render_loop_proxy_->BelongsToCurrentThread()) {
    render_loop_proxy_->PostTask(FROM_HERE, base::Bind(
        &PpapiDecryptor::DecryptAndDecodeVideo, weak_this_,
        encrypted, video_decode_cb));
    return;
  }

  DVLOG(3) << __FUNCTION__;
  if (!plugin_cdm_delegate_->DecryptAndDecodeVideo(encrypted, video_decode_cb))
    video_decode_cb.Run(kError, NULL);
}

void PpapiDecryptor::ResetDecoder(StreamType stream_type) {
  if (!render_loop_proxy_->BelongsToCurrentThread()) {
    render_loop_proxy_->PostTask(FROM_HERE, base::Bind(
        &PpapiDecryptor::ResetDecoder, weak_this_, stream_type));
    return;
  }

  DVLOG(2) << __FUNCTION__ << " - stream_type: " << stream_type;
  plugin_cdm_delegate_->ResetDecoder(stream_type);
}

void PpapiDecryptor::DeinitializeDecoder(StreamType stream_type) {
  if (!render_loop_proxy_->BelongsToCurrentThread()) {
    render_loop_proxy_->PostTask(FROM_HERE, base::Bind(
        &PpapiDecryptor::DeinitializeDecoder, weak_this_, stream_type));
    return;
  }

  DVLOG(2) << __FUNCTION__ << " - stream_type: " << stream_type;
  plugin_cdm_delegate_->DeinitializeDecoder(stream_type);
}

void PpapiDecryptor::ReportFailureToCallPlugin(uint32 reference_id) {
  DCHECK(render_loop_proxy_->BelongsToCurrentThread());
  DVLOG(1) << "Failed to call plugin.";
  key_error_cb_.Run(reference_id, kUnknownError, 0);
}

void PpapiDecryptor::OnDecoderInitialized(StreamType stream_type,
                                          bool success) {
  DCHECK(render_loop_proxy_->BelongsToCurrentThread());
  switch (stream_type) {
    case kAudio:
      DCHECK(!audio_decoder_init_cb_.is_null());
      base::ResetAndReturn(&audio_decoder_init_cb_).Run(success);
      break;
    case kVideo:
      DCHECK(!video_decoder_init_cb_.is_null());
      base::ResetAndReturn(&video_decoder_init_cb_).Run(success);
      break;
    default:
      NOTREACHED();
  }
}

void PpapiDecryptor::KeyAdded(uint32 reference_id) {
  DCHECK(render_loop_proxy_->BelongsToCurrentThread());
  key_added_cb_.Run(reference_id);
}

void PpapiDecryptor::KeyError(uint32 reference_id,
                              media::MediaKeys::KeyError error_code,
                              int system_code) {
  DCHECK(render_loop_proxy_->BelongsToCurrentThread());
  key_error_cb_.Run(reference_id, error_code, system_code);
}

void PpapiDecryptor::KeyMessage(uint32 reference_id,
                                const std::vector<uint8>& message,
                                const std::string& default_url) {
  DCHECK(render_loop_proxy_->BelongsToCurrentThread());
  key_message_cb_.Run(reference_id, message, default_url);
}

void PpapiDecryptor::SetSessionId(uint32 reference_id,
                                  const std::string& session_id) {
  DCHECK(render_loop_proxy_->BelongsToCurrentThread());
  set_session_id_cb_.Run(reference_id, session_id);
}

}  // namespace content
