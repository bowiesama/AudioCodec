/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/audio_coding/codecs/g729/audio_decoder_g729.h"

#include <string.h>

#include "webrtc/base/checks.h"
#include "webrtc/modules/audio_coding/codecs/legacy_encoded_audio_frame.h"
#include "webrtc/modules/audio_coding/codecs/g729/g729_interface.h"

namespace webrtc {

AudioDecoderG729::AudioDecoderG729() {
  WebRtcG729_CreateDec(&dec_state_);
  WebRtcG729_DecoderInit(dec_state_);
}

AudioDecoderG729::~AudioDecoderG729() {
  WebRtcG729_FreeDec(dec_state_);
}


bool AudioDecoderG729::HasDecodePlc() const {
  return false;
}

int AudioDecoderG729::DecodeInternal(const uint8_t* encoded,
                                     size_t encoded_len,
                                     int sample_rate_hz,
                                     int16_t* decoded,
                                     SpeechType* speech_type) {
  RTC_DCHECK_EQ(SampleRateHz(), sample_rate_hz);
  int16_t temp_type = 1;  // Default is speech.
  size_t ret =
    WebRtcG729_Decode(
      dec_state_,
      const_cast<int16_t*>(reinterpret_cast<const int16_t*>(encoded)),
      static_cast<int16_t>(encoded_len),
      decoded,
      &temp_type);
  *speech_type = ConvertSpeechType(temp_type);
  return static_cast<int>(ret);
}

void AudioDecoderG729::Reset() {
  WebRtcG729_DecoderInit(dec_state_);
}

std::vector<AudioDecoder::ParseResult> AudioDecoderG729::ParsePayload(
    rtc::Buffer&& payload,
    uint32_t timestamp) {
  return LegacyEncodedAudioFrame::SplitBySamples(this, std::move(payload),
                                                 timestamp, 8, 8);
}

int AudioDecoderG729::PacketDuration(const uint8_t* encoded,
                                     size_t encoded_len) const {
  int a_frames = (int) (encoded_len / 10);
  int b_frames = (int) (encoded_len % 10 ? 1 : 0);

  return (a_frames + b_frames) * 80;
}

int AudioDecoderG729::SampleRateHz() const {
  return 8000;
}

size_t AudioDecoderG729::Channels() const {
  return 1;
}

}  // namespace webrtc
