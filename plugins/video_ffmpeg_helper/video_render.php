<?php
// $Id$

/**
 * @file
 * Renders a video. This script is called concurrently by video_scheduler.php
 * This script has to be launched with "php video_render.php nid vid"
 *
 * @author Fabio Varesano <fvaresano at yahoo dot it>
 */


/**
 * video_scheduler.php configuration
*/

// set to the ffmpeg executable
define('VIDEO_RENDERING_FFMPEG_PATH', '/usr/bin/ffmpeg');

// set to the temp file path.
//IMPORTANT: the user who runs this script must have permissions to create files there. If this is not the case the default php temporary folder will be used.
define('VIDEO_RENDERING_TEMP_PATH', '/tmp/video'); 

// number of conversion jobs active at the same time
define('VIDEO_RENDERING_FFMPEG_INSTANCES', 5);

// nice value to append at the beginning of the command
define('VIDEO_RENDERING_NICE', 'nice -n 19');


/**
 * video_scheduler.php configuration ends.
 * DO NOT EDIT BELOW THIS LINE
*/

/**
 * Define some constants
*/
define('VIDEO_RENDERING_PENDING', 0);
define('VIDEO_RENDERING_ACTIVE', 5);
define('VIDEO_RENDERING_COMPLETE', 10);


include_once './includes/bootstrap.inc';
// disable error reporting for bootstrap process
error_reporting(E_ERROR);
// let's bootstrap: we will be able to use drupal apis
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
// enable full error reporting again
error_reporting(E_ALL);


// allow execution only from the command line!
if(empty($_SERVER['REQUEST_METHOD'])) {
  if($_SERVER['argc'] != 3) { // check for command line arguments
    watchdog('video_render', t('Incorrect parameters to the video_render.php script.', WATCHDOG_ERROR));
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
  
  // set the status to active
  _video_render_job_change_status($_SERVER['argv'][1], $_SERVER['argv'][2], VIDEO_RENDERING_ACTIVE);
  $job = _video_render_load_job($_SERVER['argv'][1], $_SERVER['argv'][2], VIDEO_RENDERING_ACTIVE);

  if($job == NULL) {
    watchdog('video_render', t('video_render.php has been called with an invalid job resource. exiting.'));
    die;
  }
  $command = _video_render_get_command($job);
  
  //print('executing ' . $command);
  watchdog('video_render', t('executing: ') . $command);
  
  //execute the command
  ob_start();
  passthru($command." 2>&1", $command_return);
  $command_output = ob_get_contents();
  ob_end_clean();
  
  //print $command_output;
  
  if (!file_exists($job->convfile)) {
    watchdog('video_render', t('video conversion failed. ffmpeg reported the following output: ' . $command_output, WATCHDOG_ERROR));
  }
  else {
    // move the video to the definitive location
    $file = array(
      'filename' => basename($job->origfile . ".flv"),
      'filemime' => 'application/octet-stream', // is there something better???
      'filesize' => filesize($job->convfile),
      'filepath' => $job->convfile,
      'nid' => $job->nid,
      );

    $file = ((object) $file);

    //print_r($file);
    $dest_dir = variable_get('video_upload_default_path', 'videos') .'/';

    if (file_copy($file, file_directory_path() . '/' . $dest_dir)) {
      $file->fid = db_next_id('{files}_fid');
      //print_r($file);
      db_query("INSERT INTO {files} (fid, nid, filename, filepath, filemime, filesize) VALUES (%d, %d, '%s', '%s', '%s', %d)", $file->fid, $job->nid, $file->filename, $file->filepath, $file->filemime, $file->filesize);
      
      db_query("INSERT INTO {file_revisions} (fid, vid, list, description) VALUES (%d, %d, %d, '%s')", $file->fid, $job->vid, $file->list, $file->description);
      
      // set vidfile to "" to let video_upload overwrite it with the latest uploaded file
      db_query('UPDATE {video} SET vidfile = "%s", videox = %d, videoy = %d WHERE nid=%d AND vid=%d', "", $job->calculatedx, $job->calculatedy, $job->nid, $job->vid);
      
      _video_render_job_change_status($job->nid, $job->vid, VIDEO_RENDERING_COMPLETE);
      
      // delete the vile
      unlink($job->convfile);
    }
    else {
      watchdog('video_scheduler', t('error moving video to the final directory. Check folder permissions.'), WATCHDOG_ERROR);
    }
  }
}


/**
 * Get a string cointaining the command to be executed including options
*/
function _video_render_get_command(&$job) {

  $videofile = escapeshellarg($job->origfile); // escape file name for safety
  $convfile = tempnam(VIDEO_RENDERING_TEMP_PATH, 'video-rendering');
  $audiobitrate = variable_get('video_ffmpeg_helper_auto_cvr_audio_bitrate', 64);
  $videobitrate = variable_get('video_ffmpeg_helper_auto_cvr_video_bitrate', 200);
  $size = _video_render_get_size($job);
  

  $converter = VIDEO_RENDERING_FFMPEG_PATH;
  $options = preg_replace(array('/%videofile/', '/%convertfile/', '/%audiobitrate/', '/%size/', '/%videobitrate/'), array($videofile, $convfile, $audiobitrate, $size, $videobitrate), variable_get('video_ffmpeg_helper_auto_cvr_options', '-y -i %videofile -f flv -ar 22050 -ab %audiobitrate -s %size -b %videobitrate %convertfile'));
  
  // set to the converted file output
  $job->convfile = $convfile;
  
  return VIDEO_RENDERING_NICE . " $converter $options";
}



/**
 * Calculate the converted video size basing on the width set on administration.
 * Aspect ration is maintained.
*/
function _video_render_get_size(&$job) {
  $def_width = variable_get('video_ffmpeg_helper_auto_cvr_width', 400);

  $height = $def_width * ($job->videoy / $job->videox); // do you remember proportions?? :-)

    
  $height = round($height);
  // add one if odd
  if($height % 2) {
    $height++;
  }

  $job->calculatedx = $def_width;
  $job->calculatedy = $height;

  return $def_width . 'x' . $height;
}


/**
 * Load a job
*/
function _video_render_load_job($nid, $vid, $status) {
  $result = db_query('SELECT * FROM {video_rendering} vr INNER JOIN {node} n ON vr.vid = n.vid INNER JOIN {video} v ON n.vid = v.vid WHERE n.nid = v.nid AND vr.nid = n.nid AND vr.status = %d AND n.nid = %d AND n.vid = %d', $status, $nid, $vid);
  
  return db_fetch_object($result);
}


/**
 * Change the status to $status of the job having nid=$nid and vid=$vid
*/
function _video_render_job_change_status($nid, $vid, $status) {
  $result = db_query('UPDATE {video_rendering} SET status = %d WHERE nid = %d AND vid = %d', $status, $nid, $vid);
}

?>
