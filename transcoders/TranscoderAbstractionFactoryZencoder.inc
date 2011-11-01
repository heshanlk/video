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
    if (variable_get('video_zencoder_testing_mode', FALSE))
      $this->options['input'] = variable_get('video_zencoder_test_file_path', 'http://example.com/video.mp4');
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
    $s3_get_object = FALSE;
    // Get the file system directory.
    $schema_thumb_path = file_uri_scheme($this->settings['input']['uri']) . '://' . variable_get('video_thumbnail_path', 'videos/thumbnails') . '/' . $this->settings['input']['fid'];
    file_prepare_directory($schema_thumb_path, FILE_CREATE_DIRECTORY);

    $files = array();
    // Total thumbs to generate
    $no_of_thumbnails = variable_get('video_thumbnail_count', 5);
    for ($i = 0; $i < $no_of_thumbnails; $i++) {
      $filename = file_munge_filename("thumbnail-" . $this->settings['input']['fid'] . '_' . sprintf("%04d", $i) . '.png', '', TRUE);
      //skip files already exists, this will save ffmpeg traffic
      if (!file_exists(drupal_realpath($schema_thumb_path . '/' . $filename))) {
        // Download generated thumbnails from S3 if amasons3 module exists.
        if (module_exists('amazons3') && file_uri_scheme(variable_get('video_zencoder_base_url', 'ftp://[user]:[password]@[ftp.url]')) == 's3') {
          libraries_load('awssdk');
          $key = variable_get('aws_key', '');
          $secret_key = variable_get('aws_secret_key', '');
          $bucket = variable_get('amazons3_bucket');
          $s3 = new AmazonS3($key, $secret_key);
          if ($s3->get_object_metadata($bucket, file_uri_target($schema_thumb_path) . '/' . $filename)) {
            $s3_get_object = $s3->get_object($bucket, file_uri_target($schema_thumb_path) . '/' . $filename, array('fileDownload' => drupal_realpath($schema_thumb_path . '/' . $filename)));
            if (variable_get('video_zencoder_testing_mode', FALSE))
            // change the job status
              db_update('video_queue')->fields(array('status' => VIDEO_RENDERING_COMPLETE,))->condition('fid', $this->settings['input']['fid'], '=')->execute();
          }
          if (!file_exists(drupal_realpath($schema_thumb_path . '/' . $filename)) && $s3_get_object) {
            $error_param = array('%file' => $schema_thumb_path, '%out' => $s3_get_object);
            $error_msg = t("Error downloading thumbnail for video: generated file %file does not exist.<br />S3 Output:<br />%out", $error_param);
            // Log the error message.
            watchdog('zencoder', $error_msg, array(), WATCHDOG_ERROR);
            continue;
          }
        }
      }
      // Create file object only file is exists
      if (file_exists(drupal_realpath($schema_thumb_path . '/' . $filename))) {
        $file = new stdClass();
        $file->uid = $user->uid;
        $file->status = 0;
        $file->filename = trim($filename);
        $file->uri = $schema_thumb_path . '/' . $filename;
        $file->filemime = file_get_mimetype($filename);
        $file->filesize = filesize(drupal_realpath($schema_thumb_path . '/' . $filename));
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
      $output->jobid = $zencoder_job->id;
      $output->duration = 0;
      return $output;
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
        '#title' => st('Zencoder User'),
        '#collapsible' => FALSE,
        '#collapsed' => FALSE,
        '#description' => st('Add your email address, password and <em>save configurations</em> 
          to creare your Zencoder account. It will help you to transcode and manage your videos 
          using Zencode website. Once you save your configurations then this will automatically 
          create an account on the Zencoder.com and password and all ther other relevent details 
          will be emailled to you.', array('!link' => l(st('Zencoder.com'), 'http://zencoder.com')))
      );
      $form['zencoder_user']['zencoder_username'] = array(
        '#type' => 'textfield',
        '#title' => st('Your email address'),
        '#default_value' => variable_get('zencoder_username', 'me@localhost'),
        '#size' => 50,
        '#description' => st('Make sure the email is accurate, since we will send all the password details to manage transcoding online and API key details to this.')
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
        '#title' => st('Agree Zencoder !link.', array('!link' => l(st('Terms and Conditions'), 'http://zencoder.com/terms', array('attributes' => array('target' => '_blank'))))),
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
        '#description' => t('A base S3, Cloud Files, FTP, FTPS, or SFTP directory 
          URL where we\'ll put the transcoded file. If your using ftp://, sftp:// 
          or similar then you have to <b>mount sites/default/files folder to FTP base folder</b> before 
          create videos.<br/> You can use <em>mount</em> command (mount --bind /source_dir /dest_dir_01):
          <br/> Eg : <br/><code>1. mkdir /home/USERNAME/videos</code><br/>
          <code>2. mount --bind /var/www/html/DRUPAL/sites/default/files/videos/ /home/USERNAME/files/videos</code><br/>
          <code>3. cd /home/USERNAME/videos</code><br/>
          <code>4. ls -l</code><br/>
          <code><b>It should display </b><br/>converted<br/>original<br/>thumbnails</code><br/>
          <code>5. chown -R USERNAME:USERNAME videos/</code>
          <br/><br/>Example URL patterens: <code>s3://my-output-bucket, cf://username:api_key@container/, 
          ftp://user:password@ftp.example.com</code>', array('!more' => l('More info', 'https://app.zencoder.com/docs/api/encoding/general-output-settings/base-url'))),
        '#default_value' => variable_get('video_zencoder_base_url', 'ftp://[user]:[password]@[ftp.url]'),
      );
      $form['zencoder_info']['video_zencoder_use_full_path'] = array(
        '#type' => 'checkbox',
        '#title' => t('Use full path'),
        '#default_value' => variable_get('video_zencoder_use_full_path', FALSE),
        '#description' => t('If enabled the full dir path to the file will show up in the filepath of the outputfile.<br/><b>Disable</b> this to reduse the security fall. We don\'t encourage to enble this.')
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
        '#description' => t('Enable test mode to test upload/playback locally(If you do not have any public IP to test)')
      );
      $form['zencoder_info']['testing']['video_zencoder_test_file_path'] = array(
        '#type' => 'textfield',
        '#title' => t('Path to test video file'),
        '#description' => t('Add a file path to a video file to download by Zencoder 
          to trnascode, you have to mention this file for testing if your using your 
          locabox and there is no any public ip where Zencoder can download video.'),
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
    // Catch notification
    $notification = ZencoderOutputNotification::catch_and_parse();
    // Check output/job state
    $video_output = db_query("SELECT vid, original_fid, output_fid FROM {video_output} WHERE job_id = :job_id", array(':job_id' => $notification->job->id))->fetch();
    if (!$video_output->vid)
      return;
    // get JSON post data
    watchdog('transcoder', t('Zencoder : Postback received from the Zencoder Transcoding servers for vid: !vid.', array('!vid' => $video_output->vid)));

    if ($notification->output->state == "finished") {
      if ($notification->job->state == "finished") {
        // update file metadata ie. duration
        $request = new ZencoderRequest(
                'https://app.zencoder.com/api/jobs/' . $notification->job->id,
                $this->options['api_key']
        );

        if ($request->successful) {
          $duration = round($request->results['job']['output_media_files'][0]['duration_in_ms'] / 1000);
          db_update('video_queue')->fields(array('duration' => $duration))->condition('vid', $video_output->vid, '=')->execute();
        }
        else {
          foreach ($request->errors as $error) {
            watchdog('transcoder', t('Zencoder reports errors !errors.', array('!errors' => $error)), NULL, WATCHDOG_ERROR);
          }
        }
        // change the job status
        db_update('video_queue')->fields(array('status' => VIDEO_RENDERING_COMPLETE))->condition('vid', $video_output->vid, '=')->execute();
        echo "Dubble w00t!";
        // Replace thumbnail
        $video_queue = db_query("SELECT nid FROM {video_queue} WHERE vid = :vid", array(':vid' => $video_output->vid))->fetch();
        $node = node_load($video_queue->nid);
        //get field name
        $field_name = NULL;
        foreach (field_info_instances('node', $node->type) as $key => $field) {
          if ($field['widget']['type'] == 'video_upload')
            $field_name = $key;
        }
        // download thumbnails
        $schema_thumb_path = file_uri_scheme($node->{$field_name}['und'][0]['uri']) . '://' . variable_get('video_thumbnail_path', 'videos/thumbnails') . '/' . $video_output->original_fid;
        file_prepare_directory($schema_thumb_path, FILE_CREATE_DIRECTORY);
        $no_of_thumbnails = variable_get('video_thumbnail_count', 5);
        $thumbnail_id = rand(1, $no_of_thumbnails);
        $filename = 'thumbnail-' . $video_output->original_fid . '_' . sprintf("%04d", $thumbnail_id) . '.png';
        libraries_load('awssdk');
        $key = variable_get('aws_key', '');
        $secret_key = variable_get('aws_secret_key', '');
        $bucket = variable_get('amazons3_bucket');
        $s3 = new AmazonS3($key, $secret_key);
        if ($s3->get_object_metadata($bucket, file_uri_target($schema_thumb_path) . '/' . $filename) && !file_exists(drupal_realpath($schema_thumb_path . '/' . $filename))) {
          $s3_get_object = $s3->get_object($bucket, file_uri_target($schema_thumb_path) . '/' . $filename, array('fileDownload' => drupal_realpath($schema_thumb_path . '/' . $filename)));
          $file = new stdClass();
          $file->uid = $node->uid;
          $file->status = 0;
          $file->filename = trim($filename);
          $file->uri = $schema_thumb_path . '/' . $filename;
          $file->filemime = file_get_mimetype($filename);
          $file->filesize = filesize(drupal_realpath($schema_thumb_path . '/' . $filename));
          $file->timestamp = time();
          $file = file_save($file);
          $node->{$field_name}['und'][0]['thumbnail'] = $file->fid;
          node_save($node);
        }
      }
    }
    elseif ($notification->output->state == "cancelled") {
      db_update('video_queue')->fields(array('status' => VIDEO_RENDERING_FAILED))->condition('vid', $video_output->vid, '=')->execute();
      echo "Cancelled!\n";
    }
    else {
      db_update('video_queue')->fields(array('status' => VIDEO_RENDERING_FAILED))->condition('vid', $video_output->vid, '=')->execute();
      watchdog('transcoder', t('Zencoder reports errors !errors.', array('!errors' => $notification->output->error_message)), NULL, WATCHDOG_ERROR);
      echo "Fail!\n";
    }
  }

}