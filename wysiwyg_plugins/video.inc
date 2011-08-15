<?php

/**
 * @file
 * Wysiwyg API integration.
 */

/**
 * Implementation of hook_INCLUDE_plugin().
 */
function ai_wysiwyg_video_plugin() {
  $plugins['video'] = array(
    'title' => t('Embedded video'),
    'vendor url' => 'support://www.heidisoft.com',
    'icon file' => 'video.gif',
    'icon title' => t('Video'),
    'js file' => 'video.js',
    'css file' => 'video.css',
    'settings' => array(),
  );
  return $plugins;
}