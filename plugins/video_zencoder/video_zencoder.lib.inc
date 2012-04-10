<?php
/**
 * @file
 * Class file to handle amazon s3 transfers.
 */

define('VIDEO_ZC_PENDING', 0);
define('VIDEO_ZC_WORKING', 1);
define('VIDEO_ZC_ACTIVE', 2);
define('VIDEO_ZC_FAILED', 3);

class video_zencoder_api {
  private $access_key;

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

    require_once $libfile;
    $this->zencoder = new Services_Zencoder($this->access_key);
  }

  /**
   * create transcoding job on Zencoder.com
   */
  public function create(stdClass $video) {
    if ($this->zencoder == NULL) {
      return FALSE;
    }

    // dimensions
    $dimensions = explode('x', $video->dimensions);

    // Notifications
    $notifications = array(array(
      'format' => 'json',
      'url' => variable_get('video_zencoder_postback', url('postback/jobs', array('absolute' => TRUE))),
    ));

    // S3 permissions
    $public = !variable_get('amazon_s3_private', FALSE);

    // Common output URL prefix
    $output_url_prefix = 's3://'. $video->bucket .'/'. dirname(dirname($video->filepath)) .'/converted/'. pathinfo($video->filepath, PATHINFO_FILENAME);

    // construct the output array with the presets
    $zc_outputs = array();
    foreach ($video->presets as $preset) {
      $settings = $preset->getSettings();

      if (array_key_exists('additional_settings', $settings)) {
        $settings = array_merge($settings, $settings['additional_settings']);
        unset($settings['additional_settings']);
      }

      $settings['label'] = 'VIDEO_'. $video->fid .'_'. $preset->id;
      $settings['url'] = $output_url_prefix . $preset->filenamesuffix .'.'. $preset->extension;
      $settings['public'] = $public;
      $settings['width'] = intval($dimensions[0]);
      $settings['height'] = intval($dimensions[1]);
      $settings['notifications'] = $notifications;

      $zc_outputs[] = $settings;
    }

    // Add thumbnails
    if (empty($zc_outputs)) $zc_outputs[0] = array();

    $zc_outputs[0]['thumbnails'] = array(
      'number' => intval(variable_get('video_thumbs', 5)),
      'size' => variable_get('video_thumbs_size', '160x120'),
      'base_url' => 's3://'. $video->bucket .'/'. video_thumb_path($video, TRUE),
      'prefix' => $video->fid,
    );

    try {
      return $this->zencoder->jobs->create(array(
        'api_key' => $this->access_key,
        'input' => 's3://'. $video->bucket .'/'. $video->filepath,
        'outputs' => $zc_outputs,
      ));
    }
    catch (Services_Zencoder_Exception $e) {
      watchdog('zencoder', 'Zencoder reports errors while converting %file:<br/>!errorlist', array('%file' => $video->filename, '!errorlist' => theme('item_list', $e->getErrors())), WATCHDOG_ERROR);
      return FALSE;
    }
  }

  public function cancel_job(stdClass $video) {
    try {
      $this->zencoder->jobs->cancel($video->jobid);
      return TRUE;
    }
    catch (Services_Zencoder_Exception $e) {
      watchdog('zencoder', 'Error while cancelling job @jobid for video %filepath: !errorlist, @errormsg', array('@jobid' => $video->jobid, '%filepath' => $video->filepath, '@errormsg' => $e->getMessage(), '!errorlist' => theme('item_list', $e->getErrors())), WATCHDOG_ERROR);
      return FALSE;
    }
  }

  /**
   * Create Zencoder user account
   */
  public function create_user($mail) {
    if ($this->zencoder == NULL) {
      return FALSE;
    }

    try {
      // $result is Services_Zencoder_Account
      $result = $this->zencoder->accounts->create(array(
        'terms_of_service' => '1',
        'email' => $mail,
        'affiliate_code' => 'drupal-video',
      ));

      $params = array(
        'email' => $mail,
        'api_key' => $result->api_key,
        'password' => $result->password,
      );

      variable_set('video_zencoder_api_key', $result->api_key);
      $message = drupal_mail('video_zencoder', 'video_zencoder', $mail, language_default(), $params);
      if (!$message['result']) {
        drupal_set_message(t('Unable to send e-mail. Your Zencoder Details are as below.<br/> <b>API Key</b> : @api_key<br/> <b>Password</b> : @password<br/>', array('@api_key' => $results['api_key'], '@password' => $results['password'])), 'status');
      }
      else {
        drupal_set_message(t('Your account has been created and is ready to start processing on Zencoder'));
      }

      return TRUE;
    }
    catch (Services_Zencoder_Exception $e) {
      $exerrors = $e->getErrors();
      if (is_array($exerrors) || $exerrors instanceof Services_Zencoder_Error) {
        $errors = '';

        foreach ($exerrors as $error) {
          if ($error == 'Email has already been taken') {
            drupal_set_message(t('Your account already exists on Zencoder. So <a href="@login-url">login</a> to here and enter API key below.', array('@login-url' => 'https://app.zencoder.com/session/new')));
            variable_set('video_zencoder_api_key', 'Please enter your API Key');
            return TRUE;
          }
          $errors .= $error;
        }

        return $errors;
      }

      return $e->getMessage();
    }
  }
}