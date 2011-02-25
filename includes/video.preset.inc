<?php

/**
 * This is used to set some required settings while the preset is being created.
 *
 * @return <array> - The form elements that you would like to add for the settings.
 *
 *                   Special Note:  It is required to structure all your settings using
 *                   $form['preset']['settings'] as the parent form element.  Otherwise,
 *                   they will not get serialized within the "settings" column of the database.
 * @TODO : we can add some required values there
 */
function video_preset_create_form() {
  $form = array();
  return $form;
}

/**
 * This is used to add all the other settings that you need to declare in your preset.
 * The preset object is passed to this form, so you can use that data to populate the default values
 * for your form elements by using $preset['settings']['param2'].
 *
 * @param <array> - The preset object.
 * @return <array> - The additional form settings that you would like to add to your preset.
 */
function video_preset_form($preset) {
  $form = array();
  $form['preset']['settings']['device_profile'] = array(
    '#type' => 'fieldset',
    '#title' => t('Output parameter'),
    '#description' => t('All output settings are optional. We choose sensible defaults for web and iPhone playback.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['preset']['settings']['device_profile']['device_profile'] = array(
    '#type' => 'select',
    '#title' => t('Device profile'),
    '#description' => t('A profile for your target device.'),
    '#options' => array(
      '0' => 'None',
      'mobile/advanced' => 'mobile/advanced',
      'mobile/baseline' => 'mobile/baseline',
      'mobile/legacy' => 'mobile/legacy',
      'v1/mobile/advanced' => 'v1/mobile/advanced',
      'v1/mobile/baseline' => 'v1/mobile/baseline',
      'v1/mobile/legacy' => 'v1/mobile/legacy',
      'v2/mobile/legacy' => 'v2/mobile/legacy'
    ),
    '#default_value' => (!empty($preset['settings']['device_profile'])) ? $preset['settings']['device_profile'] : 0
  );
  // video settings
  $form['preset']['settings']['video'] = array(
    '#type' => 'fieldset',
    '#title' => t('Video settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['preset']['settings']['video']['video_codec'] = array(
    '#type' => 'select',
    '#title' => t('Video codec'),
    '#description' => t('The video codec used in the video file can affect the ability to play the video on certain devices. The default codec used is H.264.'),
    '#options' => array(
      'h264' => 'H.264 (default)',
      'vp8' => 'VP8',
      'theora' => 'Theora',
      'vp6' => 'VP6',
      'mpeg4' => 'MPEG-4',
      'wmv' => 'WMV'
    ),
    '#default_value' => (!empty($preset['settings']['video_codec'])) ? $preset['settings']['video_codec'] : 'h264'
  );
  $form['preset']['settings']['video']['video_quality'] = array(
    '#type' => 'select',
    '#title' => t('Video quality'),
    '#description' => t('A target video quality. Affects bitrate and file size.'),
    '#options' => array(
      '1 - Poor quality (smaller file)',
      '2',
      '3 (default)',
      '4',
      '5 - High quality (larger file)'
    ),
    '#default_value' => (!empty($preset['settings']['video_quality'])) ? $preset['settings']['video_quality'] : 2
  );
  $form['preset']['settings']['video']['video_speed'] = array(
    '#type' => 'select',
    '#title' => t('Video speed'),
    '#description' => t('Speed of encoding. Affects compression.'),
    '#options' => array(
      '1 - Slow (better compression)',
      '2',
      '3 (default)',
      '4',
      '5 - Fast (worse compression)'
    ),
    '#default_value' => (!empty($preset['settings']['video_speed'])) ? $preset['settings']['video_speed'] : 2
  );
  $form['preset']['settings']['video']['width'] = array(
    '#type' => 'textfield',
    '#title' => t('Width'),
    '#description' => t('The maximum width of the output video (in pixels).'),
    '#default_value' => !empty($preset['settings']['width']) ? $preset['settings']['width'] : ''
  );
  $form['preset']['settings']['video']['height'] = array(
    '#type' => 'textfield',
    '#title' => t('Height'),
    '#description' => t('The maximum height of the output video (in pixels).'),
    '#default_value' => !empty($preset['settings']['height']) ? $preset['settings']['height'] : ''
  );
  $form['preset']['settings']['video']['video_aspectmode'] = array(
    '#type' => 'select',
    '#title' => t('Aspect mode'),
    '#description' => t('What to do when aspect ratio of input file does not match the target width/height aspect ratio.'),
    '#options' => array(
      'preserve' => 'Preserve aspect ratio (default)',
      'crop' => 'Crop to fit output aspect ratio',
      'pad' => 'Pad (letterbox) to fit output aspect ratio',
      'stretch' => 'Stretch (distort) to output aspect ratio'
    ),
    '#default_value' => (!empty($preset['settings']['video_aspectmode'])) ? $preset['settings']['video_aspectmode'] : 'preserve'
  );
  $form['preset']['settings']['video']['video_upscale'] = array(
    '#type' => 'checkbox',
    '#title' => t('Upscale'),
    '#description' => t('If the input file is smaller than the target output, should the file be upscaled to the target size?'),
    '#default_value' => !empty($preset['settings']['video_upscale']) ? $preset['settings']['video_upscale'] : ''
  );
  // audio settings
  $form['preset']['settings']['audio'] = array(
    '#type' => 'fieldset',
    '#title' => t('Audio settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['preset']['settings']['audio']['audio_codec'] = array(
    '#type' => 'select',
    '#title' => t('Audio codec'),
    '#description' => t('The audio codec to be used.'),
    '#options' => array(
      'aac' => 'AAC (default for most cases)',
      'mp3' => 'MP3',
      'vorbis' => 'Vorbis (default for VP8 and Theora)',
      'wma' => 'WMA'
    ),
    '#default_value' => (!empty($preset['settings']['audio_codec'])) ? $preset['settings']['audio_codec'] : 'h264'
  );
  $form['preset']['settings']['audio']['audio_quality'] = array(
    '#type' => 'select',
    '#title' => t('Audio quality'),
    '#description' => t('A target audio quality. Affects bitrate and file size.'),
    '#options' => array(
      '1 - Poor quality (smaller file)',
      '2',
      '3 (default)',
      '4',
      '5 - High quality (larger file)'
    ),
    '#default_value' => (!empty($preset['settings']['audio_quality'])) ? $preset['settings']['audio_quality'] : 2
  );
  // advanced video settings
  $form['preset']['settings']['adv_video'] = array(
    '#type' => 'fieldset',
    '#title' => t('Advanced video settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['preset']['settings']['adv_video']['deinterlace'] = array(
    '#type' => 'select',
    '#title' => t('Deinterlace'),
    '#description' => t('Note that detect mode will auto-detect and deinterlace interlaced content.'),
    '#options' => array(
      'detect' => 'Detect (default)',
      'on' => 'On (reduces quality of non-interlaced content)',
      'off' => 'Off'
    ),
    '#default_value' => (!empty($preset['settings']['deinterlace'])) ? $preset['settings']['deinterlace'] : 'detect'
  );
  $form['preset']['settings']['adv_video']['max_frame_rate'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum frame rate'),
    '#description' => t('A maximum frame rate cap (in frames per second).'),
    '#default_value' => !empty($preset['settings']['max_frame_rate']) ? $preset['settings']['max_frame_rate'] : ''
  );
  $form['preset']['settings']['adv_video']['frame_rate'] = array(
    '#type' => 'textfield',
    '#title' => t('Frame rate'),
    '#description' => t('Force a specific output frame rate (in frames per second). For best quality, do not use this setting.'),
    '#default_value' => !empty($preset['settings']['frame_rate']) ? $preset['settings']['frame_rate'] : ''
  );
  $form['preset']['settings']['adv_video']['frame_rate'] = array(
    '#type' => 'textfield',
    '#title' => t('Frame rate'),
    '#description' => t('Force a specific output frame rate (in frames per second). For best quality, do not use this setting.'),
    '#default_value' => !empty($preset['settings']['frame_rate']) ? $preset['settings']['frame_rate'] : ''
  );
  $form['preset']['settings']['adv_video']['video_bitrate'] = array(
    '#type' => 'textfield',
    '#title' => t('Video bitrate'),
    '#description' => t('A target bitrate in kbps. Not necessary if you select a Video Quality setting, unless you want to target a specific bitrate.'),
    '#default_value' => !empty($preset['settings']['video_bitrate']) ? $preset['settings']['video_bitrate'] : ''
  );
  $form['preset']['settings']['adv_video']['bitrate_cap'] = array(
    '#type' => 'textfield',
    '#title' => t('Bitrate cap'),
    '#description' => t('A bitrate cap in kbps, used for streaming servers.'),
    '#default_value' => !empty($preset['settings']['bitrate_cap']) ? $preset['settings']['bitrate_cap'] : ''
  );
  $form['preset']['settings']['adv_video']['buffer_size'] = array(
    '#type' => 'textfield',
    '#title' => t('Buffer size'),
    '#description' => t('The buffer size for the bitrate cap in kbps.'),
    '#default_value' => !empty($preset['settings']['buffer_size']) ? $preset['settings']['buffer_size'] : ''
  );
  $form['preset']['settings']['adv_video']['one_pass'] = array(
    '#type' => 'checkbox',
    '#title' => t('One pass'),
    '#description' => t('Force one-pass encoding when targeting a specific video_bitrate. True or False.'),
    '#default_value' => !empty($preset['settings']['one_pass']) ? $preset['settings']['one_pass'] : ''
  );
  $form['preset']['settings']['adv_video']['skip_video'] = array(
    '#type' => 'checkbox',
    '#title' => t('Skip video'),
    '#description' => t('The video track will be omitted from the output. You can still specify a video format, however, no video track will be present in the resulting file.'),
    '#default_value' => !empty($preset['settings']['skip_video']) ? $preset['settings']['skip_video'] : ''
  );
  // advanced audio settings
  $form['preset']['settings']['adv_audio'] = array(
    '#type' => 'fieldset',
    '#title' => t('Advanced audio settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['preset']['settings']['adv_audio']['audio_bitrate'] = array(
    '#type' => 'textfield',
    '#title' => t('Audio bitrate'),
    '#description' => t('The overall audio bitrate specified as kilobits per second (kbps, e.g. 96 or 160). This value can\'t exceed 160 kbps per channel. 96-160 is usually a good range for stereo output.'),
    '#default_value' => !empty($preset['settings']['audio_bitrate']) ? $preset['settings']['audio_bitrate'] : ''
  );
  $form['preset']['settings']['adv_audio']['audio_channels'] = array(
    '#type' => 'select',
    '#title' => t('Audio channels'),
    '#description' => t('By default we will choose the lesser of the number of audio channels in the input file or 2 (stereo).'),
    '#options' => array(
      1 => '1 - Mono',
      2 => '2 - Stereo (default)'
    ),
    '#default_value' => (!empty($preset['settings']['audio_channels'])) ? $preset['settings']['audio_channels'] : 2
  );
  $form['preset']['settings']['adv_audio']['audio_sample_rate'] = array(
    '#type' => 'textfield',
    '#title' => t('Audio sample rate'),
    '#description' => t('The sample rate of the audio in hertz. Manually setting this may cause problems, depending on the selected bitrate and number of channels.'),
    '#default_value' => !empty($preset['settings']['audio_sample_rate']) ? $preset['settings']['audio_sample_rate'] : ''
  );
  $form['preset']['settings']['adv_audio']['skip_audio'] = array(
    '#type' => 'checkbox',
    '#title' => t('Skip audio'),
    '#description' => t('The audio track will be omitted from the output. You must specify a video format and no audio track will be present in the resulting file.'),
    '#default_value' => !empty($preset['settings']['skip_audio']) ? $preset['settings']['skip_audio'] : ''
  );
  // video optimizations
  $form['preset']['settings']['vid_optimization'] = array(
    '#type' => 'fieldset',
    '#title' => t('Video Optimization'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['preset']['settings']['vid_optimization']['autolevels'] = array(
    '#type' => 'checkbox',
    '#title' => t('Autolevels'),
    '#description' => t('Automatic brightness / contrast correction.'),
    '#default_value' => !empty($preset['settings']['autolevels']) ? $preset['settings']['autolevels'] : ''
  );
  $form['preset']['settings']['vid_optimization']['deblock'] = array(
    '#type' => 'checkbox',
    '#title' => t('Deblock'),
    '#description' => t('Apply deblocking filter. Useful for highly compressed or blocky input videos.'),
    '#default_value' => !empty($preset['settings']['deblock']) ? $preset['settings']['deblock'] : ''
  );
  $form['preset']['settings']['vid_optimization']['denoise'] = array(
    '#type' => 'select',
    '#title' => t('Denoise'),
    '#description' => t('Apply denoise filter. Generally results in slightly better compression, and slightly slower encoding. Beware of any value higher than "Weak" (unless you\'re encoding animation).'),
    '#options' => array(
      0 => 'None',
      'weak' => 'Weak - usually OK for general use',
      'medium' => 'Medium',
      'strong' => 'Strong - beware',
      'strongest' => 'Strongest - beware, except for Anime'
    ),
    '#default_value' => (!empty($preset['settings']['denoise'])) ? $preset['settings']['denoise'] : 2
  );
  // Segmented Streaming
  $form['preset']['settings']['segmented_streaming'] = array(
    '#type' => 'fieldset',
    '#title' => t('Segmented Streaming'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['preset']['settings']['segmented_streaming']['segmented_type'] = array(
    '#type' => 'select',
    '#title' => t('Type'),
    '#description' => t('Set to segmented for HTTP Live Streaming.'),
    '#options' => array(
      0 => 'Standard',
      'segmented' => 'segmented'
    ),
    '#default_value' => (!empty($preset['settings']['segmented_type'])) ? $preset['settings']['segmented_type'] : 0
  );
  $form['preset']['settings']['segmented_streaming']['segmented_seconds'] = array(
    '#type' => 'textfield',
    '#title' => t('Segment Seconds'),
    '#description' => t('The maximum duration of each segment in segmented outputs.'),
    '#default_value' => !empty($preset['settings']['segmented_seconds']) ? $preset['settings']['segmented_seconds'] : ''
  );
  // Create clip
  $form['preset']['settings']['create_clip'] = array(
    '#type' => 'fieldset',
    '#title' => t('Create Clip'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['preset']['settings']['create_clip']['clip_start'] = array(
    '#type' => 'textfield',
    '#title' => t('Start clip'),
    '#description' => t('The starting point of a subclip (in timecode or number of seconds).'),
    '#default_value' => !empty($preset['settings']['clip_start']) ? $preset['settings']['clip_start'] : ''
  );
  $form['preset']['settings']['create_clip']['clip_length'] = array(
    '#type' => 'textfield',
    '#title' => t('Clip length '),
    '#description' => t('The length of the subclip (in timecode or number of seconds).'),
    '#default_value' => !empty($preset['settings']['clip_length']) ? $preset['settings']['clip_length'] : ''
  );
  // Command line parameters
  $form['preset']['settings']['command_line'] = array(
    '#type' => 'fieldset',
    '#title' => t('Command line options'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $tokes = array(
    '!cmd_path - Path to transcoder',
    '!videofile - Input video file',
    '!convertfile - Output video file',
    '!width - Width of output video',
    '!height - Height of output video'
  );
  $form['preset']['settings']['command_line']['cli_code'] = array(
    '#type' => 'textarea',
    '#title' => t('Command line codes to run'),
    '#description' => t('Please add command line codes each to run separated by a new line.') . theme('item_list', array('items' => $tokes, 'title' => 'Available Tokes')),
    '#default_value' => !empty($preset['settings']['cli_code']) ? $preset['settings']['cli_code'] : '!cmd_path -i !videofile -s !widthx!height -r 15 -b 250 -ar 22050 -ab 48 !convertfile'
  );
  return $form;
}