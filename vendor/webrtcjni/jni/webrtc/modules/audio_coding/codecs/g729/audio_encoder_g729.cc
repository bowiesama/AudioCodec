#include "webrtc/modules/audio_coding/codecs/g729/audio_encoder_g729.h"

#include <limits>
#include "webrtc/base/checks.h"
#include "webrtc/modules/audio_coding/codecs/g729/g729_interface.h"

namespace webrtc 
{

namespace
{
  const int kSampleRateHz = 8000;

  AudioEncoderG729::Config CreateConfig(const CodecInst& codec_inst) {
    AudioEncoderG729::Config config;
    config.num_channels = codec_inst.channels;
    config.frame_size_ms = codec_inst.pacsize / 8;
    config.payload_type = codec_inst.pltype;
    return config;
  }


}  // namespace

bool AudioEncoderG729::Config::IsOk() const {
  return (frame_size_ms > 0) && (frame_size_ms % 10 == 0) &&
    (num_channels >= 1);
}

AudioEncoderG729::AudioEncoderG729(const Config& config)
  : num_channels_(config.num_channels),
    payload_type_(config.payload_type),
    num_10ms_frames_per_packet_(config.frame_size_ms / 10),
    enable_dtx_(config.enable_dtx),
    num_10ms_frames_buffered_(0),
    first_timestamp_in_buffer_(0)
{
  RTC_CHECK(config.IsOk());

  RTC_CHECK_EQ(0, WebRtcG729_CreateEnc(&encoder_));
  Reset();
}

AudioEncoderG729::AudioEncoderG729(const CodecInst& codec_inst)
  : AudioEncoderG729(CreateConfig(codec_inst)) {}

AudioEncoderG729::~AudioEncoderG729() 
{
  RTC_CHECK_EQ(0, WebRtcG729_FreeEnc(encoder_));
}

int AudioEncoderG729::SampleRateHz() const 
{
  return kSampleRateHz;
}

int AudioEncoderG729::RtpTimestampRateHz() const 
{
  return kSampleRateHz;
}

size_t AudioEncoderG729::NumChannels() const
{
  return num_channels_;
}

size_t AudioEncoderG729::Num10MsFramesInNextPacket() const
{
  return num_10ms_frames_per_packet_;
}

size_t AudioEncoderG729::Max10MsFramesInAPacket() const
{
  return num_10ms_frames_per_packet_;
}

int AudioEncoderG729::GetTargetBitrate() const {
  return static_cast<int>(8000 * NumChannels());
}

void AudioEncoderG729::Reset() {
  num_10ms_frames_buffered_ = 0;
  for (size_t i = 0; i < num_channels_; ++i)
    RTC_CHECK_EQ(0, WebRtcG729_EncoderInit(encoder_, enable_dtx_ ? 1 : 0));

}

AudioEncoder::EncodedInfo AudioEncoderG729::EncodeImpl(
  uint32_t      rtp_timestamp,
  rtc::ArrayView<const int16_t> audio,

  rtc::Buffer   *encoded
  ) 
{
  EncodedInfo info;

  if (num_10ms_frames_buffered_ == 0)
    first_timestamp_in_buffer_ = rtp_timestamp;

  int16_t enc_len = 
  WebRtcG729_Encode(
    encoder_, 
    audio.data(),
    80, 
    &encoded_buffer_[num_10ms_frames_buffered_*10]);

  size_t bytes_to_encode = 0;

  if (enc_len == 10)
  {
    if (++num_10ms_frames_buffered_ == num_10ms_frames_per_packet_) 
    {
      bytes_to_encode = num_10ms_frames_buffered_*10;
      info.encoded_timestamp = first_timestamp_in_buffer_;
      info.payload_type = payload_type_;
      info.encoder_type = CodecType::kG729;

      num_10ms_frames_buffered_ = 0;
    }
  }
  else if (enc_len == 2)
  {
    bytes_to_encode = num_10ms_frames_buffered_*10 + 2;
    info.encoded_timestamp = first_timestamp_in_buffer_;
    info.payload_type = payload_type_;
    info.encoder_type = CodecType::kG729;

    info.speech = false;

    num_10ms_frames_buffered_ = 0;
  }
  else
  {
    // Unexpected result and error conditions.
    // Return accumulated encoded bytes if any.
    if (num_10ms_frames_buffered_)
    {
      bytes_to_encode = num_10ms_frames_buffered_*10;
      info.encoded_timestamp = first_timestamp_in_buffer_;
      info.payload_type = payload_type_;
      info.encoder_type = CodecType::kG729;

      num_10ms_frames_buffered_ = 0;
    }
  }

  if (bytes_to_encode)
  {
    info.encoded_bytes = encoded->AppendData(
      bytes_to_encode, [&] (rtc::ArrayView<uint8_t> encoded) {

        if (bytes_to_encode)	  
          std::memcpy(encoded.data(), encoded_buffer_, bytes_to_encode);
 
        return bytes_to_encode;
      });
  }
  
  return info;
}

size_t AudioEncoderG729::SamplesPerChannel() const
{
  return kSampleRateHz / 100 * num_10ms_frames_per_packet_;
}

}  // namespace webrtc
