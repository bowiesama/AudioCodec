#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_G729_INCLUDE_AUDIO_ENCODER_G729_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_G729_INCLUDE_AUDIO_ENCODER_G729_H_

#include <memory>

#include "webrtc/base/constructormagic.h"
#include "webrtc/modules/audio_coding/codecs/audio_encoder.h"
#include "webrtc/modules/audio_coding/codecs/g729/g729_interface.h"
#include "webrtc/modules/audio_coding/include/audio_coding_module_typedefs.h"

namespace webrtc {

class AudioEncoderG729 final : public AudioEncoder 
{
 public:
  struct Config {
    bool IsOk() const;

    int payload_type = 18;
    int frame_size_ms = 20;
    size_t num_channels = 1;
    bool enable_dtx = false;
  };

  explicit AudioEncoderG729(const Config& config);
  explicit AudioEncoderG729(const CodecInst& codec_inst);
  ~AudioEncoderG729() override;

  int SampleRateHz() const override;
  size_t NumChannels() const override;
  int RtpTimestampRateHz() const override;
  size_t Num10MsFramesInNextPacket() const override;
  size_t Max10MsFramesInAPacket() const override;
  int GetTargetBitrate() const override;
  void Reset() override;

protected:
  EncodedInfo EncodeImpl(uint32_t rtp_timestamp,
                         rtc::ArrayView<const int16_t> audio,
                         rtc::Buffer* encoded) override;

 private:
  G729_encinst_t* encoder_;
  uint8_t encoded_buffer_[120];  // Max 120 ms.

  size_t SamplesPerChannel() const;

  const size_t num_channels_;
  const int payload_type_;
  const size_t num_10ms_frames_per_packet_;
  bool enable_dtx_;

  size_t num_10ms_frames_buffered_;
  uint32_t first_timestamp_in_buffer_;
  RTC_DISALLOW_COPY_AND_ASSIGN(AudioEncoderG729);
};

}  // namespace webrtc
#endif  // WEBRTC_MODULES_AUDIO_CODING_CODECS_G729_INCLUDE_AUDIO_ENCODER_G729_H_
