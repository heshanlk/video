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

  private $jobid;
  private $access_key;
  private $limit;
  private $bucket;

  public function __construct() {
    $this->access_key = variable_get('zencoder_api_key', '');
    $this->limit = variable_get('amazon_s3_limit',5);
    $this->bucket = variable_get('amazon_s3_bucket', '');
  }


  /**
   * create transcoding job on Zencoder.com
   */
  public function create($file) {
    // API Key
    $api_key = variable_get('zencoder_api_key', '');
    // File details
    $filename = $file->filename;
    // Get varialbes
    $lable = 'VIDEO_' . $file->fid;
    $base_url = $folder;
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
    $notify_email = variable_get('zc_notify_email', 'heshanmw@gmail.com');
    $start_clip = variable_get('zc_start_clip', '');
    $clip_length = variable_get('zc_clip_length', '');

    $bucket = $this->bucket;

    // Job details
    $input_name = $bucket . '/' . $filename;
//    watchdog('zencoder', $input_name);

    $zc_output = array();
    if(!empty($lable))
      $zc_output['label'] = $lable;
    if(!empty($bucket))
      $zc_output['url'] = 's3://' . $bucket . '/' . $filename . '.flv';
    if(!empty($width))
      $zc_output['width'] = $width;
    if(!empty($height))
      $zc_output['height'] = $height;
    if($quality != 3)
      $zc_output['quality'] = $quality;
    if($speed != 3)
      $zc_output['speed'] = $speed;
    if(!empty($upscale))
      $zc_output['upscale'] = $upscale;
    if(!empty($frame_rate))
      $zc_output['frame_rate'] = $frame_rate;
    if(!empty($max_frame_rate))
      $zc_output['max_frame_rate'] = $max_frame_rate;
    if(!empty($keyframe_interval))
      $zc_output['keyframe_interval'] = $keyframe_interval;
    if(!empty($video_bitrate))
      $zc_output['video_bitrate'] = $video_bitrate;
    if(!empty($bitrate_cap))
      $zc_output['bitrate_cap'] = $bitrate_cap;
    if(!empty($buffer_size))
      $zc_output['buffer_size'] = $buffer_size;
    if(($h264_profile != 1))
      $zc_output['h264_profile'] = $h264_profile;
    if(($h264_level != 0))
      $zc_output['h264_level'] = $h264_level;
    if(!empty($skip_video))
      $zc_output['skip_video'] = $skip_video;
    if(($audio_codec != 'aac'))
      $zc_output['audio_codec'] = $audio_codec;
    if(!empty($audio_bitrate))
      $zc_output['audio_bitrate'] = $audio_bitrate;
    if(($audio_channels != 2))
      $zc_output['audio_channels'] = $audio_channels;
    if(!empty($audio_sample_rate))
      $zc_output['audio_sample_rate'] = $audio_sample_rate;
    if(!empty($skip_audio))
      $zc_output['skip_audio'] = $skip_audio;
    if(!empty($start_clip))
      $zc_output['start_clip'] = $start_clip;
    if(!empty($clip_length))
      $zc_output['clip_length'] = $clip_length;

// thumbnails
    $thumbnails['number'] = $thumb_no;
    if(!empty($thumb_size))
      $thumbnails['thumb_size'] = $thumb_size;
    if(!empty($bucket))
      $thumbnails['base_url'] = 's3://' . $bucket;
    if(!empty($lable))
      $thumbnails['prefix'] = $lable;
    $zc_output['thumbnails'] = $thumbnails;

    // Notifications
    if(!empty($notify_url))
      $notifications[] = $notify_url;
    if(!empty($notify_email))
      $notifications[] = $notify_email;
    $zc_output['notifications'] = $notifications;


    $encoding_job_json = array(
//            'test' => 1,
//            'download_connections' => -1,
            'api_key' => $this->access_key,
            'input' => 's3://' . $input_name . '',
            'output' => $zc_output
    );


    watchdog('zencoder', json_encode($encoding_job_json));

    $encoding_job = new ZencoderJob(json_encode($encoding_job_json));

    // Check if it worked
    if ($encoding_job->created) {
//      watchdog('zencoder', $encoding_job->id);
      return $encoding_job;
    }
    else {
      foreach($encoding_job->errors as $error) {
        //      echo $error."\n";
        watchdog('zencoder', 'Zencoder reports some errors. !error', array('!error' => $error), WATCHDOG_ERROR);
      }
      return false;
    }

  }

  /*
   * Inserts file object into the database.
  */
  public function insert($fid, $jobid = 0) {
    db_query("INSERT INTO {video_zencoder} (fid, jobid, status) VALUES (%d, %d, %d)", $fid, $jobid, VIDEO_ZC_PENDING);
  }

  /*
   * Updates the database after a successful transfer to amazon.
  */
  public function update($video) {
    $result = db_query("UPDATE {video_zencoder} SET jobid = %d, outputid = %d, bucket='%s', filename='%s', filepath='%s', filemime='%s', filesize='%s', status=%d, completed=%d WHERE vid=%d",
            $video->jobid, $video->outputid, $video->bucket, $video->filename, $video->filepath, $video->filemime, $video->filesize, VIDEO_ZC_ACTIVE, time(), $video->vid);
    return $result;
  }

  public function working($vid) {
    db_query("UPDATE {video_zencoder} SET status=%d WHERE vid=%d", VIDEO_ZC_WORKING, $vid);
  }

  public function failed($vid) {
    db_query("UPDATE {video_zencoder} SET status=%d WHERE vid=%d", VIDEO_ZC_FAILED, $vid);
  }
  /*
   * Verifies the existence of a file id, returns the row or false if none found.
  */
  public function verify($fid) {
    $sql = db_query("SELECT * FROM {video_zencoder} WHERE fid=%d", $fid);
    $row = db_fetch_object($sql);
    return $row;
  }

  public function delete($fid) {
    // Lets get our file no matter the status and delete it.
    if($video = $this->verify($fid)) {
      // Lets delete our record from the database.
      db_query("DELETE FROM {video_zencoder} WHERE vid=%d", $video->vid);
    }
  }
  public function get_job_id() {
    return $this->jobid;
  }
  /*
   * Gets a video object from the database.
  */
  public function get($fid) {
    $sql = db_query("SELECT * FROM {video_zencoder} WHERE fid=%d AND status=%d", $fid, VIDEO_ZC_ACTIVE);
    $row = db_fetch_object($sql);
    return $row;
  }

  /*
   * Selects the pending queue to be transfered to amazon.
  */
  public function queue() {
    $video = false;
    $sql = db_query("SELECT vid, fid FROM {video_zencoder} WHERE status=%d LIMIT %d", VIDEO_ZC_PENDING, $this->limit);
    while($row = db_fetch_object($sql)) {
      $video = false;
      // We need to check if this file id exists in our S3 table to avoid filenot found error.
      $sql_video = db_query("SELECT * FROM {video_s3} WHERE fid=%d", $row->fid);
      if($sql_video_row = db_fetch_object($sql_video)) {
        // This is a s3 file, lets verify it has been pushed and if so lets push Zencoder queue.
        module_load_include('inc', 'video', '/includes/conversion');
        if($sql_video_row->status == VIDEO_ZC_ACTIVE) {
          $video = $sql_video_row;
        }
      }
      else {
        // This is a regular video file, lets get our file object from the files table and push it to amazon.
        $sql_files = db_query("SELECT * FROM {files} WHERE fid=%d", $row->fid);
        if($sql_files_row = db_fetch_object($sql_files)) {
          $video = $sql_files_row;
        }
      }
      // If we have a video lets go ahead and send it.
      if($video) {
        // Get Output file info
        // API Key
        $api_key = variable_get('zencoder_api_key', '');

        // Update our status to working.
        $this->working($row->vid);
        $filepath = $video->filepath;
        $filename = $video->filename;
        if ($encoding_job = $this->create($video)) {
          // Update our table.
          $video->bucket = $this->bucket;
          $video->filename = $filename . '.flv';
          $video->filemime = 'video/x-flv';
          $video->vid = $row->vid;
          $video->jobid = $encoding_job->id;
          $video->outputid = $encoding_job->outputs["VIDEO" . $video->fid]->id;
          $prefix = $this->ssl ? 'https://' : 'http://';
          $video->filepath = $prefix . $video->bucket .'.s3.amazonaws.com/'. $filename . '.flv';
          if($this->update($video)) {
            watchdog('zencoder', t('Successfully created trancoding job on file: !file into the bucket %bucket on Zencoder.', array('!file' => $filepath, '%bucket' => $this->bucket)), array(), WATCHDOG_INFO);
          }
        }
        else {
          watchdog('zencoder', 'Failed to queus our file to Zencoder.', array(), WATCHDOG_ERROR);
          $this->failed($row->vid);
        }
      }
      else {
        watchdog('zencoder', 'We did not find the file id: '.$row->fid.' or it is still queued for ffmpeg processing or S3 push.', array(), WATCHDOG_DEBUG);
      }
    }
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
        drupal_set_message(t('Unable to send e-mail!. Your Zencoder Details are as below.<br/> API Key : !api_key<br/> Password : !password<br/>.', array('!api_key' => $results['api_key'], '!password' => $results['password'])), 'status');
      }
      else {
        drupal_mail('video_zencoder', 'video_zencoder', 'heshanmw@gmail.com', language_default(), $results);
        drupal_set_message(t('Your account has been created and is ready to start processing on Zencoder'));
      }
//      return $request->results;
      return true;
//      variable_set('zencoder_api_key', '');
    } else {
      $errors = '';
      foreach($request->errors as $error) {
        $errors .= $error;
      }
      return $errors;
    }

  }
}