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
define('VIDEO_RENDERING_FFMPEG_INSTANCES', 5);

/**
 * video_scheduler.php configuration ends.
 * DO NOT EDIT BELOW THIS LINE
 */

/**
 * Define some constants
 */
define('VIDEO_RENDERING_PENDING', 1);
define('VIDEO_RENDERING_ACTIVE', 5);
define('VIDEO_RENDERING_COMPLETE', 10);
define('VIDEO_RENDERING_FAILED', 20);

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
  $url = (isset($_SERVER['argv'][1])) ? escapeshellarg($_SERVER['argv'][1]) : '';
  exec("php video_render.php $job->fid $url > /dev/null &");
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
  //    $content_types = _video_get_content_types();
  //    $content_type = $content_types;
  //    $nid = _video_get_nid_by_video_token($content_type[0], $job->fid);
  //    print_r($nid);
  //    $node = node_load(array('nid' => $nid->nid));
  //    print_r($node);
    $jobs[] = $job;
  }
//  print_r($jobs);
//  exit;
  return $jobs;
}

/**
 * load file object
 */
//function _video_filde_load(){
//  return db_fetch_object(db_query('SELECT f.fid FROM {video_rendering} vr INNER JOIN {files}
//      f ON vr.fid = f.fid WHERE vr.fid = f.fid AND vr.status = %d ORDER BY f.timestamp',
//      VIDEO_RENDERING_PENDING, 0, VIDEO_RENDERING_FFMPEG_INSTANCES));
//}

?>
