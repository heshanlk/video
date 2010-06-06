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
    // New Encoding Job
    //28022009063.mp4
    //    "input": "s3://' . $bucket . '/' . $filename . '"
    //    "input": "s3://heidi_test/28022009063.mp4"
    // Get varialbes
    $lable = 'VIDEO';
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
    $thumn_no = variable_get('no_of_video_thumbs', 5);
    // TODO :
    $thumb_size = '128X128';
    $thumb_base = $baseurl;
    $thumb_prefix = $filename;
    $notify_url = variable_get('zc_notify_url', '');
    $notify_email = variable_get('zc_notify_email', 'heshanmw@gmail.com');
    $start_clip = variable_get('zc_start_clip', '');
    $clip_length = variable_get('zc_clip_length', '');

    $bucket = $this->bucket;

    // Job details
    $input_name = $bucket . '/' . $filename;
    watchdog('zencoder', $input_name);

    $encoding_job = new ZencoderJob('
{
  "input":"s3://' . $input_name . '",
  "output":[{
      ' . !empty($lable) ? '"label":"' . $lable . ',"' : '' . '
      ' . !empty($bucket) ? '"base_url":"s3://' . $bucket . ',"' : '' . '
      ' . !empty($bucket) ? '"filename":"output.flv",' : '' . '
      ' . !empty($width) ? '"width":' . $width . ',' : '' . '
      ' . !empty($height) ? '"height":' . $height . ',' : '' . '
      ' . !empty($quality) ? '"quality":' . $quality . ',' : '' . '
      ' . !empty($speed) ? '"speed":' . $speed . ',' : '' . '
      ' . !empty($upscale) ? '"upscale":' . $upscale . ',' : '' . '
      ' . !empty($frame_rate) ? '"frame_rate":' . $frame_rate . ',' : '' . '
      ' . !empty($frame_rate) ? '"max_frame_rate":' . $frame_rate . ',' : '' . '
      ' . !empty($keyframe_interval) ? '"keyframe_interval":' . $keyframe_interval . ',' : '' . '
      ' . !empty($video_bitrate) ? '"video_bitrate":' . $video_bitrate . ',' : '' . '
      ' . !empty($bitrate_cap) ? '"bitrate_cap":' . $bitrate_cap . ',' : '' . '
      ' . !empty($buffer_size) ? '"buffer_size":' . $buffer_size . ',' : '' . '
      ' . !empty($h264_profile) ? '"h264_profile":' . $h264_profile . ',' : '' . '
      ' . !empty($h264_level) ? '"h264_level":' . $h264_level . ',' : '' . '
      ' . !empty($skip_video) ? '"skip_video":' . $skip_video . ',' : '' . '
      ' . !empty($audio_codec) ? '"audio_codec":' . $audio_codec . ',' : '' . '
      ' . !empty($audio_bitrate) ? '"audio_bitrate":' . $audio_bitrate . ',' : '' . '
      ' . !empty($audio_channels) ? '"audio_channels":' . $audio_channels . ',' : '' . '
      ' . !empty($audio_sample_rate) ? '"audio_sample_rate":' . $audio_sample_rate . ',' : '' . '
      ' . !empty($skip_audio) ? '"skip_audio":' . $skip_audio . ',' : '' . '

      "thumbnails":{
      ' . !empty($thumn_no) ? '"number":' . $thumn_no . ',' : '' . '
      ' . !empty($thumb_size) ? '"number":' . $thumb_size . ',' : '' . '
      ' . !empty($bucket) ? '"base_url":"s3://' . $bucket . '",' : '' . '
      ' . !empty($lable) ? '"prefix":' . $lable . ',' : '' . '
      },
      "notifications":[
      ' . !empty($notify_url) ? '"' . $notify_url . '",' : '' . '
      ' . !empty($notify_email) ? '"' . $notify_email . '",' : '' . '
      ],
      ' . !empty($start_clip) ? '"start_clip":' . $start_clip . ',' : '' . '
      ' . !empty($clip_length) ? '"clip_length":' . $clip_length . ',' : '' . '
    }
  ],
  "api_key":"' . $this->access_key . '"
}');

    // Check if it worked
    if ($encoding_job->created) {
//      watchdog('zencoder', $encoding_job->id);
      return $encoding_job;
//      $this->jobid = $encoding_job->id;
//      // Success
//      //    echo "w00t! \n\n";
//      echo "Job ID: ".$encoding_job->id."\n";
//      echo "Output '".$encoding_job->outputs["web"]->label."' ID: ".$encoding_job->outputs["web"]->id."\n";
//      watchdog('video_render', 'success');
//          exit;
      // Store Job/Output IDs to update their status when notified or to check their progress.
    }
    else {
      // Failed
      //    echo "Fail :(\n\n";
      //    echo "Errors:\n";
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
    $result = db_query("UPDATE {video_zencoder} SET jobid=%d, bucket='%s', filename='%s', filepath='%s', filemime='%s', filesize='%s', status=%d, completed=%d WHERE vid=%d",
            $video->jobid, $video->bucket, $video->filename, $video->filepath, $video->filemime, $video->filesize, VIDEO_ZC_ACTIVE, time(), $video->vid);
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
      // We need to check if this file id exists in our S3 table to avoid filenot found error.
      $sql_video = db_query("SELECT * FROM {video_s3} WHERE fid=%d", $row->fid);
      if($sql_video_row = db_fetch_object($sql_video)) {
        // This is a s3 file, lets verify it has been pushed and if so lets push Zencoder queue.
        module_load_include('inc', 'video', '/includes/conversion');
        if($sql_video_row->status == VIDEO_S3_ACTIVE) {
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
        // Update our status to working.
        $this->working($row->vid);
        $filepath = $video->filepath;
        $filename = basename($video->filepath);
        if ($encoding_job = $this->create($video)) {
          // Update our table.
          $video->bucket = $this->bucket;
          $video->vid = $row->vid;
          $video->jobid = $encoding_job->id;
          $prefix = $this->ssl ? 'https://' : 'http://';
          $video->filepath = $prefix . $video->bucket .'.s3.amazonaws.com/'. $filename;
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
}