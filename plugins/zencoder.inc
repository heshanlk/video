<?php
//$Id$

/**
 * @file
 * Provide a api for video conversion and auto thumbnailing using ffmpeg.
 *
 * @author Heshan Wanigasooriya <heshan at heidisoft.com, heshanmw at gmail dot com>
 * TODO: add common settings from video module configurations and extend it from ffmpeg.inc
 * since we need to have executable path of ffmpeg to ffmpeg.inc we need to have it
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

// curl related data
defined('HTTP_CONNECT_TIMEOUT')
    or define('HTTP_CONNECT_TIMEOUT', 600);
defined('HTTP_LOW_SPEED_LIMIT')
    or define('HTTP_LOW_SPEED_LIMIT', 256);
defined('HTTP_LOW_SPEED_TIMEOUT')
    or define('HTTP_LOW_SPEED_TIMEOUT', 600);


// TODO : add cron API to video module
function zencoder_cron() {
  global $base_url;

  if(variable_get('video_zencoder_helper_auto_cvr_cron', true)) {
    exec("php video_scheduler.php $base_url > /dev/null &");
  }
}


/**
 * Get some informations from the video file
 */
function zencoder_get_video_info($vidfile) {
  static $zencoder_info;
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
  //  $zencoder_info[$vidfile['fid']] = $command_output;
  //    cache_set($vidfile['fid'], $command_output);
  //  }

  return $command_output;
}


/**
 * Return the video resolution
 */
function zencoder_auto_resolution(&$node) {

  if(!variable_get('video_zencoder_helper_auto_resolution', false)) {

  // call ffmpeg -i
    $zencoder_output = zencoder_get_video_info($node);

    // get resolution
    $pattern = '/Video: .*, ([0-9]{2,4}x[0-9]{2,4})/';
    preg_match_all($pattern, $zencoder_output, $matches, PREG_PATTERN_ORDER);
    $resolution = $matches[1][0];

    return explode("x", $resolution);
  }
  return null;
}


/**
 * Return the playtime seconds of a video
 */
function zencoder_auto_playtime($file) {

  if(!variable_get('video_zencoder_helper_auto_playtime', false)) {

  // call ffmpeg -i
    $zencoder_output = zencoder_get_video_info($file);

    // get playtime
    $pattern = '/Duration: ([0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9])/';
    preg_match_all($pattern, $zencoder_output, $matches, PREG_PATTERN_ORDER);
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
function zencoder_auto_thumbnail($vidfile) {
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
  $duration = zencoder_auto_playtime($vidfile);
  $files = NULL;
  for($i = 1; $i <= $count; $i++) {
  // get ffmpeg configurations
    $seek = ($duration/$count) * $i;
    $thumbfile = $tmp . "/video-thumb-for-$fid-$i.png";
    //skip files already exists, this will save ffmpeg traffic
    if (!is_file($thumbfile)) {
      $tnail = variable_get('video_transcoder_path', '/usr/bin/ffmpeg');
      $options = preg_replace(array('/%videofile/', '/%thumbfile/', '/%seek/'), array($videofile, $thumbfile, $seek), variable_get('video_zencoder_thumbnailer_options', '-i %videofile -an -y -f mjpeg -ss %seek -vframes 1 %thumbfile'));
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
        watchdog('video_ffmpeg',$error_msg, array(), WATCHDOG_ERROR);
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
function zencoder_check_exe_path($path=NULL) {
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
function zencoder_auto_convert(&$job) {
  $videofile = escapeshellarg($job->filepath); // escape file name for safety
  $convfile = tempnam(file_directory_temp(), 'video-rendering');
  $audiobitrate = variable_get('video_zencoder_helper_auto_cvr_audio_bitrate', 64);
  $videobitrate = variable_get('video_zencoder_helper_auto_cvr_video_bitrate', 200);
  $size = _video_render_get_size();
  $converter = variable_get('video_transcoder_path', '/usr/bin/ffmpeg');

  $options = preg_replace(array('/%videofile/', '/%convertfile/', '/%audiobitrate/', '/%size/', '/%videobitrate/'),
      array($videofile, $convfile, $audiobitrate, $size, $videobitrate),
      variable_get('video_zencoder_helper_auto_cvr_options',
      '-y -i %videofile -f flv -ar 22050 -ab %audiobitrate -s %size -b %videobitrate -qscale 1 %convertfile'));

  // set to the converted file output
  $job->convfile = $convfile;

  $command = VIDEO_RENDERING_NICE . " $converter $options";

  //print('executing ' . $command); die;
  watchdog('video_render', 'executing: ' . $command, array(), WATCHDOG_DEBUG);
  //   watchdog('video_render', 'Starting : ' . time());
  //execute the command
  ob_start();
  passthru($command." 2>&1", $command_return);
  $command_output = ob_get_contents();
  ob_end_clean();
  //  watchdog('video_render', 'Completed');
  //print $command_output;

  if (!file_exists($job->convfile) || !filesize($job->convfile)) {
    watchdog('video_render', 'video conversion failed. ffmpeg reported the following output: ' . $command_output, array(), WATCHDOG_ERROR);
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
//function _video_render_get_size() {
//  return variable_get('video_zencoder_width', 640) . 'x' . variable_get('video_zencoder_height', 480);
//}

function zencoder_add_job($file) {
// Make sure this points to a copy of Zencoder.php on the same server as this script.
//  require_once("../lib/zencoder/Zencoder.php");
  require_once dirname(__FILE__) . '/../lib/zencoder/Zencoder.php';

  // API Key
  $api_key = variable_get('zencoder_api_key', "");
  // File details
  $filename = $file->name;
  // Bucket
  $bucket = variable_get('s3_api_bucket', '');
  // New Encoding Job
  //28022009063.mp4
  //    "input": "s3://' . $bucket . '/' . $filename . '"
  //    "input": "s3://heidi_test/28022009063.mp4"
  $encoding_job = new ZencoderJob('
{
  "input":"s3://heidi_test/28022009063.mp4",
  "output":[{
      "label":"web",
      "base_url":"s3://heidi_test",
      "filename":"28022009063.mp4.flv",
      "width":400,
      "height":600,
      "quality":3,
      "speed":3,
      "upscale":1,
      "stretch":1,
      "frame_rate":1100,
      "max_frame_rate":2100,
      "keyframe_interval":2,
      "video_bitrate":20000,
      "bitrate_cap":32,
      "buffer_size":700,
      "h264_profile":"baseline",
      "h264_level":5,
      "skip_video":1,
      "audio_codec":"mp3",
      "audio_bitrate":123,
      "audio_channels":2,
      "audio_sample_rate":2344,
      "skip_audio":1,
      "thumbnails":{
        "number":5,
        "size":"128X128",
        "base_url":"s3://heidi_test",
        "prefix":"zc_thumb"
      },
      "notifications":[
        "http://locahost.com",
        "heshanmw@gmail.com"
      ],
      "start_clip":3,
      "clip_length":23
    }
  ],
  "api_key":"45ba476ace079f485ef96b4152a20b35"
}');

  // Check if it worked
  if ($encoding_job->created) {
  // Success
  //    echo "w00t! \n\n";
  //    echo "Job ID: ".$encoding_job->id."\n";
  //    echo "Output '".$encoding_job->outputs["web"]->label."' ID: ".$encoding_job->outputs["web"]->id."\n";
  // Store Job/Output IDs to update their status when notified or to check their progress.
  } else {
  // Failed
  //    echo "Fail :(\n\n";
  //    echo "Errors:\n";
    foreach($encoding_job->errors as $error) {
    //      echo $error."\n";
      watchdog('video_render', $error);
    }
  }
}

/* ************************************************** */
/* zencoder forms                               */
/* ************************************************** */

/**
 * Build a generic form for any module to implementm ffmpeg configuration.
 * This will give any module the ajax form configuration updates.
 * Validation and submission need to be handled by the calling module - this
 * only builds the form call this form inside your form function.
 *
 * @param $configuration
 *   An array of configuration data - could be $form_values.
 * @param $prefix
 *   A prefix for the form elelements, needed for javascript activation on
 *   complex forms (eg: media mover).
 * @return array
 *   A Drupal form array.
 */
function zencoder_configuration_form($configuration = array(), $form_prefix = '' ) {

// enable the javascript configuration options on the output type to use AJAX
// to update the allowed values
//  zencoder_enable_js($form_prefix, 'ffmpeg_output_type');

  $form['zencoder'] = array(
      '#type' => 'fieldset',
      '#title' => t("Zencoder Settings"),
      '#collapsed' => false,
      '#description' => t('All output settings are optional. We choose sensible defaults for web and iPhone playback.')
  );

  // Basic Video Settings
  $form['zencoder']['basic_video'] = array(
      '#type' => 'fieldset',
      '#title' => t("Basic Video Settings"),
      '#collapsed' => true,
      '#collapsible' => true,
      '#description' => t('')
  );

  $form['zencoder']['basic_video']['zc_width'] = array(
      '#type' => 'textfield',
      '#title' => t('Video width'),
      '#default_value' => variable_get('zc_width', ''),
      '#description' => t('Width of the converted video, of the format 600x400. This is the maximum width of the output video specified as a positive integer. In order for video compression to work properly the width should be divisible by 4 (or even better 16).'),
      '#size' => 12,
  );
  $form['zencoder']['basic_video']['zc_height'] = array(
      '#type' => 'textfield',
      '#title' => t('Video height'),
      '#default_value' => variable_get('zc_height', ''),
      '#description' => t('Width of the converted video, of the format 600x400. This is the maximum height of the output video specified as a positive integer. In order for video compression to work properly the height should be divisible by 4 (or even better 16).'),
      '#size' => 12,
  );
  $quality = array(
      1=>'1 - Poor Quality (Smaller file)',
      2=>2,
      3=>'3 - Default',
      4=>4,
      5=>'5 - High Quality (Larger file)'
  );
  $form['zencoder']['basic_video']['zc_quality'] = array(
      '#type' => 'select',
      '#title' => t('Quality'),
      '#options' => $quality,
      '#default_value' => variable_get('zc_quality', 3),
      '#description' => t('This is the desired output video quality. A higher quality setting will mean higher bitrates and higher file sizes. A quality setting of 5 will be nearly lossless and a setting of 1 will be quite compressed and may not look great. Higher quality encoding is also a bit slower than lower quality encoding. As a rule of thumb, lowering quality by a level will reduce file size by about 40%. A quality setting of 3-4 usually looks pretty good. Note that the actual bitrate will vary when using the quality setting, depending on the type of video. Even at the same quality setting, low-complexity video (like a screencast) will generally result in lower bitrates than high-complexity video (like a movie).'),
  );
  $speed = array(
      1=>'1 - Slow (Better Compression)',
      2=>2,
      3=>'3 - Default',
      4=>4,
      5=>'5 - Fast (Worst Compression)'
  );
  $form['zencoder']['basic_video']['zc_speed'] = array(
      '#type' => 'select',
      '#title' => t('Speed'),
      '#options' => $speed,
      '#default_value' => variable_get('zc_speed', 3),
      '#description' => t('This is the desired speed of encoding. A lower speed setting will mean slower encode times, but lower file sizes and better quality video. A high speed setting will transcode quickly, but compression will be less efficient and result in larger files with lower quality output.'),
  );

  // Advaced Video Settings
  $form['zencoder']['adv_video'] = array(
      '#type' => 'fieldset',
      '#title' => t("Advanced Video Settings"),
      '#collapsed' => true,
      '#collapsible' => true,
      '#description' => t('')
  );

  $form['zencoder']['adv_video']['zc_upscale'] = array(
      '#type' => 'checkbox',
      '#title' => t('Upscale?'),
      '#description' => t("If a video is received that is smaller than your maximum frame size (width and height settings), this setting will determine if we will increase the size of the input video to the supplied width and height. For example, if width is set to 640 and the input file is 320x240, the upscale option will force the output video to expand to 640x480. If false, the output video will match the input's original size."),
      '#default_value' => variable_get('zc_upscale', ''),
  );
  $form['zencoder']['adv_video']['zc_stretch'] = array(
      '#type' => 'checkbox',
      '#title' => t('Stretch?'),
      '#description' => t("If true, the aspect ratio of the original file may be distorted to fit the aspect ratio of the supplied width and height. By default, aspect ratio will always be preserved."),
      '#default_value' => variable_get('zc_stretch', ''),
  );
  $form['zencoder']['adv_video']['zc_frame_rate'] = array(
      '#type' => 'textfield',
      '#title' => t('Frame Rate '),
      '#default_value' => variable_get('zc_frame_rate', ''),
      '#description' => t('The output frame rate to use specified as a decimal number (e.g. 15 or 24.98). Unless you need to target a specific frame rate, you might be better off using Maximum Frame Rate setting. By default, the original frame rate will be preserved.'),
      '#size' => 12,
  );
  $form['zencoder']['adv_video']['zc_max_frame_rate'] = array(
      '#type' => 'textfield',
      '#title' => t('Maximum Frame Rate'),
      '#default_value' => variable_get('zc_max_frame_rate', ''),
      '#description' => t('The maximum frame rate to allow specified as a decimal number (e.g. 15 or 24.98). If the original frame rate is lower than this value, it will not be increased. Otherwise, it will be lowered to the max frame rate. By default, the original frame rate will be preserved.'),
      '#size' => 12,
  );
  $form['zencoder']['adv_video']['zc_key_frame_interval'] = array(
      '#type' => 'textfield',
      '#title' => t('Keyframe Interval'),
      '#default_value' => variable_get('zc_key_frame_interval', ''),
      '#description' => t('By default, a keyframe will be created every 250 frames. Specifying a keyframe interval will allow you to create more or less keyframes in your video. Sometimes the number of keyframes can affect video scrubbing. A greater number of keyframes will increase the size of your output file. Keyframe interval should be specified as a positive integer. For example, a value of 100 will create a keyframe every 100 frames.'),
      '#size' => 12,
  );
  $form['zencoder']['adv_video']['zc_vid_bit_rate'] = array(
      '#type' => 'textfield',
      '#title' => t('Video Bitrate'),
      '#default_value' => variable_get('zc_vid_bit_rate', ''),
      '#description' => t('The target video bitrate specified as kilobits per second (Kbps, e.g. 300 or 500). If you set quality, you don\'t need to set bitrate, unless you want to target a specific bitrate.'),
      '#size' => 12,
  );
  $form['zencoder']['adv_video']['zc_bit_rate_cap'] = array(
      '#type' => 'textfield',
      '#title' => t('Bitrate Cap'),
      '#default_value' => variable_get('zc_bit_rate_cap', ''),
      '#description' => t('A bitrate cap specified as kilobits per second (Kbps, e.g. 300 or 500). Specifying a quality alone will vary the bitrate to match the content of the video potentially introducing high peak bitrates. This setting will prevent peaks in the video bitrate, resulting in a file that looks worse but plays more smoothly. We recommend only using this for streaming or for devices that require it.'),
      '#size' => 12,
  );
  $form['zencoder']['adv_video']['zc_buffer_size'] = array(
      '#type' => 'textfield',
      '#title' => t('Buffer Size'),
      '#default_value' => variable_get('zc_buffer_size', ''),
      '#description' => t('The buffer size specified as kilobits per second (Kbps, e.g. 300 or 500). This is another setting useful for playback on specific devices. Check your streaming server or device settings for tips, or set this to the same as the bitrate cap.'),
      '#size' => 12,
  );
  $profile = array(
      'baseline' => 'Baseline - Default',
      'main' => 'Main',
      'high' => 'High - Best for web playback'
  );
  $form['zencoder']['adv_video']['zc_h245_profile'] = array(
      '#type' => 'select',
      '#title' => t('H.264 Profile'),
      '#options' => $profile,
      '#default_value' => variable_get('zc_h245_profile', 1),
      '#description' => t('The H.264 standard defines various sets of capabilities, which are referred to as profiles, targeting specific classes of applications. The currently supported profiles are baseline, main, and high. The baseline profile will produce video that looks worse, but requires less CPU on playback and is required for playback on some devices (e.g. iPhone or iPod). The high profile will look best, but can be CPU intensive to playback. By default, the baseline profile will be used to maximize compatibility.'),
  );
  $level = array(
      '' => '',
      '1' => '1',
      '1.1' => '1.1',
      '1.2' => '1.2',
      '1.3' => '1.3',
      '2' => '2',
      '2.1' => '2.1',
      '2.2' => '2.2',
      '3' => '3',
      '3.1' => '3.1',
      '3.2' => '3.2',
      '4' => '4',
      '4.1' => '4.1',
      '4.2' => '4.2',
      '5' => '5',
      '5.1' => '5.1'
  );
  $form['zencoder']['adv_video']['zc_h245_level'] = array(
      '#type' => 'select',
      '#title' => t('H.264 Level'),
      '#options' => $level,
      '#default_value' => variable_get('zc_h245_level', 0),
      '#description' => t('Similar to profiles, the H.264 standard defines various levels that will limit the size and complexity of the output file. We recommend leaving this blank unless your output device requires it. iPhone video can\'t exceed level 3, Blu-Ray is 4.1, and older iPods max out at 1.3. By default, level 3 will be used to ensure iPhone playback.'),
  );
  $form['zencoder']['adv_video']['zc_skip_video'] = array(
      '#type' => 'checkbox',
      '#title' => t('Skip Video'),
      '#description' => t("The video track will be omitted from the output. You can still specify a video format, however, no video track will be present in the resulting file."),
      '#default_value' => variable_get('zc_skip_video', ''),
  );

  // Advanced Audio Settings
  $form['zencoder']['adv_audio'] = array(
      '#type' => 'fieldset',
      '#title' => t("Advanced Audio Settings"),
      '#collapsed' => true,
      '#collapsible' => true,
      '#description' => t('')
  );
  $audio_codec = array(
      'aac' => 'AAC - Default',
      'mp3' => 'MP3'
  );
  $form['zencoder']['adv_audio']['zc_audio_codec'] = array(
      '#type' => 'select',
      '#title' => t('Audio Codec'),
      '#options' => $audio_codec,
      '#default_value' => variable_get('zc_audio_codec', 'aac'),
      '#description' => t('The audio codec used in the video file can affect the ability to play the video on certain devices. The default codec used is AAC and should only be changed if the playback device you are targeting requires something different.'),
  );
  $form['zencoder']['adv_audio']['zc_audio_bitrate'] = array(
      '#type' => 'textfield',
      '#title' => t('Audio Bitrate'),
      '#default_value' => variable_get('zc_audio_bitrate', ''),
      '#description' => t('The overall audio bitrate specified as kilobits per second (Kbps, e.g. 96 or 160). This value can\'t exceed 160 Kbps per channel. 96-160 is usually a good range for stereo output.'),
      '#size' => 12,
  );
  $audio_channel = array(
      1 => 'Mono',
      2 => 'Stereo - Default'
  );
  $form['zencoder']['adv_audio']['zc_audio_channels'] = array(
      '#type' => 'select',
      '#title' => t('Audio Channels'),
      '#options' => $audio_channel,
      '#default_value' => variable_get('zc_audio_channels', 2),
      '#description' => t('By default we will choose the lesser of the number of audio channels in the input file or 2 (stereo).'),
  );
  $form['zencoder']['adv_audio']['zc_audio_sample_rate'] = array(
      '#type' => 'textfield',
      '#title' => t('Audio Sample Rate'),
      '#default_value' => variable_get('zc_audio_sample_rate', ''),
      '#description' => t('The sample rate of the audio file specified in hertz (e.g. 44100 or 22050). A sample rate of 44100 is the best option for web playback. 22050 and 48000 are also valid options. Warning: the wrong setting may cause encoding problems. By default, 44100 will be used.'),
      '#size' => 12,
  );
  $form['zencoder']['adv_audio']['zc_skip_audio'] = array(
      '#type' => 'checkbox',
      '#title' => t('Skip Audio'),
      '#description' => t("The audio track will be omitted from the output. You must specify a video format and no audio track will be present in the resulting file."),
      '#default_value' => variable_get('zc_skip_audio', ''),
  );

  // Notification Settings
  $form['zencoder']['notification'] = array(
      '#type' => 'fieldset',
      '#title' => t("Notification Settings"),
      '#collapsed' => true,
      '#collapsible' => true,
      '#description' => t('')
  );
  $form['zencoder']['notification']['zc_notify_url'] = array(
      '#type' => 'textfield',
      '#title' => t('URL'),
      '#default_value' => variable_get('zc_notify_url', ''),
      '#description' => t('We will POST a JSON notification to the URL provided. We will consider the notification successful if we receive a 200 OK response.'),
  );
  $form['zencoder']['notification']['zc_notify_email'] = array(
      '#type' => 'textfield',
      '#title' => t('Email Address'),
      '#default_value' => variable_get('zc_notify_email', ''),
      '#description' => t('We will send a notification email to the URL provided. The notification will contain basic file status information.'),
  );


  // Other Settings
  $form['zencoder']['other'] = array(
      '#type' => 'fieldset',
      '#title' => t("Other Settings"),
      '#collapsed' => true,
      '#collapsible' => true,
      '#description' => t('')
  );
  $form['zencoder']['other']['zc_start_clip'] = array(
      '#type' => 'textfield',
      '#title' => t('Start Clip'),
      '#default_value' => variable_get('zc_start_clip', ''),
      '#description' => t('Creates a subclip from the input file, starting at either a timecode or a number of seconds. Format: 0:01:23.6 or 88.6 for 1 minute and 23.6 seconds.'),
      '#size' => 12,
  );
  $form['zencoder']['other']['zc_clip_length'] = array(
      '#type' => 'textfield',
      '#title' => t('Clip Length'),
      '#default_value' => variable_get('zc_clip_length', ''),
      '#description' => t('Creates a subclip from the input file of the specified length using either a timecode or a number of seconds. Format: 0:00:45.3 or 45.3 for 45.3 seconds.'),
      '#size' => 12,
  );
  return $form;
}
