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
  db_query('INSERT INTO {video_rendering} (vid, nid, origfile, pid, status, started, completed)
      VALUES (%d, %d, \'%s\', %d, %d, %d, %d)',
      $node->vid, $node->nid, $file->filepath, 0, VIDEO_RENDERING_PENDING, 0, 0);
  drupal_set_message(t('Video submission queued for processing. Please wait: our servers are preparing your video for web displaying.'));
}
