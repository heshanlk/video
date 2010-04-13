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

function video_auto_transcode_process(&$element) { }

/**
 * Load temp file
 * @param <type> $fid
 * @return <type>
 */
function video_tmp_load_video($fid) {
  $result = db_query('SELECT f.* FROM {video_rendering} vr LEFT JOIN {files} f ON vr.fid = f.fid WHERE f.fid=vr.fid AND f.fid = %d', $fid);
  return db_fetch_object($result);
}

/**
 * Add a video conversion rendering process to the queue
 */
function video_auto_transcode_add_to_queue(&$element, $op) {
  $file = $element['#value'];
  $convert = false;
  //we need to check if this fid has already been added to the database AND that there is in fact a fid
  if(is_array($file) && isset($file['fid']) && !empty($file['fid'])) {
    $fid = $file['fid'];
    //setup our conversion class and check for the fid existence.
    //module_load_include('inc', 'video', '/includes/conversion');
    //$video_conversion = new video_conversion;
    switch($op) {
      case 'insert':
      //if(!$video = $video_conversion->load_video($fid)) {
        if(!$video = video_tmp_load_video($fid)) {
        // do specific tasks defined in trancoders
          _video_render_add_job($file);
          //video has not been added to the que yet so lets add it.
          db_query('INSERT INTO {video_rendering} (fid, pid, status, started, completed) VALUES (%d, %d, %d, %d, %d)', $fid, 0, VIDEO_RENDERING_PENDING, 0, 0);
          drupal_set_message(t('Video submission queued for processing. Please wait: our servers are preparing your video for web displaying.'));
        }
        break;
      case 'update':
      //this is an update
        $old_fid = $element['#default_value']['fid'];
        $serialized_data['old_file'] = $old_fid;
        if($old_fid != $fid) {
          db_query('DELETE FROM {video_rendering} WHERE fid = %d', $old_fid);
          //lets verify that we haven't added this video already.  Multiple validation fails will cause this to be ran more than once
          //if(!$video = $video_conversion->load_video($fid)) {
          if(!$video = video_tmp_load_video($fid)) {
          // do specific tasks defined in trancoders
            _video_render_add_job($file);
            db_query('INSERT INTO {video_rendering} (fid, pid, status, started, completed, serialized_data) VALUES (%d, %d, %d, %d, %d, \'%s\')', $fid, 0, VIDEO_RENDERING_PENDING, 0, 0, serialize($serialized_data));
            drupal_set_message(t('Video submission queued for processing. Please wait: our servers are preparing your video for web displaying.'));
          }
          $convert = true;
        }
        break;
    }

  //our video should be in the database pending, lets see if we need to convert it now.
  //check if we are going from unselected to selected or if this is a new video and we have checked the checkbox
    /*
    if(((!isset($element['#default_value']['data']['convert_video_on_save']) || !$element['#default_value']['data']['convert_video_on_save']) && $file['data']['convert_video_on_save']) || ($convert && $file['data']['convert_video_on_save'])) {
      if(!$video_conversion->process($fid)) {
        drupal_set_message(t('Something went wrong with your video conversion.  Please check your recent log entries for further debugging.'), 'error');
      }
      else {
        drupal_set_message(t('Successfully converted your video.'));
      }
    }
    */
  }
}

/**
 * What are these functions used for?
 */

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

function _video_get_nid_by_video_token($content_type, $fid) {
  $field_info = _video_get_field_by_content_type($content_type);
  $query = "SELECT nid FROM {%s} WHERE %s = '%s'";
  $result = db_query($query, $field_info['table'], $field_info['field_name'] .'_fid', $fid);
  $nid = db_fetch_object($result);
  return $nid;
}

/**
 *
 * @param <type> $job
 */
function _video_render_add_job(&$job) {
  $transcoder = variable_get('vid_convertor', 'ffmpeg');
  module_load_include('inc', 'video', '/plugins/' . $transcoder);
  $function = variable_get('vid_convertor', 'ffmpeg') . '_add_job';
  if (function_exists($function)) {
    $function($job);
  }
  else {
  //    drupal_set_message(t('Transcoder not configured properly'), 'error');
  }
}