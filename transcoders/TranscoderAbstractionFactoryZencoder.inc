<?php

class TranscoderAbstractionFactoryZencoder extends TranscoderAbstractionFactory implements TranscoderFactoryInterface {

  public function __construct() {
    parent::__construct();
    module_load_include('php', 'video', 'libraries/zencoder-php/Zencoder');
  }

  public function setInput($file, $input_frame_rate=0, $validate_decode_codecs = TRUE) {
    parent::setInput($file, $input_frame_rate = 0, $validate_decode_codecs = TRUE);
  }

  public function setOptions($options = NULL) {
    parent::setOptions($options = NULL);
  }

  public function setOutput($output_directory, $output_name, $overwrite_mode = FILE_EXISTS_REPLACE) {
    parent::setOutput($output_directory, $output_name, $overwrite_mode = FILE_EXISTS_REPLACE);
  }

  public function extractFrames() {
    return;
    global $user;
    // Setup our thmbnail path.
    $video_thumb_path = variable_get('video_thumb_path', 'videos/thumbnails');
    $final_thumb_path = file_default_scheme() . '://' . $video_thumb_path . '/' . $video['fid'];

    // Ensure the destination directory exists and is writable.
    file_prepare_directory($final_thumb_path, FILE_CREATE_DIRECTORY);

    $files = array();
    // no thumbnails to generate
    $number_of_thumbs = variable_get('video_thumbs', 5);
    for ($i = 0; $i < $number_of_thumbs; $i++) {
      // @TODO Remove hard coded file types
      $filename = $video['fid'] . '_' . sprintf("%04d", $i) . '.png';
      $thumbfile = $final_thumb_path . '/' . $filename;
      //skip files already exists, this will save ffmpeg traffic
      if (!file_exists(drupal_realpath($thumbfile))) {
        $default = drupal_get_path('module', 'video') . '/images/no-thumb.png';
        // Download generated thumbnails from S3.
        libraries_load('awssdk');
        $key = variable_get('media_amazon_key', '');
        $secret_key = variable_get('media_amazon_key_secret', '');
        $bucket = variable_get('media_amazon_s3_bucket', '');
        $s3 = new AmazonS3($key, $secret_key);
        if ($s3->get_object_metadata($bucket, $video_thumb_path . '/' . $video['fid'] . '/' . $filename))
          $s3->get_object($bucket, $video_thumb_path . '/' . $video['fid'] . '/' . $filename, array('fileDownload' => drupal_realpath($thumbfile)));
        else {
          $thumbfile = $final_thumb_path . '/no-thumb.png';
          @copy($default, drupal_realpath($thumbfile), FILE_EXISTS_REPLACE);
        }
//          $thumbfile = drupal_get_path('module', 'video') . '/images/no_thumb.gif';
        if (!file_exists(drupal_realpath($thumbfile))) {
          $error_param = array('%file' => $thumbfile, '%out' => $s3_get_object);
          $error_msg = t("Error downloading thumbnail for video: generated file %file does not exist.<br />S3 Output:<br />%out", $error_param);
          // Log the error message.
          watchdog('zencoder', $error_msg, array(), WATCHDOG_ERROR);
          continue;
        }
      }
      // Begin building the file object.
      // @TODO : use file_munge_filename()
      $file = new stdClass();
      $file->uid = $user->uid;
      $file->status = 0;
      $file->filename = trim($filename);
      $file->uri = $thumbfile;
      $file->filemime = file_get_mimetype($filename);
      $file->filesize = filesize(drupal_realpath($thumbfile));
      $file->timestamp = time();
      $files[] = $file;
    }
    return $files;
  }

  public function execute() {
    
  }

  public function getName() {
    return 'Zencoder';
  }

  public function getValue() {
    return 'TranscoderAbstractionFactoryZencoder';
  }

  public function adminSettings() {
    global $user;

    $form = array();
    $form['video_zencoder_start'] = array(
      '#type' => 'markup',
      '#markup' => '<div id="' . $this->getValue() . '">',
    );
    $zencoder_api = variable_get('video_zencoder_api_key', null);
    if (!isset($zencoder_api) && empty($zencoder_api)) {
      $form['zencoder_user'] = array(
        '#type' => 'fieldset',
        '#title' => t('Zencoder User'),
        '#collapsible' => FALSE,
        '#collapsed' => FALSE,
        '#description' => t('Add your email address, password and <em>save configurations</em> to creare your Zencoder account. It will help you to transcode and manage your videos using Zencode website. Once you save your configurations then this will automatically create an account on the Zencoder.com and password and all ther other relevent details will be emailled to you.', array('!link' => l(t('Zencoder.com'), 'http://zencoder.com')))
      );
      $form['zencoder_user']['zencoder_username'] = array(
        '#type' => 'textfield',
        '#title' => t('Your email address'),
        '#default_value' => variable_get('zencoder_username', 'me@localhost'),
        '#size' => 50,
        '#description' => t('Make sure the email is accurate, since we will send all the password details to manage transcoding online and API key details to this.')
      );
//      $form['zencoder_user']['zencoder_password'] = array(
//        '#type' => 'password',
//        '#title' => t('Password'),
//        '#default_value' => variable_get('zencoder_password', ''),
//        '#size' => 50,
//        '#description' => t('Make sure the password is protect in somewhere, since it will need to login back to the Zencoder website.')
//      );
      $form['zencoder_user']['agree_terms_zencoder'] = array(
        '#type' => 'checkbox',
        '#title' => t('Agree Zencoder !link.', array('!link' => l(t('Terms and Conditions'), 'http://zencoder.com/terms', array('attributes' => array('target' => '_blank'))))),
        '#default_value' => variable_get('agree_terms_zencoder', TRUE),
      );
    }
    else {
      // Zencoder API is exists
      $form['zencoder_info'] = array(
        '#type' => 'fieldset',
        '#title' => t('Zencoder API'),
        '#collapsible' => FALSE,
        '#collapsed' => FALSE,
      );
      $form['zencoder_info']['video_zencoder_api_key'] = array(
        '#type' => 'textfield',
        '#title' => t('Zencoder API Key'),
        '#default_value' => variable_get('video_zencoder_api_key', null),
        '#description' => t('Zencoder API Key. Click <b>Reset to default</b> button to add new account.')
      );
      $form['zencoder_info']['video_thumbnail_count'] = array(
        '#type' => 'textfield',
        '#title' => t('Number of thumbnails'),
        '#description' => t('Number of thumbnails to display from video.'),
        '#default_value' => variable_get('video_thumbnail_count', 5),
        '#size' => 5
      );
      $form['zencoder_info']['video_thumbnail_count_size'] = array(
        '#type' => 'select',
        '#title' => t('Dimention of thumbnails'),
        '#default_value' => variable_get('video_thumbnail_size', '320x240'),
        '#options' => video_explode("\n", variable_get("video_metadata_dimensions", video_default_dimensions()))
      );
      global $base_url;
      $form['zencoder_info']['video_zencoder_postback'] = array(
        '#type' => 'textfield',
        '#title' => t('Postback URL for Zencoder'),
        '#description' => t('Important : Do not change this if you do not know
          what your doing. This postback URL will receive video data when they are completed transcoding.'),
        '#default_value' => variable_get('video_zencoder_postback', $base_url . '/postback/jobs'),
      );
    }
    $form['video_zencoder_end'] = array(
      '#type' => 'markup',
      '#markup' => '</div>',
    );
    return $form;
  }

  public function adminSettingsValidate($form, &$form_state) {
    if (variable_get('video_zencoder_api_key', FALSE))
      return;
    // check terms and condition
    if ($form_state['values']['agree_terms_zencoder'] == 0) {
      form_set_error('agree_terms_zencoder', t('You must agree !link.', array('!link' => l(t('terms and conditions'), 'http://zencoder.com/terms'))));
    }
    // check for email exists
    // Validate the e-mail address:
    if ($error = user_validate_mail($form_state['values']['zencoder_username'])) {
      form_set_error('zencoder_username', $error);
    }

    // get the API key from zencoder and save it to variable
    if (!form_get_errors()) {
      $user = new stdClass;
      $user->email = $form_state['values']['zencoder_username'];
//      $user->password = $form_state['values']['zencoder_password'];
      $result = $this->createUser($user);
      if ($result !== TRUE)
        form_set_error('zencoder_username', $result);
    }
  }

  protected function createUser($user) {
    $request = new ZencoderRequest(
            'https://app.zencoder.com/api/account',
            FALSE, // API key isn't needed for new account creation
            array(
              "terms_of_service" => "1",
              "email" => $user->email,
//              "password" => $user->password,
              "affiliate_code" => "drupal-video"
            )
    );
    if ($request->successful) {
      $results = $request->results;
      variable_set('video_zencoder_api_key', $results['api_key']);
      drupal_set_message(t('Your Zencoder Details are as below.<br/><b>API Key</b> : !api_key<br/> <b>Password</b> : !password<br/> You can now login to !zencoder website and track your transcoding jobs online. Make sure you <b>save user/pass combination somewhere</b> before proceed.', array('!api_key' => $results['api_key'], '!password' => $results['password'], '!zencoder' => l('Zencoder', 'http://zencoder.com'))), 'status');
      return TRUE;
    }
    else {
      $errors = '';
      foreach ($request->errors as $error) {
        $errors .= $error;
      }
      return $errors;
    }
  }

}