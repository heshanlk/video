<?php
//$Id$
/**
 * @file
 * Renders a video. This script is called concurrently by video_scheduler.php
 * This script has to be launched with "php video_render.php nid vid"
 * If you are not using sites/default/settings.php as your settings file,
 * add an optional parameter for the drupal site url:
 * "php video_render.php nid vid http://example.com/" or
 * "php video_render.php nid vid http://example.org/drupal/"
 *
 * @author Heshan Wanigasooriya <heshan at heidisoft dot com , heshanmw at gmail dot com>
 * @todo
 */


/**
 * video_scheduler.php configuration
 */

// nice value to append at the beginning of the command
define('VIDEO_RENDERING_NICE', 'nice -n 19');


/**
 * Define some constants
 */
define('VIDEO_RENDERING_PENDING', 1);
define('VIDEO_RENDERING_ACTIVE', 5);
define('VIDEO_RENDERING_COMPLETE', 10);
define('VIDEO_RENDERING_FAILED', 20);

if (isset($_SERVER['argv'][3])) {
  $url = parse_url($_SERVER['argv'][3]);
  $_SERVER['SCRIPT_NAME'] = $url['path'];
  $_SERVER['HTTP_HOST'] = $url['host'];
}

include_once('./includes/bootstrap.inc');
//module_load_include('/includes/bootstrap.inc', 'video_render', 'includes/bootstrap');
//
// disable error reporting for bootstrap process
error_reporting(E_ERROR);
// let's bootstrap: we will be able to use drupal apis
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
// enable full error reporting again
error_reporting(E_ALL);


// allow execution only from the command line!
if(empty($_SERVER['REQUEST_METHOD'])) {
  if($_SERVER['argc'] < 3) { // check for command line arguments
    watchdog('video_render', 'Incorrect parameters to the video_render.php script.', WATCHDOG_ERROR);
    print t('Incorrect parameters');
  }
  else {
    video_render_main();
  }
}
else {
  print t('This script is only executable from the command line.');
  die();
}

print("\n");

function video_render_main() {

// get parameters passed from command line
  $fid = $_SERVER['argv'][1];

  // set the status to active
  _video_render_job_change_status($fid, VIDEO_RENDERING_ACTIVE);
  // load the job object
  $job = _video_render_load_job($fid);

  if($job == NULL) {
    watchdog('video_render', 'video_render.php has been called with an invalid job resource. exiting.');
    die;
  }

  // get file object
  $job = _video_render_get_converted_file($job);
  $file = $job['converted'];
  $tmpfile = $file->filepath;
  // the above no more works as token supports - use dirname
  $dest_dir = dirname($job->filepath) . '/';

  if (file_copy($file, $dest_dir)) {
  //update the file table entry and copy file content to new one
    $file->fid = $fid;
    drupal_write_record ('files', $file, 'fid');
    drupal_write_record ('files', $job);
    _video_render_job_change_status($fid, VIDEO_RENDERING_COMPLETE);
    watchdog('video_render', 'successfully converted %orig to %dest', array('%orig' => $job->filepath, '%dest' => $file->filepath));
    // delete the temp file
    unlink($tmpfile);
  }
  else {
    _video_render_job_change_status($fid, VIDEO_RENDERING_FAILED);
    // get the username of the process owner
    $ownerarray = posix_getpwuid(posix_getuid());
    $owner=$ownerarray['name'];
    // get the username of the destination folder owner
    $fownerarray = posix_getpwuid(fileowner($dest_dir));
    $fowner=$fownerarray['name'];
    // get destination folder permissions
    $perms = substr(sprintf('%o', fileperms($dest_dir)), -4);
    watchdog('video_render', 'error moving video %vid_file with nid = %nid to %dir the final directory. Check folder permissions.<br />The script was run by %uname .<br />The folder owner is %fowner .<br />The folder permissions are %perms .', array('%vid_file' => $job->origfile, '%nid' => $job->nid, '%dir' => $dest_dir, '%uname' => $owner, '%fowner' => $fowner, '%perms' => $perms), WATCHDOG_ERROR);
  }
}


/**
 * Get a string cointaining the command to be executed including options
 */
function _video_render_get_converted_file(&$job) {
  $transcoder = variable_get('vid_convertor', 'ffmpeg');
  module_load_include('inc', 'video', '/plugins/' . $transcoder);
  $function = variable_get('vid_convertor', 'ffmpeg') . '_auto_convert';
  if (function_exists($function)) {
  //    $thumbs = ffmpeg_auto_thumbnail($file);
    $function($job);
  }
  else {
    drupal_set_message(t('Transcoder not configured properly'), 'error');
  }
}


/**
 * Load a job
 */
function _video_render_load_job($fid, $status) {
  $result = db_query('SELECT f.filepath, f.filesize, f.filename, f.filemime, f.filesize, f.status, f.uid FROM {video_rendering} vr INNER JOIN {files}
      f ON vr.fid = f.fid WHERE vr.fid = f.fid AND vr.status = %d AND f.status = %d AND f.fid = %d',
      VIDEO_RENDERING_PENDING, FILE_STATUS_PERMANENT, $fid);
  return db_fetch_object($result);
}


/**
 * Change the status to $status of the job having nid=$nid and vid=$vid
 */
function _video_render_job_change_status($fid, $status) {
  $result = db_query('UPDATE {video_rendering} SET status = %d WHERE fid = %d ', $status, $fid);
}

/**
 * Set the video_encoded_fid in the video table
 * We store -1 as video_encoded_fid if the encoding failed
 */
function _video_render_set_video_encoded_fid($nid, $vid, $encoded_fid) {
  db_lock_table('video');
  $node = db_fetch_object(db_query("SELECT serialized_data FROM {video} WHERE nid = %d AND vid = %d", $nid, $vid));
  $node->serial_data = unserialize($node->serialized_data);
  //GMM: save fid of previously encoded file
  $old_fid = $node->serial_data['video_encoded_fid'];
  $node->serial_data['video_encoded_fid'] = $encoded_fid;
  $node->serialized_data = serialize($node->serial_data);
  db_query("UPDATE {video} SET serialized_data = '%s' WHERE nid = %d AND vid = %d", $node->serialized_data, $nid, $vid);
  db_unlock_tables();
  // GMM: update status on previously encoded fid to 0 so drupal will delete
  if($old_fid > 0)
    db_query("UPDATE {files} SET status = %d WHERE fid = %d", 0, $old_fid);
}

?>
