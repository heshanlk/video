<?php
// $Id$

/**
 * @file
 * Provides the administration settings for the Flowplayer Drupal module.
 */

/**
 * Settings Form
 *
 * @return
 *   string of form content or error message
 */
function video_admin_settings() {
  global $base_url;

  $form = array();

  $form['menu'] = array(
      '#type' => 'fieldset',
      '#title' => t('General Behavior'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE
  );

  //  $form['menu']['video_displaydownloadmenutab'] = array(
  //    '#type' => 'checkbox',
  //    '#title' => t('Display download menu tab'),
  //    '#default_value' => variable_get('video_displaydownloadmenutab', 1),
  //    '#description' => t('Toggle display of menu tab to download video from the node page.')
  //  );
  //  $form['menu']['video_displaydownloadlink'] = array(
  //    '#type' => 'checkbox',
  //    '#title' => t('Display download link'),
  //    '#default_value' => variable_get('video_displaydownloadlink', 1),
  //    '#description' => t('Toggle display of "download" link (below the node content in most themes).')
  //  );
  $form['menu']['video_displayplaytime'] = array(
      '#type' => 'checkbox',
      '#title' => t('Display playtime'),
      '#default_value' => variable_get('video_displayplaytime', 1),
      '#description' => t('Toggle the display of the playtime for a video.')
  );
  $form['menu']['video_displayfilesize'] = array(
      '#type' => 'checkbox',
      '#title' => t('Display filesize'),
      '#default_value' => variable_get('video_displayfilesize', 1),
      '#description' => t('Toggle the display of the filesize for a video.')
  );
  $form['menu']['video_autoplay'] = array(
      '#type' => 'checkbox',
      '#title' => t('Automatically start video on page load'),
      '#default_value' => variable_get('video_autoplay', TRUE),
      '#description' => t('Start the video when the page and video loads')
  );

  $form['ffmpeg'] = array(
      '#type' => 'fieldset',
      '#title' => t('Advanced Configurations'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE
  );

  


variable_get('video_thumb_path', 'video_thumbs');
variable_get('no_of_video_thumbs', 5);
variable_get('video_ffmpeg_helper_ffmpeg_path', '/usr/bin/ffmpeg');
variable_get('video_image_thumbnailer_options', '-i %videofile -an -y -f mjpeg -ss %seek -vframes 1 %thumbfile');

  return system_settings_form($form);
}

/**
 * Form API callback to validate the upload settings form.
 *
 * Keeps the use from showing the play tab or the play link
 * if they have chosen to display the video in the node body.
 *
 *
 */
function video_admin_settings_validate($form, &$form_state) {
  ; // for future use
}
?>
