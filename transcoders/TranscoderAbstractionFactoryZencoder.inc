<?php

class TranscoderAbstractionFactoryZencoder extends TranscoderAbstractionFactory implements TranscoderFactoryInterface {

  public function __construct() {
    parent::__construct();
  }

  public function setInput($file, $input_frame_rate=0, $validate_decode_codecs = TRUE) {
    parent::setInput();
  }

  public function setOptions($options = NULL) {
    parent::setOptions();
  }

  public function setOutput($output_directory, $output_name, $overwrite_mode = FILE_EXISTS_REPLACE) {
    parent::setOutput();
  }

  public function extractFrames() {
    
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
        '#description' => t('Save configurations to creare your Zencoder account to transcode and manage your videos using Zencode  API. Once you save your configurations then this will automatically create an account on the Zencoder.com and password and all ther other relevent details will be emailled to you.', array('!link' => l(t('Zencoder.com'), 'http://zencoder.com')))
      );
      $form['zencoder_user']['zencoder_username'] = array(
        '#type' => 'textfield',
        '#title' => t('Your email address'),
        '#default_value' => variable_get('zencoder_username', 'me@localhost'),
        '#size' => 50,
        '#description' => t('Make sure the email is accurate, since we will send all the password details to manage transcoding online and API key details to this.')
      );
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
        '#type' => 'textfield',
        '#title' => t('Dimention of thumbnails'),
        '#description' => t('Size of thumbnails to extract from video.'),
        '#default_value' => variable_get('video_thumbnail_count_size', '160x120'),
        '#size' => 10
      );
      global $base_url;
      $form['zencoder_info']['video_zencoder_postback'] = array(
        '#type' => 'textfield',
        '#title' => t('Postback URL for Zencoder'),
        '#description' => t('Important : Do not change this if you do not know
          what your doing.<br/> This postback URL will receive video data when they are completed.'),
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
      $email = $form_state['values']['zencoder_username'];
      module_load_include('inc', 'video_zencoder', '/includes/zencoder');
      $zc = new video_zencoder_api;
      $user = new stdClass;
      $user->email = $email;
      $result = $zc->create_user($user);
      if ($result !== TRUE)
        form_set_error('zencoder_username', $result);
    }
  }

}