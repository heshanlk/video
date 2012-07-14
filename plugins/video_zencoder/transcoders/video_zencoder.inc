<?php
/**
 * @file
 * Transcoder class file to handle Zencoder settings and conversions.
 */

// Register this file as a CTools plugin
$plugin = array();

module_load_include('lib.inc', 'video');

class video_zencoder implements video_transcoder {
  private $name = 'Zencoder';
  private $value = 'video_zencoder';

  public function generate_thumbnails(stdClass $video) {
    global $user;

    $job = $this->load_job($video->fid);
    $complete = $job && $job->status == VIDEO_RENDERING_COMPLETE;

    $final_thumb_path = video_thumb_path($video);
    $number_of_thumbs = $complete ? variable_get('video_thumbs', 5) : 1;

    $files = array();
    for ($i = 0; $i < $number_of_thumbs; $i++) {
      $filename = $video->fid .'_'. sprintf('%04d', $i) .'.png';
      $thumbfile = $final_thumb_path .'/'. $filename;

      if (!$complete) {
        $default = drupal_get_path('module', 'video') .'/images/no-thumb.png';
        $thumbfile = video_thumb_path(NULL, FALSE) .'/no-thumb.png';

        // The default file is shared between videos
        if (!is_file($thumbfile)) {
          file_copy($default, $thumbfile, FILE_EXISTS_REPLACE);
        }
      }
      elseif (!is_file($thumbfile)) {
        break;
      }

      // Begin building the file object.
      $file = new stdClass();
      $file->uid = $user->uid;
      $file->status = FILE_STATUS_TEMPORARY;
      $file->filename = $filename;
      $file->filepath = $thumbfile;
      $file->filemime = 'image/png';
      $file->filesize = filesize($thumbfile);
      $file->timestamp = time();
      $files[] = $file;
    }

    return $files;
  }

  public function video_converted_extension() {
    return variable_get('video_zencoder_ext', 'flv');
  }

  public function convert_video(stdClass $video) {
    // get the active jobs and check for the status
    if ($video->status == VIDEO_RENDERING_ACTIVE) {
      return FALSE;
    }

    if (empty($video->bucket)) {
      watchdog('zencoder', 'You must activate the Video Amazon S3 module to work with Zencoder, file @fid does not appear to be uploaded to an Amazon S3 bucket.', array('@fid' => $video->fid), WATCHDOG_ERROR);
      return FALSE;
    }

    $this->change_status($video, VIDEO_RENDERING_ACTIVE);

    module_load_include('lib.inc', 'video_zencoder');
    $zc = new video_zencoder_api();

    if ($encoding_job = $zc->create($video)) {
      $video->jobid = $encoding_job->id;
      $video->data = new stdClass();

      foreach ($encoding_job->outputs as $output) {
        $video->data->{$output->id} = new stdClass();
        $video->data->{$output->id}->id = $output->id;
        $video->data->{$output->id}->label = $output->label;
        $video->data->{$output->id}->url = $output->url;
      }

      db_query('UPDATE {video_zencoder} SET jobid = %d, completed=%d, data="%s" WHERE vid=%d', $video->jobid, time(), serialize($video->data), $video->vid);
      watchdog('zencoder', 'Successfully created Zencoder trancoding job @jobid for video @video.', array('@jobid' => $video->jobid, '@video' => $video->filename), WATCHDOG_INFO);
      return TRUE;
    }

    watchdog('zencoder', 'Failed to queue file %file to Zencoder.', array('%file' => $video->filepath), WATCHDOG_ERROR);
    $this->change_status($video, VIDEO_RENDERING_FAILED);
    return FALSE;
  }

  public function get_name() {
    return $this->name;
  }

  public function get_value() {
    return $this->value;
  }

  public function get_help() {
    return l(t('Zencoder'), 'http://zencoder.com/');
  }

  public function admin_settings(&$form_state) {
    global $user;
    // check amazon s3 module is exists or not
    if (!module_exists('video_s3')) {
      drupal_set_message(t('You must enable Video Amazon S3 module to enable this module.'), 'error');
    }

    $form = array();
    $form['video_zencoder_start'] = array(
      '#type' => 'markup',
      '#value' => '<div id="video_zencoder">',
    );
    $zencoder_api = variable_get('video_zencoder_api_key', NULL);
    if (empty($zencoder_api)) {
      $form['zencoder_user'] = array(
        '#type' => 'fieldset',
        '#title' => t('Zencoder user'),
        '#collapsible' => FALSE,
        '#collapsed' => FALSE,
        '#description' => t('Save configurations to create your !link account to transcode and manage your videos using Zencoder API. Once you save your configuration this will automatically create an account on Zencoder.com and password and all ther other relevant details will be emailed to you.', array('!link' => l(t('Zencoder.com'), 'http://zencoder.com')))
      );
      $form['zencoder_user']['zencoder_username'] = array(
        '#type' => 'textfield',
        '#title' => t('Your email address'),
        '#default_value' => variable_get('zencoder_username', variable_get('site_mail', '')),
        '#size' => 50,
        '#description' => t('Make sure the email is accurate, since we will send all the password details to manage transcoding online and API key details to this.<br/>If you already have a Zencoder account, enter the e-mail address that is associated with your Zencoder account.')
      );

      $form['zencoder_user']['agree_terms_zencoder'] = array(
        '#type' => 'checkbox',
        '#title' => t('Agree to Zencoder terms and conditions.'),
        '#default_value' => variable_get('agree_terms_zencoder', FALSE),
        '#description' => t('Read terms and conditions on !link.', array('!link' => l(t('Zencoder.com'), 'http://zencoder.com'))),
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
        '#title' => t('Zencoder API key'),
        '#default_value' => variable_get('video_zencoder_api_key', NULL),
        '#description' => t('Zencoder API Key. Click <b>Reset to default</b> button to add a new account.')
      );
      $form['zencoder_info']['video_thumbs'] = array(
        '#type' => 'textfield',
        '#title' => t('Number of thumbnails'),
        '#description' => t('Number of thumbnails to display from video.'),
        '#default_value' => variable_get('video_thumbs', 5),
        '#size' => 5
      );
      $form['zencoder_info']['video_thumbs_size'] = array(
        '#type' => 'textfield',
        '#title' => t('Dimension of thumbnails'),
        '#description' => t('Size of thumbnails to extract from video.'),
        '#default_value' => variable_get('video_thumbs_size', '160x120'),
        '#size' => 10
      );
      global $base_url;
      $form['zencoder_info']['video_zencoder_postback'] = array(
        '#type' => 'textfield',
        '#title' => t('Postback URL for Zencoder'),
        '#description' => t('Important: Do not change this if you do not know what your doing.<br/>This postback URL will receive video data when they are completed.'),
        '#default_value' => variable_get('video_zencoder_postback', url('postback/jobs', array('absolute' => TRUE))),
      );
    }
    $form['video_zencoder_end'] = array(
      '#type' => 'markup',
      '#value' => '</div>',
    );
    return $form;
  }

  public function admin_settings_validate($form, &$form_state) {
    $zencoder_api = isset($form_state['values']['video_zencoder_api_key']) ? $form_state['values']['video_zencoder_api_key'] : NULL;
    if (!empty($zencoder_api) || $form_state['values']['vid_convertor'] != 'video_zencoder') {
      if (variable_get('vid_filesystem', 'drupal') != 'video_s3') {
        form_set_error('video_zencoder', t('You must enable Amazon S3 at !link.', array('!link' => l(t('the File System tab'), 'admin/settings/video/filesystem'))));
      }
      return;
    }

    $errors = FALSE;
    // check terms and condition
    if ($form_state['values']['agree_terms_zencoder'] == 0) {
      $errors = TRUE;
      form_set_error('agree_terms_zencoder', t('You must agree to the terms and conditions.'));
    }
    // check for email exists
    // Validate the e-mail address:
    if ($error = user_validate_mail($form_state['values']['zencoder_username'])) {
      $errors = TRUE;
      form_set_error('zencoder_username', $error);
    }

    // get the API key from zencoder and save it to variable
    if (!$errors) {
      module_load_include('lib.inc', 'video_zencoder');
      $zc = new video_zencoder_api();

      $result = $zc->create_user($form_state['values']['zencoder_username']);
      if ($result !== TRUE) {
        form_set_error('zencoder_username', $result);
      }
    }
  }

  public function preset_settings(&$form_state, video_preset $preset) {
    $settings = $preset->getSettings();

    $form = array();

    $form['help'] = array(
      '#type' => 'item',
      '#title' => t('Note'),
      '#value' => t('Take a look at the <a href="@zencoder-api">Zencoder API documentation</a> for more information about these configuration options.
It is usually not necessary to change these settings as Zencoder has sensible defaults for each output format.', array('@zencoder-api' => 'https://app.zencoder.com/docs/api/encoding')),
    );

    $onetofive = array('' => '') + array_combine(range(1, 5), range(1, 5));
    $truefalse = array('' => '', 'true' => t('Yes'), 'false' => t('No'));
    $videocodecs = array('', 'h264', 'mpeg4', 'theora', 'vp6', 'vp8', 'wmv');
    $audiocodecs = array('', 'aac', 'amr', 'mp3', 'vorbis', 'wma');
    $audiochannels = array('', 1, 2);

    $form['quality'] = array(
      '#type' => 'select',
      '#title' => t('Video quality'),
      '#default_value' => isset($settings['quality']) ? $settings['quality'] : '',
      '#options' => $onetofive,
    );

    $form['audio_quality'] = array(
      '#type' => 'select',
      '#title' => t('Audio quality'),
      '#default_value' => isset($settings['audio_quality']) ? $settings['audio_quality'] : '',
      '#options' => $onetofive,
    );

    $form['speed'] = array(
      '#type' => 'select',
      '#title' => t('Transcoding speed'),
      '#default_value' => isset($settings['speed']) ? $settings['speed'] : '',
      '#options' => $onetofive,
    );

    $form['upscale'] = array(
      '#type' => 'select',
      '#title' => t('Allow upscaling'),
      '#default_value' => isset($settings['upscale']) ? $settings['upscale'] : '',
      '#options' => $truefalse,
    );

    $form['frame_rate'] = array(
      '#type' => 'textfield',
      '#title' => t('Frame rate'),
      '#default_value' => isset($settings['frame_rate']) ? $settings['frame_rate'] : '',
      '#maxlength' => 15,
      '#size' => 15,
    );

    $form['max_frame_rate'] = array(
      '#type' => 'textfield',
      '#title' => t('Maximum frame rate'),
      '#default_value' => isset($settings['max_frame_rate']) ? $settings['max_frame_rate'] : '',
      '#maxlength' => 15,
      '#size' => 15,
    );

    $form['video_codec'] = array(
      '#type' => 'select',
      '#title' => t('Video codec'),
      '#default_value' => isset($settings['video_codec']) ? $settings['video_codec'] : '',
      '#options' => array_combine($videocodecs, $videocodecs),
    );

    $form['audio_codec'] = array(
      '#type' => 'select',
      '#title' => t('Audio codec'),
      '#default_value' => isset($settings['audio_codec']) ? $settings['audio_codec'] : '',
      '#options' => array_combine($audiocodecs, $audiocodecs),
    );

    $form['video_bitrate'] = array(
      '#type' => 'textfield',
      '#title' => t('Video bit rate'),
      '#default_value' => isset($settings['video_bitrate']) ? $settings['video_bitrate'] : '',
      '#field_suffix' => t('kbps'),
      '#maxlength' => 15,
      '#size' => 15,
    );

    $form['audio_bitrate'] = array(
      '#type' => 'textfield',
      '#title' => t('Audio bit rate'),
      '#default_value' => isset($settings['audio_bitrate']) ? $settings['audio_bitrate'] : '',
      '#field_suffix' => t('kbps'),
      '#maxlength' => 15,
      '#size' => 15,
    );

    $form['audio_sample_rate'] = array(
      '#type' => 'textfield',
      '#title' => t('Audio sample rate'),
      '#default_value' => isset($settings['audio_sample_rate']) ? $settings['audio_sample_rate'] : '',
      '#field_suffix' => t('Hz'),
      '#maxlength' => 15,
      '#size' => 15,
    );

    $form['audio_channels'] = array(
      '#type' => 'select',
      '#title' => t('Number of audio channels'),
      '#default_value' => isset($settings['audio_channels']) ? $settings['audio_channels'] : '',
      '#options' => array_combine($audiochannels, $audiochannels),
    );

    $as = '';
    if (isset($settings['additional_settings']) && is_array($settings['additional_settings'])) {
      $as = $this->settingsArrayToString($settings['additional_settings']);
    }

    $form['additional_settings'] = array(
      '#type' => 'textarea',
      '#title' => t('Additional settings'),
      '#default_value' => $as,
      '#rows' => 5,
      '#description' => '<p>'. t('Enter any other <a href="@zencoder-api">Zencoder encoding settings</a> in this text area. Enter each setting on a separate line and separate the setting name (case sensitive) and the value with a colon. To create a nested structure, use two spaces. Malformed settings will be removed.', array('@zencoder-api' => 'https://app.zencoder.com/docs/api/encoding')) . '</p>' .
      '<p>'. t('Example:') .'</p>'.
      '<blockquote><pre>clip_length: 00:00:10.0
watermarks:
  url: http://example.com/watermark.gif
  y: 10
  x: 10</pre></blockquote>',
      '#wysiwyg' => FALSE,
    );

    return $form;
  }

  public function preset_settings_validate($form, &$form_state) {
    $v =& $form_state['values'];

    foreach (array('frame_rate', 'max_frame_rate') as $k) {
      if (!empty($v[$k]) && !preg_match('#^\d+(\.\d+)?$#', $v[$k])) {
        form_error($form[$k], t('The value of the %setting setting must be a floating point number.', array('%setting' => $form[$k]['#title'])));
      }
    }

    foreach (array('video_bitrate', 'audio_bitrate', 'audio_sample_rate', 'audio_channels') as $k) {
      if (!empty($v[$k])) {
        if (!preg_match('#^\d+$#', $v[$k])) {
          form_error($form[$k], t('The value of the %setting setting must be a positive integer.', array('%setting' => $form[$k]['#title'])));
        }
        else {
          $v[$k] = intval($v[$k]);
        }
      }
    }

    if (!empty($v['additional_settings'])) {
      // Remove unnecessary new lines
      $asstr = trim($v['additional_settings']);
      $asstr = preg_replace('#[\r\n]+#', "\n", $asstr);

      // Parse the lines into a structure
      $lines = explode("\n", $asstr);
      $as = array();
      $stack = array();
      array_unshift($stack, &$as);

      foreach ($lines as $lineno => $line) {
        $parts = explode(':', $line, 2);
        if (count($parts) != 2) {
          continue;
        }

        $key = rtrim($parts[0]);
        $value = trim($parts[1]);

        if ($value != '') {
          // Check the spaces of the key
          $level = strspn($key, ' ') / 2;
          if ($level != abs($level)) {
            continue;
          }

          // Check if we should go one level lower
          if ($level == count($stack) - 2) {
            array_shift($stack);
          }

          if ($level == count($stack) - 1) {
            // Add a value to the current stack top
            $stack[0][ltrim($key)] = $value;
          }
        }
        else {
          $stack[0][$key] = array();
          array_unshift($stack, &$stack[0][$key]);
        }

        $lines[$lineno] = $key .': '. $value;
      }

      $v['additional_settings'] = $as;
    }
  }

  private function settingsArrayToString(array $settings, $level = 0) {
    $output = '';

    foreach ($settings as $k => $v) {
      $output .= str_repeat('  ', $level);
      $output .= $k .':';

      if (is_array($v)) {
        $output .= "\n";
        $output .= $this->settingsArrayToString($v, $level + 1);
      }
      else {
        $output .= ' '. $v ."\n";
      }
    }

    return $output;
  }

  /**
   * Return the dimensions of a video
   */
  public function get_dimensions($filepath) {
    // @TODO get object properties
    return NULL;
  }

  public function create_job(stdClass $video) {
    $video->status = VIDEO_RENDERING_PENDING;
    return db_query('INSERT INTO {video_zencoder} (fid, status, dimensions) VALUES (%d, %d, "%s")', $video->fid, VIDEO_RENDERING_PENDING, $video->dimensions);
  }

  public function update_job(stdClass $video) {
    if (!$this->load_job($video->fid)) {
      return;
    }

    // Lets update our table to include the nid
    db_query('UPDATE {video_zencoder} SET nid = %d WHERE fid=%d', $video->nid, $video->fid);
  }

  public function delete_job(stdClass $video) {
    // Cancel the job
    if ($video->status == VIDEO_RENDERING_ACTIVE && !empty($video->jobid)) {
      module_load_include('lib.inc', 'video_zencoder');
      $zc = new video_zencoder_api();
      $zc->cancel_job($video);
    }

    db_query('DELETE FROM {video_zencoder} WHERE fid = %d', $video->fid);
  }

  public function load_job($fid) {
    $job = db_fetch_object(db_query('SELECT f.fid, f.filename, f.filepath, f.filemime, f.filesize, zc.vid, zc.nid, zc.dimensions, zc.status, zc.data, zc.jobid FROM {video_zencoder} zc INNER JOIN {files} f ON zc.fid = f.fid WHERE f.fid = %d', $fid));
    if (!empty($job)) {
      $job->data = unserialize($job->data);
      return $job;
    }

    return FALSE;
  }

  public function load_job_queue($num) {
    $videos = array();

    $result = db_query_range('SELECT f.fid, f.filename, f.filepath, f.filemime, f.filesize, zc.vid, zc.nid, zc.dimensions, zc.status, zc.data, zc.jobid FROM {video_zencoder} zc LEFT JOIN {files} f ON zc.fid = f.fid WHERE zc.status = %d AND f.status = %d ORDER BY f.timestamp', VIDEO_RENDERING_PENDING, FILE_STATUS_PERMANENT, 0, $num);
    while ($row = db_fetch_object($result)) {
      $row->data = unserialize($row->data);
      $videos[] = $row;
    }

    return $videos;
  }

  /**
   * @todo : replace with the load job method
   */
  public function load_completed_job(stdClass $video) {
    $result = db_fetch_object(db_query('SELECT data FROM {video_zencoder} WHERE fid = %d', $video->fid));
    foreach (unserialize($result->data) as $value) {
      $path = parse_url($value->url, PHP_URL_PATH);
      $extension = pathinfo($path, PATHINFO_EXTENSION);
      $video->files->{$extension} = new stdClass();
      $video->files->{$extension}->filename = pathinfo($path, PATHINFO_FILENAME) .'.'. $extension;
      $video->files->{$extension}->filepath = drupal_substr($path, 1); // Remove the leading slash
      $video->files->{$extension}->url = $value->url; // Authentication tokens are added by video_s3->load()
      $video->files->{$extension}->extension = $extension;
      $video->files->{$extension}->filemime = file_get_mimetype($value->url);
      $video->player = drupal_strtolower($extension);
    }
    return $video;
  }

  /**
   * Change the status of the file.
   */
  public function change_status(stdClass $video, $status) {
    $video->status = $status;
    db_query('UPDATE {video_zencoder} SET status = %d WHERE vid = %d ', $status, $video->vid);
  }

  public function requirements() {
    return array();
  }

  /**
   * Implementation of get_original_path_by_converted_path().
   *
   * All Zencoder files are stored on S3, so video_file_download will never
   * have to call this method.
   */
  public function get_original_path_by_converted_path($filepath) {
    return NULL;
  }
}
