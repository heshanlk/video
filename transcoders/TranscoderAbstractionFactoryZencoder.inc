<?php
/**
 * @file
 * File containing class TranscoderAbstractionFactoryZencoder
 */

/**
 * Class that handles Zencoder transcoding.
 */
class TranscoderAbstractionFactoryZencoder extends TranscoderAbstractionFactory implements TranscoderFactoryInterface {
  protected $options = array();
  private $postbackurl;

  public function __construct() {
    parent::__construct();
    $this->options['api_key'] = variable_get('video_zencoder_api_key', NULL);
    $this->postbackurl = variable_get('video_zencoder_postback', url('postback/jobs', array('absolute' => TRUE)));
  }

  public function setInput($file, $input_frame_rate = 0, $validate_decode_codecs = TRUE) {
    parent::setInput($file, $input_frame_rate, $validate_decode_codecs);
    $this->options['input'] = file_create_url($this->settings['input']['uri']);

    if (variable_get('video_zencoder_testing_mode', FALSE)) {
      $this->options['input'] = variable_get('video_zencoder_test_file_path', 'http://example.com/video.mp4');
    }
  }

  public function setOptions($options = NULL) {
    global $base_url;
    if (is_array($options)) {
      foreach ($options as $key => $value) {
        if (empty($value) || $value === 'none')
          continue;
        switch ($key) {
          case 'video_extension':
            $this->options['output']['format'] = $value;
            break;
          case 'video_preset':
          case 'default':
            break;
          case 'wxh':
            $this->options['output']['size'] = $value;
            break;
          case 'video_quality':
            $this->options['output']['quality'] = $value;
            break;
          case 'video_speed':
            $this->options['output']['speed'] = $value;
            break;
          case 'video_upscale':
            $this->options['output']['upscale'] = $value;
            break;
          case 'one_pass':
            $this->options['output']['one_pass'] = ($value == 1 ) ? TRUE : FALSE;
            break;
          default :
            $this->options['output'][$key] = $value;
        }
      }
      // set notifications
      $this->options['output']['notifications']['format'] = 'json';
      $this->options['output']['notifications']['url'] = $this->postbackurl;
      // thumbnails
      $this->options['output']['thumbnails']['number'] = variable_get('video_thumbnail_count', 5);
      $this->options['output']['thumbnails']['thumb_size'] = variable_get('video_thumbnail_size', '320x240');
      $schema_thumb_path = file_uri_scheme($this->settings['input']['uri']) . '://' . variable_get('video_thumbnail_path', 'videos/thumbnails') . '/' . $this->settings['input']['fid'];
      $final_path = (variable_get('video_zencoder_use_full_path', FALSE)) ? drupal_realpath($schema_thumb_path) : '/' . file_uri_target($schema_thumb_path);
      $this->options['output']['thumbnails']['base_url'] = variable_get('video_zencoder_base_url', 'ftp://[user]:[password]@[ftp.url]') . $final_path;
      $this->options['output']['thumbnails']['prefix'] = 'thumbnail-' . $this->settings['input']['fid'];
    }
  }

  public function setOutput($output_directory, $output_name, $overwrite_mode = FILE_EXISTS_REPLACE) {
    parent::setOutput($output_directory, $output_name, $overwrite_mode);
    $this->options['output']['label'] = 'video-' . $this->settings['input']['fid'];
    // show full path is not secure as its visible in file path
    $final_path = (variable_get('video_zencoder_use_full_path', FALSE)) ? drupal_realpath($this->settings['base_url']) : '/' . file_uri_target($this->settings['base_url']);
    $this->options['output']['url'] = variable_get('video_zencoder_base_url', 'ftp://[user]:[password]@[ftp.url]') . $final_path . '/' . $this->settings['filename'];
    $this->options['output']['base_url'] = variable_get('video_zencoder_base_url', 'ftp://[user]:[password]@[ftp.url]') . $final_path;
    $this->options['output']['filename'] = $this->settings['filename'];
    // set all video to public, there is no way to handle private videos
    $this->options['output']['public'] = 1;
  }

  public function extractFrames() {
    global $user;

    // Get the file system directory.
    $thumb_path = variable_get('video_thumbnail_path', 'videos/thumbnails') . '/' . $this->settings['input']['fid'];
    $schema_thumb_path = file_uri_scheme($this->settings['input']['uri']) . '://' . $thumb_path;
    file_prepare_directory($schema_thumb_path, FILE_CREATE_DIRECTORY);

    // Check if S3 should be loaded.
    $s3 = NULL;
    if (module_exists('amazons3') && self::isS3Url(variable_get('video_zencoder_base_url'))) {
      $s3 = self::createS3Client();
      $bucket = variable_get('amazons3_bucket');
    }

    $files = array();
    // Total thumbs to generate
    $no_of_thumbnails = variable_get('video_thumbnail_count', 5);
    for ($i = 0; $i < $no_of_thumbnails; $i++) {
      $filename = file_munge_filename('thumbnail-' . $this->settings['input']['fid'] . '_' . sprintf('%04d', $i) . '.png', '', TRUE);
      $filepath = drupal_realpath($schema_thumb_path . '/' . $filename);

      // Download file from S3, if available
      if (!file_exists($filepath) && $s3 != NULL) {
        $s3_get_object = NULL;
        if ($s3->get_object_metadata($bucket, $thumb_path . '/' . $filename)) {
          $s3_get_object = $s3->get_object($bucket, $thumb_path . '/' . $filename, array('fileDownload' => $filepath));
          if (variable_get('video_zencoder_testing_mode', FALSE)) {
            db_update('video_queue')
              ->fields(array('status' => VIDEO_RENDERING_COMPLETE))
              ->condition('fid', $this->settings['input']['fid'])->execute();
          }
        }
        if (!file_exists($filepath) && $s3_get_object) {
          watchdog('zencoder',
            'Error downloading thumbnail for video %filepath: generated file %thumbpath does not exist.<br />S3 Output:<br />%out',
            array('%filepath' => $this->settings['input']['filepath'], '%thumbpath' => $filepath, '%out' => $s3_get_object),
            WATCHDOG_ERROR);
          continue;
        }
      }

      // Create file object only file is exists
      if (file_exists(drupal_realpath($schema_thumb_path . '/' . $filename))) {
        $file = new stdClass();
        $file->uid = $user->uid;
        $file->status = 0;
        $file->filename = $filename;
        $file->uri = $schema_thumb_path . '/' . $filename;
        $file->filemime = 'image/png';
        $file->filesize = filesize(drupal_realpath($schema_thumb_path . '/' . $filename));
        $file->timestamp = time();
        $files[] = $file;
      }
    }

    return !empty($files) ? $files : FALSE;
  }

  public function execute() {
    libraries_load('zencoder');
    $zencoder = new Services_Zencoder();

    try {
      $encoding_job = $zencoder->jobs->create($this->options);

      // Setup our converted video object
      // for the outout uri we should change ftp://, sftp:// -> public:// or private://
      $output_uri = $this->options['output']['base_url'] . '/' . $this->settings['filename'];

      switch (file_uri_scheme($output_uri)) {
        case 'ftp':
        case 'sftp':
          // thanks to D7 stream wrappers
          $output_uri = file_uri_scheme($this->settings['input']['uri']) . '://' . file_uri_target($this->settings['base_url']) . '/' . $this->settings['filename'];
          break;
        case 's3':
          $final_path = (variable_get('video_zencoder_use_full_path', FALSE)) ? drupal_realpath($this->settings['base_url']) : '/' . file_uri_target($this->settings['base_url']);
          $output_uri = 's3:/' . $final_path . '/' . $this->settings['filename'];
          break;
      }

      $output = new stdClass();
      $output->filename = $this->settings['filename'];
      $output->uri = $output_uri;
      $output->filemime = file_get_mimetype($output_uri);
      $output->filesize = filesize(drupal_realpath($this->settings['input']['uri']));
      $output->timestamp = time();
      $output->jobid = $encoding_job->id;
      $output->duration = 0;

      return $output;
    }
    catch (Services_Zencoder_Exception $e) {
      $errors = $e->getErrors();
      $this->errors['execute'] = $errors;
      watchdog('zencoder', 'Zencoder reports errors while converting %file:<br/>!errorlist', array('%file' => $this->settings['filename'], '!errorlist' => theme('item_list', array('items' => $errors))), WATCHDOG_ERROR);
      return FALSE;
    }
  }

  public function getName() {
    return 'Zencoder';
  }

  public function getValue() {
    return 'TranscoderAbstractionFactoryZencoder';
  }

  public function isAvailable(&$errormsg) {
    registry_rebuild();

    if (!module_exists('zencoderapi')) {
      $errormsg = t('You must install and enable the Zencoder API module to use Zencoder to transcode videos.');
      return FALSE;
    }
    elseif (!class_exists('Services_Zencoder')) {
      $errormsg = t('The Zencoder API module has not been setup properly.');
      return FALSE;
    }

    return TRUE;
  }

  public function getVersion() {
    return '1.2';
  }

  public function adminSettings() {
    global $user, $base_url;

    $t = get_t();

    $form = array();
    $form['video_zencoder_start'] = array(
      '#type' => 'markup',
      '#markup' => '<div id="' . $this->getValue() . '">',
    );
    $zencoder_api = variable_get('video_zencoder_api_key', NULL);
    if (empty($zencoder_api)) {
      $form['zencoder_user'] = array(
        '#type' => 'fieldset',
        '#title' => $t('Zencoder setup'),
        '#collapsible' => FALSE,
        '#collapsed' => FALSE,
        '#description' => $t('Add your email address, password and <em>save configurations</em> to create your Zencoder account. It will help you to transcode and manage your videos using Zencode website. Once you save your configurations then this will automatically create an account on the Zencoder.com and password and all ther other relevent details will be emailed to you.', array('!link' => l($t('Zencoder.com'), 'http://zencoder.com')))
      );
      $form['zencoder_user']['zencoder_username'] = array(
        '#type' => 'textfield',
        '#title' => $t('Your email address'),
        '#default_value' => variable_get('site_mail', 'me@localhost'),
        '#size' => 50,
        '#description' => $t('Make sure the email is accurate, since we will send all the password details to manage transcoding online and API key details to this.')
      );
      $form['zencoder_user']['agree_terms_zencoder'] = array(
        '#type' => 'checkbox',
        '#title' => $t('Agree Zencoder !link.', array('!link' => l($t('Terms and Conditions'), 'http://zencoder.com/terms', array('attributes' => array('target' => '_blank'))))),
        '#default_value' => variable_get('agree_terms_zencoder', TRUE),
      );
    }
    else {
      // Zencoder API is exists
      $form['zencoder_info'] = array(
        '#type' => 'fieldset',
        '#title' => t('Zencoder'),
        '#collapsible' => FALSE,
        '#collapsed' => FALSE,
      );
      $form['zencoder_info']['video_zencoder_api_key'] = array(
        '#type' => 'textfield',
        '#title' => t('Zencoder API key'),
        '#default_value' => variable_get('video_zencoder_api_key', NULL),
        '#description' => t('Zencoder API Key. Click <b>Reset to default</b> button to add new account.')
      );
      $form['zencoder_info']['video_thumbnail_count'] = array(
        '#type' => 'textfield',
        '#title' => t('Number of thumbnails'),
        '#description' => t('Number of thumbnails to display from video.'),
        '#default_value' => variable_get('video_thumbnail_count', 5),
        '#size' => 5
      );
      $form['zencoder_info']['video_thumbnail_size'] = array(
        '#type' => 'select',
        '#title' => t('Dimension of thumbnails'),
        '#default_value' => variable_get('video_thumbnail_size', '320x240'),
        '#options' => video_explode("\n", variable_get("video_metadata_dimensions", video_default_dimensions()))
      );
      $form['zencoder_info']['video_zencoder_postback'] = array(
        '#type' => 'textfield',
        '#title' => t('Postback URL for Zencoder'),
        '#description' => t('Important : Don\'t change this if you don\'t know what you\'re doing. This postback URL will receive video data as video transcodings are completed.'),
        '#default_value' => $this->postbackurl,
      );
      $form['zencoder_info']['video_zencoder_base_url'] = array(
        '#type' => 'textfield',
        '#title' => t('Output file base'),
        '#description' => t('A base S3, Cloud Files, FTP, FTPS, or SFTP directory URL where we\'ll put the transcoded file. If you\'re using ftp://, sftp://, etc.
          then you must <b>mount sites/default/files folder as the FTP base folder</b> before
          creating videos.<br/> You can use the <em>mount</em> command (mount --bind /source_dir /dest_dir_01):
          <br/> Eg : <br/><code>1. mkdir /home/USERNAME/videos</code><br/>
          <code>2. mount --bind /var/www/html/DRUPAL/sites/default/files/videos/ /home/USERNAME/files/videos</code><br/>
          <code>3. cd /home/USERNAME/videos</code><br/>
          <code>4. ls -l</code><br/>
          <code><b>It should display </b><br/>converted<br/>original<br/>thumbnails</code><br/>
          <code>5. chown -R USERNAME:USERNAME videos/</code>
          <br/><br/>Example URL patterns: <code>s3://my-output-bucket, cf://username:api_key@container/,
          ftp://user:password@ftp.example.com</code>', array('!more' => l(t('More info'), 'https://app.zencoder.com/docs/api/encoding/general-output-settings/base-url'))),
        '#default_value' => variable_get('video_zencoder_base_url', 'ftp://[user]:[password]@[ftp.url]'),
      );
      $form['zencoder_info']['video_zencoder_use_full_path'] = array(
        '#type' => 'checkbox',
        '#title' => t('Use full path'),
        '#default_value' => variable_get('video_zencoder_use_full_path', FALSE),
        '#description' => t('If enabled the full dir path to the file will show up in the filepath of the outputfile.<br/><b>Disable</b> this to reduce the security risk. We discourage enabling this option.')
      );
      // testing
      $form['zencoder_info']['testing'] = array(
        '#type' => 'fieldset',
        '#title' => t('Testing mode'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
      );
      $form['zencoder_info']['testing']['video_zencoder_testing_mode'] = array(
        '#type' => 'checkbox',
        '#title' => t('Test mode'),
        '#default_value' => variable_get('video_zencoder_testing_mode', FALSE),
        '#description' => t('Enable test mode to test upload/playback locally (if you have no public IP to test)')
      );
      $form['zencoder_info']['testing']['video_zencoder_test_file_path'] = array(
        '#type' => 'textfield',
        '#title' => t('Path to test video file'),
        '#description' => t('Add the path to a video file for Zencoder to transcode.
          You must use this file for testing when using a local machine with no public IP
          address from which Zencoder can download video.'),
        '#default_value' => variable_get('video_zencoder_test_file_path', 'http://example.com/video.mp4'),
      );
    }
    $form['video_zencoder_end'] = array(
      '#type' => 'markup',
      '#markup' => '</div>',
    );
    return $form;
  }

  public function adminSettingsValidate($form, &$form_state) {
    $v = $form_state['values'];

    if (variable_get('video_zencoder_api_key', FALSE)) {
      // Check S3 settings
      if (!empty($v['video_zencoder_base_url'])) {
        if (self::isS3Url($v['video_zencoder_base_url'])) {
          $s3compat = self::checkS3Compatibility();
          if (!empty($s3compat)) {
            form_error($form['zencoder_info']['video_zencoder_base_url'],
              t('Using Amazon S3 base URLs is not possible because of the following errors:') .
              theme('item_list', array('items' => $s3compat))
            );
          }
        }
      }
    }
    else {
      // check terms and condition
      if ($form_state['values']['agree_terms_zencoder'] == 0) {
        form_set_error('agree_terms_zencoder', t('You must agree to the !link.', array('!link' => l(t('terms and conditions'), 'http://zencoder.com/terms'))));
      }
      // check for email exists
      // Validate the e-mail address:
      if ($error = user_validate_mail($form_state['values']['zencoder_username'])) {
        form_set_error('zencoder_username', $error);
      }

      // get the API key from zencoder and save it to variable
      if (!form_get_errors()) {
        $mail = $form_state['values']['zencoder_username'];
        $result = $this->createUser($mail);
        if ($result !== TRUE) {
          form_set_error('zencoder_username', $result);
        }
        else {
          // Unset the form values because they do not need to be saved.
          unset($form_state['values']['zencoder_username']);
          unset($form_state['values']['agree_terms_zencoder']);
        }
      }
    }
  }

  /**
   * Create Zencoder user account
   */
  protected function createUser($mail) {
    libraries_load('zencoder');
    $zencoder = new Services_Zencoder();

    try {
      // $result is Services_Zencoder_Account
      $result = $zencoder->accounts->create(array(
        'terms_of_service' => '1',
        'email' => $mail,
        'affiliate_code' => 'drupal-video',
      ));

      variable_set('video_zencoder_api_key', $result->api_key);
      drupal_set_message(t('Your Zencoder details are as below.<br/><b>API Key</b> : @api_key<br/> <b>Password</b> : @password<br/> You can now login to !zencoder website and track your transcoding jobs online. Make sure you <b>save user/pass combination somewhere</b> before proceed.', array('@api_key' => $result->api_key, '@password' => $result->password, '!zencoder' => l(t('Zencoder'), 'http://zencoder.com'))), 'status');

      return TRUE;
    }
    catch (Services_Zencoder_Exception $e) {
      $errors = '';
      foreach ($e->getErrors() as $error) {
        if ($error == 'Email has already been taken') {
          drupal_set_message(t('Your account already exists on Zencoder. So <a href="@login-url">login</a> to here and enter API key below.', array('@login-url' => 'https://app.zencoder.com/session/new')));
          variable_set('video_zencoder_api_key', t('Please enter your API key'));
          return TRUE;
        }
        $errors .= $error;
      }

      return $errors;
    }
  }

  public function processPostback() {
    libraries_load('zencoder');
    $zencoder = new Services_Zencoder();

    $notification = $zencoder->notifications->parseIncoming();

    // Check output/job state
    $jobid = intval($notification->job->id);
    $video_output = db_query('SELECT vid, original_fid, output_fid FROM {video_output} WHERE job_id = :job_id', array(':job_id' => $jobid))->fetch();
    if (!$video_output) {
      return;
    }
    $vid = intval($video_output->vid);

    // get JSON post data
    watchdog('transcoder', 'Zencoder: Postback received from the Zencoder Transcoding servers for vid: @vid, Zencoder job id: @jobid.', array('@vid' => $vid, '@jobid' => $jobid));

    if ($notification->output->state == 'finished') {
      if ($notification->job->state != 'finished') {
        return;
      }

      $durationms = round($notification->output->duration_in_ms / 1000);
      db_update('video_queue')
        ->fields(array('duration' => $durationms, 'status' => VIDEO_RENDERING_COMPLETE))
        ->condition('vid', $vid)->execute();

      // Replace thumbnail
      $video_queue = db_query('SELECT entity_id, entity_type FROM {video_queue} WHERE vid = :vid', array(':vid' => $vid))->fetch();
      $entities = entity_load($video_queue->entity_type, array($video_queue->entity_id));
      $entity = reset($entities);

      // Get field name
      $field_name = NULL;
      $bundle = !empty($entity->type) ? $entity->type : $entity->node_type;
      foreach (field_info_instances($video_queue->entity_type, $bundle) as $key => $field) {
        if ($field['widget']['type'] == 'video_upload') {
          $field_name = $key;
        }
      }

      // download thumbnails
      $schema_thumb_path = file_uri_scheme($entity->{$field_name}['und'][0]['uri']) . '://' . variable_get('video_thumbnail_path', 'videos/thumbnails') . '/' . $video_output->original_fid;
      file_prepare_directory($schema_thumb_path, FILE_CREATE_DIRECTORY);
      $no_of_thumbnails = variable_get('video_thumbnail_count', 5);
      $thumbnail_id = rand(1, $no_of_thumbnails);
      $filename = 'thumbnail-' . $video_output->original_fid . '_' . sprintf('%04d', $thumbnail_id) . '.png';

      $bucket = variable_get('amazons3_bucket');

      $s3 = self::createS3Client();
      if ($s3->get_object_metadata($bucket, file_uri_target($schema_thumb_path) . '/' . $filename) && !file_exists(drupal_realpath($schema_thumb_path . '/' . $filename))) {
        $s3_get_object = $s3->get_object($bucket, file_uri_target($schema_thumb_path) . '/' . $filename, array('fileDownload' => drupal_realpath($schema_thumb_path . '/' . $filename)));

        $file = new stdClass();
        $file->uid = $entity->uid;
        $file->status = 0;
        $file->filename = $filename;
        $file->uri = $schema_thumb_path . '/' . $filename;
        $file->filemime = 'image/png';
        $file->filesize = filesize(drupal_realpath($schema_thumb_path . '/' . $filename));
        $file->timestamp = time();
        $file = file_save($file);
        $entity->{$field_name}['und'][0]['thumbnail'] = $file->fid;

        switch ($video_queue->entity_type) {
          case 'node':
            node_save($entity);
            break;
          case 'comment':
            comment_save($entity);
            break;
        }
      }
    }
    elseif ($notification->output->state == 'cancelled') {
      db_update('video_queue')->fields(array('status' => VIDEO_RENDERING_FAILED))->condition('vid', $vid)->execute();
      echo "Cancelled!\n";
    }
    else {
      $errorlink = t('no specific information given');
      if (!empty($notification->output->error_message)) {
        if (!empty($notification->output->error_link)) {
          $errordetail = l(t($notification->output->error_message), $notification->output->error_link);
        }
        else {
          $errordetail = t($notification->output->error_message);
        }
      }

      db_update('video_queue')->fields(array('status' => VIDEO_RENDERING_FAILED))->condition('vid', $vid)->execute();
      watchdog('transcoder', 'Zencoder reports errors for postback: !errordetail', array('!errordetail' => $errordetail), WATCHDOG_ERROR);
      echo "Failure!\n";
    }
  }

  /**
   * Creates a new Amazon S3 client object.
   *
   * @return
   *   AmazonS3 instance.
   */
  private static function createS3Client($key = NULL, $secret = NULL) {
    $info = libraries_load('awssdk');
    if (!$info['installed']) {
      return NULL;
    }

    $key = $key == NULL ? variable_get('aws_key', '') : $key;

    if (version_compare($info['version'], '1.5', '>=')) {
      $secret = $secret == NULL ? variable_get('aws_secret', '') : $secret;
      return new AmazonS3(array('key' => $key, 'secret' => $secret));
    }
    else {
      $secret = $secret == NULL ? variable_get('aws_secret_key', '') : $secret;
      return new AmazonS3($key, $secret);
    }
  }

  public static function checkS3Compatibility() {
    $errors = array();

    if (!module_exists('amazons3')) {
      $errors[] = t('You need to install and enable the <a href="@amazons3-url">Amazon S3</a> module.', array('@amazons3-url' => url('http://drupal.org/project/amazons3')));
    }

    if (variable_get('aws_key', NULL) === NULL) {
      $errors[] = t('You need to setup the <a href="@awssdk-config-page">Amazon Web Services access keys</a>.', array('@awssdk-config-page' => url('admin/config/media/awssdk')));
    }

    if (variable_get('amazons3_bucket', NULL) === NULL) {
      $errors[] = t('You need to setup the default <a href="@amazons3-config-page">Amazon S3 bucket name</a>.', array('@amazons3-config-page' => url('admin/config/media/amazons3')));
    }

    return $errors;
  }

  public static function isS3Url($url) {
    return file_uri_scheme($url) == 's3';
  }
}
