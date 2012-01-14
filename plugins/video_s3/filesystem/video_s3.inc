<?php
/*
 * @file
 * Class file used to store videos in Amazon S3.
 *
 */
class video_s3 implements filesystem_interface {

  protected $params = array();
  protected $name = 'Amazon S3';
  protected $value = 'video_s3';

  public function __construct() {

  }

  public function load_file($video) {
    $transcoder = variable_get('vid_convertor', 'video_ffmpeg');

    module_load_include('lib.inc', 'video_s3');
    $s3 = new video_amazon_s3();

    if ($amazon = $s3->get($video->fid)) {
      foreach ($video->files as $key => &$file) {
        $s3->connect();
        $file->url = $s3->getVideoUrl($file->filepath);

        // For old video's (pre-4.5), the filename property is actually a path
        // and no ffmpeg converted files were saved to S3.
        if (strpos('/', $amazon->filename) !== FALSE && $transcoder == 'video_ffmpeg') {
          $file->url = $s3->getVideoUrl($amazon->filename);
        }
      }
    }
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/filesystem_interface#get_name()
   */
  public function get_name() {
    return $this->name;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/filesystem_interface#get_help()
   */
  public function get_help() {
    return t('Amazon Simple Storage Service (!s3) to store your video files. This free\'s up bandwidth from your site, providing a faster experience for your users. Simply enable this and enter your authentication details and your done!', array('!s3' => l(t('Amazon S3'), 'http://aws.amazon.com/s3/')));
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/filesystem_interface#get_value()
   */
  public function get_value() {
    return $this->value;
  }

  public function admin_settings() {
    $form = array();

    $form['amazon_s3_ssl'] = array(
      '#type' => 'checkbox',
      '#title' => t('Use HTTPS file links'),
      '#default_value' => variable_get('amazon_s3_ssl', FALSE),
      '#description' => t('If you would like to use SSL when transfering your files enable this option.'),
    );

    $form['amazon_s3_delete_local'] = array(
      '#type' => 'checkbox',
      '#title' => t('Delete local file after uploading to S3'),
      '#default_value' => variable_get('amazon_s3_delete_local', FALSE),
      '#description' => t('Replaces the original file on the local file system with an empty file to reduce disk space usage. The file is not removed as Drupal and FileField expect a file to be present.'),
    );

    $form['amazon_s3_private'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enable private file storage'),
      '#default_value' => variable_get('amazon_s3_private', FALSE),
      '#description' => t('If you would like to use private file storage for your files enable this option. Videos are displayed using temporary URLs that expire after an amount of time that is configurable below.'),
    );

    $form['amazon_s3_lifetime'] = array(
      '#type' => 'textfield',
      '#title' => t('Private URL lifetime'),
      '#default_value' => variable_get('amazon_s3_lifetime', 1800),
      '#size' => 5,
      '#description' => t('The number of seconds a URL to a private file is valid.'),
    );

    $form['amazon_s3_access_key'] = array(
      '#type' => 'textfield',
      '#title' => t('Access Key ID'),
      '#default_value' => variable_get('amazon_s3_access_key', ''),
      '#size' => 50,
      '#element_validate' => array('_video_s3_fsadmin_validate'),
    );

    $form['amazon_s3_secret_access_key'] = array(
      '#type' => 'password',
      '#title' => t('Secret Access Key'),
      '#default_value' => variable_get('amazon_s3_secret_access_key', ''),
      '#description' => t('Once saved, you do not need to re-enter your secret key.  If you need to update your key, then fill this out to update it.'),
      '#size' => 50,
      '#element_validate' => array('_video_s3_fsadmin_validate_secret', '_video_s3_fsadmin_validate'),
    );

    //@todo Maybe move this to the admin settings page instead of global?
    $form['amazon_s3_bucket'] = array(
      '#type' => 'textfield',
      '#title' => t('Bucket'),
      '#description' => t('Enter the bucket you wish to store your videos in.  If the bucket doesn\'t exist the system will attempt to create it.'),
      '#default_value' => variable_get('amazon_s3_bucket', ''),
      '#size' => 50,
      '#element_validate' => array('_video_s3_fsadmin_validate'),
    );

    // cloud front
    $form['amazon_s3_cf_domain'] = array(
      '#type' => 'textfield',
      '#title' => t('CloudFront domain name'),
      '#description' => t('If you are using Amazon CloudFront with this bucket, enter the CloudFront domain name. This will probably be something like <em>X.cloudfront.net</em> where <em>X</em> is a series of random-looking numbers and letters. Do not include <em>http://</em> at the beginning.'),
      '#default_value' => variable_get('amazon_s3_cf_domain', ''),
      '#size' => 50,
    );

    // Cloud Front and private files do not work together
    if (variable_get('amazon_s3_private', FALSE)) {
      $form['amazon_s3_cf_domain']['#description'] = t('Amazon CloudFront and S3 private file storage cannot be used together. Disable private files to enable this setting.');
      $form['amazon_s3_cf_domain']['#disabled'] = TRUE;
    }

    $form['headers_fset'] = array(
      '#type' => 'fieldset',
      '#title' => t('Headers'),
    );

    $form['headers_fset']['amazon_s3_expires_offset'] = array(
      '#type' => 'select',
      '#title' => t('Expires header'),
      '#options' => array(
        'none' => t('(none)'),
        '0' => t('File is always expired (Unix epoch, 1 Jan 1970, 00:00:00 UTC)'),
      ),
      '#default_value' => variable_get('amazon_s3_expires_offset', 604800),
      '#description' => t('Amazon S3 can be told to send an Expires header (<a href="http://developer.yahoo.com/performance/rules.html#expires">learn more</a>) with served files. This will induce most browsers to cache the file longer, reducing the frequency that it is re-downloaded from the server. This can save on bandwidth charges and provide a faster experience for visitors. However, files which will be frequently updated with the same filename should use a low Expires offset, or else visitors won\'t see &quot;fresh&quot; data. Also, Expires headers will be updated for currently-uploaded files on cron runs, so the interval set here should be greater than or equal to your server\'s cron run interval for best results.'),
    );

    $form['headers_fset']['amazon_s3_cache_control_max_age'] = array(
      '#type' => 'select',
      '#title' => t('max-age parameter of Cache-Control header'),
      '#options' => array(
        'none' => t('(none)'),
      ),
      '#default_value' => variable_get('amazon_s3_cache_control_max_age', 'none'),
      '#description' => t('When using Amazon CloudFront, the max-age parameter of the Cache-Control header (<a href="http://condor.depaul.edu/~dmumaugh/readings/handouts/SE435/HTTP/node24.html">learn more</a>) tells CloudFront\'s edge servers how frequently they should check the Amazon S3 bucket to see if the files they have cached have been changed or deleted. If you need to ensure that files on edge servers are updated quickly after a video is changed or deleted, set this to a low value; if videos are rarely changed or deleted, or visitors seeing stale data is not a problem, set this to a high value for speed and/or to save on the cost of transferring files from your S3 server to the edge servers. If no value is selected, Amazon\'s default of one day will be used.'),
    );
    foreach (array(
              300, // 5 min
              600, // 10 min
              1800, // 30 min
              3600, // 1 hr
              14400, // 4 hr
              28800, // 8 hr
              43200, // 12 hr
              86400, // 1 day
              172800, // 2 day
              604800, // 1 wk
              1209600, // 2 wk
              2419200, // 4 wk
              4838400, // 8 wk
              14515200, // 24 wk
              31536000, // 1 yr (365 day)
              63072000, // 2 yr
              157680000, // 5 yr
              315360000, // 10 yr
    ) as $time) {
      $interval = format_interval($time, 1);
      $form['headers_fset']['amazon_s3_expires_offset']['#options'][$time] = $interval;
      if ($time >= 3600) {
        // The minimum Cache-Control; max-age value Amazon will accept is one hour.
        $form['headers_fset']['amazon_s3_cache_control_max_age']['#options'][$time] = $interval;
      }
    }

    //lets show our buckets in table format with a delete link.
    //@todo add permissions
    //were enabled, that means they have successfully connected and created a bucket.
    if (variable_get('amazon_s3_access_key', false) && _video_s3_is_active_fs()) {
      module_load_include('lib.inc', 'video_s3');
      $s3 = new video_amazon_s3();
      $s3->connect();
      $buckets = $s3->s3->get_bucket_list();

      // Setup our header.
      $header = array(t('Bucket Name'), t('Actions'));
      $rows = array();
      foreach ($buckets as $bucket) {
        if (!$this->isValidBucketName($bucket)) {
          $rows[] = array($bucket, t('Unsupported bucket name'));
        }
        else {
          $title = $bucket;
          
          if ($bucket == variable_get('amazon_s3_bucket', '')) {
            $title = '<strong>'. $title .' ('. t('active bucket') .')</strong>';
          }

          $actions = l(t('Delete'), 'admin/settings/video/amazon_s3/bucket/' . $bucket . '/delete');
          $rows[] = array($title, $actions);
        }
      }
      $form['amazon_info'] = array(
        '#type' => 'fieldset',
        '#title' => t('Amazon S3 Information'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
      );
      $form['amazon_info']['buckets'] = array(
        '#type' => 'markup',
        '#value' => theme('table', $header, $rows),
      );
    }
    return $form;
  }

  public function admin_settings_validate($form, &$form_state) {
    // Check for CURL
    if (!extension_loaded('curl') && !@dl(PHP_SHLIB_SUFFIX == 'so' ? 'curl.so' : 'php_curl.dll')) {
      form_set_error('amazon_s3', t('The CURL extension is not loaded.'));
      return;
    }
    
    $bucket = $form_state['values']['amazon_s3_bucket'];
    // S3 buckets must contain only lower case alphanumeric characters, dots and dashes.
    if (!$this->isValidBucketName($bucket)) {
      form_set_error('amazon_s3_bucket', t('S3 bucket names must contain only lower case alphanumeric characters, dots and dashes.'));
    }

    $ssl = isset($form_state['values']['amazon_s3_ssl']) && $form_state['values']['amazon_s3_ssl'];
    $access_key = $form_state['values']['amazon_s3_access_key'];
    $secret_key = $form_state['values']['amazon_s3_secret_access_key'];

    if (empty($access_key) || empty($secret_key)) {
      // There is no point in continuing if there is no access info
      return;
    }

    // Lets verify our credentials and verify our bucket exists, if not attempt to create it.
    module_load_include('lib.inc', 'video_s3');
    $s3 = new video_amazon_s3();
    $s3->connect($access_key, $secret_key, $ssl);
    $buckets = $s3->s3->get_bucket_list();

    if (!in_array($bucket, $buckets)) {
      // Create a bucket with public read access
      // TODO: region selection
      $response = $s3->s3->create_bucket($bucket, AmazonS3::REGION_US_E1, AmazonS3::ACL_PUBLIC);
      if ($response->isOK()) {
        drupal_set_message(t('Successfully created the bucket %bucket.', array('%bucket' => $bucket)));
      }
      else {
        form_set_error('amazon_s3_bucket', t('Could not verify or create the bucket %bucket.', array('%bucket' => $bucket)));
        $bucket = null;
      }
    }

    // Always check the access rights, in case the bucket was created
    // outside of Drupal or before the Zencoder module was active.
    if ($bucket != null && module_exists('video_zencoder')) {
      if ($s3->setZencoderAccessPolicy($bucket)) {
        drupal_set_message(t('Successfully granted write access for bucket %bucket to Zencoder.', array('%bucket' => $bucket)));
      }
    }
  }

  private function isValidBucketName($name) {
    return preg_match('/^[a-z0-9.-]+$/', $name);
  }
}

/**
 * Replace the empty form value with the variable value.
 * 
 * Used for the access secret.
 * 
 * @param array $element
 * @param array $form_state
 */
function _video_s3_fsadmin_validate_secret($element, &$form_state) {
  $key = end($element['#parents']);
  $val = $form_state['values'][$key];
  $existing = variable_get($key, NULL);
  
  if ($val === '' && $existing !== NULL) {
    $form_state['values'][$key] = $existing;
  }
}

/**
 * Handle required fields that are only required when
 * video_s3 is the selected file system.
 * 
 * @param array $element
 * @param array $form_state
 */
function _video_s3_fsadmin_validate($element, $form_state) {
  if ($form_state['values']['vid_filesystem'] != 'video_s3') {
    return;
  }

  $key = end($element['#parents']);
  $val = $form_state['values'][$key];

  if (empty($val)) {
    form_error($element, t('!name field is required.', array('!name' => $element['#title'])));
  }
}
