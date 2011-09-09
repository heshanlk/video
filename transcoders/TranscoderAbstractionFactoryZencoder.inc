<?php

class TranscoderAbstractionFactoryZencoder extends TranscoderAbstractionFactory implements TranscoderFactoryInterface {

  protected $options = array();

  public function __construct() {
    parent::__construct();
    module_load_include('php', 'video', 'libraries/zencoder-php/Zencoder');
    $this->options['api_key'] = variable_get('video_zencoder_api_key', NULL);
  }

  public function setInput($file, $input_frame_rate=0, $validate_decode_codecs = TRUE) {
    parent::setInput($file, $input_frame_rate, $validate_decode_codecs);
    $this->options['input'] = file_create_url($this->settings['input']['uri']);
    //http://heidisoft.drupal.video.s3.amazonaws.com/sites/demo.heidisoft.com/files/videos/converted/ecstasypromo.mp4
    $this->options['input'] = 'http://heidisoft.drupal.video.s3.amazonaws.com/sites/demo.heidisoft.com/files/videos/converted/ecstasypromo.mp4';
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
      $this->options['output']['notifications']['url'] = variable_get('video_zencoder_postback', $base_url . '/postback/jobs');
      // thumbnails
      $this->options['output']['thumbnails']['number'] = variable_get('video_thumbnail_count', 5);
      $this->options['output']['thumbnails']['thumb_size'] = variable_get('video_thumbnail_size', '320x240');
      $schema_thumb_path = file_uri_scheme($this->settings['input']['uri']) . '://' . variable_get('video_thumbnail_path', 'videos/thumbnails') . '/' . $this->settings['input']['fid'];
      $final_path = (variable_get('video_zencoder_use_full_path', TRUE)) ? drupal_realpath($schema_thumb_path) : '/' . file_uri_target($schema_thumb_path);
      $this->options['output']['thumbnails']['base_url'] = variable_get('video_zencoder_base_url', 'ftp://[user]:[password]@[ftp.url]') . $final_path;
      $this->options['output']['thumbnails']['prefix'] = 'thumbnail-' . $this->settings['input']['fid'];
    }
  }

  public function setOutput($output_directory, $output_name, $overwrite_mode = FILE_EXISTS_REPLACE) {
    parent::setOutput($output_directory, $output_name, $overwrite_mode);
    $this->options['output']['label'] = 'video-' . $this->settings['input']['fid'];
    // show full path is not secure as its visible in file path
    $final_path = (variable_get('video_zencoder_use_full_path', TRUE)) ? drupal_realpath($this->settings['base_url']) : '/' . file_uri_target($this->settings['base_url']);
    $this->options['output']['url'] = variable_get('video_zencoder_base_url', 'ftp://[user]:[password]@[ftp.url]') . $final_path . '/' . $this->settings['filename'];
    $this->options['output']['base_url'] = variable_get('video_zencoder_base_url', 'ftp://[user]:[password]@[ftp.url]') . $final_path;
    $this->options['output']['filename'] = $this->settings['filename'];
  }

  public function extractFrames() {
    global $user;
    // Get the file system directory.
    $schema_thumb_path = file_uri_scheme($this->settings['input']['uri']) . '://' . variable_get('video_thumbnail_path', 'videos/thumbnails') . '/' . $this->settings['input']['fid'];
    file_prepare_directory($schema_thumb_path, FILE_CREATE_DIRECTORY);

    $files = array();
    // Total thumbs to generate
    $no_of_thumbnails = variable_get('video_thumbnail_count', 5);
    for ($i = 0; $i < $no_of_thumbnails; $i++) {
      $filename = file_munge_filename("thumbnail-" . $this->settings['input']['fid'] . '_' . sprintf("%04d", $i) . 'png', '', TRUE);
      $thumbfile = $schema_thumb_path . '/' . $filename;
      //skip files already exists, this will save ffmpeg traffic
      if (!file_exists(drupal_realpath($thumbfile))) {
        // Download generated thumbnails from S3 if amasons3 module exists.
        if (module_exists('amazons3')) {
          libraries_load('awssdk');
          $key = variable_get('media_amazon_key', '');
          $secret_key = variable_get('media_amazon_key_secret', '');
          $bucket = variable_get('media_amazon_s3_bucket', '');
          $s3 = new AmazonS3($key, $secret_key);
          if ($s3->get_object_metadata($bucket, $video_thumb_path . '/' . $video['fid'] . '/' . $filename))
            $s3->get_object($bucket, $video_thumb_path . '/' . $video['fid'] . '/' . $filename, array('fileDownload' => drupal_realpath($thumbfile)));
          if (!file_exists(drupal_realpath($thumbfile))) {
            $error_param = array('%file' => $thumbfile, '%out' => $s3_get_object);
            $error_msg = t("Error downloading thumbnail for video: generated file %file does not exist.<br />S3 Output:<br />%out", $error_param);
            // Log the error message.
            watchdog('zencoder', $error_msg, array(), WATCHDOG_ERROR);
            continue;
          }
        }
      }
      // Create file object only file is exists
      if (file_exists(drupal_realpath($thumbfile))) {
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
    }
    return!empty($files) ? $files : FALSE;
  }

  public function execute() {
    $zencoder_job = new ZencoderJob(drupal_json_encode($this->options));
    if ($zencoder_job->created) {
      // Setup our converted video object
      // for the outout uri we should change ftp://, sftp:// -> public:// or private://
      $output_uri = $this->options['output']['base_url'] . '/' . $this->settings['filename'];
      switch (file_uri_scheme($output_uri)) {
        case 'fpt':
        case 'sftp':
          // thanks to D7 stream wrappers
          $output_uri = file_uri_scheme($this->settings['input']['uri']) . '://' . file_uri_target($this->settings['base_url']) . '/' . $this->settings['filename'];
          break;
      }
      $output = new stdClass();
      $output->filename = $this->settings['filename'];
      $output->uri = $output_uri;
      $output->filemime = file_get_mimetype($output_uri);
      $output->filesize = filesize(drupal_realpath($output_uri));
      $output->timestamp = time();
      $zencoder_output = $zencoder_job->outputs[$this->options['output']['label']]->id;
      $output->jobid = $zencoder_output;
      return $output;
//      return $encoding_job;
    }
    else {
      foreach ($zencoder_job->errors as $error) {
        $this->errors['execute'][] = $error;
        watchdog('transocoder', 'Zencoder reports some errors. !error', array('!error' => $error), WATCHDOG_ERROR);
      }
      return FALSE;
    }
  }

  public function getName() {
    return 'Zencoder';
  }

  public function getValue() {
    return 'TranscoderAbstractionFactoryZencoder';
  }

  public function getVersion() {
    return '1.2';
  }

  public function adminSettings() {
    global $user;
    global $base_url;
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
        '#title' => t('Dimention of thumbnails'),
        '#default_value' => variable_get('video_thumbnail_size', '320x240'),
        '#options' => video_explode("\n", variable_get("video_metadata_dimensions", video_default_dimensions()))
      );
      $form['zencoder_info']['video_zencoder_postback'] = array(
        '#type' => 'textfield',
        '#title' => t('Postback URL for Zencoder'),
        '#description' => t('Important : Do not change this if you do not know
          what your doing. This postback URL will receive video data when they are completed transcoding.'),
        '#default_value' => variable_get('video_zencoder_postback', $base_url . '/postback/jobs'),
      );
      $form['zencoder_info']['video_zencoder_base_url'] = array(
        '#type' => 'textfield',
        '#title' => t('Output file base'),
        '#description' => t('A base S3, Cloud Files, FTP, FTPS, or SFTP directory URL where we\'ll put the transcoded file, without a filename. If you do not supply a filename, we will generate a random filename plus an appropriate extension. !more. <br/>Eg: s3://my-output-bucket/, cf://username:api_key@container/, ftp://user:password@ftp.example.com', array('!more' => l('More info', 'https://app.zencoder.com/docs/api/encoding/general-output-settings/base-url'))),
        '#default_value' => variable_get('video_zencoder_base_url', 'ftp://[user]:[password]@[ftp.url]'),
      );
      $form['zencoder_info']['video_zencoder_use_full_path'] = array(
        '#type' => 'checkbox',
        '#title' => t('Use full path'),
        '#default_value' => variable_get('video_zencoder_use_full_path', TRUE),
        '#description' => t('If enabled the full dir path to the file will show up in the filepath of the outputfile.<br/><b>Disable</b> this to reduse the security fall, if your using S3:// and <b>must enable</b> this if your using ftp://, sftp://')
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

  public function processPostback() {
    // get JSON post data
    $data = file_get_contents("php://input");
    watchdog('zencoder', t('Postback received from the Zencoder Transcoding servers.' . serialize($data)));
    $video = drupal_json_decode($data);
    $zc_job_state = trim($video->job->state);
    $zc_output_state = trim($video->output->state);
    if ($zc_output_state == 'finished' && $zc_job_state == 'finished')
      $video->output->state = VIDEO_RENDERING_COMPLETE;
    if ($zc_output_state == 'failed' || $zc_job_state == 'failed')
      $video->output->state = VIDEO_RENDERING_FAILED;
    if ($zc_job_state == 'processing') {
      watchdog('zencoder', t('Job !jobid is processing.', array('!jobid' => $video->job->id)));
      return;
    }
    $video_output = db_query("SELECT vid, original_fid, output_fid FROM {video_output} WHERE job_id = :job_id", array(':job_id' => $video->job->id))->fetch();
    if ($video->output->state == VIDEO_RENDERING_COMPLETE) {
      // change the job status
      db_update('video_queue')->fields(array('status' => VIDEO_RENDERING_COMPLETE,))->condition('vid', $video_output->vid, '=')->execute();
      // load the original file
      $file = file_load($video_output->original_fid);
      // update the thumbanils
      // Setup our thmbnail path.
      $schema_thumb_path = file_uri_scheme($file->uri) . '://' . variable_get('video_thumbnail_path', 'videos/thumbnails') . '/' . $file->fid;
      file_prepare_directory($schema_thumb_path, FILE_CREATE_DIRECTORY);
      //    $i = rand(0, (variable_get('no_of_video_thumbnail_count', 5) - 1));
      $filename = 'thumbnail-' . $file->fid . '_' . sprintf("%04d", 1) . '.png';
      $thumbnail_path = $final_thumb_path . '/' . $filename;
      // s3 download to local server
      if (module_exists('amazons3') && file_uri_scheme(variable_get('video_zencoder_base_url', 'ftp://[user]:[password]@[ftp.url]')) == 's3') {
        libraries_load('awssdk');
        $key = variable_get('aws_key', '');
        $secret_key = variable_get('aws_secret_key', '');
        $bucket = file_uri_target(variable_get('video_zencoder_base_url', 'ftp://[user]:[password]@[ftp.url]'));
        $s3 = new AmazonS3($key, $secret_key);
        if ($s3->get_object_metadata($bucket, file_uri_target($schema_thumb_path) . '/' . $filename)) {
          $response = $s3->get_object($bucket, file_uri_target($schema_thumb_path) . $filename, array('fileDownload' => drupal_realpath($schema_thumb_path)));
          $default = $final_thumb_path . '/no-thumb.png';
          @unlink(drupal_realpath($default));
          if ($response->status == 200)
            watchdog('zencoder', t('Successfully downloaded the thumbnails file and replaced the default image.'));
          else
            watchdog('zencoder', t('Download thumbanils files is failed.'), array(), WATCHDOG_ERROR);
        }
        watchdog('zencoder', t('No thumbanils found.', array('!id' => $video->job->id, '!states' => $zc_output_state)));
      }
    } else if ($video->output->state == VIDEO_RENDERING_FAILED) {
      //    echo 'working failed';
      $zc->changeStatus($vid, VIDEO_RENDERING_FAILED);
      watchdog('zencoder', t('Zencoder job failed converting videos, please login to zencoder web and check the erros.', array()), NULL, WATCHDOG_ERROR);
    }
    else {
      echo 'URL check was sucess!.';
    }
  }

}