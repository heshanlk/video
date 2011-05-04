<?php

/*
 * @file
 * Transcoder class file to handle ffmpeg_wrapper settings and conversions.
 * @TODO
 * - Cancel transcode job when delete.
 * - Add number of cronjobs pertime  set to 5 now.
 * - Replace load_completed_job with load_job
 * - Add a metadata extractor class to extract width and height
 *
 */

class video_zencoder implements transcoder_interface {

  private $name = 'Zencoder';
  private $value = 'video_zencoder';

  public function generate_thumbnails($video) {
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

  public function convert_video($video) {
    // get the active jobs and check for the status
    if ($video->video_status == VIDEO_RENDERING_ACTIVE) {
      return;
    }
    // This will update our current video status to active.
    $this->change_status($video->vid, VIDEO_RENDERING_ACTIVE);
    module_load_include('inc', 'video_zencoder', '/includes/zencoder');
    $zc = new video_zencoder_api;
    if ($encoding_job = $zc->create($video)) {
      // Update our table.
      $video->vid = $video->vid;
      //job id
      $video->jobid = $encoding_job->id;
      $outputs = new stdClass();
//        print_r($encoding_job->outputs);
      foreach ($encoding_job->outputs as $output) {
        $outputs->{$output->id}->id = $output->id;
        $outputs->{$output->id}->label = $output->label;
        $outputs->{$output->id}->url = $output->url;
        $outputs->{$output->id}->state = $output->state;
        $outputs->{$output->id}->error_message = $output->error_message;
        $outputs->{$output->id}->error_link = $output->error_link;
      }
      $video->data = serialize($outputs);
      // write output values to the table
      if ($this->update($video)) {
        watchdog('zencoder', t('Successfully created trancoding job on !jobid.', array('!jobid' => $video->jobid)), array(), WATCHDOG_INFO);
      }
    }
    else {
      watchdog('zencoder', 'Failed to queus our file to Zencoder.', array(), WATCHDOG_ERROR);
      $this->change_status($video->vid, VIDEO_RENDERING_FAILED);
      return FALSE;
    }
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
    $help = l(t('Zencoder'), 'http://zencoder.com/');
    return $help;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#admin_settings()
   */
  public function admin_settings() {
    global $user;
    // check amazon s3 module is exists or not
    if (!module_exists('media_amazon'))
      drupal_set_message(t('You must enable Video Amazon S3 Module to enable this module.'), 'error');

    $form = array();
    $form['video_zencoder_start'] = array(
      '#type' => 'markup',
      '#markup' => '<div id="video_zencoder">',
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
      $form['zencoder_info']['video_thumbs'] = array(
        '#type' => 'textfield',
        '#title' => t('Number of thumbnails'),
        '#description' => t('Number of thumbnails to display from video.'),
        '#default_value' => variable_get('video_thumbs', 5),
        '#size' => 5
      );
      $form['zencoder_info']['video_thumbs_size'] = array(
        '#type' => 'textfield',
        '#title' => t('Dimention of thumbnails'),
        '#description' => t('Size of thumbnails to extract from video.'),
        '#default_value' => variable_get('video_thumbs_size', '160x120'),
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

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#admin_settings_validate()
   */
  public function admin_settings_validate($form, &$form_state) {
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

  /**
   * Return the dimensions of a video
   */
  public function get_dimensions($video) {
    // @TODO get object properties
    return;
  }

  // Returns available codecs
  public function get_codecs() {
    $codecs = array(
      'encode' => array(
        'video' => array(
          'h264' => 'H.264 (default)',
          'vp8' => 'VP8',
          'theora' => 'Theora',
          'vp6' => 'VP6',
          'mpeg4' => 'MPEG-4',
          'wmv' => 'WMV'
        ),
        'audio' => array(
          'aac' => 'AAC (default for most cases)',
          'mp3' => 'MP3',
          'vorbis' => 'Vorbis (default for VP8 and Theora)',
          'wma' => 'WMA'
        )
      ),
      'decoding' => array()
    );
    return $codecs;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#create_job()
   */
  public function create_job($video, $nid) {
    return db_insert('video_zencoder')
        ->fields(array(
          'fid' => $video['fid'],
          'nid' => $nid,
          'status' => VIDEO_RENDERING_PENDING,
          'dimensions' => $video['dimensions'],
        ))
        ->execute();
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#delete_job()
   */
  public function delete_job($video) {
    $video = (object) $video;
    if (!$video = $this->load_job($video->fid))
      return;
    // converted output values
    $converted = unserialize($video->data);
    if (!empty($converted)) {
      foreach ($converted as $file) {
        if (file_exists(drupal_realpath($file->uri)))
          @drupal_unlink($file->uri);
      }
    }
    //now delete our rows.
    db_delete('video_zencoder')
        ->condition('fid', $video->fid)
        ->execute();
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#load_job()
   */
  public function load_job($fid) {
    $job = null;
    $job = db_query("SELECT f.*, vf.vid, vf.nid, vf.dimensions, vf.data, vf.status as video_status
      FROM {video_zencoder} vf LEFT JOIN {file_managed} f ON vf.fid = f.fid WHERE f.fid=vf.fid AND f.fid = :fid", array(':fid' => $fid))
        ->fetch();
    if (!empty($job))
      return $job;
    else
      return FALSE;
  }

  /**
   * Load job by jobid
   * @return type 
   */
  public function load_job_by_jobid($jobid) {
    $job = db_query("SELECT f.*, vf.vid, vf.nid, vf.dimensions, vf.data, vf.status as video_status
      FROM {video_zencoder} vf LEFT JOIN {file_managed} f ON vf.fid = f.fid WHERE f.fid=vf.fid AND f.jobid = :jobid", array(':jobid' => $jobid))
        ->fetch();
    if (!empty($job))
      return $job;
    else
      return FALSE;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#load_job_queue()
   */
  public function load_job_queue() {
    $total_videos = variable_get('video_ffmpeg_instances', 5);
    $videos = array();
    $result = db_query_range('SELECT f.*, vf.vid, vf.nid, vf.dimensions, vf.status as video_status
      FROM {video_zencoder} vf LEFT JOIN {file_managed} f ON vf.fid = f.fid
      WHERE vf.status = :vstatus AND f.status = :fstatus ORDER BY f.timestamp', 0, $total_videos, array(':vstatus' => VIDEO_RENDERING_PENDING, ':fstatus' => FILE_STATUS_PERMANENT));
    foreach ($result as $row) {
      $videos[] = $row;
    }
    return $videos;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#load_completed_job()
   */
  public function load_completed_job(&$video) {
    $file = $this->load_job($video->fid);
    $data = unserialize($file->data);
    if (!empty($data))
      foreach ($data as $value) {
        $extension = pathinfo(drupal_realpath($value->uri), PATHINFO_EXTENSION);
        $video->files->{$extension}->filename = $value->filename;
        $video->files->{$extension}->filepath = $value->uri;
        $video->files->{$extension}->filemime = file_get_mimetype($value->uri);
        $video->files->{$extension}->url = file_create_url($value->uri);
        $video->files->{$extension}->uri = $value->uri;
        $video->files->{$extension}->extension = $extension;
        $video->player = strtolower($extension);
      }
    else
      return FALSE;
  }

  /**
   * Change the status of the file.
   *
   * @param (int) $vid
   * @param (int) $status
   */
  public function change_status($vid, $status) {
    db_update('video_zencoder')->fields(array(
          'status' => $status,))
        ->condition('vid', $vid, '=')
        ->execute();
  }

  /*
   * Updates the database after a successful transfer to amazon.
   */

  private function update($video) {
    return db_update('video_zencoder')->fields(array(
          'jobid' => $video->jobid,
          'completed' => time(),
          'data' => $video->data))
        ->condition('vid', $vid, '=')
        ->execute();
  }

}