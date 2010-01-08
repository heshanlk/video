<?php
//$Id$

/**
 * @file
 * Provide a api for video conversion and auto thumbnailing using ffmpeg.
 *
 * @author Heshan Wanigasooriya <heshan at heidisoft.com, heshanmw at gmail dot com>
 */

/**
 * Define some constants
 */
define('VIDEO_RENDERING_PENDING', 1);
define('VIDEO_RENDERING_ACTIVE', 5);
define('VIDEO_RENDERING_COMPLETE', 10);
define('VIDEO_RENDERING_FAILED', 20);


// TODO : add cron API to video module
function ffmpeg_cron() {
  global $base_url;

  if(variable_get('video_ffmpeg_helper_auto_cvr_cron', true)) {
    exec("php video_scheduler.php $base_url > /dev/null &");
  }
}


/**
 * Get some informations from the video file
 */
function ffmpeg_get_video_info($vidfile) {
  static $ffmpeg_info;
  $fid = $vidfile['fid'];
  //  $command_output = cache_get($fid);
  //  if(empty($command_output)) {
  // escape file name for safety
  $file = escapeshellarg($vidfile['filepath']);
  // create the full command to execute
  $command = variable_get('video_transcoder_path', '/usr/bin/ffmpeg') . ' -i ' . $file;

  //execute the command
  ob_start();
  passthru($command." 2>&1", $command_return);
  $command_output = ob_get_contents();
  ob_end_clean();

  // cache the result for further calls
  //  $ffmpeg_info[$vidfile['fid']] = $command_output;
  //    cache_set($vidfile['fid'], $command_output);
  //  }

  return $command_output;
}


/**
 * Return the video resolution
 */
function ffmpeg_auto_resolution(&$node) {

  if(!variable_get('video_ffmpeg_helper_auto_resolution', false)) {

  // call ffmpeg -i
    $ffmpeg_output = ffmpeg_get_video_info($node);

    // get resolution
    $pattern = '/Video: .*, ([0-9]{2,4}x[0-9]{2,4})/';
    preg_match_all($pattern, $ffmpeg_output, $matches, PREG_PATTERN_ORDER);
    $resolution = $matches[1][0];

    return explode("x", $resolution);
  }
  return null;
}


/**
 * Return the playtime seconds of a video
 */
function ffmpeg_auto_playtime($file) {

  if(!variable_get('video_ffmpeg_helper_auto_playtime', false)) {

  // call ffmpeg -i
    $ffmpeg_output = ffmpeg_get_video_info($file);

    // get playtime
    $pattern = '/Duration: ([0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9])/';
    preg_match_all($pattern, $ffmpeg_output, $matches, PREG_PATTERN_ORDER);
    $playtime = $matches[1][0];

    // ffmpeg return lenght as 00:00:31.1 Let's get playtime from that
    $hmsmm = explode(":", $playtime);

    $tmp = explode(".", $hmsmm[2]);
    $seconds = $tmp[0];

    $hours = $hmsmm[0];
    $minutes = $hmsmm[1];

    return $seconds + ($hours * 3600) + ($minutes * 60);
  }
}

/**
 * Generates a thumbnail from the video file
 * Implementing hook_auto_thumbnail on inc
 *
 * @param $vidfile
 *   object with element information
 *
 * @return
 *   a drupal file objects
 */
function ffmpeg_auto_thumbnail($vidfile) {
  global $user;
  $uploaded_file = $vidfile;
  $fid = $uploaded_file["fid"];

  // are we debugging?
  // escape the filename for safety
  $videofile = escapeshellarg($uploaded_file['filepath']);
  $thumb_path = variable_get('video_thumb_path', 'video_thumbs');
  //files will save in files/video_thumbs/#fileId folder
  $tmp = file_directory_path(). '/' . $thumb_path . '/' . $fid;
  // Ensure the destination directory exists and is writable.
  $directories = explode('/', $tmp);
  //  array_pop($directories); // Remove the file itself.
  // Get the file system directory.
  $file_system = file_directory_path();
  foreach ($directories as $directory) {
    $full_path = isset($full_path) ? $full_path . '/' . $directory : $directory;
    // Don't check directories outside the file system path.
    if (strpos($full_path, $file_system) === 0) {
      field_file_check_directory($full_path, FILE_CREATE_DIRECTORY);
    }
  }
  $count = variable_get('no_of_video_thumbs', 5);
  $duration = ffmpeg_auto_playtime($vidfile);
  $files = NULL;
  for($i = 1; $i <= $count; $i++) {
  // get ffmpeg configurations
    $seek = ($duration/$count) * $i;
    $thumbfile = $tmp . "/video-thumb-for-$fid-$i.png";
    //skip files already exists, this will save ffmpeg traffic
    if (!is_file($thumbfile)) {
      $tnail = variable_get('video_transcoder_path', '/usr/bin/ffmpeg');
      $options = preg_replace(array('/%videofile/', '/%thumbfile/', '/%seek/'), array($videofile, $thumbfile, $seek), variable_get('video_ffmpeg_thumbnailer_options', '-i %videofile -an -y -f mjpeg -ss %seek -vframes 1 %thumbfile'));
      //    $options = preg_replace(array('/%videofile/', '/%tmp/', '/%id/', '/%interval/'), array($videofile, $tmp, $i, ($duration/$count)), variable_get('video_image_thumbnailer_options', '-ss %id*%interval -i %videofile -vframes 1 %thumbfile'));
      //  ffmpeg -ss $i*$interval -i intro.mov -vframes 1 -s 320x240 thumb_$i.jpg
      //ffmpeg -i superstunt_8uiarzrh.mp4 -r 0.1 -ss 00:00:5 -f image2 img/images%02d.png
      ////ffmpeg -i superstunt_8uiarzrh.mp4 -r 0.05 -ss 00:00:5 -f image2 img/images%1d.jpg
      // executes the command
      $command = "$tnail $options";
      ob_start();
      passthru($command." 2>&1", $tnail_return);
      $tnail_output = ob_get_contents();
      ob_end_clean();
      if (!file_exists($thumbfile)) {
        $error_param = array(
            '%file' => $thumbfile,
            '%cmd' => $command,
            '%out' => $tnail_output,
        );
        $error_msg = t("error generating thumbnail for video: generated file %file does not exist.<br />Command Executed:<br />%cmd<br />Command Output:<br />%out", $error_param);
        // let's log this
        watchdog('video_ffmpeg',$error_msg);
      }
    }
    // Begin building file object.
    //TODO : use file_munge_filename()
    $file = new stdClass();
    $file->uid      = $user->uid;
    $file->status   = FILE_STATUS_TEMPORARY;
    $file->filename = trim("video-thumb-for-$fid-$i.png");
    $file->filepath = $thumbfile;
    $file->filemime = file_get_mimetype("video-thumb-for-$fid-$i.png");
    $file->filesize = filesize($thumbfile);
    $file->timestamp = time();
    $files[] = $file;
  }
  return $files;
}

/**
 * Implementing hook_chcek_exepath() on inc
 * To check the the path is executable or not
 * @param <type> path to check
 * @return bool TRUE/FALSE
 */
function ffmpeg_check_exe_path($path=NULL) {
  if (!$path) {
    $path = variable_get('video_transcoder_path', '/usr/bin/ffmpeg');
  }
  if (function_exists('is_executable')) {
    $test = 'is_executable';
  } else {
    $test = 'file_exists';
  }
  return $test($path);
}

/**
 * Implementing hook_auto_convert();
 * @param <type> $job
 */
function ffmpeg_auto_convert(&$job) {
  $videofile = escapeshellarg($job->filepath); // escape file name for safety
  $convfile = tempnam(file_directory_temp(), 'video-rendering');
  $audiobitrate = variable_get('video_ffmpeg_helper_auto_cvr_audio_bitrate', 64);
  $videobitrate = variable_get('video_ffmpeg_helper_auto_cvr_video_bitrate', 200);
  $size = _video_render_get_size();
  $converter = variable_get('video_transcoder_path', '/usr/bin/ffmpeg');
  
  $options = preg_replace(array('/%videofile/', '/%convertfile/', '/%audiobitrate/', '/%size/', '/%videobitrate/'),
      array($videofile, $convfile, $audiobitrate, $size, $videobitrate),
      variable_get('video_ffmpeg_helper_auto_cvr_options',
      '-y -i %videofile -f flv -ar 22050 -ab %audiobitrate -s %size -b %videobitrate -qscale 1 %convertfile'));

  // set to the converted file output
  $job->convfile = $convfile;

  $command = VIDEO_RENDERING_NICE . " $converter $options";

  //print('executing ' . $command); die;
  watchdog('video_render', 'executing: ' . $command);
//   watchdog('video_render', 'Starting : ' . time());
  //execute the command
  ob_start();
  passthru($command." 2>&1", $command_return);
  $command_output = ob_get_contents();
  ob_end_clean();
//  watchdog('video_render', 'Completed : ' . time());
  //print $command_output;

  if (!file_exists($job->convfile) || !filesize($job->convfile)) {
    watchdog('video_render', 'video conversion failed. ffmpeg reported the following output: ' . $command_output);
  //    _video_render_set_video_encoded_fid($job->nid, $job->vid, -1);
  //    _video_render_job_change_status($job->nid, $job->vid, VIDEO_RENDERING_FAILED);
  }
  else {
    $file_name = basename($job->filename . ".flv");
    $file = new stdClass();
    $file->uid      = $job->uid;
    $file->status   = FILE_STATUS_PERMANENT;
    $file->filename = basename($file_name);
    $file->filepath = $job->convfile;
    $file->filemime = file_get_mimetype($file_name);
    $file->filesize = filesize($job->convfile);
    $file->timestamp = time();
    
    $job->converted = $file;
  }
}


/**
 * Calculate the converted video size basing on the width set on administration.
 * Aspect ration is maintained.
 */
function _video_render_get_size() {
  return variable_get('video_ffmpeg_width', 640) . 'x' . variable_get('video_ffmpeg_height', 360);
}