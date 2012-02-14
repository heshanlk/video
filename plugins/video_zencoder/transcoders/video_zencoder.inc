<?php
/*
 * @file
 * Transcoder class file to handle Zencoder settings and conversions.
 * @TODO
 * - Cancel transcode job when delete.
 * - Add number of cronjobs pertime  set to 5 now.
 * - Replace load_completed_job with load_job
 * - Add a metadata extractor class to extract width and height
 *
 */
module_load_include('inc', 'video', 'includes/transcoder');

class video_zencoder implements transcoder_interface {
  private $name = 'Zencoder';
  private $value = 'video_zencoder';

  public function generate_thumbnails($video) {
    global $user;
    
    $job = $this->load_job($video['fid']);
    $complete = $job && $job->video_status == VIDEO_RENDERING_COMPLETE;
    
    $final_thumb_path = video_thumb_path($video);
    $number_of_thumbs = $complete ? variable_get('video_thumbs', 5) : 1;
    
    $files = array();
    for ($i = 0; $i < $number_of_thumbs; $i++) {
      $filename = $video['fid'] .'_'. sprintf('%04d', $i) .'.png';
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

  public function convert_video($video) {
    // load the S3 lib
    module_load_include('lib.inc', 'video_s3');
    $s3 = new video_amazon_s3();

    // get the active jobs and check for the status
    if ($video->video_status == VIDEO_RENDERING_ACTIVE) {
      return FALSE;
    }

    // We need to check if this file id exists in our S3 table to avoid file not found error.
    if ($s3_file = $s3->verify($video->fid)) {
      // This is a s3 file, lets verify it has been pushed and if so lets push to Zencoder queue.
      if ($s3_file->status == VIDEO_S3_COMPLETE) {
        $video_s3 = $s3_file;
      }
    }
    else {
      watchdog('zencoder', 'You must activate the Video S3 module to work with Zencoder, file is not found in S3 table.', array(), WATCHDOG_ERROR);
      return FALSE;
    }

    // If we have a video lets go ahead and send it.
    if ($video_s3 != NULL) {
      $this->change_status($video->vid, VIDEO_RENDERING_ACTIVE);
      $video_s3->dimensions = $video->dimensions;
      $video_s3->presets = $video->presets;

      module_load_include('lib.inc', 'video_zencoder');
      $zc = new video_zencoder_api();

      if ($encoding_job = $zc->create($video_s3)) {
        // Update our table.
        $video->vid = $video->vid;

        //job id
        $video->jobid = $encoding_job->id;
        $outputs = new stdClass();
        foreach ($encoding_job->outputs as $output) {
          $outputs->{$output->id}->id = $output->id;
          $outputs->{$output->id}->label = $output->label;
          $outputs->{$output->id}->url = $output->url;
        }
        $video->data = serialize($outputs);
        
        // write output values to the table
        if ($this->update($video)) {
          watchdog('zencoder', 'Successfully created Zencoder trancoding job @jobid for video @video.', array('@jobid' => $video->jobid, '@video' => $video_s3->filename), WATCHDOG_INFO);
        }
      } else {
        watchdog('zencoder', 'Failed to queue file %file to Zencoder.', array('%file' => $s3_file->filepath), WATCHDOG_ERROR);
        $this->change_status($video->vid, VIDEO_RENDERING_FAILED);
      }
    } else {
      watchdog('zencoder', 'We did not find the file id @fid or it is still queued for S3 push.', array('@fid' => $video->fid), WATCHDOG_DEBUG);
    }

    return FALSE;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#get_name()
   */
  public function get_name() {
    return $this->name;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#get_value()
   */
  public function get_value() {
    return $this->value;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#get_help()
   */
  public function get_help() {
    return l(t('Zencoder'), 'http://zencoder.com/');
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#admin_settings()
   */
  public function admin_settings() {
    global $user;
    // check amazon s3 module is exists or not
    if (!module_exists('video_s3')) {
      drupal_set_message(t('You must enable Video Amazon S3 Module to enable this module.'), 'error');
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
        '#title' => t('Zencoder User'),
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
        '#title' => t('Agree Zencoder Terms and Conditions.'),
        '#default_value' => variable_get('agree_terms_zencoder', TRUE),
        '#description' => t('Read terms and conditions on !link.', array('!link' => l(t('Zencoder.com'), 'http://zencoder.com'))),
      );
    } else {
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

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#admin_settings_validate()
   */
  public function admin_settings_validate($form, &$form_state) {
    $zencoder_api = isset($form_state['values']['video_zencoder_api_key']) ? $form_state['values']['video_zencoder_api_key'] : NULL;
    if (!empty($zencoder_api) || $form_state['values']['vid_convertor'] != 'video_zencoder') {
      if (variable_get('vid_filesystem', 'drupal') != 'video_s3') {
        form_set_error('video_zencoder', t('You must enable Amazon S3 at !link.', array('!link' => l(t('the File System tab'), 'admin/settings/video/filesystem'))));
      }
      return;
    }

    $errors = false;
    // check terms and condition
    if ($form_state['values']['agree_terms_zencoder'] == 0) {
      $errors = true;
      form_set_error('agree_terms_zencoder', t('You must agree to the terms and conditions.'));
    }
    // check for email exists
    // Validate the e-mail address:
    if ($error = user_validate_mail($form_state['values']['zencoder_username'])) {
      $errors = true;
      form_set_error('zencoder_username', $error);
    }

    // get the API key from zencoder and save it to variable
    if (!$errors) {
      module_load_include('lib.inc', 'video_zencoder');
      $zc = new video_zencoder_api();

      $result = $zc->create_user($form_state['values']['zencoder_username']);
      if ($result !== true) {
        form_set_error('zencoder_username', $result);
      }
    }
  }

  /**
   * Return the dimensions of a video
   */
  public function get_dimensions($video) {
    // @TODO get object properties
    return NULL;
  }

  public function create_job($video) {
    return db_query("INSERT INTO {video_zencoder} (fid, status, dimensions) VALUES (%d, %d, '%s')", $video['fid'], VIDEO_RENDERING_PENDING, $video['dimensions']);
  }

  public function update_job($video) {
    if (!$this->load_job($video['fid']))
      return;
    //lets update our table to include the nid
    db_query("UPDATE {video_zencoder} SET nid=%d WHERE fid=%d", $video['nid'], $video['fid']);
  }

  public function delete_job($video) {
    if (!$this->load_job($video->fid))
      return;
    //lets get all our videos and unlink them
    $sql = db_query("SELECT vid FROM {video_zencoder} WHERE fid=%d", $video->fid);
    //we loop here as future development will include multiple video types (HTML 5)
    while ($row = db_fetch_object($sql)) {
      // @TODO : cancel the job to transcode
    }
    //now delete our rows.
    db_query('DELETE FROM {video_zencoder} WHERE fid = %d', $video->fid);
  }

  public function load_job($fid) {
    $job = db_fetch_object(db_query('SELECT f.*, vf.vid, vf.nid, vf.dimensions, vf.status as video_status FROM {video_zencoder} vf INNER JOIN {files} f ON vf.fid = f.fid WHERE f.fid = %d', $fid));
    if (empty($job)) {
      return FALSE;
    }
    return $job;
  }

  public function load_job_queue() {
    // load jobs with status as pending and active both
    $total_videos = variable_get('video_ffmpeg_instances', 5);
    $videos = array();
    $result = db_query_range('SELECT f.*, vf.vid, vf.nid, vf.dimensions, vf.status as video_status FROM {video_zencoder} vf LEFT JOIN {files} f ON vf.fid = f.fid WHERE vf.status = %d AND f.status = %d ORDER BY f.timestamp',
            VIDEO_RENDERING_PENDING, FILE_STATUS_PERMANENT, 0, $total_videos);

    while ($row = db_fetch_object($result)) {
      $videos[] = $row;
    }
    return $videos;
  }

  /**
   * @todo : replace with the load job method
   * @param <type> $video
   * @return <type>
   */
  public function load_completed_job(&$video) {
    $video_row = db_fetch_object(db_query('SELECT data FROM {video_zencoder} WHERE fid = %d', $video->fid));
    $data = unserialize($video_row->data);

    if (empty($data))
      return $video;

    foreach ($data as $value) {
      $path = parse_url($value->url, PHP_URL_PATH);
      $extension = pathinfo($path, PATHINFO_EXTENSION);
      $video->files->{$extension} = new stdClass();
      $video->files->{$extension}->filename = pathinfo($path, PATHINFO_FILENAME) .'.'. $extension;
      $video->files->{$extension}->filepath = substr($path, 1); // Remove the leading slash
      $video->files->{$extension}->url = $value->url; // Authentication tokens are added by video_s3->load()
      $video->files->{$extension}->extension = $extension;
      $video->files->{$extension}->filemime = file_get_mimetype($value->url);
      $video->player = strtolower($extension);
    }

    return $video;
  }

  /**
   * Change the status of the file.
   *
   * @param (int) $vid
   * @param (int) $status
   */
  public function change_status($vid, $status) {
    db_query('UPDATE {video_zencoder} SET status = %d WHERE vid = %d ', $status, $vid);
  }

  /*
   * Updates the database after a successful transfer to amazon.
   */

  private function update($video) {
    return db_query("UPDATE {video_zencoder} SET jobid = %d, completed=%d, data='%s' WHERE vid=%d",
            $video->jobid, time(), $video->data, $video->vid);
  }
}
