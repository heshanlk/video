<?php
// $Id$

/**
 * @file
 * Provides the administration settings for the Video Drupal module.
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
  // check ffmpeg_wrapper module installed or not
  if ($form_state['values']['vid_convertor'] == 'video_ffmpeg_wrapper' && !module_exists('ffmpeg_wrapper')) {
    form_set_error('vid_convertor', t('You need to download and install the !ffmpeg_wrapper module to enable this option.' , array('!ffmpeg_wrapper' => l(t('FFMPEG Wrapper'), 'http://drupal.org/project/ffmpeg_wrapper'))));
  }

  // @TODO : add validation with the API call back move to the API class
  $zencoder_api = variable_get('zencoder_api_key', null);
  if(isset ($zencoder_api) && !empty ($zencoder_api) || $form_state['values']['vid_convertor'] != 'video_zencoder')
    return true;

  $errors = false;
  // check terms and condition
  if($form_state['values']['agree_terms_zencoder'] == 0) {
    $errors = true;
    form_set_error('agree_terms_zencoder', t('You must agree terms and conditions.' , array()));
  }
  // check for email exists
  // Validate the e-mail address:
  if ($error = user_validate_mail($form_state['values']['zencoder_username'])) {
    $errors = true;
    form_set_error('zencoder_username', $error);
  }

  // get the API key from zencoder and save it to variable
  if(!$errors) {
    $email = $form_state['values']['zencoder_username'];
    module_load_include('inc', 'video_zencoder', '/includes/zencoder');
    $zc = new video_zencoder_api;
//    if(!($error = $zc->create_user($user)))
//            form_set_error('zencoder_username', $error);
    $user = new stdClass;
    $user->email = $email;
    $result = $zc->create_user($user);
    if($result !== true)
      form_set_error('zencoder_username', $result);

  }
}

function video_general_admin_settings() {
  $form = array();
  $form['video_autoplay'] = array(
    '#type' => 'checkbox',
    '#title' => t('Automatically start video on page load'),
    '#default_value' => variable_get('video_autoplay', TRUE),
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
    '#title' => t('Bypass Video Conversion'),
    '#default_value' => variable_get('video_bypass_conversion', FALSE),
    '#description' => t('Bypass video conversion when creating videos.')
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

function video_players_admin_settings() {
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

  foreach($extensions as $ext => $player) {
    $form['extensions']['video_extension_'.$ext] = array(
      '#type' => 'select',
      '#title' => t('Extension:'). '  '.$ext,
      '#default_value' => variable_get('video_extension_'.$ext, $player),
      '#options' => $players,
      '#prefix' => '<div class="video_select" rel="'.$ext.'">',
      '#suffix' => '</div>',
    );

    $form['extensions']['video_extension_'.$ext.'_flash_player'] = array(
      '#type' => !empty($flv_players) ? 'radios' : 'markup',
      '#title' => t('Flash Player for').' '.$ext,
      '#value' => !empty($flv_players) ? '' : t('No flash players detected.<br />You need to install !swf_tools or !flowplayer.', array('!swf_tools' => l(t('SWF Tools') , 'http://www.drupal.org/project/swftools'), '!flowplayer' => l(t('Flowplayer'), 'http://www.drupal.org/project/flowplayer'))),
      '#options' => $flv_players,
      '#default_value' => variable_get('video_extension_'.$ext.'_flash_player', ''),
      '#prefix' => '<div class="admin_flv_player_wrapper" id="flv_player_'.$ext.'">',
      '#suffix' => '</div>',
    );
  }
  return system_settings_form($form);
}

function video_metadata_admin_settings() {
  module_load_include('inc', 'video', '/includes/metadata');
  $metadata = new video_metadata;
  $form = $metadata->admin_settings();
  return system_settings_form($form);
}

function video_cron_admin_settings() {
  $form = array();
  $form['video_cron'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use Drupals built in cron.'),
    '#default_value' => variable_get('video_cron', FALSE),
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