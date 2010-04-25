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
  return system_settings_form($form);
}

function video_metadata_admin_settings() {
  module_load_include('inc', 'video', '/includes/metadata');
  $metadata = new video_metadata;
  $form = $metadata->admin_settings();
  return system_settings_form($form);
}