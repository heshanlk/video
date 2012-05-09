<?php
/**
 * @file
 * Class file used to wrap the transcoder helper functions.
 */

class Transcoder {
  /**
   * Extract frames from the video file. This helper function will interact with
   * only the database and it will save all the thumbnail file reference in to
   * the database.
   *
   * @return
   *   array of file objects, or false on failure
   */
  public function extractFrames(array $video, array $field) {
    global $user;

    $thumbnails = db_query('SELECT f.* FROM {file_managed} f INNER JOIN {video_thumbnails} tn ON tn.thumbnailfid = f.fid WHERE tn.videofid = :fid ORDER BY f.fid', array(':fid' => $video['fid']))->fetchAllAssoc('fid');
    if (!empty($thumbnails)) {
      return $thumbnails;
    }

    $scheme = !empty($field['settings']['uri_scheme_thumbnails']) ? $field['settings']['uri_scheme_thumbnails'] : 'public';
    $format = !empty($field['settings']['thumbnail_format']) ? $field['settings']['thumbnail_format'] : 'png';
    $factory = new TranscoderAbstractionAbstractFactory();
    $transcoder = $factory->getProduct();
    $transcoder->setInput($video);
    $thumbnails = $transcoder->extractFrames($scheme, $format);

    if ($thumbnails === FALSE) {
      return FALSE;
    }

    foreach ($thumbnails as $thumb) {
      // Determine whether there is an existing thumbnail
      $thumb->fid = $origfid = (int)db_query('SELECT fid FROM {file_managed} WHERE uri = :uri', array(':uri' => $thumb->uri))->fetchField(0);
      $thumb->uid = (int)$user->uid;
      // For the media module
      $thumb->type = 'image';
      file_save($thumb);

      if (empty($origfid)) {
        db_insert('video_thumbnails')->fields(array('videofid' => $video['fid'], 'thumbnailfid' => $thumb->fid))->execute();
      }
    }

    return $thumbnails;
  }

  /**
   * This helper function will help to execute video conversion job by loading
   * job from the database and once it completed saving its data in to the
   * database.
   *
   * @param $video
   * @return
   *   array of converted file objects
   */
  public function executeConversion(stdClass $video) {
    global $user;

    // load the presets
    $this->changeStatus($video->vid, VIDEO_RENDERING_ACTIVE);

    // update the video conversion start time
    db_update('video_queue')->fields(array('started' => time()))->condition('vid', $video->vid)->execute();

    $video_preset = new Preset();
    $presets = $video_preset->properties();
    // if no presets enabled then write an error log
    if (empty($presets)) {
      watchdog('transcoder', 'No preset enabled. Please !presets_message.', array('!presets_message' => l(t('enable or create new preset'), 'admin/config/media/video/presets')), WATCHDOG_ERROR, 'admin/config/media/video/presets');
      return FALSE;
    }

    // Find the scheme and thumbnail format for the converted videos
    $converted_scheme = file_uri_scheme($video->uri);
    $thumbnail_format = 'png';
    if (!empty($video->data['field_name'])) {
      $field = field_info_field($video->data['field_name']);
      if (!empty($field['settings']['uri_scheme_converted'])) {
        $converted_scheme = $field['settings']['uri_scheme_converted'];
      }
      if (!empty($field['settings']['thumbnail_format'])) {
        $thumbnail_format = $field['settings']['thumbnail_format'];
      }
    }

    $factory = new TranscoderAbstractionAbstractFactory();
    $transcoder = $factory->getProduct();
    $transcoder->setInput((array) $video);
    $output = array();
    $transcodingsuccess = TRUE;
    $output_directory = str_replace('original', 'converted', drupal_dirname($video->uri)) . '/' . $video->fid;
    $output_directory = $converted_scheme . '://' . file_uri_target($output_directory);

    if (!file_prepare_directory($output_directory, FILE_CREATE_DIRECTORY)) {
      watchdog('transcoder', 'Video conversion failed. Could not create the directory: %dir', array('%dir' => $output_directory), WATCHDOG_ERROR);
      return FALSE;
    }

    foreach ($presets as $name => $preset) {
      // override the widthXheight if enabled
      $preset['settings']['wxh'] = (variable_get('video_use_preset_wxh', FALSE)) ? $preset['settings']['wxh'] : $video->dimensions;
      $preset['settings']['thumbnails']['format'] = $thumbnail_format;

      // set transcoder options
      if (!$transcoder->setOptions($preset['settings'])) {
        // setOptions should write to the watchdog log.
        $transcodingsuccess = FALSE;
        break;
      }

      $output_name = file_munge_filename(str_replace(' ', '_', pathinfo($video->filename, PATHINFO_FILENAME) . ' ' . strtolower($name)) . '_' . time() . '.' . $preset['settings']['video_extension'], '');
      $transcoder->setOutput($output_directory, $output_name);
      if ($output_file = $transcoder->execute()) {
        $output[] = $output_file;
      }
      else {
        $transcodingsuccess = FALSE;
        break;
      }
    }

    // update the video conversion completed time
    db_update('video_queue')->fields(array('completed' => time()))->condition('vid', $video->vid)->execute();

    // add files to file_managed table and add reference to the file_usage table
    if (!empty($output) && $transcodingsuccess) {
      $this->cleanConverted($video->vid);
      foreach ($output as $file) {
        $file->status = FILE_STATUS_PERMANENT;
        $file->uid = $video->uid;
        // if media module exists add type as an image
        if (module_exists('media')) {
          $file->type = 'video';
        }
        drupal_write_record('file_managed', $file);
        file_usage_add($file, 'file', $video->entity_type, $video->entity_id);
        $output_vid = array(
          'vid' => $video->vid,
          'original_fid' => $video->fid,
          'output_fid' => $file->fid,
          'job_id' => !empty($file->jobid) ? $file->jobid : NULL,
        );
        drupal_write_record('video_output', $output_vid);

        // add duration to the video_queue table
        db_update('video_queue')->fields(array('duration' => round($file->duration)))->condition('vid', $video->vid)->execute();

        // Change the status if the file exists.
        // this happens for ffmpeg and other transcoders that transcoder
        // directly. For Zencoder, the status is changed later.
        if (file_exists($file->uri)) {
          $this->changeStatus($video->vid, VIDEO_RENDERING_COMPLETE);
        }
      }
    }

    if (!$transcodingsuccess) {
      $this->changeStatus($video->vid, VIDEO_RENDERING_FAILED);
      return FALSE;
    }

    return $output;
  }

  /**
   * This helper function clean the database records if exist for current job.
   */
  protected function cleanConverted($vid) {
    // @todo : if this impact on performance then think about other way
    $result = db_select('video_output', 'vo')
        ->fields('vo', array('original_fid', 'output_fid'))
        ->condition('vid', $vid)
        ->execute();
    foreach ($result as $file) {
      // delete from file_managed
      db_delete('file_managed')
          ->condition('fid', $file->output_fid)
          ->execute();
      // delete from file_usagle
      db_delete('file_usage')
          ->condition('fid', $file->output_fid)
          ->execute();
      // delete from video_output
      db_delete('video_output')
          ->condition('output_fid', $file->output_fid)
          ->execute();
    }
  }

  /**
   * Get admin settings forms from the transcoder classes and construct the admin
   * form will do here.
   */
  public function adminSettings() {
    // @todo use Drupal Form API status facility for this
    $form = array();
    $options = $this->_transcoders();
    $form['video_convertor'] = array(
      '#type' => 'radios',
      '#title' => t('Video transcoder'),
      '#default_value' => variable_get('video_convertor', 'TranscoderAbstractionFactoryZencoder'),
      '#options' => $options['radios'],
      '#description' => '<p>' . t('Select a video transcoder will help you convert videos and generate thumbnails.') . '</p>' . theme('item_list', array('items' => $options['help'])),
      '#prefix' => '<div id="transcoder-radios">',
      '#suffix' => '</div>',
    );
    $form = $form + $options['admin_settings'];
    return $form;
  }

  /**
   * Get all transcoders implemented to work with video  module and get its
   * values and names to display in admin settings form.
   */
  private function _transcoders() {
    $files = array();
    // Lets find our transcoder classes and build our radio options
    // We do this by scanning our transcoders folder
    $form = array('radios' => array(), 'help' => array(), 'admin_settings' => array());
    // check inside sub modules
    $modules = module_list();
    $files = array();
    foreach ($modules as $module) {
      $mobule_files = array();
      $module_path = drupal_get_path('module', $module) . '/transcoders';
      foreach (file_scan_directory($module_path, '/.*\.inc/') as $filekey => $file) {
        $file->module = $module;
        $mobule_files[] = $file;
      }
      $files = array_merge($files, $mobule_files);
    }
    foreach ($files as $file) {
      module_load_include('inc', $file->module, '/transcoders/' . $file->name);
      $focus = new $file->name;

      $errorMessage = '';
      if (!$focus->isAvailable($errorMessage)) {
        $form['help'][] = t('@name is unavailable: !errormessage', array('@name' => $focus->getName(), '!errormessage' => $errorMessage));
      }
      else {
        $form['radios'][$file->name] = check_plain($focus->getName());
        $form['admin_settings'] = $form['admin_settings'] + $focus->adminSettings();
      }
    }
    return $form;
  }

  /**
   * Change the status of the job, this will effect in database and use when
   * represent a job data.
   */
  public function changeStatus($vid, $status) {
    db_update('video_queue')
      ->fields(array('status' => $status))
      ->condition('vid', $vid)
      ->execute();
  }
}
