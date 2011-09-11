<?php

/**
 * @file rules integration for the video module
 *
 * @addtogroup rules
 * @{
 */

/**
 * Implements hook_rules_file_info() on behalf of the video module.
 */
function video_rules_file_info() {
  return array('video.rules.inc');
}

/**
 * Implements hook_rules_event_info() on behalf of the video module.
 */
function video_rules_event_info() {
  $items = array(
    'video_complete' => array(
      'label' => t('After video convertion completed (Success)'),
      'group' => t('Video'),
      'variables' => rules_events_video_variables(t('created content')),
      'access callback' => 'rules_video_integration_access',
    ),
    'video_failed' => array(
      'label' => t('After video convertion completed (Failed)'),
      'group' => t('Video'),
      'variables' => rules_events_video_variables(t('created content')),
      'access callback' => 'rules_video_integration_access',
    ),
  );
  return $items;
}

/**
 * Returns some parameter suitable for using it with a node
 */
function rules_events_video_variables($node_label, $update = FALSE) {
  $args = array(
    'node' => array('type' => 'node', 'label' => $node_label),
  );
  if ($update) {
    $args += array(
      'node_unchanged' => array(
        'type' => 'node',
        'label' => t('unchanged content'),
        'handler' => 'rules_events_entity_unchanged',
      ),
    );
  }
  return $args;
}