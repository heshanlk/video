<?php
//$Id$

/**
 * @file
 * Enable conversion control on video module.
 *
 * @author Heshan Wanigasooriya <heshan at heidisoft dot com, heshanmw at gmail dot com>
 */


/**
 * Define some constants
 */
define('VIDEO_RENDERING_PENDING', 1);
define('VIDEO_RENDERING_ACTIVE', 5);
define('VIDEO_RENDERING_COMPLETE', 10);
define('VIDEO_RENDERING_FAILED', 20);

function video_auto_transcode_process(&$element) {
//  $file = $element['#value'];
//  $element['data']['old_vid'] = array(
//      '#type' => 'value',
//      '#value' => $file['fid']
//    );
//  $element['data']['vid_transcode_status'] = array(
//      '#type' => 'value',
//      '#value' => VIDEO_RENDERING_PENDING
//    );
}

/**
 * Add a video conversion rendering process to the queue
 */
function video_auto_transcode_add_to_queue(&$node) {
  $content_types = _video_get_content_types();
  foreach ($content_types as $content_type) {
    $nid = _video_get_nid_by_video_token($content_type, $video_token);
    $query = "SELECT id FROM {cdn2_videos} WHERE nid = %d";
    $result = db_result(db_query($query, $nid));
  }
  
  db_query('INSERT INTO {video_rendering} (vid, nid, origfile, pid, status, started, completed)
      VALUES (%d, %d, \'%s\', %d, %d, %d, %d)',
      $node->vid, $node->nid, $file->filepath, 0, VIDEO_RENDERING_PENDING, 0, 0);
  drupal_set_message(t('Video submission queued for processing. Please wait: our servers are preparing your video for web displaying.'));
}

function _video_get_content_types() {
  $widget_info = uploadfield_widget_info();
  $content_types = array();
  foreach ($widget_info as $key => $value) {
    $query = "SELECT type_name FROM {content_node_field_instance} WHERE widget_type='%s'";
    $result = db_query($query, $key);
    while ($row = db_fetch_object($result)) {
      $content_types[] = $row->type_name;
    }
  }
  return $content_types;
}

function _video_get_field_by_content_type($content_type) {
  $type = content_types($content_type);

  $fields = $type['fields'];
  $field_name = '';
  foreach ($fields as $field) {
    if ($field['type'] == 'filefield') {
      $db_info = content_database_info($field);
      $db_info = array_merge($db_info, $field);
      return $db_info;
    }
  }
}

function _video_get_nid_by_video_token($content_type, $video_token) {
  $field_info = _video_get_field_by_content_type($content_type);
  $query = "SELECT nid FROM {%s} WHERE %s = '%s'";
  $result = db_query($query, $field_info['table'], $field_info['columns']['video_token']['column'], $video_token);
  $nid = db_fetch_object($result);
  return $nid->nid;
}