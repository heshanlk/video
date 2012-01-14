<?php
/*
 * @file
 * Class file to handle amazon s3 transfers.
 *
 */

define('VIDEO_ZC_PENDING', 0);
define('VIDEO_ZC_WORKING', 1);
define('VIDEO_ZC_ACTIVE', 2);
define('VIDEO_ZC_FAILED', 3);

class video_zencoder_api {
  private $access_key;
  private $limit;
  private $bucket;
  
  /**
   * @var Services_Zencoder
   */
  private $zencoder;

  public function __construct() {
    // Create the Zencoder class
    $libpath = libraries_get_path('zencoder');
    $libfile = $libpath .'/Services/Zencoder.php';

    if (!file_exists($libfile)) {
      drupal_set_message(t('The Zencoder library has not been installed correctly. See the <a href="@drupal-status-page">Drupal status page</a> for more information.', array('@drupal-status-page' => url('admin/reports/status'))), 'error');
      return;
    }

    $this->access_key = variable_get('video_zencoder_api_key', NULL);
    $this->limit = variable_get('amazon_s3_limit', 5);
    $this->bucket = variable_get('amazon_s3_bucket', '');

    require_once $libfile;
    $this->zencoder = new Services_Zencoder($this->access_key);
  }

  /**
   * create transcoding job on Zencoder.com
   */
  public function create($file) {
    if ($this->zencoder == NULL) {
      return FALSE;
    }

    // dimensions
    $dimensions = explode('x', $file->dimensions);

    // Notifications
    $notifications = array(array(
      'format' => 'json',
      'url' => variable_get('video_zencoder_postback', url('postback/jobs', array('absolute' => TRUE))),
    ));

    // S3 permissions
    $public = !variable_get('amazon_s3_private', FALSE);

    // Common output URL prefix
    $output_url_prefix = 's3://'. $this->bucket .'/'. pathinfo($file->filepath, PATHINFO_DIRNAME) .'/converted/'. pathinfo($file->filepath, PATHINFO_FILENAME) .'.';

    // construct the output array with the presets
    $zc_outputs = array();
    foreach ($file->presets as $name => $preset) {
      $zc_output = array();
      $quality = $preset['quality'];
      $speed = $preset['speed'];
      $upscale = $preset['upscale'];
      $stretch = $preset['stretch'];
      $frame_rate = $preset['frame_rate'];
      $max_frame_rate = $preset['max_frame_rate'];
      $keyframe_interval = $preset['keyframe_interval'];
      $video_bitrate = $preset['video_bitrate'];
      $bitrate_cap = $preset['bitrate_cap'];
      $buffer_size = $preset['buffer_size'];
      $h264_profile = $preset['h264_profile'];
      $h264_level = $preset['h264_level'];
      $skip_video = $preset['skip_video'];
      $audio_codec = $preset['audio_codec'];
      $audio_bitrate = $preset['audio_bitrate'];
      $audio_channels = $preset['audio_channels'];
      $audio_sample_rate = $preset['audio_sample_rate'];
      $skip_audio = $preset['skip_audio'];
      $start_clip = $preset['start_clip'];
      $clip_length = $preset['clip_length'];

      $zc_output['label'] = 'VIDEO_'. $name .'_'. $file->fid;
      $zc_output['url'] = $output_url_prefix . $preset['extension'];
      $zc_output['public'] = $public;
      $zc_output['width'] = $dimensions[0];
      $zc_output['height'] = $dimensions[1];

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

      //notifications
      $zc_output['notifications'] = $notifications;
      $zc_outputs[] = $zc_output;
    }

    // Add thumbnails
    if (empty($zc_outputs)) $zc_outputs[0] = array();
    $video_thumb_path = variable_get('video_thumb_path', 'video_thumbs');
    $final_thumb_path = file_directory_path() .'/'. $video_thumb_path .'/'. $file->fid;

    $zc_outputs[0]['thumbnails'] = array(
      'number' => intval(variable_get('video_thumbs', 5)),
      'size' => variable_get('video_thumbs_size', '160x120'),
      'base_url' => 's3://'. $this->bucket .'/'. $final_thumb_path,
      'prefix' => $file->fid,
    );

    $encoding_job_json = array(
      'api_key' => $this->access_key,
      'input' => 's3://'. $this->bucket .'/'. $file->filepath,
      'outputs' => $zc_outputs,
    );

    try {
      return $this->zencoder->jobs->create($encoding_job_json);
    }
    catch (Services_Zencoder_Exception $e) {
      watchdog('zencoder', 'Zencoder reports errors while converting %file:<br/>!errorlist', array('%file' => $file->filename, '!errorlist' => theme('item_list', $e->getErrors())), WATCHDOG_ERROR);
      return FALSE;
    }
  }

  /**
   * Create Zencoder user account
   */
  public function create_user($email) {
    if ($this->zencoder == NULL) {
      return FALSE;
    }

    try {
      // $result is Services_Zencoder_Account
      $result = $this->zencoder->accounts->create(array(
        'terms_of_service' => '1',
        'email' => $email,
        'affiliate_code' => 'drupal-video',
      ));
      
      $params = array(
        'email' => $email,
        'api_key' => $result->api_key,
        'password' => $result->password,
      );
      
      variable_set('video_zencoder_api_key', $result->api_key);
      $message = drupal_mail('video_zencoder', 'video_zencoder', $email, language_default(), $params);
      if (!$message['result']) {
        drupal_set_message(t('Unable to send e-mail!. Your Zencoder Details are as below.<br/> <b>API Key</b> : !api_key<br/> <b>Password</b> : !password<br/>', array('!api_key' => $results['api_key'], '!password' => $results['password'])), 'status');
      }
      else {
        drupal_set_message(t('Your account has been created and is ready to start processing on Zencoder'));
      }

      return TRUE;
    }
    catch (Services_Zencoder_Exception $e) {
      $errors = '';
      foreach ($e->getErrors() as $error) {
        if ($error == 'Email has already been taken') {
          drupal_set_message(t('Your account already exists on Zencoder. So !login to here and enter API key below.', array('!login' => l(t('login'), 'https://app.zencoder.com/session/new'))));
          variable_set('video_zencoder_api_key', 'Please enter your API Key');
          return TRUE;
        }
        $errors .= $error;
      }

      return $errors;
    }
  }
}