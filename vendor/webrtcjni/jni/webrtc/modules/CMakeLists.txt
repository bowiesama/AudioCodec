project(ilbc)

add_definitions(
    -DWEBRTC_POSIX
)

set(ILBC_DIR        ${CMAKE_CURRENT_LIST_DIR}/audio_coding/codecs/ilbc)
set(PROCESSING_DIR  ${CMAKE_CURRENT_LIST_DIR}/../common_audio/signal_processing)

include_directories(
    ..
    ../common_audio/signal_processing/include
    ${ILBC_DIR}/interface
    ${PROCESSING_DIR}/include
)

file(GLOB ILBC_FILES ${ILBC_DIR}/*.c)

set(PROCESSING_FILES
    ${PROCESSING_DIR}/auto_corr_to_refl_coef.c
    ${PROCESSING_DIR}/auto_correlation.c
    ${PROCESSING_DIR}/complex_fft.c
    ${PROCESSING_DIR}/copy_set_operations.c
    ${PROCESSING_DIR}/cross_correlation.c
    ${PROCESSING_DIR}/division_operations.c
    ${PROCESSING_DIR}/dot_product_with_scale.c
    ${PROCESSING_DIR}/downsample_fast.c
    ${PROCESSING_DIR}/energy.c
    ${PROCESSING_DIR}/filter_ar.c
    ${PROCESSING_DIR}/filter_ar_fast_q12.c
    ${PROCESSING_DIR}/filter_ma_fast_q12.c
    ${PROCESSING_DIR}/get_hanning_window.c
    ${PROCESSING_DIR}/get_scaling_square.c
    ${PROCESSING_DIR}/ilbc_specific_functions.c
    ${PROCESSING_DIR}/levinson_durbin.c
    ${PROCESSING_DIR}/lpc_to_refl_coef.c
    ${PROCESSING_DIR}/min_max_operations.c
    ${PROCESSING_DIR}/randomization_functions.c
    ${PROCESSING_DIR}/real_fft.c
    ${PROCESSING_DIR}/refl_coef_to_lpc.c
    ${PROCESSING_DIR}/resample.c
    ${PROCESSING_DIR}/resample_48khz.c
    ${PROCESSING_DIR}/resample_by_2.c
    ${PROCESSING_DIR}/resample_by_2_internal.c
    ${PROCESSING_DIR}/resample_fractional.c
    ${PROCESSING_DIR}/spl_init.c
    ${PROCESSING_DIR}/spl_sqrt.c
    ${PROCESSING_DIR}/spl_sqrt_floor.c
    ${PROCESSING_DIR}/spl_version.c
    ${PROCESSING_DIR}/splitting_filter.c
    ${PROCESSING_DIR}/sqrt_of_one_minus_x_squared.c
    ${PROCESSING_DIR}/vector_scaling_operations.c
    ${PROCESSING_DIR}/webrtc_fft_t_1024_8.c
    ${PROCESSING_DIR}/webrtc_fft_t_rad.c
    ${PROCESSING_DIR}/complex_bit_reverse.c
)


add_library(ilbc ${WME_LIB_TYPE}
    ${ILBC_FILES}
    ${PROCESSING_FILES}
)
