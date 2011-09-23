<?php

/**
 * @file
 * Wysiwyg API integration.
 */

/**
 * Implementation of hook_INCLUDE_plugin().
 */
function video_video_plugin() {
  drupal_add_js(drupal_get_path('module', 'video') . '/libraries/jquery.modalframe/jquery-ui-modalframe.js');
  drupal_add_js(drupal_get_path('module', 'video') . '/libraries/jquery.modalframe/tmt_jquery_modalframe.js');
  drupal_add_css(drupal_get_path('module', 'video') . '/libraries/jquery.modalframe/jquery-ui-theme.css');
//  drupal_add_js(drupal_get_path('module', 'video') . '/js/video_wysiwyg.js');
  $plugins['video'] = array(
    'title' => t('Video'),
    'vendor url' => 'http://drupal.org/project/video',
    'icon file' => 'video.gif',
    'icon title' => t('Video'),
    'js file' => 'video.js',
    'css file' => 'video.css',
    'settings' => array(
      'golbal' => array(
        'url' => base_path() . 'video/browser?render=video-popup'
      )
    ),
  );
  return $plugins;
}