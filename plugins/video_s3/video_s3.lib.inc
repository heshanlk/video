<?php

/*
 * @file
 * Class file to handle amazon s3 transfers.
 *
 */

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
  private $libfile;
  
  /**
   * @var AmazonS3
   */
  public $s3;

  public function __construct() {
    // Create the Zencoder class
    $libpath = libraries_get_path('awssdk');
    $this->libfile = $libpath .'/sdk.class.php';
    
    if (!file_exists($this->libfile)) {
      drupal_set_message(t('The Amazon SDK for PHP has not been installed correctly. See the <a href="@drupal-status-page">Drupal status page</a> for more information.', array('@drupal-status-page' => url('admin/reports/status'))), 'error');
      return;
    }
    
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
    require_once $this->libfile;
    
    $credential = new stdClass();
    $credential->key = $access_key;
    $credential->secret = $secret_key;
    $credential->token = NULL;
    
    CFCredentials::set(array($credential));
    
    $this->s3 = new AmazonS3();
    $this->s3->use_ssl = $ssl;
  }

  /*
   * Verifies the existence of a file id, returns the row or false if none found.
   */
  public function verify($fid) {
    $sql = db_query('SELECT * FROM {video_s3} WHERE fid = %d', $fid);
    return db_fetch_object($sql);
  }

  /*
   * Gets a video object from the database.
   */
  public function get($fid) {
    $sql = db_query('SELECT * FROM {video_s3} WHERE fid = %d AND status = %d', $fid, VIDEO_S3_COMPLETE);
    return db_fetch_object($sql);
  }

  /*
   * Inserts file object into the database.
   */
  public function insert($fid) {
    db_query('INSERT INTO {video_s3} (fid, status) VALUES (%d, %d)', $fid, VIDEO_S3_PENDING);
  }

  /*
   * Updates the database after a successful transfer to amazon.
   */
  public function update($video) {
    return db_query("UPDATE {video_s3} SET bucket='%s', filename='%s', filepath='%s', filemime='%s', filesize='%s', status=%d, completed=%d WHERE vid=%d",
            $video->bucket, $video->filename, $video->filepath, $video->filemime, $video->filesize, VIDEO_S3_COMPLETE, time(), $video->vid);
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
        $filepath = $video->filepath;
        // For old video's (pre-4.5), the filename property is actually the path we need
        if (strpos('/', $video->filename) !== FALSE) {
          $filepath = $video->filename;
        }
        $this->s3->delete_object($video->bucket, $filepath);

        // Remove ffmpeg converted files
        $row = db_fetch_object(db_query('SELECT data FROM {video_files} WHERE data IS NOT NULL AND fid = %d', $fid));
        if ($row) {
          foreach (unserialize($row->data) as $subvideo) {
            $this->s3->delete_object($video->bucket, $subvideo->filepath);
          }
        }

        // TODO: move this to video_zencoder
        if (module_exists('video_zencoder')) {
          // Delete converted files added by Zencoder
          $row = db_fetch_object(db_query('SELECT data FROM {video_zencoder} WHERE data IS NOT NULL AND fid = %d', $fid));
          if ($row) {
            foreach (unserialize($row->data) as $subvideo) {
              $parts = parse_url($subvideo->url);
              $this->s3->delete_object($video->bucket, ltrim($parts['path'], '/'));
            }
          }

          // Delete thumbnails added by Zencoder
          $thumb_folder = video_thumb_path($video, FALSE) .'/';
          $thumbfilenames = $this->s3->get_object_list($video->bucket, array('prefix' => $thumb_folder));
          if (!empty($thumbfilenames)) {
            $objects = array();
            foreach ($thumbfilenames as $thumb) {
              $objects[] = array('key' => $thumb);
            }
            $this->s3->delete_objects($video->bucket, array('objects' => $objects));
          }
        }
      }

      // Lets delete our record from the database.
      db_query('DELETE FROM {video_s3} WHERE vid = %d', $video->vid);
    }
  }

  /*
   * Selects the pending queue to be transfered to amazon.
   */
  public function queue() {
    $sql = db_query("SELECT vid, fid FROM {video_s3} WHERE status = %d LIMIT %d", VIDEO_S3_PENDING, $this->limit);
    while ($row = db_fetch_object($sql)) {
      module_load_include('inc', 'video', '/includes/conversion');
      
      // We need to check if this file id exists in our transcoding table.
      // Skip unfinished ffmpeg jobs
      $result = db_query('SELECT f.*, vf.data FROM {files} f LEFT JOIN {video_files} vf USING(fid) WHERE f.fid = %d AND (vf.status IS NULL OR vf.status = %d)', $row->fid, VIDEO_RENDERING_COMPLETE);
      if (!($video = db_fetch_object($result))) {
        $this->watchdog('Could not find the file id %fid or it is still queued for ffmpeg processing.', array('%fid' => $row->fid), WATCHDOG_DEBUG, $row);
        continue;
      }

      // Update our status to working.
      $this->working($row->vid);

      $success = true;

      // Add all ffmpeg converted variants
      if (!empty($video->data)) {
        foreach (unserialize($video->data) as $file) {
          if (!$this->pushFile($file)) {
            $success = false;
          }
        }
      }

      if ($success) {
        // Upload the original
        if ($video = $this->pushFile($video)) {
          $video->vid = $row->vid;
          $this->update($video);
        }
        else {
          $success = false;
        }
      }

      if (!$success) {
        $this->failed($row->vid);
      }
    }
    
    // Update Expires headers on currently-uploaded files.
    // First, make sure this is a good time to do this. It doesn't make much
    // sense to do this more often than the Expires offset.
    $expires_offset = variable_get('amazon_s3_expires_offset', 604800);
    if ($expires_offset !== 'none' && $expires_offset != 0 && variable_get('amazon_s3_expires_last_cron', 0) + $expires_offset < time()) {
      $active = db_query('SELECT bucket, filename, filepath, filemime FROM {video_s3} WHERE status = %d', VIDEO_S3_COMPLETE);
      $permission = variable_get('amazon_s3_private', FALSE) ? AmazonS3::ACL_PRIVATE : AmazonS3::ACL_PUBLIC;
      
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
        $this->update_headers($file, $permission, $headers);
      }
      
      variable_set('amazon_s3_expires_last_cron', time());
    }
  }

  private function pushFile(stdClass $file) {
    $expires_offset = variable_get('amazon_s3_expires_offset', 604800);
    $perm = variable_get('amazon_s3_private', FALSE) ? AmazonS3::ACL_PRIVATE : AmazonS3::ACL_PUBLIC;
    $cc = variable_get('amazon_s3_cache_control_max_age', 'none');

    $headers = array();
    
    if ($expires_offset !== 'none') {
      $headers['Expires'] = gmdate('r', $expires_offset == 0 ? 0 : (time() + $expires_offset));
    }
    if ($cc !== 'none') {
      $headers['Cache-Control'] = 'max-age='. $cc;
    }

    // Increase the database timeout to prevent database errors after a long upload
    _video_db_increase_timeout();

    $response = $this->s3->create_object($this->bucket, $file->filepath, array(
      'fileUpload' => $file->filepath,
      'acl' => $perm,
      'contentType' => $file->filemime,
      'headers' => $headers,
    ));
    
    if ($response->isOK()) {
      // If private, set permissions for Zencoder to read
      if ($perm == AmazonS3::ACL_PRIVATE) {
        $this->setZencoderAccessPolicy($this->bucket, $file->filepath);
      }

      $file->bucket = $this->bucket;
      $s3url = 'http://'. $file->bucket .'.s3.amazonaws.com/'. drupal_urlencode($file->filepath);

      // remove local file
      if (variable_get('amazon_s3_delete_local', FALSE)) {
        $this->replaceLocalFile($file->filepath, $s3url);
      }

      $this->watchdog(
        'Successfully uploaded %file to Amazon S3 location <a href="@s3-url">@s3-path</a> and permission %permission.',
        array(
          '%file' => $file->filename,
          '@s3-url' => $s3url,
          '@s3-path' => $file->filepath,
          '%permission' => $perm,
        ), WATCHDOG_INFO, $file);

      return $file;
    }

    $this->watchdog('Failed to upload %file to Amazon S3.', array('%file' => $file->filepath), WATCHDOG_ERROR, $file);

    return NULL;
  }

  public function getVideoUrl($filepath) {
    if (variable_get('amazon_s3_private', FALSE)) {
      return $this->get_authenticated_url($filepath);
    }
    else {
      $cfdomain = variable_get('amazon_s3_cf_domain', FALSE);

      return
        ($this->ssl ? 'https://' : 'http://') .
        ($cfdomain ? $cfdomain .'/' : ($this->bucket .'.s3.amazonaws.com/')) .
        drupal_urlencode($filepath); // Escape spaces
    }
  }

  public function get_object_info($filepath) {
    return $this->s3->get_object_headers($this->bucket, $filepath)->isOK();
  }

  public function get_authenticated_url($filepath) {
    $lifetime = (int)variable_get('amazon_s3_lifetime', 1800);
    $url = $this->s3->get_object_url($this->bucket, $filepath, time() + $lifetime);

    if ($this->ssl) {
      $url = 'https://'. substr($url, 7);
    }

    return $url;
  }

  public function get_object($filepath, $saveTo = false) {
    return $this->s3->get_object($this->bucket, $filepath, array(
      'fileDownload' => $saveTo,
    ));
  }

  private function update_headers($file, $permission, $headers) {
    $filepath = $file->filepath;
    // For old video's (pre-4.5), the filename property is actually the path we need
    if (strpos('/', $file->filename) !== FALSE) {
      $filepath = $file->filename;
    }
    
    // Reset the Content-Type header usually sent when the S3 library puts a
    // file - we'll lose it otherwise.
    $headers['Content-Type'] = $file->filemime;
    
    $item = array(
      'bucket' => $file->bucket,
      'filename' => $filepath,
    );
    
    return $this->s3->copy_object($item, $item, array(
      'acl' => $permission,
      'headers' => $headers,
    ))->isOK();
  }

  /**
  * Set access control policy to zencoder if module is enabled
  *
  * @todo Add this to video_zencoder module
  * @param string $bucket
  * @param string $filepath
  * @param string $perm WRITE, READ or auto. If auto, $perm is set to READ for files and WRITE for buckets
  * @return bool|null True if the settings have been applied, false on error, NULL when settings already set or zencoder not enabled.
  */
  public function setZencoderAccessPolicy($bucket, $filepath = '', $perm = 'auto') {
    if (!module_exists('video_zencoder')) {
      return NULL;
    }

    if ($perm == 'auto') {
      $perm = empty($filepath) ? AmazonS3::GRANT_WRITE : AmazonS3::GRANT_READ;
    }

    if (empty($filepath)) {
      $acp = $this->s3->get_bucket_acl($bucket);
    }
    else {
      $acp = $this->s3->get_object_acl($bucket, $filepath);
    }

    // Store existing ACLs to preserve them when adding zencoder
    $acl = array();
    
    // check if the acl is already present
    foreach ($acp->body->AccessControlList->Grant as $grant) {
      if ($grant->Grantee->DisplayName == 'zencodertv' && $grant->Permission == $perm) {
        return NULL;
      }
      $acl[] = array(
        'id' => isset($grant->Grantee->URI) ? (string)$grant->Grantee->URI : (string)$grant->Grantee->ID,
        'permission' => (string)$grant->Permission,
      );
    }

    // Add the Zencoder credentials
    $acl[] = array('id' => 'aws@zencoder.com', 'permission' => $perm);

    if (empty($filepath)) {
      return $this->s3->set_bucket_acl($bucket, $acl)->isOK();
    }
    else {
      return $this->s3->set_object_acl($bucket, $filepath, $acl)->isOK();
    }
  }
  
  private function watchdog($message, $variables = array(), $severity = WATCHDOG_NOTICE, $nid = NULL) {
    $link = NULL;

    if (is_object($nid)) {
      if (isset($nid->nid) && $nid->nid > 0) {
        $link = l(t('view node'), 'node/'. intval($nid->nid));
      }
    }
    elseif($nid > 0) {
      $link = l(t('view node'), 'node/'. intval($nid));
    }

    watchdog('amazon_s3', $message, $variables, $severity, $link);
  }
  
  /**
   * Replace a file with a text file to reduce disk space usage.
   * 
   * @param string $filepath
   * @param string $s3url
   */
  private function replaceLocalFile($filepath, $s3url) {
    if (file_delete($filepath) !== FALSE) {
      $fp = fopen($filepath, 'w+');
      if (!$fp) {
        return FALSE;
      }
      
      fwrite($fp, 'Moved to '. $s3url);
      fclose($fp);
      chmod($filepath, 0644);
    }
  }
}
