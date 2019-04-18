/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_G729_AUDIO_DECODER_G729_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_G729_AUDIO_DECODER_G729_H_

#include "webrtc/base/constructormagic.h"
#include "webrtc/modules/audio_coding/codecs/audio_decoder.h"

typedef struct WebRtcG729DecInst G729DecInst;

namespace webrtc {

class AudioDecoderG729 final : public AudioDecoder {
 public:
  AudioDecoderG729();
  ~AudioDecoderG729() override;
  bool HasDecodePlc() const override;
  void Reset() override;
  std::vector<ParseResult> ParsePayload(rtc::Buffer&& payload,
                                        uint32_t timestamp) override;
  int PacketDuration(const uint8_t* encoded, size_t encoded_len) const override;
  int SampleRateHz() const override;
  size_t Channels() const override;

 protected:
  int DecodeInternal(const uint8_t* encoded,
                     size_t encoded_len,
                     int sample_rate_hz,
                     int16_t* decoded,
                     SpeechType* speech_type) override;

 private:
  G729DecInst* dec_state_;
  RTC_DISALLOW_COPY_AND_ASSIGN(AudioDecoderG729);
};

}  // namespace webrtc

#endif  // WEBRTC_MODULES_AUDIO_CODING_CODECS_G729_AUDIO_DECODER_G729_H_
