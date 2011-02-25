<?php

/*
 * @file
 * Class file to handle amazon s3 transfers.
 *
 */

// Include our class file.
if (!class_exists('S3'))
  require_once('S3.php');

define('VIDEO_S3_PENDING', 1);
define('VIDEO_S3_ACTIVE', 5);
define('VIDEO_S3_COMPLETE', 10);
define('VIDEO_S3_FAILED', 20);

class video_amazon_s3 {

  private $access_key;
  private $secret_key;
  private $ssl;
  private $limit;
  private $bucket;
  public $s3;

  public function __construct() {
    $this->access_key = variable_get('amazon_s3_access_key', '');
    $this->secret_key = variable_get('amazon_s3_secret_access_key', '');
    $this->ssl = variable_get('amazon_s3_ssl', FALSE);
    $this->limit = variable_get('amazon_s3_limit', 5);
    $this->bucket = variable_get('amazon_s3_bucket', '');
  }

  public function connect($access_key = '', $secret_key = '', $ssl = FALSE) {
    $access_key = $access_key ? $access_key : $this->access_key;
    $secret_key = $secret_key ? $secret_key : $this->secret_key;
    $ssl = $ssl ? $ssl : $this->ssl;
    // Make our connection to Amazon.
    $this->s3 = new S3($access_key, $secret_key, $ssl);
  }

  /*
   * Verifies the existence of a file id, returns the row or false if none found.
   */

  public function verify($fid) {
    $sql = db_query("SELECT * FROM {video_s3} WHERE fid=%d", $fid);
    $row = db_fetch_object($sql);
    return $row;
  }

  /*
   * Gets a video object from the database.
   */

  public function get($fid) {
    $sql = db_query("SELECT * FROM {video_s3} WHERE fid=%d AND status=%d", $fid, VIDEO_S3_COMPLETE);
    $row = db_fetch_object($sql);
    return $row;
  }

  /*
   * Inserts file object into the database.
   */

  public function insert($fid) {
    db_query("INSERT INTO {video_s3} (fid, status) VALUES (%d, %d)", $fid, VIDEO_S3_PENDING);
  }

  /*
   * Updates the database after a successful transfer to amazon.
   */

  public function update($video) {
    $result = db_query("UPDATE {video_s3} SET bucket='%s', filename='%s', filepath='%s', filemime='%s', filesize='%s', status=%d, completed=%d WHERE vid=%d",
            $video->bucket, $video->filename, $video->filepath, $video->filemime, $video->filesize, VIDEO_S3_COMPLETE, time(), $video->vid);
    return $result;
  }

  public function working($vid) {
    db_query("UPDATE {video_s3} SET status=%d WHERE vid=%d", VIDEO_S3_ACTIVE, $vid);
  }

  public function failed($vid) {
    db_query("UPDATE {video_s3} SET status=%d WHERE vid=%d", VIDEO_S3_FAILED, $vid);
  }

  public function delete($fid) {
    // Lets get our file no matter the status and delete it.
    if ($video = $this->verify($fid)) {
      if ($video->bucket) {
        // It has been pushed to amazon so lets remove it.
        $this->s3->deleteObject($video->bucket, $video->filename);
      }
      // Lets delete our record from the database.
      db_query("DELETE FROM {video_s3} WHERE vid=%d", $video->vid);
    }
  }

  /*
   * Selects the pending queue to be transfered to amazon.
   */

  public function queue() {
    $video = false;
    $sql = db_query("SELECT vid, fid FROM {video_s3} WHERE status=%d LIMIT %d", VIDEO_S3_PENDING, $this->limit);
    while ($row = db_fetch_object($sql)) {
      $video = false;
      // We need to check if this file id exists in our transcoding table.
      $sql_video = db_query("SELECT * FROM {video_files} WHERE fid=%d", $row->fid);
      if ($sql_video_row = db_fetch_object($sql_video)) {
        // This is a transcoded file, lets verify it has been transcoded and if so lets push it to amazon.
        module_load_include('inc', 'video', '/includes/conversion');
        if ($sql_video_row->status == VIDEO_RENDERING_COMPLETE) {
          $video = $sql_video_row;
        }
      } else {
        // This is a regular video file, lets get our file object from the files table and push it to amazon.
        $sql_files = db_query("SELECT * FROM {files} WHERE fid=%d", $row->fid);
        if ($sql_files_row = db_fetch_object($sql_files)) {
          $video = $sql_files_row;
        }
      }
      // If we have a video lets go ahead and send it.
      if ($video) {
        // Update our status to working.
        $this->working($row->vid);
        $filepath = $video->filepath;
        // get the folder path as file object
        $filename = $video->filepath;
        // use the file object as file name
        $video->filename = $filename;
        $perm = (variable_get('amazon_s3_private', FALSE) == FALSE) ? S3::ACL_PUBLIC_READ : S3::ACL_PRIVATE;
        $expires_interval = variable_get('amazon_s3_expires_offset', 604800);
        if ($expires_interval === 'none') {
          $headers = array();
        } else {
          $headers = array('Expires' => gmdate('r', $expires_interval == 0 ? 0 : (time() + $expires_interval)));
        }
        $cc = variable_get('amazon_s3_cache_control_max_age', 'none');
        if ($cc !== 'none') {
          $headers['Cache-Control'] = 'max-age=' . $cc;
        }

        if ($this->s3->putObject($this->s3->inputFile($filepath), $this->bucket, $filename, $perm, array(), $headers)) {
          // Update our table.
          $video->bucket = $this->bucket;
          $video->vid = $row->vid;
          $prefix = $this->ssl ? 'https://' : 'http://';
          $video->filepath = $prefix . $video->bucket . '.s3.amazonaws.com/' . $filename;
          if ($this->update($video)) {
            // remove local file
            if (variable_get('amazon_s3_delete_local', FALSE))
              unlink($filename);
            watchdog('amazon_s3', t('Successfully uploaded our file: !file into the bucket %bucket on the Amazon S3 server.', array('!file' => $filepath, '%bucket' => $this->bucket)), array(), WATCHDOG_INFO);
          }
        } else {
          watchdog('amazon_s3', 'Failed to upload our file to the amazon s3 server.', array(), WATCHDOG_ERROR);
          $this->failed($row->vid);
        }
      } else {
        watchdog('amazon_s3', 'We did not find the file id: ' . $row->fid . ' or it is still queued for ffmpeg processing.', array(), WATCHDOG_DEBUG);
      }
    }
    // Update Expires headers on currently-uploaded files.
    // First, make sure this is a good time to do this. It doesn't make much
    // sense to do this more often than the Expires offset.
    $expires_offset = variable_get('amazon_s3_expires_offset', 604800);
    if ($expires_offset !== 'none' && $expires_offset != 0 && variable_get('amazon_s3_expires_last_cron', 0) + $expires_offset < time()) {
      $active = db_query('SELECT bucket, filename FROM {video_s3} WHERE status = %d', VIDEO_S3_COMPLETE);
      $headers = array('Expires' => gmdate('r', time() + $expires_offset));
      // Note that Cache-Control headers are always relative values (X seconds
      // in the future from the point they are sent), so we don't need to update
      // them regularly like we do with Expires headers. However, if we don't
      // send one, the one that is set (if any) will be deleted. (Also, if the
      // human has changed this setting on the administration page, we want to
      // update video info accordingly.)
      // @todo: Logic problems: This only updates when expires headers update…
      // Might want to find a way so that these update immediately when the
      // admin settings form is submitted.
      $cc = variable_get('amazon_s3_cache_control_max_age', 'none');
      if ($cc !== 'none') {
        $headers['Cache-Control'] = 'max-age=' . $cc;
      }
      while ($file = db_fetch_object($active)) {
        $this->update_headers($file->bucket, $file->filename, $headers);
      }
    }
    variable_set('amazon_s3_expires_last_cron', time());
  }

  public function get_object_info($object) {
    return $this->s3->getObjectInfo($this->bucket, $object);
  }

  public function get_authenticated_url($object) {
    $lifetime = variable_get('amazon_s3_lifetime', '1800');
    return $this->s3->getAuthenticatedURL($this->bucket, $object, $lifetime);
  }

  public function get_object($object, $saveTo = false) {
    return $this->s3->getObject($this->bucket, $object, $saveTo);
  }

  public function update_headers($bucket, $object, $headers) {
    // Reset the Content-Type header usually sent when the S3 library puts a
    // file - we'll lose it otherwise.
    if (!isset($headers['Content-Type'])) {
      $headers['Content-Type'] = $this->s3->__getMimeType($object);
    }
    return $this->s3->copyObject($bucket, $object, $bucket, $object, variable_get('amazon_s3_private', FALSE) == FALSE ? S3::ACL_PUBLIC_READ : S3::ACL_PRIVATE, array(), $headers);
  }

}