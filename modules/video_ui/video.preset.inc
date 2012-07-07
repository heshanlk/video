<?php
/**
 * @file
 * Administrative interface for maintaining video presets.
 */

define('VIDEO_PRESET_MAX_LENGTH', 64);

/**
 * All the other settings that you need to declare in your preset.
 *
 * The preset object is passed to this form, so you can use that data to
 * populate the default values for your form elements by using
 * $preset['settings']['param2'].
 *
 * @param $preset
 *   The preset object.
 * @return
 *   The additional form settings that you would like to add to your preset.
 */
function video_preset_default_form($form, &$form_state, $preset) {
  $factory = new TranscoderAbstractionAbstractFactory();
  $transcoder = $factory->getProduct();
  $codecs = $transcoder->getCodecs();
  $formats = $transcoder->getAvailableFormats('muxing');
  $pixelformats = $transcoder->getPixelFormats();

  $form = array();

  // video settings
  $form['settings']['video'] = array(
    '#type' => 'fieldset',
    '#title' => t('Video settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE
  );

  $form['settings']['video']['video_extension'] = array(
    '#type' => 'select',
    '#title' => t('Video output extension'),
    '#description' => t('Extension of the output video.'),
    '#options' => $formats,
    '#default_value' => (!empty($preset['settings']['video_extension'])) ? $preset['settings']['video_extension'] : 'mp4',
    '#required' => TRUE,
  );

  $form['settings']['video']['video_codec'] = array(
    '#type' => 'select',
    '#title' => t('Video codec'),
    '#description' => t('The video codec used in the video file can affect the ability to play the video on certain devices.'),
    '#options' => $codecs['encode']['video'],
    '#default_value' => (!empty($preset['settings']['video_codec'])) ? $preset['settings']['video_codec'] : '',
  );

  $form['settings']['video']['video_preset'] = array(
    '#type' => 'select',
    '#title' => t('FFmpeg video preset'),
    '#description' => t('A preset file contains a sequence of option=value pairs, one for each line, specifying a sequence of options which would be awkward to specify on the command line. Lines starting with the hash (\'#\') character are ignored and are used to provide comments. Check the &quot;presets&quot; directory in the FFmpeg source tree for examples. See the !doc. Newer FFmpeg installations do not supply libx264 presets anymore, so &quot;!optionnamenone&quot; should be selected. If FFmpeg fails with an error related to presets, please also select &quot;!optionnamenone&quot;. In other cases, an error message may suggest that you should select one of the available options. This setting requires some experimentation.', array('!doc' => l(t('FFmpeg documentation'), url('http://ffmpeg.org/ffmpeg.html#Preset-files')), '!optionnamenone' => t('None'))),
    '#options' => array(
      '' => t('None'),
      'libx264-baseline' => 'libx264-baseline',
      'libx264-default' => 'libx264-default',
      'libx264-fast' => 'libx264-fast',
      'libx264-faster' => 'libx264-faster',
      'libx264-hq' => 'libx264-hq',
      'libx264-ipod320' => 'libx264-ipod320',
      'libx264-ipod640' => 'libx264-ipod640',
      'libx264-main' => 'libx264-main',
      'libx264-max' => 'libx264-max',
      'libx264-medium' => 'libx264-medium',
      'libx264-normal' => 'libx264-normal',
      'libx264-slow' => 'libx264-slow',
      'libx264-slower' => 'libx264-slower',
      'libx264-superfast' => 'libx264-superfast',
      'libx264-ultrafast' => 'libx264-ultrafast',
      'libx264-veryfast' => 'libx264-veryfast',
      'libvpx-1080p' => 'libvpx-1080p',
      'libvpx-1080p50_60' => 'libvpx-1080p50_60',
      'libvpx-360p' => 'libvpx-360p',
      'libvpx-720p' => 'libvpx-720p',
      'libvpx-720p50_60' => 'libvpx-720p50_60',
      'libx264-lossless_fast' => 'libx264-lossless_fast',
      'libx264-lossless_max' => 'libx264-lossless_max',
      'libx264-lossless_medium' => 'libx264-lossless_medium',
      'libx264-lossless_slow' => 'libx264-lossless_slow',
      'libx264-lossless_slower' => 'libx264-lossless_slower',
      'libx264-lossless_ultrafast' => 'libx264-lossless_ultrafast',
    ),
    '#default_value' => (!empty($preset['settings']['video_preset'])) ? $preset['settings']['video_preset'] : '',
  );

  $form['settings']['video']['video_quality'] = array(
    '#type' => 'select',
    '#title' => t('Video quality'),
    '#description' => t('A target video quality. Affects bitrate and file size.'),
    '#options' => array(
      'none' => t('None'),
      1 => '1 - Poor quality (smaller file)',
      2 => '2',
      3 => '3' . ' (' . t('default') . ')',
      4 => '4',
      5 => '5 - High quality (larger file)'
    ),
    '#default_value' => (!empty($preset['settings']['video_quality'])) ? $preset['settings']['video_quality'] : 3,
  );
  $form['settings']['video']['video_speed'] = array(
    '#type' => 'select',
    '#title' => t('Video speed'),
    '#description' => t('Speed of encoding. Affects compression.'),
    '#options' => array(
      'none' => t('None'),
      1 => '1 - Slow (better compression)',
      2 => '2',
      3 => '3' . ' (' . t('default') . ')',
      4 => '4',
      5 => '5 - Fast (worse compression)'
    ),
    '#default_value' => (!empty($preset['settings']['video_speed'])) ? $preset['settings']['video_speed'] : 3,
  );
  $form['settings']['video']['wxh'] = array(
    '#type' => 'select',
    '#title' => t('Dimensions'),
    '#description' => t('Select the desired widthxheight of the video player. You can add your own dimensions from !settings.', array('!settings' => l(t('Video metadata settings'), 'admin/config/media/video/metadata'))),
    '#default_value' => !empty($preset['settings']['wxh']) ? $preset['settings']['wxh'] : '640x480',
    '#options' => video_utility::getDimensions(),
  );
  $form['settings']['video']['video_aspectmode'] = array(
    '#type' => 'select',
    '#title' => t('Aspect mode'),
    '#description' => t('What to do when aspect ratio of input file does not match the target width/height aspect ratio.'),
    '#options' => array(
      'preserve' => t('Preserve aspect ratio') . ' (' . t('default') . ')',
      'crop' => t('Crop to fit output aspect ratio'),
      'pad' => t('Pad (letterbox) to fit output aspect ratio'),
      'stretch' => t('Stretch (distort) to output aspect ratio'),
    ),
    '#default_value' => (!empty($preset['settings']['video_aspectmode'])) ? $preset['settings']['video_aspectmode'] : 'preserve',
  );
  $form['settings']['video']['video_upscale'] = array(
    '#type' => 'checkbox',
    '#title' => t('Upscale'),
    '#description' => t('If the input file is smaller than the target output, should the file be upscaled to the target size?'),
    '#default_value' => !empty($preset['settings']['video_upscale']) ? $preset['settings']['video_upscale'] : ''
  );

  // audio settings
  $form['settings']['audio'] = array(
    '#type' => 'fieldset',
    '#title' => t('Audio settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['settings']['audio']['audio_codec'] = array(
    '#type' => 'select',
    '#title' => t('Audio codec'),
    '#description' => t('The audio codec to be used.'),
    '#options' => $codecs['encode']['audio'],
    '#default_value' => (!empty($preset['settings']['audio_codec'])) ? $preset['settings']['audio_codec'] : '',
  );
  $form['settings']['audio']['audio_quality'] = array(
    '#type' => 'select',
    '#title' => t('Audio quality'),
    '#description' => t('A target audio quality. Affects bitrate and file size.'),
    '#options' => array(
      '' => t('None'),
      1 => '1 - Poor quality (smaller file)',
      2 => '2',
      3 => '3' . ' (' . t('default') . ')',
      4 => '4',
      5 => '5 - High quality (larger file)'
    ),
    '#default_value' => (!empty($preset['settings']['audio_quality'])) ? $preset['settings']['audio_quality'] : 3,
  );

  // advanced video settings
  $form['settings']['adv_video'] = array(
    '#type' => 'fieldset',
    '#title' => t('Advanced video settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['settings']['adv_video']['deinterlace'] = array(
    '#type' => 'select',
    '#title' => t('Deinterlace'),
    '#description' => t('Note that detect mode will auto-detect and deinterlace interlaced content.'),
    '#options' => array(
      'detect' => 'Detect' . ' (' . t('default') . ')',
      'on' => 'On (reduces quality of non-interlaced content)',
      'off' => 'Off'
    ),
    '#default_value' => (!empty($preset['settings']['deinterlace'])) ? $preset['settings']['deinterlace'] : 'detect'
  );
  $form['settings']['adv_video']['max_frame_rate'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum frame rate'),
    '#description' => t('A maximum frame rate cap (in frames per second).'),
    '#default_value' => !empty($preset['settings']['max_frame_rate']) ? $preset['settings']['max_frame_rate'] : ''
  );
  $form['settings']['adv_video']['frame_rate'] = array(
    '#type' => 'textfield',
    '#title' => t('Frame rate'),
    '#description' => t('Force a specific output frame rate (in frames per second). For best quality, do not use this setting.'),
    '#default_value' => !empty($preset['settings']['frame_rate']) ? $preset['settings']['frame_rate'] : ''
  );
  $form['settings']['adv_video']['keyframe_interval'] = array(
    '#type' => 'textfield',
    '#title' => t('Key frame interval'),
    '#description' => t('By default, a keyframe will be created at most every 250 frames. Specifying a different keyframe interval will allow you to create more or fewer keyframes in your video. A greater number of keyframes will increase the size of your output file, but will allow for more precise scrubbing in most players. Keyframe interval should be specified as a positive integer. For example, a value of 100 will create a keyframe every 100 frames.'),
    '#default_value' => !empty($preset['settings']['keyframe_interval']) ? $preset['settings']['keyframe_interval'] : ''
  );
  $form['settings']['adv_video']['video_bitrate'] = array(
    '#type' => 'textfield',
    '#title' => t('Video bitrate'),
    '#description' => t('A target bitrate in kbps. Not necessary if you select a Video Quality setting, unless you want to target a specific bitrate.'),
    '#default_value' => !empty($preset['settings']['video_bitrate']) ? $preset['settings']['video_bitrate'] : '',
  );
  $form['settings']['adv_video']['bitrate_cap'] = array(
    '#type' => 'textfield',
    '#title' => t('Bitrate cap'),
    '#description' => t('A bitrate cap in kbps, used for streaming servers.'),
    '#default_value' => !empty($preset['settings']['bitrate_cap']) ? $preset['settings']['bitrate_cap'] : ''
  );
  $form['settings']['adv_video']['buffer_size'] = array(
    '#type' => 'textfield',
    '#title' => t('Buffer size'),
    '#description' => t('The buffer size for the bitrate cap in kbps.'),
    '#default_value' => !empty($preset['settings']['buffer_size']) ? $preset['settings']['buffer_size'] : ''
  );
  $form['settings']['adv_video']['one_pass'] = array(
    '#type' => 'checkbox',
    '#title' => t('Force one-pass encoding'),
    '#default_value' => !empty($preset['settings']['one_pass']) ? $preset['settings']['one_pass'] : ''
  );
  $form['settings']['adv_video']['skip_video'] = array(
    '#type' => 'checkbox',
    '#title' => t('Skip video'),
    '#description' => t('The video track will be omitted from the output. You can still specify a video format, however, no video track will be present in the resulting file.'),
    '#default_value' => !empty($preset['settings']['skip_video']) ? $preset['settings']['skip_video'] : ''
  );

  // Not all transcoders support setting the pixel format
  if (!empty($pixelformats)) {
    $pixelformatoptions = array('' => t('Same as input video'));
    $pixelformatoptions = array_merge($pixelformatoptions, array_combine($pixelformats, $pixelformats));
    $form['settings']['adv_video']['pixel_format'] = array(
      '#type' => 'select',
      '#title' => t('Pixel format'),
      '#description' => t('The pixel format of the output file. Yuv420p is a safe choice, yuvj420p is not supported by at least Google Chrome. If you select <em>!optionname</em> and the input video is yuvj420p, the output video will not be playable on Chrome.', array('!optionname' => t('Same as input video'))),
      '#options' => $pixelformatoptions,
      '#default_value' => !empty($preset['settings']['pixel_format']) ? $preset['settings']['pixel_format'] : '',
    );
  }
  // Copy the value if present so the value isn't lost when people change transcoders
  elseif (isset($preset['settings']['pixel_format'])) {
    $form['settings']['adv_video']['pixel_format'] = array(
      '#type' => 'value',
      '#value' => $preset['settings']['pixel_format'],
    );
  }

  $profiles = array('' => t('None'), 'baseline' => 'Baseline', 'main' => 'Main', 'high' => 'High');
  $form['settings']['adv_video']['h264_profile'] = array(
    '#type' => 'select',
    '#title' => t('H.264 profile'),
    '#description' => t('Use Baseline for maximum compatibility with players. Select !optionnamenone when this is not an H.264 preset or when setting the profile causes errors.', array('!optionnamenone' => t('None'))),
    '#options' => $profiles,
    '#default_value' => !empty($preset['settings']['h264_profile']) ? $preset['settings']['h264_profile'] : '',
  );

  // advanced audio settings
  $form['settings']['adv_audio'] = array(
    '#type' => 'fieldset',
    '#title' => t('Advanced audio settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['settings']['adv_audio']['audio_bitrate'] = array(
    '#type' => 'textfield',
    '#title' => t('Audio bitrate'),
    '#description' => t('The overall audio bitrate specified as kilobits per second (kbps, e.g. 96 or 160). This value can\'t exceed 160 kbps per channel. 96-160 is usually a good range for stereo output.'),
    '#default_value' => !empty($preset['settings']['audio_bitrate']) ? $preset['settings']['audio_bitrate'] : ''
  );
  $form['settings']['adv_audio']['audio_channels'] = array(
    '#type' => 'select',
    '#title' => t('Audio channels'),
    '#description' => t('By default we will choose the lesser of the number of audio channels in the input file or 2 (stereo).'),
    '#options' => array(
      1 => '1 - Mono',
      2 => '2 - Stereo' . ' (' . t('default') . ')'
    ),
    '#default_value' => (!empty($preset['settings']['audio_channels'])) ? $preset['settings']['audio_channels'] : 2
  );
  $form['settings']['adv_audio']['audio_sample_rate'] = array(
    '#type' => 'textfield',
    '#title' => t('Audio sample rate'),
    '#description' => t('The sample rate of the audio in hertz. Manually setting this may cause problems, depending on the selected bitrate and number of channels.'),
    '#default_value' => !empty($preset['settings']['audio_sample_rate']) ? $preset['settings']['audio_sample_rate'] : ''
  );
  $form['settings']['adv_audio']['skip_audio'] = array(
    '#type' => 'checkbox',
    '#title' => t('Skip audio'),
    '#description' => t('The audio track will be omitted from the output. You must specify a video format and no audio track will be present in the resulting file.'),
    '#default_value' => !empty($preset['settings']['skip_audio']) ? $preset['settings']['skip_audio'] : ''
  );

  // Watermark
  $form['settings']['watermark'] = array(
    '#type' => 'fieldset',
    '#title' => t('Video watermark'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#description' => t('At this moment this only works when using the Zencoder transcoder.'),
  );
  $form['settings']['watermark']['video_watermark_enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable watermark video'),
    '#default_value' => !empty($preset['settings']['video_watermark_enabled']) ? $preset['settings']['video_watermark_enabled'] : FALSE,
  );
  $destination = file_default_scheme() . '://videos/watermark';
  $form['settings']['watermark']['video_watermark_fid'] = array(
    '#type' => 'managed_file',
    '#title' => t('Upload watermark image'),
    '#description' => t('Watermark image should be a PNG or JPG image. The file will be uploaded to %destination.', array('%destination' => $destination)),
    '#default_value' => !empty($preset['settings']['video_watermark_fid']) ? $preset['settings']['video_watermark_fid'] : 0,
    '#upload_location' => $destination,
    '#upload_validators' => array('file_validate_extensions' => array('jpg png'), 'file_validate_is_image' => array()),
    '#states' => array(
      'visible' => array(
        ':input[id=edit-video-watermark-enabled]' => array('checked' => TRUE),
      ),
      'required' => array(
        ':input[id=edit-video-watermark-enabled]' => array('checked' => TRUE),
      ),
    ),
  );
  $form['settings']['watermark']['video_watermark_x'] = array(
    '#type' => 'textfield',
    '#title' => t('Left position'),
    '#description' => t('Where to place the watermark relative to the left of the video. Use a negative number to place the watermark relative to the right of the video.'),
    '#default_value' => isset($preset['settings']['video_watermark_x']) ? $preset['settings']['video_watermark_x'] : 5,
    '#size' => 10,
    '#maxlength' => 10,
    '#field_suffix' => 'px',
    '#states' => array(
      'visible' => array(
        ':input[id=edit-video-watermark-enabled]' => array('checked' => TRUE),
      ),
    ),
  );
  $form['settings']['watermark']['video_watermark_y'] = array(
    '#type' => 'textfield',
    '#title' => t('Top position'),
    '#description' => t('Where to place the watermark relative to the top of the video. Use a negative number to place the watermark relative to the bottom of the video.'),
    '#default_value' => isset($preset['settings']['video_watermark_y']) ? $preset['settings']['video_watermark_y'] : 5,
    '#size' => 10,
    '#maxlength' => 10,
    '#field_suffix' => 'px',
    '#states' => array(
      'visible' => array(
        ':input[id=edit-video-watermark-enabled]' => array('checked' => TRUE),
      ),
    ),
  );
  $form['settings']['watermark']['video_watermark_width'] = array(
    '#type' => 'textfield',
    '#title' => t('Width'),
    '#description' => t('The width of the watermark. Use pixels or append a % sign to indicate a percentage relative to the width of the video. If left empty, the width will be the original width maximized by the video width.'),
    '#default_value' => isset($preset['settings']['video_watermark_width']) ? $preset['settings']['video_watermark_width'] : '',
    '#size' => 10,
    '#maxlength' => 10,
    '#states' => array(
      'visible' => array(
        ':input[id=edit-video-watermark-enabled]' => array('checked' => TRUE),
      ),
    ),
  );
  $form['settings']['watermark']['video_watermark_height'] = array(
    '#type' => 'textfield',
    '#title' => t('Height'),
    '#description' => t('The height of the watermark. Use pixels or append a % sign to indicate a percentage relative to the height of the video. If left empty, the width will be the original height maximized by the video height.'),
    '#default_value' => isset($preset['settings']['video_watermark_width']) ? $preset['settings']['video_watermark_width'] : '',
    '#size' => 10,
    '#maxlength' => 10,
    '#states' => array(
      'visible' => array(
        ':input[id=edit-video-watermark-enabled]' => array('checked' => TRUE),
      ),
    ),
  );
  $form['settings']['watermark']['video_watermark_origin'] = array(
    '#title' => t('Origin'),
    '#type' => 'select',
    '#options' => array(
      'content' => t('content: visible video area') . ' (' . t('default') . ')',
      'frame' => t('frame: video area including padding'),
    ),
    '#default_value' => isset($preset['settings']['video_watermark_origin']) ? $preset['settings']['video_watermark_origin'] : 'content',
    '#states' => array(
      'visible' => array(
        ':input[id=edit-video-watermark-enabled]' => array('checked' => TRUE),
      ),
    ),
  );
  /*
  Not supported by Zencoder anyway
  $form['settings']['watermark']['video_watermark_onlyforaudio'] = array(
    '#type' => 'checkbox',
    '#title' => t('Only add watermark for audio files'),
    '#description' => t('Use this function to create video files using an audio input file and a static image.'),
    '#default_value' => !empty($preset['settings']['video_watermark_onlyforaudio']) ? $preset['settings']['video_watermark_onlyforaudio'] : FALSE,
    '#states' => array(
      'visible' => array(
        ':input[id=edit-video-watermark-enabled]' => array('checked' => TRUE),
      ),
    ),
  );
  */

  // video optimizations
  $form['settings']['vid_optimization'] = array(
    '#type' => 'fieldset',
    '#title' => t('Video optimization'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['settings']['vid_optimization']['autolevels'] = array(
    '#type' => 'checkbox',
    '#title' => t('Autolevels'),
    '#description' => t('Automatic brightness / contrast correction.'),
    '#default_value' => !empty($preset['settings']['autolevels']) ? $preset['settings']['autolevels'] : ''
  );
  $form['settings']['vid_optimization']['deblock'] = array(
    '#type' => 'checkbox',
    '#title' => t('Deblock'),
    '#description' => t('Apply deblocking filter. Useful for highly compressed or blocky input videos.'),
    '#default_value' => !empty($preset['settings']['deblock']) ? $preset['settings']['deblock'] : ''
  );
  $form['settings']['vid_optimization']['denoise'] = array(
    '#type' => 'select',
    '#title' => t('Denoise'),
    '#description' => t('Apply denoise filter. Generally results in slightly better compression and slightly slower encoding. Beware of any value higher than "Weak" (unless you\'re encoding animation).'),
    '#options' => array(
      '' => t('None'),
      'weak' => 'Weak - usually OK for general use',
      'medium' => 'Medium',
      'strong' => 'Strong - beware',
      'strongest' => 'Strongest - beware, except for Anime'
    ),
    '#default_value' => (!empty($preset['settings']['denoise'])) ? $preset['settings']['denoise'] : 2
  );

  // Create clip
  $form['settings']['create_clip'] = array(
    '#type' => 'fieldset',
    '#title' => t('Create clip'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['settings']['create_clip']['clip_start'] = array(
    '#type' => 'textfield',
    '#title' => t('Start clip'),
    '#description' => t('The starting point of a subclip (in timecode or number of seconds).'),
    '#default_value' => !empty($preset['settings']['clip_start']) ? $preset['settings']['clip_start'] : ''
  );
  $form['settings']['create_clip']['clip_length'] = array(
    '#type' => 'textfield',
    '#title' => t('Clip length'),
    '#description' => t('The length of the subclip (in timecode or number of seconds).'),
    '#default_value' => !empty($preset['settings']['clip_length']) ? $preset['settings']['clip_length'] : ''
  );

  return $form;
}

/**
 * Preset form
 */
function video_preset_form($form, &$form_state, $preset = FALSE) {
  $form = array();
  $form_state['preset'] = $preset;

  // basic preset details
  $form['preset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Preset information'),
    '#weight' => -10,
  );
  $form['preset']['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Preset name'),
    '#maxlength' => VIDEO_PRESET_MAX_LENGTH,
    '#description' => t('Spaces are NOT allowed. The only permitted punctuation is period, hyphen, apostrophe, and underscore.'),
    '#required' => TRUE,
    '#weight' => -10,
    '#element_validate' => array('_video_preset_name_validate'),
    '#default_value' => !empty($preset['name']) ? $preset['name'] : ''
  );
  $form['preset']['description'] = array(
    '#type' => 'textfield',
    '#title' => t('Description'),
    '#description' => t('Add a brief description to your preset name.'),
    '#weight' => -9,
    '#default_value' => !empty($preset['description']) ? $preset['description'] : ''
  );

  // Preset settings
  $form += video_preset_default_form($form, $form_state, $preset);

  // Buttons
  $form['actions'] = array(
      '#type' => 'actions',
      '#weight' => 40,
  );

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save preset'),
  );

  if ($preset) {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#submit' => array('video_preset_delete_submit'),
    );
  }
  else {
    $form['actions']['cancel'] = array(
      '#type' => 'link',
      '#title' => t('Cancel'),
      '#href' => 'admin/config/media/video/presets',
    );
  }

  // Add the form submit handler.
  $form['#submit'][] = 'video_preset_submit';
  $form['#validate'][] = 'video_preset_validate';
  return $form;
}

/**
 * Validation for the preset form.
 */
function video_preset_validate($form, &$form_state) {
  $v =& $form_state['values'];

  // Silently uncheck the checkbox if there is no watermark file
  if (!empty($v['video_watermark_enabled']) && empty($v['video_watermark_fid'])) {
    $v['video_watermark_enabled'] = 0;
  }
}

/**
 * Submit handler for the preset form.
 */
function video_preset_submit($form, &$form_state) {
  $old = isset($form_state['preset']) ? $form_state['preset'] : NULL;

  $preset = array();
  if ($old) {
    $preset['pid'] = $form_state['preset']['pid'];
  }

  $preset['name'] = $form_state['values']['name'];
  $preset['description'] = $form_state['values']['description'];
  // unset unwanted values saved to database
  unset($form_state['values']['pid'], $form_state['values']['name'], $form_state['values']['description'], $form_state['values']['submit'], $form_state['values']['delete'], $form_state['values']['form_build_id'], $form_state['values']['form_token'], $form_state['values']['form_id'], $form_state['values']['op']);
  $preset['settings'] = video_preset_array_flatten($form_state['values']);

  // Save this preset.
  $preset = video_preset_save($preset);

  // Save new watermark image if exists
  if (!empty($form_state['values']['video_watermark_fid'])) {
    $file = file_load($form_state['values']['video_watermark_fid']);
    if ($file->status != FILE_STATUS_PERMANENT) {
      $file->status = FILE_STATUS_PERMANENT;
      file_save($file);
      file_usage_add($file, 'video', 'preset', $preset['pid']);
    }
  }

  // Remove the old watermark if different
  if ($old && !empty($old['settings']['video_watermark_fid']) && $old['settings']['video_watermark_fid'] != $form_state['values']['video_watermark_fid']) {
    $oldfile = file_load($old['settings']['video_watermark_fid']);
    if (!empty($oldfile)) {
      file_usage_delete($oldfile, 'video');
      $oldfile->status = 0;
      file_save($oldfile);
    }
  }

  drupal_set_message(t('Preset %preset successfully saved.', array('%preset' => $preset['name'])));
  $form_state['redirect'] = 'admin/config/media/video/presets';
}

/**
 * Flatten the settings array.
 */
function video_preset_array_flatten($array, $return=array()) {
  foreach ($array as $key => $value) {
    if (is_array($value)) {
      $return = video_preset_array_flatten($value, $return);
    }
    else {
      $return[$key] = $value;
    }
  }
  return $return;
}

/**
 * Button submit function: handle the 'Delete' button on the preset form.
 */
function video_preset_delete_submit($form, &$form_state) {
  $preset['name'] = $form_state['values']['name'];
  $form_state['redirect'] = 'admin/config/media/video/presets/preset/' . rawurlencode($preset['name']) . '/delete';
}

/**
 * Menu callback; delete a single preset.
 */
function video_preset_delete_confirm($form, &$form_state, $preset) {
  $form['name'] = array('#type' => 'value', '#value' => $preset['name']);
  return confirm_form($form, t('Are you sure you want to delete the preset %name?', array('%name' => $preset['name'])), 'admin/config/media/video/presets', '<p>' . t('This action cannot be undone.') . '</p>', t('Delete'));
}

/**
 * Execute preset deletion
 */
function video_preset_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    video_preset_delete($form_state['values']['name']);
  }

  $form_state['redirect'] = 'admin/config/media/video/presets';
}

/**
 * Validation for the preset form.
 */
function _video_preset_name_validate($element, &$form_state) {
  $error = video_validate_preset_name($form_state['values']['name']);
  if ($error && !isset($form_state['preset'])) {
    form_set_error('name', $error);
  }
}

/**
 * Imports a preset.
 */
function video_preset_import_form($form, &$form_state) {
  $form['preset'] = array(
    '#title' => t('Preset code'),
    '#type' => 'textarea',
    '#rows' => 10,
    '#description' => t('Copy the export text and paste it into this text field to import a new preset.'),
    '#wysiwyg' => FALSE,
  );

  $form['actions'] = array(
    '#type' => 'actions',
  );

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import preset'),
  );

  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => 'admin/config/media/video/presets',
  );

  $form['#submit'][] = 'video_preset_import_submit';
  $form['#validate'][] = 'video_preset_import_validate';

  return $form;
}

/**
 * Validation for the preset import form.
 */
function video_preset_import_validate($form, &$form_state) {
  $preset = '';

  // Get the preset that they declared in the text field.
  ob_start();
  eval($form_state['values']['preset']);
  ob_end_clean();

  if (is_array($preset)) {
    $name = isset($preset['name']) ? $preset['name'] : '';
    if ($error = video_validate_preset_name($name)) {
      form_set_error('name', $error);
    }
  }
  else {
    form_set_error('name', 'Invalid preset import.');
  }

  $form_state['preset'] = &$preset;
}

/**
 * Submit handler for the preset import form.
 */
function video_preset_import_submit($form, &$form_state) {
  $preset = $form_state['preset'];
  video_preset_save($preset);
  $form_state['redirect'] = 'admin/config/media/video/presets/preset/' . $preset['name'];
}

/**
 * The preset export form.
 */
function video_preset_export_form($form, &$form_state, $preset) {
  // Unset the unnecessary elements.
  unset($preset['pid']);
  unset($preset['default']);

  // @todo: support watermarks for export
  if (!empty($preset['settings']['video_watermark_fid'])) {
    unset($preset['settings']['video_watermark_fid']);
    $preset['settings']['video_watermark_enabled'] = 0;
    drupal_set_message(t('Unfortunately, exporting the watermark is not yet possible. After importing the preset you should upload your watermark file again.'), 'warning');
  }

  // Get the code string representation.
  $code = var_export($preset, TRUE);

  // Make sure to format the arrays like drupal.
  $code = str_replace("=> \n  ", '=> ', $code);

  // Add the preset variable.
  $code = '$preset = ' . $code . ';';

  $lines = substr_count($code, "\n");
  $form['export'] = array(
    '#title' => t('Export preset'),
    '#type' => 'textarea',
    '#value' => $code,
    '#rows' => $lines,
    '#description' => t('Copy the export text and paste it into another preset using the import function.'),
    '#wysiwyg' => FALSE,
  );

  // Return the form.
  return $form;
}

/**
 * Get's the player settings for the given player.
 */
function video_player_settings_form($preset) {
  $settings_form = $preset['player'] . '_player_settings_form';
  if (function_exists($settings_form)) {
    return $settings_form($preset);
  }
  return array();
}

/**
 * Title callback for a preset.
 */
function video_preset_page_title($preset) {
  return $preset['name'];
}

/**
 * Saves a new preset.
 */
function video_preset_save($preset) {
  // If they provide the pid, then this needs to be an update.
  $pid = (isset($preset['pid']) && $preset['pid']) ? array('pid') : array();
  $object = (object)$preset;

  // Save or update a preset.
  drupal_write_record('video_preset', $object, $pid);

  $preset['pid'] = $object->pid;

  return $preset;
}

/**
 * Returns the default presets.
 */
function video_preset_get_default_presets() {
  $default_presets = module_invoke_all('video_default_presets');
  drupal_alter('video_default_presets', $default_presets);
  return $default_presets;
}

/**
 * Gets a list of all presets.
 */
function video_preset_get_presets() {
  $presets = array();
  $normal_presets = array();

  // Get all the presets from the database.
  $result = db_select('video_preset', 'p')
    ->fields('p')
    ->orderBy('p.name', 'ASC')
    ->execute();

  // Iterate through all the presets and structure them in an array.
  foreach ($result as $preset) {
    $preset = (array) $preset;
    $preset['default'] = FALSE;
    $preset['settings'] = $preset['settings'] ? unserialize($preset['settings']) : array();
    $presets[$preset['pid']] = $preset;
    $normal_presets[$preset['name']] = $preset['pid'];
  }

  // Now allow other modules to add their default presets.
  foreach (video_preset_get_default_presets() as $preset) {
    // adding default TRUE
    $preset['default'] = TRUE;
    if (!empty($preset['name']) && !isset($normal_presets[$preset['name']])) {
      $presets[$preset['name']] = $preset;
    }
  }

  return $presets;
}

/**
 * Get's the preset.
 */
function video_get_preset($preset_name) {
  // Get the preset from the database.
  $preset = db_select('video_preset', 'p')
    ->fields('p')
    ->condition('p.name', $preset_name)
    ->execute()
    ->fetchAssoc();
  if ($preset) {
    $preset['settings'] = $preset['settings'] ? unserialize($preset['settings']) : array();
  }
  else {
    // Get all of the default presets.
    $default_presets = video_preset_get_default_presets();

    // Check to see if this preset exists.
    if (isset($default_presets[$preset_name])) {
      // Make this our preset
      $preset = $default_presets[$preset_name];
    }
  }

  // Return the preset.
  return $preset;
}

/**
 * Deletes a preset
 */
function video_preset_delete($preset_name) {
  $preset = video_get_preset($preset_name);
  if (!empty($preset['settings']['video_watermark_fid'])) {
    $file = file_load($preset['settings']['video_watermark_fid']);
    if (!empty($file)) {
      file_usage_delete($file, 'video');
      $file->status = 0;
      file_save($file);
    }
  }

  // Delete the preset.
  db_delete('video_preset')
    ->condition('name', $preset_name)
    ->execute();

  // Rebuild Theme Registry
  drupal_theme_rebuild();
}

/**
 * Checks to see if another preset is already taken.
 */
function video_preset_name_exists($preset_name) {
  // Get the default presets.
  $default_presets = video_preset_get_default_presets();

  // See if there is a default preset name.
  if ($default_presets && isset($default_presets[$preset_name])) {
    return TRUE;
  }
  else {
    return (bool) db_select('video_preset', 'p')
    ->fields('p')
    ->condition('p.name', $preset_name)
    ->range(0, 1)
    ->execute()
    ->fetchField();
  }
}

/**
 * Verify the syntax of the given prefix name.
 *
 * Borrowed from the user.module.   :)
 */
function video_validate_preset_name($name) {
  if (!$name) {
    return t('You must enter a preset.');
  }
  if (video_preset_name_exists($name)) {
    return t('The preset name %name is already taken.', array('%name' => $name));
  }
  if (strpos($name, ' ') !== FALSE) {
    return t('The preset name cannot contain a space.');
  }
  if (is_numeric($name[0])) {
    return t('The preset name cannot begin with a number.');
  }
  if (preg_match('/[^a-z0-9_]/i', $name)) {
    return t('The preset name contains an illegal character.');
  }
  if (drupal_strlen($name) > VIDEO_PRESET_MAX_LENGTH) {
    return t('The preset name %name is too long: it must be %max characters or less.', array('%name' => $name, '%max' => VIDEO_PRESET_MAX_LENGTH));
  }
}

/**
 * Check for default preset
 */
function video_is_default_preset($preset_name) {
  // Get the default presets.
  $default_presets = video_preset_get_default_presets();

  // See if there is a default preset name.
  return ($default_presets && isset($default_presets[$preset_name]));
}
