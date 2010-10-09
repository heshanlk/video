<?php

//$Id$
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
    $this->access_key = variable_get('zencoder_api_key', '');
    $this->limit = variable_get('amazon_s3_limit', 5);
    $this->bucket = variable_get('amazon_s3_bucket', '');
  }

  /**
   * create transcoding job on Zencoder.com
   */
  public function create($file) {
    global $base_url;
    // API Key
    $api_key = variable_get('zencoder_api_key', '');
    // File details
    $filename = $file->filename;
    //file type
    $filetype = variable_get('video_zencoder_ext', 'flv');
    // Get varialbes
    $lable = 'VIDEO_' . $file->fid;
//    $base_url = $folder;
    $public = (variable_get('amazon_s3_private', FALSE)) ? 0 : 1;
    $width = variable_get('zc_width', '');
    $height = variable_get('zc_height', '');
    $quality = variable_get('zc_quality', 3);
    $speed = variable_get('zc_speed', 3);
    $upscale = variable_get('zc_upscale', '');
    $stretch = variable_get('zc_stretch', '');
    $frame_rate = variable_get('zc_frame_rate', '');
    $max_frame_rate = variable_get('zc_max_frame_rate', '');
    $keyframe_interval = variable_get('zc_key_frame_interval', '');
    $video_bitrate = variable_get('zc_vid_bit_rate', '');
    $bitrate_cap = variable_get('zc_bit_rate_cap', '');
    $buffer_size = variable_get('zc_buffer_size', '');
    $h264_profile = variable_get('zc_h245_profile', 1);
    $h264_level = variable_get('zc_h245_level', 0);
    $skip_video = variable_get('zc_skip_video', '');
    $audio_codec = variable_get('zc_audio_codec', 'aac');
    $audio_bitrate = variable_get('zc_audio_bitrate', '');
    $audio_channels = variable_get('zc_audio_channels', 2);
    $audio_sample_rate = variable_get('zc_audio_sample_rate', '');
    $skip_audio = variable_get('zc_skip_audio', '');
    $thumb_no = variable_get('no_of_video_thumbs', 5);
    // TODO :
    $thumb_size = '160x120';
    $thumb_base = $baseurl;
    $thumb_prefix = $filename;
    $notify_url = variable_get('zc_notify_url', '');
    $notify_email = variable_get('zc_notify_email', '');
    $start_clip = variable_get('zc_start_clip', '');
    $clip_length = variable_get('zc_clip_length', '');

    $bucket = $this->bucket;

    // Job details
    $input_name = $bucket . '/' . $filename;
//    watchdog('zencoder', $input_name);

    $zc_output = array();
    if (!empty($lable))
      $zc_output['label'] = $lable;
    if (!empty($bucket))
      $zc_output['url'] = 's3://' . $bucket . '/' . $filename . '.' . $filetype;
    if (!empty($public))
      $zc_output['public'] = $public;
    if (!empty($width))
      $zc_output['width'] = $width;
    if (!empty($height))
      $zc_output['height'] = $height;
    if ($quality != 3)
      $zc_output['quality'] = $quality;
    if ($speed != 3)
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
    if (($h264_profile != 1))
      $zc_output['h264_profile'] = $h264_profile;
    if (($h264_level != 0))
      $zc_output['h264_level'] = $h264_level;
    if (!empty($skip_video))
      $zc_output['skip_video'] = $skip_video;
    if (($audio_codec != 'aac'))
      $zc_output['audio_codec'] = $audio_codec;
    if (!empty($audio_bitrate))
      $zc_output['audio_bitrate'] = $audio_bitrate;
    if (($audio_channels != 2))
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
    // Setup our thmbnail path.
    $video_thumb_path = variable_get('video_thumb_path', 'video_thumbs');
    $final_thumb_path = file_directory_path() . '/' . $video_thumb_path . '/' . $file->fid;

    $thumbnails['number'] = $thumb_no;
    if (!empty($thumb_size))
      $thumbnails['thumb_size'] = $thumb_size;
    if (!empty($bucket))
      $thumbnails['base_url'] = 's3://' . $bucket . '/' . $final_thumb_path;
    if (!empty($file->fid))
      $thumbnails['prefix'] = $file->fid;
    $zc_output['thumbnails'] = $thumbnails;

    // Notifications
//    if(!empty($notify_url))
//    $notifications[] = array('format' => 'json', 'url' => 'http://123.231.58.90/drupal-6/postback/jobs');
//    print $base_url;
    $url = $base_url . '/postback/jobs';
//    $url = 'http://123.231.47.131/drupal-6/postback/jobs';
    $notifications[] = array('format' => 'json', 'url' => $url);
//    base_path()
//    $notifications[] = 'http://123.231.9.136/drupal-6/postback/jobs';
    if (!empty($notify_email))
      $notifications[] = $notify_email;
    $zc_output['notifications'] = $notifications;


    $encoding_job_json = array(
//            'test' => 1,
//            'download_connections' => -1,
      'api_key' => $this->access_key,
      'input' => 's3://' . $input_name . '',
      'output' => $zc_output
    );


//    watchdog('zencoder', json_encode($encoding_job_json));

    $encoding_job = new ZencoderJob(json_encode($encoding_job_json));

    // Check if it worked
    if ($encoding_job->created) {
//      print_r($encoding_job);
//      watchdog('zencoder', serialize($encoding_job));
      return $encoding_job;
    } else {
      foreach ($encoding_job->errors as $error) {
        //      echo $error."\n";
        watchdog('zencoder', 'Zencoder reports some errors. !error', array('!error' => $error), WATCHDOG_ERROR);
      }
      return false;
    }
  }

  /*
   * Updates the database after a successful transfer to amazon.
   */

  public function update($video) {
    $result = db_query("UPDATE {video_zencoder} SET filepath='%s', status=%d, completed=%d WHERE jobid=%d AND outputid = %d",
            $video->output->url, $video->output->state, time(), $video->job->id, $video->output->id);
    return $result;
  }

  /*
   * Verifies the existence of a file id, returns the row or false if none found.
   */

  public function load_job($jobid) {
    $sql = db_query("SELECT * FROM {video_zencoder} WHERE jobid=%d", $jobid);
    $row = db_fetch_object($sql);
    return $row;
  }

  /**
   * Create Zencoder user account
   */
  public function create_user($user) {

    $request = new ZencoderRequest(
            'https://app.zencoder.com/api/account',
            false, // API key isn't needed for new account creation
            array(
              "terms_of_service" => "1",
              "email" => $user->email,
              "affiliate_code" => "drupal-video"
            )
    );

    if ($request->successful) {
      $results = $request->results;
      variable_set('zencoder_api_key', $results['api_key']);
      $message = drupal_mail('video_zencoder', 'video_zencoder', $user->email, language_default(), $results);
      if (!$message['result']) {
        drupal_set_message(t('Unable to send e-mail!. Your Zencoder Details are as below.<br/> <b>API Key</b> : !api_key<br/> <b>Password</b> : !password<br/>', array('!api_key' => $results['api_key'], '!password' => $results['password'])), 'status');
      } else {
//        drupal_mail('video_zencoder', 'video_zencoder', 'heshanmw@gmail.com', language_default(), $results);
        drupal_set_message(t('Your account has been created and is ready to start processing on Zencoder'));
      }
//      return $request->results;
      return true;
//      variable_set('zencoder_api_key', '');
    } else {
      $errors = '';
      foreach ($request->errors as $error) {
        $errors .= $error;
      }
      return $errors;
    }
  }

}