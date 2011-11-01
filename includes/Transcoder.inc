<?php

/*
 * @file
 * Class file used to wrap the transcoder helper functions.
 * 
 */

class Transcoder {

  /**
   * Extract frames from the video file. This helper function will intract with
   * only the database and it will save all the thumbanil file reference in to
   * the database.
   * @param type $video
   * @return type 
   */
  public function extractFrames($video) {
    // Save thiumnails to the vide_thumbnails table
    $thumbnails = array();
    $vid = $video['fid'];
    $factory = new TranscoderAbstractionAbstractFactory();
    $transcoder = $factory->getProduct();
    $transcoder->setInput($video);
    $thumbanils = $transcoder->extractFrames();
    if (!empty($thumbanils)) {
      foreach ($thumbanils as $file) {
        // if media module exists add type as an image
        if (module_exists('media'))
          $file->type = 'image';
        if (variable_get('video_thumb_save_all', FALSE))
          $file->status = FILE_STATUS_PERMANENT;
        $existing_file = file_load_multiple(array(), array('uri' => $file->uri));
        if ($existing_file) // check thumbnail file exists
          $file = (array) $existing_file;
        else { // create new file entries for thumbnails
          drupal_write_record('file_managed', $file);
          $file = file_load_multiple(array(), array('uri' => $file->uri));
        }
        if (!empty($file))
          $thumbnails = array_merge($file, $thumbnails);
      }
      $exists = db_query('SELECT 1 FROM {video_thumbnails} WHERE vid = :vid', array(':vid' => $vid))->fetchField();
      if ($exists == FALSE) { // returns TRUE is there is a record.
        $insertquery = db_insert('video_thumbnails') // Table name no longer needs {}
            ->fields(array(
              'vid' => $vid,
              'thumbnails' => serialize($thumbnails),
            ))
            ->execute();
      }
      else {
        $updatequery = db_update('video_thumbnails')
            ->fields(array(
              'thumbnails' => serialize($thumbnails),
            ))
            ->condition('vid', $vid)
            ->execute();
      }
      return unserialize(db_query('SELECT thumbnails FROM {video_thumbnails} WHERE vid = :vid', array(':vid' => $vid))->fetchField());
    }
    else
      return FALSE;
  }

  /**
   * This helper function will help to execute video conversion job by loading
   * job from the database and once it completed saving its data in to the
   * database.
   * @global type $user
   * @param type $video
   * @return type 
   */
  public function executeConvertion($video) {
    global $user;
    $output = array();
    // load the presets
    $this->changeStatus($video->vid, VIDEO_RENDERING_ACTIVE);
    // update the video conversion start time
    db_update('video_queue')->fields(array('started' => time()))->condition('vid', $video->vid, '=')->execute();

    $video_preset = new Preset();
    $presets = $video_preset->properties();
    // if no presets enabled then write an error log
    if (empty($presets)) {
      watchdog('transcoder', 'No preset enabled. Please !presets_message.', array('!presets_message' => l('enable or create new preset', 'admin/config/media/video/presets')), WATCHDOG_ERROR, 'admin/config/media/video/presets');
      return FALSE;
    }
    $factory = new TranscoderAbstractionAbstractFactory();
    $transcoder = $factory->getProduct();
    $transcoder->setInput((array) $video);
    foreach ($presets as $name => $preset) {
      // override the widthXheight if enabled
      $preset['settings']['wxh'] = (variable_get('video_use_preset_wxh', FALSE)) ? $preset['settings']['wxh'] : $video->dimensions;
      // set transcoder options
      $transcoder->setOptions($preset['settings']);
      //set up output path
      $output_directory = str_replace('original', 'converted', drupal_dirname($video->uri)) . '/' . $video->fid;
      if (!file_prepare_directory($output_directory, FILE_CREATE_DIRECTORY)) {
        watchdog('transcoder', 'Video conversion failed.  Could not create the directory: ' . $output_directory, array(), WATCHDOG_ERROR);
        return FALSE;
      }
      $output_name = file_munge_filename(str_replace(' ', '_', pathinfo($video->filename, PATHINFO_FILENAME) . ' ' . strtolower($name)) . '_' . time() . '.' . $preset['settings']['video_extension'], '');
      $transcoder->setOutput($output_directory, $output_name);
      if ($output_file = $transcoder->execute()) {
        // check for errors before set the output file
        if ($transcoder->checkErrors())
          return FALSE;
        $output[] = $output_file;
      }
    }
    // update the video conversion completed time
    db_update('video_queue')->fields(array('completed' => time()))->condition('vid', $video->vid, '=')->execute();
    // add files to file_manged table and add reference to the file_usage table
    if (isset($output) && !empty($output) && is_array($output)) {
      $this->cleanConverted($video->vid);
      foreach ($output as $file) {
        if (!isset($file->jobid))
          $file->jobid = $video->vid;
        $file->status = FILE_STATUS_PERMANENT;
        $file->uid = $video->uid;
        // if media module exists add type as an image
        if (module_exists('media'))
          $file->type = 'video';
        drupal_write_record('file_managed', $file);
        $output_vid = array(
          'vid' => $video->vid,
          'original_fid' => $video->fid,
          'output_fid' => $file->fid,
          'job_id' => $file->jobid
        );
        drupal_write_record('video_output', $output_vid);
        // add duration to the video_queue table
        db_update('video_queue')->fields(array('duration' => round($file->duration)))->condition('vid', $video->vid, '=')->execute();
        // change the status if files are exists
        if (file_exists(drupal_realpath($file->uri)))
          $this->changeStatus($video->vid, VIDEO_RENDERING_COMPLETE);
      }
      return $output;
    }
    else {
      return FALSE;
    }
  }

  /**
   * This helper function clean the database records if exist for current job.
   * @param type $vid 
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
   * @return type 
   */
  public function adminSettings() {
    //@todo use Drupal Form API status facility for this
    $form = array();
    $options = $this->_transcoders();
    $form['video_convertor'] = array(
      '#type' => 'radios',
      '#title' => t('Video transcoder'),
      '#default_value' => variable_get('video_convertor', 'TranscoderAbstractionFactoryZencoder'),
      '#options' => $options['radios'],
      '#description' => t('Selecting a video transcoder will help you convert videos and generate thumbnails. !list', array('!list' => theme('item_list', array('items' => $options['help'])))),
      '#prefix' => '<div id="transcoder-radios">',
      '#suffix' => '</div>',
    );
    $form = $form + $options['admin_settings'];
    return $form;
  }

  /**
   * Get all transcoders implemented to work with video  module and get its 
   * values and names to display in admin settings form.
   * @return type 
   */
  private function _transcoders() {
    // @TODO : think to change this to observer patteren
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
      $form['radios'][$file->name] = $focus->getName();
//      $form['help'][] = $focus->getHelp();
      $form['admin_settings'] = $form['admin_settings'] + $focus->adminSettings();
    }
    return $form;
  }

  /**
   * Change the status of the job, this will effect in database and use when 
   * represent a job data.
   * @param type $vid
   * @param type $status 
   */
  public function changeStatus($vid, $status) {
    db_update('video_queue')->fields(array(
          'status' => $status,))
        ->condition('vid', $vid, '=')
        ->execute();
  }

}