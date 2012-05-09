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

  public function setOptions(array $options) {
    foreach ($options as $key => $value) {
      if (empty($value) || $value === 'none') {
        continue;
      }

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
          $this->options['output']['one_pass'] = $value == 1;
          break;
        default :
          $this->options['output'][$key] = $value;
      }
    }

    // set notifications
    $this->options['output']['notifications']['format'] = 'json';
    $this->options['output']['notifications']['url'] = $this->postbackurl;

    // thumbnails
    $this->options['output']['thumbnails'] = array(
      'format' => $this->options['output']['thumbnails']['format'],
      'number' => intval(variable_get('video_thumbnail_count', 5)),
      'thumb_size' => variable_get('video_thumbnail_size', '320x240'),
      'prefix' => 'thumbnail-' . $this->settings['input']['fid'],
    );

    return TRUE;
  }

  public function setOutput($output_directory, $output_name, $overwrite_mode = FILE_EXISTS_REPLACE) {
    parent::setOutput($output_directory, $output_name, $overwrite_mode);
    $this->options['output']['label'] = 'video-' . $this->settings['input']['fid'];
    $this->options['output']['filename'] = $this->settings['filename'];
    $this->options['output']['public'] = 1;
  }

  /**
   * For new videos, this function is never called, because all thumbnails are
   * extracted and saved to the databases during the post back handler in
   * TranscoderAbstractionFactoryZencoder::processPostback().
   *
   * @see TranscoderFactoryInterface::extractFrames()
   */
  public function extractFrames($destinationScheme, $format) {
    // Check if the job has been completed.
    // If the job has not been completed, don't bother checking for
    // thumbnails
    $job = $this->loadJob();
    if (empty($job)) {
      return array();
    }
    // No thumbnails available yet
    if ($job->video_status != VIDEO_RENDERING_COMPLETE) {
      return array();
    }

    $fid = $this->settings['input']['fid'];
    $path = variable_get('video_thumbnail_path', 'videos/thumbnails') . '/' . $fid;

    // Get the file system directory.
    $dsturibase = $destinationScheme . '://' . $path . '/';
    file_prepare_directory($dsturibase, FILE_CREATE_DIRECTORY);
    $dstwrapper = file_stream_wrapper_get_instance_by_scheme($destinationScheme);

    // Where to copy the thumbnails from.
    $final_path = variable_get('video_zencoder_use_full_path', FALSE) ? drupal_realpath(file_uri_scheme($this->settings['input']['uri']) . '://' . $path) : '/' . $path;
    $srcuribase = variable_get('video_zencoder_base_url') . $final_path . '/';

    $thumbs = array();
    // Total thumbs to generate
    $no_of_thumbnails = variable_get('video_thumbnail_count', 5);
    for ($i = 0; $i < $no_of_thumbnails; $i++) {
      $filename = file_munge_filename('thumbnail-' . $fid . '_' . sprintf('%04d', $i) . '.png', '', TRUE);
      $dsturi = $dsturibase . $filename;

      // Download file from S3, if available
      if (!file_exists($dsturi)) {
        $srcuri = $srcuribase . $filename;
        if (!file_exists($srcuri)) {
          watchdog('zencoder',
            'Error downloading thumbnail for video %filename: %thumbpath does not exist.',
            array('%filename' => $this->settings['input']['filename'], '%thumbpath' => $srcuri),
            WATCHDOG_ERROR);
          break;
        }

        copy($srcuri, $dsturi);
        if (variable_get('video_zencoder_testing_mode', FALSE)) {
          db_update('video_queue')
            ->fields(array('status' => VIDEO_RENDERING_COMPLETE))
            ->condition('fid', $fid)->execute();
        }

        // Delete the source, it is no longer needed
        drupal_unlink($srcuri);
      }

      $thumb = new stdClass();
      $thumb->status = 0;
      $thumb->filename = $filename;
      $thumb->uri = $dsturi;
      $thumb->filemime = $dstwrapper->getMimeType($dsturi);
      $thumbs[] = $thumb;
    }

    return !empty($thumbs) ? $thumbs : FALSE;
  }

  public function execute() {
    libraries_load('zencoder');
    $zencoder = new Services_Zencoder();

    try {
      $encoding_job = $zencoder->jobs->create($this->options);
      $output_uri = $this->settings['base_url'] . '/' . $this->settings['filename'];

      $output = new stdClass();
      $output->filename = $this->settings['filename'];
      $output->uri = $output_uri;
      $output->filemime = file_get_mimetype($output_uri);
      $output->filesize = 0;
      $output->timestamp = time();
      $output->jobid = intval($encoding_job->id);
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
        '#size' => 5,
      );
      $form['zencoder_info']['video_thumbnail_size'] = array(
        '#type' => 'select',
        '#title' => t('Dimension of thumbnails'),
        '#default_value' => variable_get('video_thumbnail_size', '320x240'),
        '#options' => video_utility::getDimensions(),
      );
      $form['zencoder_info']['video_zencoder_postback'] = array(
        '#type' => 'textfield',
        '#title' => t('Postback URL for Zencoder'),
        '#description' =>
          t('Important : Don\'t change this if you don\'t know what you\'re doing. This postback URL will receive video data as video transcodings are completed.') . '<br/>' .
          t('Default: %value', array('%value' => url('postback/jobs', array('absolute' => TRUE)))),
        '#default_value' => $this->postbackurl,
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
      // Check the postback URL
      $testurl = $v['video_zencoder_postback'];
      $testcode = md5(mt_rand(0, REQUEST_TIME));
      if (strpos($testurl, '?') === FALSE) {
        $testurl .= '?test=1';
      }
      else {
        $testurl .= '&test=1';
      }
      variable_set('video_postback_test', $testcode);
      $result = drupal_http_request($testurl);
      variable_del('video_postback_test');

      if ($result->code != 200) {
        form_error(
          $form['zencoder_info']['video_zencoder_postback'],
          t('The postback URL cannot be retrieved: @error (@code).',
          array('@code' => $result->code, '@error' => empty($result->error) ? t('unknown error') : $result->error))
        );
      }
      elseif (empty($result->data) || $result->data != $testcode) {
        form_error($form['zencoder_info']['video_zencoder_postback'], t('The postback URL is not valid: returned data contains unexpected value &quot;@value&quot;.', array('@value' => $result->data)));
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

    try {
      $notification = $zencoder->notifications->parseIncoming();
    } catch (Services_Zencoder_Exception $e) {
      watchdog('transcoder', 'Zencoder: Postback received from the Zencoder Transcoding could not be decoded: @errormsg', array('@errormsg' => $e->getMessage()));
      echo 'Bad request';
      return;
    }

    // Check output/job state
    $jobid = intval($notification->job->id);
    $video_output = db_query('SELECT vid, original_fid, output_fid FROM {video_output} WHERE job_id = :job_id', array(':job_id' => $jobid))->fetch();
    if (empty($video_output)) {
      echo 'Not found';
      return;
    }
    $vid = intval($video_output->vid);

    // get JSON post data
    watchdog('transcoder', 'Zencoder: Postback received from the Zencoder Transcoding servers for vid: @vid, Zencoder job id: @jobid.', array('@vid' => $vid, '@jobid' => $jobid));

    // Find all error situations
    if ($notification->output->state == 'cancelled') {
      db_update('video_queue')->fields(array('status' => VIDEO_RENDERING_FAILED))->condition('vid', $vid)->execute();
      echo 'Cancelled';
      return;
    }

    if ($notification->job->state != 'finished') {
      return;
    }

    if ($notification->output->state != 'finished') {
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

    $video = $this->loadJob($vid, 'vid');
    if (empty($video)) {
      echo 'Not found';
      return;
    }

    // Move the converted video to its final destination
    $outputfile = file_load($video_output->output_fid);
    if (empty($outputfile)) {
      echo 'Output file not found in database';
      return;
    }
    copy($notification->output->url, $outputfile->uri);
    $outputfile->filesize = $notification->output->file_size_in_bytes;
    drupal_write_record('file_managed', $outputfile, 'fid');

    // Zencoder creates multiple jobs for the same vid if multiple presets are active.
    // If a previous job has put this video in complete state, bail out now.
    if ($video->video_status == VIDEO_RENDERING_COMPLETE) {
      return;
    }

    // Actual processing of the response
    $video->duration = round($notification->output->duration_in_ms / 1000);
    $video->status = VIDEO_RENDERING_COMPLETE;
    drupal_write_record('video_queue', $video, 'vid');

    // Find the entity to which the file belongs
    $entity = video_utility::loadEntity($video->entity_type, $video->entity_id);
    if (empty($entity)) {
      watchdog('transcoder', 'The entity to which the transcoded video belongs can\'t be found anymore. Entity type: @entity-type, entity id: @entity-id.', array('@entity-type' => $video->entity_type, '@entity-id' => $video->entity_id), WATCHDOG_ERROR);
      return;
    }

    // Clear the field cache. Normally, node_save() does this, but that function is not invoked in all cases
    cache_clear_all('field:' . $video->entity_type . $video->entity_id, 'cache_field');

    // Retrieve the thumbnails from the notification structure
    $thumbnails = $notification->output->thumbnails[0]['images'];
    if (empty($thumbnails)) {
      // If there are no thumbnails, quit now.
      return;
    }

    // The following information was saved in TranscoderFactory::createJob()
    $fieldname = $video->data['field_name'];
    $field = field_info_field($fieldname);
    $langcode = $video->data['langcode'];
    $delta = $video->data['delta'];

    // Insanity checks
    if (empty($entity->{$fieldname}[$langcode][$delta])) {
      // The field can't be found anymore. This may be a problem.
      watchdog('transcoder', 'The field to which video @filename was uploaded doesn\'t seem to exist anymore. Entity type: @entity-type, entity id: @entity-id, field name: @fieldname, field language: @langcode, delta: @delta.', array('@filename' => $video->filename, '@entity-type' => $video->entity_type, '@entity-id' => $video->entity_id, '@fieldname' => $fieldname, '@langcode' => $langcode, '@delta' => $delta), WATCHDOG_WARNING);
      return;
    }
    if ($entity->{$fieldname}[$langcode][$delta]['fid'] != $video->fid) {
      // The field does not contain the file we uploaded.
      watchdog('transcoder', 'The field to which video @filename was uploaded doesn\'t seem to contain this video anymore. Entity type: @entity-type, entity id: @entity-id, field name: @fieldname, field language: @langcode, delta: @delta.', array('@filename' => $video->filename, '@entity-type' => $video->entity_type, '@entity-id' => $video->entity_id, '@fieldname' => $fieldname, '@langcode' => $langcode, '@delta' => $delta), WATCHDOG_WARNING);
      return;
    }

    // Destination of thumbnails
    $thumbscheme = !empty($field['settings']['uri_scheme_thumbnails']) ? $field['settings']['uri_scheme_thumbnails'] : 'public';
    $thumburibase =  $thumbscheme . '://' . variable_get('video_thumbnail_path', 'videos/thumbnails') . '/' . $video->fid . '/';
    file_prepare_directory($thumburibase, FILE_CREATE_DIRECTORY);
    $thumbwrapper = file_stream_wrapper_get_instance_by_scheme($thumbscheme);

    // Turn the thumbnails into managed files.
    // Because two jobs for the same video may finish simultaneously, lock here so
    // there are no errors when inserting the files.
    if (!lock_acquire('video_zencoder_thumbnails:' . $video->fid, count($thumbnails) * 30)) {
      if (lock_wait('video_zencoder_thumbnails:' . $video->fid, count($thumbnails) * 30)) {
        watchdog('transcoder', 'Failed to acquire lock to download thumbnails for @video-filename.', array('@video-filename' => $video->filename), WATCHDOG_ERROR);
        return;
      }
    }
    $existingthumbs = db_query('SELECT f.uri, f.fid FROM {file_managed} f INNER JOIN {video_thumbnails} t ON (f.fid = t.thumbnailfid) WHERE t.videofid = :fid', array(':fid' => $video->fid))->fetchAllKeyed();
    $thumbs = array();
    $tnid = 0;
    foreach ($thumbnails as $thumbnail) {
      $urlpath = parse_url($thumbnail['url'], PHP_URL_PATH);
      $ext = video_utility::getExtension($urlpath);
      $thumb = new stdClass();
      $thumb->uid = $entity->uid;
      $thumb->status = FILE_STATUS_PERMANENT;
      $thumb->filename = 'thumbnail-' . $video->fid . '_' . sprintf('%04d', $tnid++) . '.' . $ext;
      $thumb->uri = $thumburibase . $thumb->filename;
      $thumb->filemime = $thumbwrapper->getMimeType($thumb->uri);
      $thumb->type = 'image'; // For the media module
      if (isset($existingthumbs[$thumb->uri])) {
        $thumb->fid = intval($existingthumbs[$thumb->uri]);
      }

      if (!copy($thumbnail['url'], $thumb->uri)) {
        watchdog('transcoder', 'Could not copy @thumbsrc to @thumbdest.', array('@thumbsrc' => $thumbnail['url'], '@thumbdest' => $thumb->uri), WATCHDOG_ERROR);
        continue;
      }

      file_save($thumb);

      // Saving to video_thumbnails and file_usage is only necessary when this is a new thumbnail
      if (!isset($existingthumbs[$thumb->uri])) {
        db_insert('video_thumbnails')->fields(array('videofid' => $video->fid, 'thumbnailfid' => $thumb->fid))->execute();
        file_usage_add($thumb, 'file', $video->entity_type, $video->entity_id);
      }

      $thumbs[$thumb->fid] = $thumb;
    }
    lock_release('video_zencoder_thumbnails:' . $video->fid);

    // Skip setting the thumbnail if there are no thumbnails or when the current value is already valid
    $currentthumb = isset($entity->{$fieldname}[$langcode][$delta]['thumbnail']) ? intval($entity->{$fieldname}[$langcode][$delta]['thumbnail']) : 0;
    if (empty($thumbs) || isset($thumbs[$currentthumb])) {
      return;
    }

    // Set a random thumbnail fid on the entity and save the entity
    $entity->{$fieldname}[$langcode][$delta]['thumbnail'] = array_rand($thumbs);

    switch ($video->entity_type) {
      case 'node':
        node_save($entity);
        break;
      case 'comment':
        comment_save($entity);
        break;
      default:
        // entity_save() is supplied by the entity module
        if (function_exists('entity_save')) {
          entity_save($entity);
        }
        break;
    }
  }
}
