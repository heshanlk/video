<?php
/**
 * @file
 * Drush commands for the Video module
 */

/**
 * Implementation of hook_drush_command().
 */
function video_drush_command() {
  $items = array();

  $items['video-scheduler'] = array(
    'description' => 'Run video transcoder scheduler',
    'callback' => 'drush_video_scheduler',
    'drupal dependencies' => array('video'),
    'options' => array(
      '--limit' => 'Change the number of video items to transcode',
    ),
  );

  return $items;
}

function drush_video_scheduler() {
  $limit = (int) drush_get_option('limit', variable_get('video_queue_batchsize', 5));

  // Initialize the theme so it does not initialize during the script,
  // when the working directory may have been changed.
  init_theme();

  video_run_queue($limit);
}