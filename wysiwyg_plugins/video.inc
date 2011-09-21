<?php

/**
 * @file
 * Wysiwyg API integration.
 */

/**
 * Implementation of hook_INCLUDE_plugin().
 */
function video_video_plugin() {
  $plugins['video'] = array(
    'title' => t('Video'),
    'vendor url' => 'http://drupal.org/project/video',
    'icon file' => 'video.gif',
    'icon title' => t('Video'),
    'js file' => 'video.js',
    'css file' => 'video.css',
    'settings' => array(
      'dialog' => array(
        'url' => base_path() . 'index.php?q=video/wysiwyg',
        'width' => 300,
        'height' => 400,
      ),
    ),
  );
  return $plugins;
}