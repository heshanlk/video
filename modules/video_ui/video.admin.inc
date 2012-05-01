<?php
/**
 * @file
 * Provides the administration settings for the Video Drupal module.
 */

/**
 * Video transcoder admin settings
 */
function video_transcoder_admin_settings() {
  $transcoder = new Transcoder;
  $form = $transcoder->adminSettings();
  return system_settings_form($form);
}

/**
 * Form API callback to validate the upload settings form.
 */
function video_transcoder_admin_settings_validate($form, &$form_state) {
  // add validations by transcoder interface
  $transcoder = $form_state['values']['video_convertor'];
  $factory = new TranscoderAbstractionAbstractFactory();
  $transcoder = $factory->getProduct($transcoder);
  $transcoder->adminSettingsValidate($form, $form_state);
}

/**
 * Video preset admin settings
 */
function video_preset_admin_settings() {
  $preset = new Preset();
  $form = $preset->adminSettings();
  return system_settings_form($form);
}

/**
 * Video general admin settings
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
    '#title' => t('Bypass video conversion'),
    '#default_value' => variable_get('video_bypass_conversion', FALSE),
    '#description' => t('Bypass video conversion when creating videos.')
  );
  $form['video_convert_on_save'] = array(
    '#type' => 'checkbox',
    '#title' => t('Video convert on node submit'),
    '#default_value' => variable_get('video_convert_on_save', FALSE),
    '#description' => t('Convert videos on node submit will enable by default for all users.')
  );
  $form['video_use_default_thumb'] = array(
    '#type' => 'checkbox',
    '#title' => t('Override video thumbnails with default thumbnail'),
    '#default_value' => variable_get('video_use_default_thumb', FALSE),
    '#description' => t('Override auto thumbnails with default thumbnail.')
  );
  $form['video_publish_on_complete'] = array(
    '#type' => 'checkbox',
    '#title' => t('Publish when conversion complete'),
    '#default_value' => variable_get('video_publish_on_complete', FALSE),
    '#description' => t('This feature is now fully controlled by !rules module. Download the module and configure rules to take effect when video conversions have completed or failed.', array('!rules' => l(t('Rules'), 'http://drupal.org/project/rules'))),
    '#disabled' => TRUE
  );
  return system_settings_form($form);
}

/**
 * Video player admin settings
 */
function video_players_admin_settings() {
  $form = array();
  $form['extensions'] = array(
    '#type' => 'fieldset',
    '#title' => t('Video extensions'),
    '#description' => t('Here you can map specific players to each video extension type.'),
  );
  // lets get all our supported extensions and players.
  $extensions = video_utility::getVideoExtensions();
  $players = video_utility::getVideoPlayers();
  $flv_players = video_video_flv_players();
  $html5_players = video_video_html5_players();

  foreach ($extensions as $ext => $player) {
    $form['extensions']['video_extension_' . $ext] = array(
      '#type' => 'select',
      '#title' => t('Extension:') . '  ' . $ext,
      '#default_value' => variable_get('video_extension_' . $ext, $player),
      '#options' => $players,
      '#required' => TRUE,
    );
    // For flash
    if (!empty($flv_players)) {
      $value = variable_get('video_extension_' . $ext . '_flash_player', '');
      if (empty($value) || !isset($flv_players[$value])) {
        $value = key($flv_players);
      }
      $form['extensions']['video_extension_' . $ext . '_flash_player'] = array(
        '#type' => 'radios',
        '#title' => t('Flash player for @extension', array('@extension' => $ext)),
        '#options' => $flv_players,
        '#default_value' => $value,
        '#required' => TRUE,
        '#states' => array(
          'visible' => array(
            ':input[id=edit-video-extension-' . $ext . ']' => array('value' => 'video_play_flv'),
          ),
        ),
      );
    }
    else {
      $form['extensions']['video_extension_' . $ext . '_flash_player']['type'] = array(
        '#type' => 'markup',
        '#markup' => t('No flash players detected.<br />You need to install !flowplayer or !jwplayer.', array('!flowplayer' => l(t('Flowplayer API'), 'http://www.drupal.org/project/flowplayer'), '!jwplayer' => l(t('JW Player'), 'http://www.drupal.org/project/jw_player'))),
        '#states' => array(
          'visible' => array(
            ':input[id=edit-video-extension-' . $ext . ']' => array('value' => 'video_play_flv'),
          ),
        ),
      );
    }
    // for HTML5
    if (!empty($html5_players)) {
      $value = variable_get('video_extension_' . $ext . '_html5_player', '');
      if (empty($value) || !isset($html5_players[$value])) {
        $value = key($html5_players);
      }
      $form['extensions']['video_extension_' . $ext . '_html5_player'] = array(
        '#type' => 'radios',
        '#title' => t('HTML5 player for @extension', array('@extension' => $ext)),
        '#options' => $html5_players,
        '#markup' => t('Additional HTML5 players module.<br />You can install !videojs.', array('!videojs' => l(t('Video.js'), 'http://drupal.org/project/videojs'))),
        '#default_value' => $value,
        '#required' => TRUE,
        '#states' => array(
          'visible' => array(
            ':input[id=edit-video-extension-' . $ext . ']' => array('value' => 'video_play_html5'),
          ),
        ),
      );
    }
  }
  return system_settings_form($form);
}

/**
 * Video Metadata admin settings
 */
function video_metadata_admin_settings() {
  $metadata = new Metadata;
  $form = $metadata->adminSettings();
  return system_settings_form($form);
}

function video_metadata_admin_settings_validate($form, &$form_state) {
  // add vallidations by metadata interface
  $metadata = $form_state['values']['video_metadata'];
  $metadata = new Metadata($metadata);
  $metadata->adminSettingsValidate($form, $form_state);
}

/**
 * Video cron admin settings
 */
function video_cron_admin_settings() {
  $form = array();
  $form['video_cron'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use Drupal\'s built-in cron.'),
    '#default_value' => variable_get('video_cron', TRUE),
    '#description' => t('Check this box to use Drupal\'s built in cron hook. Be aware that video transcoding is very resource-intensive. If you use poor man\'s cron, this option is strongly discouraged. We suggest you set up your cron to call this function through CLI instead of WGET.'),
  );
  $form['video_queue_timeout'] = array(
    '#type' => 'textfield',
    '#title' => t('Video queue timeout (s).'),
    '#default_value' => variable_get('video_queue_timeout', 90),
    '#description' => t('The maximum time allowed for a video transcode to complete. Use a larger value for larger-sized video.'),
  );
  $form['video_ffmpeg_instances'] = array(
    '#type' => 'textfield',
    '#title' => t('Total videos to convert during each cron process.'),
    '#default_value' => variable_get('video_ffmpeg_instances', 5),
    '#description' => t('How many videos do you want to process on each cron run?  Either through hook_cron or video_scheduler.php.'),
  );
  return system_settings_form($form);
}

/**
 * Return our possible flash players.
 *
 * When extending this method, also change the error message in
 * video_players_admin_settings().
 */
function video_video_flv_players() {
  $options = array();
  if (module_exists('flowplayer')) {
    $options['flowplayer'] = t('Flowplayer');
  }
  if (module_exists('jw_player')) {
    $options['jwplayer'] = t('JW Player');
  }
  return $options;
}

/**
 * Return our possible flash players.
 */
function video_video_html5_players() {
  $options = array();
  if (module_exists('videojs')) {
    $options['videojs'] = t('Video.js');
  }
  if (module_exists('video')) {
    $options['video'] = t('Video for everybody');
  }
  return $options;
}