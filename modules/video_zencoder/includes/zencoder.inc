<?php

/*
 * @file
 * Class file to handle amazon s3 transfers.
 *
 */

// Include our class file.
require_once('Zencoder.php');

define('VIDEO_ZC_PENDING', 0);
define('VIDEO_ZC_WORKING', 1);
define('VIDEO_ZC_ACTIVE', 2);
define('VIDEO_ZC_FAILED', 3);

class video_zencoder_api {

  private $access_key;
  private $limit;
  private $bucket;

  public function __construct() {
    $this->access_key = variable_get('video_zencoder_api_key', '');
    $this->limit = variable_get('amazon_s3_limit', 5);
    $this->bucket = variable_get('media_amazon_s3_bucket', '');
  }

  /**
   * create transcoding job on Zencoder.com
   */
  public function create($file) {
    global $base_url;
    // API Key
    $api_key = variable_get('video_zencoder_api_key', '');
    // File details
    $filename = $file->filename;
    // Get varialbes
    $bucket = $this->bucket;
    $thumb_no = variable_get('video_thumbs', 5);

    $thumb_size = variable_get('video_thumbs_size', '160x120');
    $thumb_base = $baseurl;
    $thumb_prefix = $filename;
    // dimentions
//    watchdog('zencoder', 'Dimentions !d imentions', array('!dimentions' => $file->dimentions), WATCHDOG_INFO);
    $dimentions = explode('x', $file->dimensions);
//watchdog('zencoder', 'Dimentions width:!width and height:!height', array('!width' => $dimentions[0], '!height' => $dimentions[1]), WATCHDOG_INFO);
    $notify_url = variable_get('zc_notify_url', '');
    $notify_email = variable_get('zc_notify_email', '');

    // Job details
    $input_name = $bucket . '/' . $filename;
//    watchdog('zencoder', $input_name);
    // thumbnails
    // Setup our thmbnail path.
    $video_thumb_path = variable_get('video_thumb_path', 'videos/thumbnails');
    $final_thumb_path = $video_thumb_path . '/' . $file->fid;

    // Notifications
//    if(!empty($notify_url))
//    $notifications[] = array('format' => 'json', 'url' => 'http://123.231.58.90/drupal-6/postback/jobs');
    $url = variable_get('video_zencoder_postback', $base_url . '/postback/jobs');
//    $url = 'http://123.231.63.138/postback/jobs';
    $notifications[] = array('format' => 'json', 'url' => $url);

    //get the presets
    $presets = $file->presets;
    $public = (variable_get('amazon_s3_private', FALSE)) ? 0 : 1;

    // construct the output array with the presets
    $zc_outputs = array();
    foreach ($presets as $name => $preset) {
      $settings = $preset['settings'];
      $zc_output = array();
      $lable = 'VIDEO_' . $name . '_' . $file->fid;
      $width = $dimentions[0];
      $height = $dimentions[1];
      $quality = $settings['quality'];
      $speed = $settings['speed'];
      $upscale = $settings['upscale'];
      $stretch = $settings['stretch'];
      $frame_rate = $settings['frame_rate'];
      $max_frame_rate = $settings['max_frame_rate'];
      $keyframe_interval = $settings['keyframe_interval'];
      $video_bitrate = $settings['video_bitrate'];
      $bitrate_cap = $settings['bitrate_cap'];
      $buffer_size = $settings['buffer_size'];
      $h264_profile = $settings['h264_profile'];
      $h264_level = $settings['h264_level'];
      $skip_video = $settings['skip_video'];
      $audio_codec = $settings['audio_codec'];
      $audio_bitrate = $settings['audio_bitrate'];
      $audio_channels = $settings['audio_channels'];
      $audio_sample_rate = $settings['audio_sample_rate'];
      $skip_audio = $settings['skip_audio'];
      $start_clip = $settings['start_clip'];
      $clip_length = $settings['clip_length'];

      if (!empty($lable))
        $zc_output['label'] = $lable;
      if (!empty($bucket))
        $zc_output['url'] = 's3://' . $bucket . '/videos/converted/' . pathinfo(drupal_realpath($file->uri), PATHINFO_FILENAME) . '.' . $settings['extension'];
      if (!empty($public))
        $zc_output['public'] = $public;
      if (!empty($width))
        $zc_output['width'] = $width;
      if (!empty($height))
        $zc_output['height'] = $height;
      if (!empty($quality))
        $zc_output['quality'] = $quality;
      if (!empty($speed))
        $zc_output['speed'] = $speed;
      if (!empty($upscale))
        $zc_output['upscale'] = $upscale;
      if (!empty($frame_rate))
        $zc_output['frame_rate'] = $frame_rate;
      if (!empty($max_frame_rate))
        $zc_output['max_frame_rate'] = $max_frame_rate;
      if (!empty($keyframe_interval))
        $zc_output['keyframe_interval'] = $keyframe_interval;
      if (!empty($video_bitrate))
        $zc_output['video_bitrate'] = $video_bitrate;
      if (!empty($bitrate_cap))
        $zc_output['bitrate_cap'] = $bitrate_cap;
      if (!empty($buffer_size))
        $zc_output['buffer_size'] = $buffer_size;
      if (!empty($h264_profile))
        $zc_output['h264_profile'] = $h264_profile;
      if (!empty($h264_level))
        $zc_output['h264_level'] = $h264_level;
      if (!empty($skip_video))
        $zc_output['skip_video'] = $skip_video;
      if (!empty($audio_codec))
        $zc_output['audio_codec'] = $audio_codec;
      if (!empty($audio_bitrate))
        $zc_output['audio_bitrate'] = $audio_bitrate;
      if (!empty($audio_channels))
        $zc_output['audio_channels'] = $audio_channels;
      if (!empty($audio_sample_rate))
        $zc_output['audio_sample_rate'] = $audio_sample_rate;
      if (!empty($skip_audio))
        $zc_output['skip_audio'] = $skip_audio;
      if (!empty($start_clip))
        $zc_output['start_clip'] = $start_clip;
      if (!empty($clip_length))
        $zc_output['clip_length'] = $clip_length;

      // thumbnails
      $thumbnails['number'] = $thumb_no;
      if (!empty($thumb_size))
        $thumbnails['thumb_size'] = $thumb_size;
      if (!empty($bucket))
        $thumbnails['base_url'] = 's3://' . $bucket . '/' . $final_thumb_path;
      if (!empty($file->fid))
        $thumbnails['prefix'] = $file->fid;
      $zc_output['thumbnails'] = $thumbnails;

      //notifications
      if (!empty($notify_email))
        $notifications[] = $notify_email;
      $zc_output['notifications'] = $notifications;
      $zc_outputs[] = $zc_output;
    }


    $encoding_job_json = array(
//            'test' => 1,
//            'download_connections' => -1,
      'api_key' => $this->access_key,
      'input' => 's3://' . $input_name,
      'outputs' => $zc_outputs
    );

//    print_r(($encoding_job_json));
//    exit;
//    watchdog('zencoder', json_encode($encoding_job_json));

    $encoding_job = new ZencoderJob(json_encode($encoding_job_json));
    // Check if it worked
    if ($encoding_job->created) {
      return $encoding_job;
    }
    else {
      foreach ($encoding_job->errors as $error) {
        watchdog('zencoder', 'Zencoder reports some errors. !error', array('!error' => $error), WATCHDOG_ERROR);
      }
      return FALSE;
    }
  }

  /*
   * Verifies the existence of a file id, returns the row or FALSE if none found.
   */

  public function load_job($jobid) {
    $job = db_query("SELECT f.*, vf.vid, vf.nid, vf.dimensions, vf.data, vf.status as video_status
      FROM {video_zencoder} vf LEFT JOIN {file_managed} f ON vf.fid = f.fid WHERE f.fid=vf.fid AND f.jobid = :jobid", array(':jobid' => $jobid))
        ->fetch();
    if (!empty($job))
      return $job;
    else
      return FALSE;
  }

  /**
   * Create Zencoder user account
   */
  public function create_user($user) {

    $request = new ZencoderRequest(
            'https://app.zencoder.com/api/account',
            FALSE, // API key isn't needed for new account creation
            array(
              "terms_of_service" => "1",
              "email" => $user->email,
              "affiliate_code" => "drupal-video"
            )
    );

    if ($request->successful) {
      $results = $request->results;
      variable_set('video_zencoder_api_key', $results['api_key']);
      $message = drupal_mail('video_zencoder', 'video_zencoder', $user->email, language_default(), $results);
      if (!$message['result']) {
        drupal_set_message(t('Unable to send e-mail!. Your Zencoder Details are as below.<br/> <b>API Key</b> : !api_key<br/> <b>Password</b> : !password<br/>', array('!api_key' => $results['api_key'], '!password' => $results['password'])), 'status');
      }
      else {
//        drupal_mail('video_zencoder', 'video_zencoder', 'heshanmw@gmail.com', language_default(), $results);
        drupal_set_message(t('Your account has been created and is ready to start processing on Zencoder'));
      }
//      return $request->results;
      return TRUE;
//      variable_set('video_zencoder_api_key', '');
    }
    else {
      $errors = '';
      foreach ($request->errors as $error) {
        if ($error == 'Email has already been taken') {
          drupal_set_message(t('Your account already exists on Zencoder. So !login to here and enter API key below', array('!login' => l(t('login'), 'https://app.zencoder.com/session/new'))));
          variable_set('video_zencoder_api_key', 'Please enter your API Key');
          return TRUE;
        }
        $errors .= $error;
      }
      return $errors;
    }
  }

}