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
  $form['mp4_play_in_flowplayer'] = array(
      '#type' => 'checkbox',
      '#title' => t('Play MP4 files in the Flash Player'),
      '#default_value' => variable_get('mp4_play_in_flowplayer', FALSE),
      '#description' => t('Do you want to play .mp4 files in the flash player?  These files must be H264 encoded in order to work.')
  );
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