<?php
//$Id$

/**
 * @file
 * Enable conversion control on video module.
 *
 * @author Heshan Wanigasooriya <heshan at heidisoft dot com, heshanmw at gmail dot com>
 */

function video_auto_transcode_process(&$element) { }

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
    module_load_include('inc', 'video', '/includes/conversion');
    $video_conversion = new video_conversion;
    switch($op) {
      case 'insert':
        if(!$video = $video_conversion->load_video($fid)) {
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
          if(!$video = $video_conversion->load_video($fid)) {
            db_query('INSERT INTO {video_rendering} (fid, pid, status, started, completed, serialized_data) VALUES (%d, %d, %d, %d, %d, \'%s\')', $fid, 0, VIDEO_RENDERING_PENDING, 0, 0, serialize($serialized_data));
            drupal_set_message(t('Video submission queued for processing. Please wait: our servers are preparing your video for web displaying.'));
          }
          $convert = true;
        }
        break;
    }

    // Our video should be in the database pending, lets see if we need to convert it now.
    // Check if we are going from unselected to selected or if this is a new video and we have checked the checkbox
    if(((!isset($element['#default_value']['data']['convert_video_on_save']) || !$element['#default_value']['data']['convert_video_on_save']) && $file['data']['convert_video_on_save']) || ($convert && $file['data']['convert_video_on_save'])) {
      if(!$video_conversion->process($fid)) {
        drupal_set_message(t('Something went wrong with your video conversion.  Please check your recent log entries for further debugging.'), 'error');
      }
      else {
        drupal_set_message(t('Successfully converted your video.'));
      }
    }
  }
}