<?php
/**
 * @file
 * Class file to handle amazon s3 transfers.
 */

define('VIDEO_S3_PENDING', 1);
define('VIDEO_S3_ACTIVE', 5);
define('VIDEO_S3_COMPLETE', 10);
define('VIDEO_S3_FAILED', 20);

class video_amazon_s3 {
  private $access_key;
  private $secret_key;
  private $ssl;
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
    return db_fetch_object(db_query('SELECT * FROM {video_s3} WHERE fid = %d', $fid));
  }

  /*
   * Gets a video object from the database.
   */
  public function get($fid) {
    return db_fetch_object(db_query('SELECT * FROM {video_s3} WHERE fid = %d AND status = %d', $fid, VIDEO_S3_COMPLETE));
  }

  public function pushFile(stdClass $file, $keeplocal = FALSE) {
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

      if (!$keeplocal) {
        file_delete($file->filepath);
      }

      $this->watchdog(
        'Successfully uploaded %file to Amazon S3 location <a href="@s3-url">@s3-path</a> and permission %permission.',
        array(
          '%file' => $file->filename,
          '@s3-url' => 'http://'. $file->bucket .'.s3.amazonaws.com/'. drupal_urlencode($file->filepath),
          '@s3-path' => $file->filepath,
          '%permission' => $perm,
        ), WATCHDOG_INFO, $file);

      return $file;
    }

    $this->watchdog('Failed to upload %file to Amazon S3.', array('%file' => $file->filepath), WATCHDOG_ERROR, $file);

    return NULL;
  }

  public function getVideoUrl($filepath, $bucket = NULL) {
    if (variable_get('amazon_s3_private', FALSE)) {
      return $this->get_authenticated_url($filepath, $bucket);
    }

    $cfdomain = variable_get('amazon_s3_cf_domain', FALSE);
    $bucket = $bucket == NULL ? $this->bucket : $bucket;

    return
      ($this->ssl ? 'https://' : 'http://') .
      ($cfdomain ? $cfdomain .'/' : ($bucket .'.s3.amazonaws.com/')) .
      drupal_urlencode($filepath); // Escape spaces
  }

  public function get_object_info($filepath) {
    return $this->s3->get_object_headers($this->bucket, $filepath)->isOK();
  }

  public function get_authenticated_url($filepath, $bucket = NULL) {
    $lifetime = (int)variable_get('amazon_s3_lifetime', 1800);
    $bucket = $bucket == NULL ? $this->bucket : $bucket;

    $url = $this->s3->get_object_url($bucket, $filepath, time() + $lifetime);
    if ($this->ssl) {
      $url = 'https://'. drupal_substr($url, 7);
    }

    return $url;
  }

  public function get_object($filepath, $saveTo = FALSE) {
    return $this->s3->get_object($this->bucket, $filepath, array(
      'fileDownload' => $saveTo,
    ));
  }

  /**
   * Update Expires headers on currently-uploaded files.
   */
  public function updateAllExpiresHeaders() {
    // First, make sure this is a good time to do this. It doesn't make much
    // sense to do this more often than the Expires offset.
    $expires_offset = variable_get('amazon_s3_expires_offset', 604800);
    if ($expires_offset === 'none' || $expires_offset === 0) {
      return;
    }

    if (variable_get('amazon_s3_expires_last_cron', 0) + $expires_offset > time()) {
      return;
    }

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
      $headers['Cache-Control'] = 'max-age='. $cc;
    }

    while ($file = db_fetch_object($active)) {
      $this->update_headers($file, $permission, $headers);
    }

    variable_set('amazon_s3_expires_last_cron', time());
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
    elseif ($nid > 0) {
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
  public function replaceLocalFile($filepath, $s3url) {
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
