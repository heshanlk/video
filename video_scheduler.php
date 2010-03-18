<?php
//$Id$
/**
 * @file
 * Implement video rendering scheduling.
 * If you are not using sites/default/settings.php as your settings file,
 * add an optional parameter for the drupal site url:
 * "php video_scheduler.php http://example.com/" or
 * "php video_scheduler.php http://example.org/drupal/"
 *
 * @author Heshan Wanigasooriya <heshan at heidisoft dot com, heshanmw at gmail dot com>
 *
 */


/**
 * video_scheduler.php configuration
 */

// number of conversion jobs active at the same time
defined('VIDEO_RENDERING_FFMPEG_INSTANCES')
  or define('VIDEO_RENDERING_FFMPEG_INSTANCES', 5);

/**
 * video_scheduler.php configuration ends.
 * DO NOT EDIT BELOW THIS LINE
 */

/**
 * Define some constants
 */
defined('VIDEO_RENDERING_PENDING')
  or define('VIDEO_RENDERING_PENDING', 1);
defined('VIDEO_RENDERING_ACTIVE')
  or define('VIDEO_RENDERING_ACTIVE', 5);
defined('VIDEO_RENDERING_COMPLETE')
  or define('VIDEO_RENDERING_COMPLETE', 10);
defined('VIDEO_RENDERING_FAILED')
  or define('VIDEO_RENDERING_FAILED', 20);


if (isset($_SERVER['argv'][1])) {
  $url = parse_url($_SERVER['argv'][1]);
  $_SERVER['SCRIPT_NAME'] = $url['path'];
  $_SERVER['HTTP_HOST'] = $url['host'];
}

include_once('./includes/bootstrap.inc');
//module_load_include('/includes/bootstrap.inc', 'video_scheduler', 'includes/bootstrap');
// disable error reporting for bootstrap process
error_reporting(E_ERROR);
// let's bootstrap: we will be able to use drupal apis
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
// enable full error reporting again
error_reporting(E_ALL);

//watchdog('video_scheduler', 'starting video conversion jobs.', array(), WATCHDOG_DEBUG);
// allow execution only from the command line!
if(empty($_SERVER['REQUEST_METHOD'])) {
  video_scheduler_main();
}
else {
  print ('This script is only executable from the command line.');
  die();
}



/**
 * Main for video_scheduler.php
 */
function video_scheduler_main() {
//  echo 'ok';
  if($jobs = video_scheduler_select()) {
    foreach ($jobs as $job) {
      video_scheduler_start($job);
    }
  }
  else {
    watchdog('video_scheduler', 'no video conversion jobs to schedule.', array(), WATCHDOG_DEBUG);
  }
}


/**
 * Starts rendering for a job
 */
function video_scheduler_start($job) {
//  $url = (isset($_SERVER['argv'][1])) ? escapeshellarg($_SERVER['argv'][1]) : '';
//  watchdog('video_scheduler', 'Execute video_render.php %url and %job', array('%url'=>$url, '%job'=>$job->fid), WATCHDOG_DEBUG);
//  exec("/usr/local/bin/php /home/freja/public_html/v5-dev/video_render.php $job->fid $url > /dev/null &");
  video_render_main($job->fid);
}


/**
 * Select VIDEO_RENDERING_FFMPEG_INSTANCES jobs from the queue
 *
 * @return an array containing jobs
 */
function video_scheduler_select() {
// load node and its file object
  module_load_include('inc', 'uploadfield', '/uploadfield_convert');
  $jobs = array();
  $i = 0;
  $result = db_query_range('SELECT f.fid, f.filepath, f.filesize, f.filename, f.filemime, f.status FROM {video_rendering} vr INNER JOIN {files}
      f ON vr.fid = f.fid WHERE vr.fid = f.fid AND vr.status = %d AND f.status = %d ORDER BY f.timestamp',
      VIDEO_RENDERING_PENDING, FILE_STATUS_PERMANENT, 0, VIDEO_RENDERING_FFMPEG_INSTANCES);
  while($job = db_fetch_object($result)) {
    $jobs[] = $job;
  }
  //  print_r($jobs);
  //  exit;
  return $jobs;
}

/**
 * Video Rendering Process
 *
 */
function video_render_main($job_fid) {

// get parameters passed from command line
  $fid = $job_fid;
  $job = NULL;
  // set the status to active
  _video_render_job_change_status($fid, VIDEO_RENDERING_ACTIVE);
  // load the job object
  $job = _video_render_load_job($fid);

  if(empty($job)) {
    watchdog('video_render', 'video_render.php has been called with an invalid job resource. exiting.', array(), WATCHDOG_ERROR);
    die;
  }

  // get file object
  _video_render_get_converted_file(&$job);
  $file = $job->converted;

  if(empty($file)) {
    watchdog('video_render', 'converted file is an empty file.', array(), WATCHDOG_ERROR);
    _video_render_job_change_status($fid, VIDEO_RENDERING_FAILED);
    die;
  }


  $tmpfile = $file->filepath;

  // the above no more works as token supports - use dirname
  $dest_dir = dirname($job->filepath) . '/';

  if (file_copy($file, $dest_dir)) {
  //update the file table entry and copy file content to new one
    $file->fid = $fid;
    //update file with new
    drupal_write_record ('files', $file, 'fid');
    //add new file entry
    drupal_write_record ('files', $job);
    // TODO : add data of rendering
    _video_render_job_change_status($fid, VIDEO_RENDERING_COMPLETE);
    // clear all cacahe data
    // cache_clear_all();
    // drupal_flush_all_caches();
    cache_clear_all("*", 'cache_content', true);
    watchdog('video_render', 'successfully converted %orig to %dest', array('%orig' => $job->filepath, '%dest' => $file->filepath), WATCHDOG_INFO);
    // delete the temp file
//    unlink($tmpfile);
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
  //    watchdog('video_render', 'calling to converter API %conv', array('%conv' => $transcoder));
    $function(&$job);
//    if(! $success) {
//       watchdog('video_render', 'error transcoding vide. existing.', array(), WATCHDOG_ERROR);
//    }
  }
  else {
  //    drupal_set_message(t('Transcoder not configured properly'), 'error');
    print ('Transcoder not configured properly');
  }
}


/**
 * Load a job
 */
function _video_render_load_job($fid) {
//  watchdog('video_render', 'Loading contents for file id %fid', array('%fid' => $fid));
  $result = db_query('SELECT f.filepath, f.filesize, f.filename, f.filemime, f.filesize, f.status, f.uid
      FROM {video_rendering} vr INNER JOIN {files} f
      ON vr.fid = f.fid WHERE vr.fid = f.fid AND f.status = %d AND f.fid = %d',
      FILE_STATUS_PERMANENT, $fid);
  return db_fetch_object($result);
}


/**
 * Change the status to $status of the job having nid=$nid and vid=$vid
 */
function _video_render_job_change_status($fid, $status) {
  $result = db_query('UPDATE {video_rendering} SET status = %d WHERE fid = %d ', $status, $fid);
}

?>
