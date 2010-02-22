<?php
//$Id$

/**
 * @file
 * Provide a api for video conversion and auto thumbnailing using ffmpeg.
 *
 * You must have ffmpeg_wrapper module installed in order to use this
 *
 * @author Heshan Wanigasooriya <heshan at heidisoft.com, heshanmw at gmail dot com>
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

// nice value to append at the beginning of the command
defined('VIDEO_RENDERING_NICE')
  or define('VIDEO_RENDERING_NICE', 'nice -n 19');


// TODO : add cron API to video module
function ffmpeg_wrapper_cron() {
  global $base_url;

  if(variable_get('video_ffmpeg_helper_auto_cvr_cron', true)) {
    exec("php video_scheduler.php $base_url > /dev/null &");
  }
}


/**
 * Get some informations from the video file
 */
function ffmpeg_wrapper_get_video_info($vidfile) {
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
function ffmpeg_wrapper_auto_resolution(&$vidfile) {

  if(!variable_get('video_ffmpeg_helper_auto_resolution', false)) {

  // call ffmpeg -i
    $filepath = escapeshellarg($vidfile['filepath']);
    $ffmpeg_output = ffmpeg_wrapper_file_data($filepath);

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
function ffmpeg_wrapper_auto_playtime($file) {

  if(!variable_get('video_ffmpeg_helper_auto_playtime', false)) {

  // call ffmpeg -i
    $ffmpeg_output = ffmpeg_wrapper_get_video_info($file);

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
function ffmpeg_wrapper_auto_thumbnail($vidfile) {
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
  // set file path
  $filepath = $vidfile['filepath'];
  // calling ffmpeg_wrapper_file_data function
  $file_data = ffmpeg_wrapper_file_data($filepath);
  $duration = $file_data['duration'];
  $files = NULL;
  for($i = 1; $i <= $count; $i++) {
  // get ffmpeg configurations
    $seek = ($duration/$count) * $i;
    $thumbfile = $tmp . "/video-thumb-for-$fid-$i.png";
    //skip files already exists, this will save ffmpeg traffic
    if (!is_file($thumbfile)) {
//      $tnail = variable_get('video_transcoder_path', '/usr/bin/ffmpeg');
      $options = preg_replace(array('/%videofile/', '/%thumbfile/', '/%seek/'), array($videofile, $thumbfile, $seek), variable_get('video_ffmpeg_thumbnailer_options', '-i %videofile -an -y -f mjpeg -ss %seek -vframes 1 %thumbfile'));
      // executes the command
      $tnail_output = ffmpeg_wrapper_run_command($options, $error_check = true, $path = '');
//      $command = "$tnail $options";
//      ob_start();
//      passthru($command." 2>&1", $tnail_return);
//      $tnail_output = ob_get_contents();
//      ob_end_clean();
      if (!file_exists($thumbfile)) {
        $error_param = array(
            '%file' => $thumbfile,
            '%cmd' => $options,
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
function ffmpeg_wrapper_check_exe_path($path = NULL) {
  return ffmpeg_wrapper_executable();
}

/**
 * Implementing hook_auto_convert();
 * @param <type> $job
 */
//function ffmpeg_wrapper_auto_convert(&$job) {
//  $videofile = escapeshellarg($job->filepath); // escape file name for safety
//  $convfile = tempnam(file_directory_temp(), 'video-rendering');
//  $audiobitrate = variable_get('video_ffmpeg_helper_auto_cvr_audio_bitrate', 64);
//  $videobitrate = variable_get('video_ffmpeg_helper_auto_cvr_video_bitrate', 200);
//  $size = _video_render_get_size();
////  $converter = variable_get('video_transcoder_path', '/usr/bin/ffmpeg');
//
//  $options = preg_replace(array('/%videofile/', '/%convertfile/', '/%audiobitrate/', '/%size/', '/%videobitrate/'),
//      array($videofile, $convfile, $audiobitrate, $size, $videobitrate),
//      variable_get('video_ffmpeg_helper_auto_cvr_options',
//      '-y -i %videofile -f flv -ar 22050 -ab %audiobitrate -s %size -b %videobitrate -qscale 1 %convertfile'));
//
//  // set to the converted file output
//  $job->convfile = $convfile;
//
//  // run conversion commands from ffmpeg_wrapper module
//  $command_output = ffmpeg_wrapper_run_command($options, $error_check = true, $path = '');
//
////  $command = VIDEO_RENDERING_NICE . " $converter $options";
//
//  //print('executing ' . $command); die;
//  watchdog('video_render', 'executing: ' . $options);
////   watchdog('video_render', 'Starting : ' . time());
//  //execute the command
////  ob_start();
////  passthru($command." 2>&1", $command_return);
////  $command_output = ob_get_contents();
////  ob_end_clean();
////  watchdog('video_render', 'Completed');
//  //print $command_output;
//
//  if (!file_exists($job->convfile) || !filesize($job->convfile)) {
//    watchdog('video_render', 'video conversion failed. ffmpeg reported the following output: ' . $command_output);
//  //    _video_render_set_video_encoded_fid($job->nid, $job->vid, -1);
//  //    _video_render_job_change_status($job->nid, $job->vid, VIDEO_RENDERING_FAILED);
//  }
//  else {
//    $file_name = basename($job->filename . ".flv");
//    $file = new stdClass();
//    $file->uid      = $job->uid;
//    $file->status   = FILE_STATUS_PERMANENT;
//    $file->filename = basename($file_name);
//    $file->filepath = $job->convfile;
//    $file->filemime = file_get_mimetype($file_name);
//    $file->filesize = filesize($job->convfile);
//    $file->timestamp = time();
//
//    $job->converted = $file;
//  }
//}


/**
 * This runs FFmpeg based on the form data passed into it.
 * @param string $input_file
 *   path to the file to operate on
 * @param array $params
 *   configuration options in the format set in the ffmpeg_wrapper_configuration_form()
 * @param string $output_file_path
 *   where to place the file, assumes same dir as $input_file. No trailing slash
 * @param object $ffmpeg_object
 *   contains debug information that calling functions can utilize
 * @return string
 *
 */
function ffmpeg_wrapper_auto_convert(&$job) {

  $ffmpeg_object = new stdClass();
  // check configuration are pass of then use global $conf
  if(empty ($params)){
    global $conf;
    $params = $conf;
  }

  $input_file = $job->filepath; // escape file name for safety

  // first error check, make sure that we can decode this kind of file
  if (! ffmpeg_wrapper_can_decode($input_file)) {
    $message = 'FFmpeg Wrapper can not decode this file: !file';
    $variables = array('!file' => l($input_file, file_create_url($input_file)));
    watchdog('video_render', $message, $variables, WATCHDOG_ERROR);
    $ffmpeg_object->errors[] = $message;
    return false;
  }

  // build the output file path if we don't have one. Use the output type as the extension.
  $output_file = file_create_filename(basename($input_file) .'.'. $params['ffmpeg_output_type'], ($output_file_path ? $output_file_path : dirname($input_file)));

  // did the admin define a specific FFmpeg comand to run?
  //  we only run what the admin specified
  if ($params['ffmpeg_video_custom']) {
    $options[] = str_replace(array('%in_file', '%out_file'), array($input_file, $output_file), $params['ffmpeg_video_custom_command']);
  }
  // build a standard configuration
  else {
    // build the ffmpeg command structure out
    $options = array();

    // input file
    $options[] = "-i '". $input_file ."'";

    // build the watermark config
    if ($params['ffmpeg_video_wm']) {
      $options[] = "-vhook '". ffmpeg_wrapper_path_to_vhook('watermark.so') ." -f ". $params['ffmpeg_video_wm_file'] ."'";
    }

    // build the audio config
    if ($params['ffmpeg_audio_advanced']) {

      // use a specifc codec?
      if ($params['ffmpeg_audio_acodec']) {
        $options[] =  '-acodec '. $params['ffmpeg_audio_acodec'];
      }

      // use a specific sample rate?
      if ($params['ffmpeg_audio_ar'] ) {
        $options[] = '-ar '. $params['ffmpeg_audio_ar'];
      }

      // use a specific bit rate?
      if ($params['ffmpeg_audio_ab']) {
        $options[] = '-ab '. $params['ffmpeg_audio_ab'];
      }
    }

    // build the video config
    if ($params['ffmpeg_video_advanced']) {

      // is codec set?
      if ($params['ffmpeg_video_vcodec']) {
        $options[] = '-vcodec '. $params['ffmpeg_video_vcodec'];
      }

      // is frame size set?
      if ($params['ffmpeg_video_size']) {
        $options[] = '-s '. $params[$params['ffmpeg_video_size'] == 'other' ? 'ffmpeg_video_size_other' : 'ffmpeg_video_size'];
      }

      // is the bit rate set?
      if ($params['ffmpeg_video_br']) {
        $options[] = '-b '. $params['ffmpeg_video_br'];
      }

      // is frame rate set?
      if ($params['ffmpeg_video_fps']) {
        $options[] = '-r '. $params['ffmpeg_video_fps'];
      }
    }

    // implement truncating
    if ($params['ffmpeg_time_advanced']) {
      $options[] = '-t '. $params['ffmpeg_time'];
    }

    // add the output file
    $options[] = "'". $output_file ."'";
  }

  $ffmpeg_object->command = implode(" ", $options);

  // run ffmpeg with  out error checking
  if (! $success = ffmpeg_wrapper_run_command($ffmpeg_object->command)) {
    watchdog('video_render', 'video conversion failed. ffmpeg reported the following output: ' . $success);
    return false;
  }

  // successful convert, make a note in the log
  $message = 'FFmpeg converted this file: @file';
  $message .= '<br />'. 'FFmpeg ran this command: <br /><pre> !command </pre>';
  $variables = array('@file' => $output_file, '!command' =>  $ffmpeg_object->command);
  watchdog('video_render', $message, $variables, WATCHDOG_NOTICE);

  $ffmpeg_object->output_file = $output_file;

  if (!file_exists($output_file) || !filesize($output_file)) {
    watchdog('video_render', 'video conversion failed. ffmpeg reported the following output: ' . $command_output);
  //    _video_render_set_video_encoded_fid($job->nid, $job->vid, -1);
  //    _video_render_job_change_status($job->nid, $job->vid, VIDEO_RENDERING_FAILED);
  }
  else {
    $file_name = basename($output_file);
    $file = new stdClass();
    $file->uid      = $job->uid;
    $file->status   = FILE_STATUS_PERMANENT;
    $file->filename = basename($file_name);
    $file->filepath = $output_file;
    $file->filemime = file_get_mimetype($file_name);
    $file->filesize = filesize($output_file);
    $file->timestamp = time();

    $job->converted = $file;
  }
}
