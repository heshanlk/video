<?php
/**
 * @file
 * Provides the administration settings for the Video Drupal module.
 */
video_add_adminjs();

/**
 * Video transcoder admin settings
 * @return <type>
 */
function video_transcoder_admin_settings() {
  module_load_include('inc', 'video', '/includes/transcoder');
  $transcoder = new video_transcoder;
  $form = $transcoder->admin_settings();
  return system_settings_form($form);
}

/**
 * Form API callback to validate the upload settings form.
 */
function video_transcoder_admin_settings_validate($form, &$form_state) {
  // add validations by transcoder interface
  $transcoder = $form_state['values']['vid_convertor'];
  module_load_include('inc', 'video', '/includes/transcoder');
  $transcoder = new video_transcoder($transcoder);
  $transcoder->admin_settings_validate($form, $form_state);
}

/**
 * Video transcoder admin settings
 * @return <type>
 */
function video_preset_admin_settings() {
  module_load_include('inc', 'video', '/includes/preset');
  $preset = new video_preset();
  $form = $preset->admin_settings();
  return system_settings_form($form);
}

/**
 * Video general admin settings
 * @return <type>
 */
function video_general_admin_settings() {
  $form = array();
  $form['video_autoplay'] = array(
    '#type' => 'checkbox',
    '#title' => t('Automatically start video on page load'),
    '#default_value' => variable_get('video_autoplay', FALSE),
    '#description' => t('Start the video when the page and video loads')
  );
  $form['video_autobuffering'] = array(
    '#type' => 'checkbox',
    '#title' => t('Automatically start video buffering'),
    '#default_value' => variable_get('video_autobuffering', TRUE),
    '#description' => t('Start buffering video when the page and video loads')
  );
  $form['video_bypass_conversion'] = array(
    '#type' => 'checkbox',
    '#title' => t('Bypass auto conversion by default'),
    '#default_value' => variable_get('video_bypass_conversion', FALSE),
    '#description' => t('Bypass video conversion when creating videos. This setting can be overriden by users with the %perm permission.', array('%checkbox' => t('Bypass auto conversion'), '%perm' => t('bypass conversion video')))
  );
  $form['video_convert_on_save'] = array(
    '#type' => 'checkbox',
    '#title' => t('Video Convert on Node Submit'),
    '#default_value' => variable_get('video_convert_on_save', FALSE),
    '#description' => t('Convert videos on node submit.')
  );
  $form['video_use_default_thumb'] = array(
    '#type' => 'checkbox',
    '#title' => t('Override Auto Thumbnails with Default'),
    '#default_value' => variable_get('video_use_default_thumb', FALSE),
    '#description' => t('Override auto thumbnails with default thumbnail.')
  );
  return system_settings_form($form);
}

/**
 * Video player admin settings
 * @return <type>
 */
function video_players_admin_settings($form_state) {
  // Check for SWF Tools and Amazon S3 Private URLs
  if (module_exists('video_s3') && module_exists('swftools') && variable_get('vid_filesystem', NULL) == 'video_s3' && variable_get('amazon_s3_private', FALSE)) {
    drupal_set_message(t('Using the Flowplayer FLV player via SWF Tools does not work when the S3 %setting-name setting is turned on. Install and enable the <a href="@flowplayer-api-url">Flowplayer API</a> module to use Flowplayer.', array('%setting-name' => t('Enable private file storage'), '@flowplayer-api-url' => 'http://drupal.org/project/flowplayer')), 'warning');
  }

  $form = array();
  $form['extensions'] = array(
    '#type' => 'fieldset',
    '#title' => t('Video Extensions'),
    '#description' => t('Here you can map specific players to each video extension type.'),
  );
  //lets get all our supported extensions and players.
  $extensions = video_video_extensions();
  $players = video_video_players();
  $flv_players = video_video_flv_players();
  $html5_players = video_video_html5_players();

  foreach ($extensions as $ext => $player) {
    $form['extensions']['video_extension_' . $ext] = array(
      '#type' => 'select',
      '#title' => t('Extension:') . '  ' . $ext,
      '#default_value' => variable_get('video_extension_' . $ext, $player),
      '#options' => $players,
      '#prefix' => '<div class="video_select" rel="' . $ext . '">',
      '#suffix' => '</div>',
    );

    $form['extensions']['video_extension_' . $ext . '_flash_player'] = array(
      '#type' => !empty($flv_players) ? 'radios' : 'markup',
      '#title' => t('Flash Player for') . ' ' . $ext,
      '#value' => !empty($flv_players) ? '' : t('No flash players detected.<br />You need to install !swf_tools or !flowplayer.', array('!swf_tools' => l(t('SWF Tools'), 'http://www.drupal.org/project/swftools'), '!flowplayer' => l(t('Flowplayer'), 'http://www.drupal.org/project/flowplayer'))),
      '#options' => $flv_players,
      '#default_value' => variable_get('video_extension_' . $ext . '_flash_player', ''),
      '#prefix' => '<div class="admin_flv_player_wrapper" id="flv_player_' . $ext . '">',
      '#suffix' => '</div>',
    );
    $form['extensions']['video_extension_' . $ext . '_html5_player'] = array(
      '#type' => !empty($html5_players) ? 'radios' : 'markup',
      '#title' => t('HTML5 Player for') . ' ' . $ext,
      '#value' => !empty($html5_players) ? '' : t('No HTML5 players detected.<br />You need to install !videojs.', array('!videojs' => l(t('Video.js'), 'http://www.drupal.org/project/videojs'), '!flowplayer' => l(t('Flowplayer'), 'http://www.drupal.org/project/flowplayer'))),
      '#options' => $html5_players,
      '#default_value' => variable_get('video_extension_' . $ext . '_html5_player', ''),
      '#prefix' => '<div class="admin_html5_player_wrapper" id="html5_player_' . $ext . '">',
      '#suffix' => '</div>',
    );
  }
  
  $form['extra'] = array(
    '#type' => 'fieldset',
    '#title' => t('Player Settings'),
  );
  
  if (isset($flv_players['flowplayer'])) {
    $form['extra']['video_flowplayer_extraplayerheight'] = array(
      '#type' => 'textfield',
      '#title' => t('Flowplayer: add vertical space to accommodate the control bar'),
      '#field_suffix' => 'pixels',
      '#default_value' => variable_get('video_flowplayer_extraplayerheight', 24),
      '#description' => t('The control bar in Flowplayer 3.2 and up uses an overlay on top of the video, so the player size is equal to the video size. For custom control bars or older Flowplayer versions you can use this field to add vertical space to the player height.'),
      '#size' => 5,
      '#maxlength' => 3,
      '#element_validate' => array('_video_validate_number'),
    );
  }
  
  if (count($form['extra']) == 2) {
    unset($form['extra']);
  }
  
  return system_settings_form($form);
}

function _video_validate_number($element, $form_state) {
  $key = end($element['#parents']);
  $val = $form_state['values'][$key];
  
  if (!empty($val) && (!preg_match('#^\d+$#', $val) || intval($val) < 0)) {
    form_error($element, t('You must enter an integer value for field %field.', array('%field' => $element['#title'])));
  }
}

/**
 * Video Metadata admin settings
 * @return <type>
 */
function video_metadata_admin_settings() {
  module_load_include('inc', 'video', '/includes/metadata');
  $metadata = new video_metadata;
  $form = $metadata->admin_settings();
  return system_settings_form($form);
}

function video_metadata_admin_settings_validate($form, &$form_state) {
  // add vallidations by metadata interface
  $metadata = $form_state['values']['vid_metadata'];
  module_load_include('inc', 'video', '/includes/metadata');
  $metadata = new video_metadata($metadata);
  $metadata->admin_settings_validate($form, $form_state);
}

/**
 * Video cron admin settings
 * @return <type>
 */
function video_cron_admin_settings() {
  $form = array();
  $form['video_cron'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use Drupals built in cron.'),
    '#default_value' => variable_get('video_cron', TRUE),
    '#description' => t('If you would like to use Drupals built in cron hook, check this box.  Please be warned that transcoding videos is very resource intensive.  If you use poor mans cron, I highly discourage this option.  I also suggest you setup your cron to call this function through CLI instead of WGET.'),
  );
  $form['video_ffmpeg_instances'] = array(
    '#type' => 'textfield',
    '#title' => t('Total videos to convert during each cron process.'),
    '#default_value' => variable_get('video_ffmpeg_instances', 5),
    '#description' => t('How many videos do you want to process on each cron run?  Either through hook_cron or the video_scheduler.php.'),
  );
  return system_settings_form($form);
}

/**
 * File system admin settings
 * @return <type>
 */
function video_filesystem_admin_settings() {
  module_load_include('inc', 'video', '/includes/filesystem');
  $filesystem = new video_filesystem;
  $form = $filesystem->admin_settings();
  return system_settings_form($form);
}

function video_filesystem_admin_settings_validate($form, &$form_state) {
  // add vallidations by metadata interface
  $filesystem = $form_state['values']['vid_filesystem'];
  module_load_include('inc', 'video', '/includes/filesystem');
  $filesystem = new video_filesystem($filesystem);
  $filesystem->admin_settings_validate($form, $form_state);
}

/**
 * Return our possible flash players.
 */
function video_video_flv_players() {
  $options = array();
  if (module_exists('swftools')) {
    $options['swftools'] = t('SWF Tools');
  }
  if (module_exists('flowplayer')) {
    $options['flowplayer'] = t('Flowplayer');
  }
  return $options;
}

/**
 * Return our possible html5 players.
 */
function video_video_html5_players() {
  $options = array();
  if (module_exists('videojs')) {
    $options['videojs'] = t('Video.js');
  }
  if (module_exists('video')) {
    $options['video'] = t('Video (Default)');
  }
  return $options;
}
