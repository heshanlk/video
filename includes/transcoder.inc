<?php

/*
 * @file
 * Class file used to wrap the transcoder functions.
 * 
 */

class Transcoder {

  public function extractFrames($video) {
    // Save thiumnails to the vide_thumbnails table
    $thumbnails = array();
    $vid = $video['fid'];
    $factory = new TranscoderAbstractionAbstractFactory();
    $transcoder = $factory->getProduct();
    $transcoder->setInput($video);
    $thumbanils = $transcoder->extractFrames();
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

  public function executeConvertion($video) {
    global $user;
    // load the presets
    $this->changeStatus($video->vid, VIDEO_RENDERING_ACTIVE);
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
        $this->changeStatus($video->vid, VIDEO_RENDERING_COMPLETE);
        $output[] = $output_file;
      }
    }
//    $output = $this->transcoder->convert_video($video);
    // add files to file_manged table and add reference to the file_usage table
    if (is_array($output)) {
      $this->cleanConverted($video->vid);
      foreach ($output as $file) {
        $file->status = FILE_STATUS_PERMANENT;
        $file->uid = $user->uid;
        // if media module exists add type as an image
        if (module_exists('media'))
          $file->type = 'video';
        drupal_write_record('file_managed', $file);
        $output_vid = array('vid' => $video->vid, 'original_fid' => $video->fid, 'output_fid' => $file->fid);
        drupal_write_record('video_output', $output_vid);
      }
    }
    // if successfully converted the video then update the status to publish
    if ($output && variable_get('video_publish_on_complete', TRUE)) {
      // Update our node id to published.  We do not do a node_load as it causes editing problems when saving.
      db_update('node')
          ->fields(array(
            'status' => NODE_PUBLISHED))
          ->condition('nid', $video->nid, '=')
          ->execute();
      db_update('node_revision')
          ->fields(array(
            'status' => NODE_PUBLISHED))
          ->condition('nid', $video->nid, '=')
          ->execute();
    }
    return $output;
  }

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

  public function adminSettings() {
    //@todo use Drupal Form API status facility for this
    $form = array();
    $options = $this->_transcoders();
    $form['video_convertor'] = array(
      '#type' => 'radios',
      '#title' => t('Video transcoder'),
      '#default_value' => variable_get('video_convertor', 'video_ffmpeg'),
      '#options' => $options['radios'],
      '#description' => t('Selecting a video transcoder will help you convert videos and generate thumbnails. !list', array('!list' => theme('item_list', array('items' => $options['help'])))),
      '#prefix' => '<div id="transcoder-radios">',
      '#suffix' => '</div>',
    );
    $form = $form + $options['admin_settings'];
    return $form;
  }

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
//    //we need to move our video/thumbnail fieldsets to the bottom of our form as they are used for each trancoder
//    $autothumb = $form['admin_settings']['autothumb'];
//    $autoconv = $form['admin_settings']['autoconv'];
//    unset($form['admin_settings']['autothumb'], $form['admin_settings']['autoconv']);
//    if(!$this->transcoder->is_wsod())
//    $form['admin_settings']['autothumb'] = $autothumb;
//    $form['admin_settings']['autoconv'] = $autoconv;
    return $form;
  }

  public function deleteJob($video) {
    $video = (object) $video;
    if (!$video = $this->loadJob($video->fid))
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
    db_delete('video_queue')
        ->condition('fid', $video->fid)
        ->execute();
  }

  public function loadJob($fid) {
    $job = null;
    $job = db_query("SELECT f.*, vf.vid, vf.nid, vf.dimensions, vf.data, vf.status as video_status
      FROM {video_queue} vf LEFT JOIN {file_managed} f ON vf.fid = f.fid WHERE f.fid=vf.fid AND f.fid = :fid", array(':fid' => $fid))
        ->fetch();
    if (!empty($job))
      return $job;
    else
      return FALSE;
  }

  public function loadJobQueue() {
    $total_videos = variable_get('video_ffmpeg_instances', 5);
    $videos = array();
    $result = db_query_range('SELECT f.*, vf.vid, vf.nid, vf.dimensions, vf.status as video_status
      FROM {video_queue} vf LEFT JOIN {file_managed} f ON vf.fid = f.fid
      WHERE vf.status = :vstatus AND f.status = :fstatus ORDER BY f.timestamp', 0, $total_videos, array(':vstatus' => VIDEO_RENDERING_PENDING, ':fstatus' => FILE_STATUS_PERMANENT));
    foreach ($result as $row) {
      $videos[] = $row;
    }
    return $videos;
  }

  public function loadCompletedJob(&$video) {
    $file = $this->loadJob($video->fid);
    $data = unserialize($file->data);
    if (!empty($data))
      foreach ($data as $value) {
        $extension = pathinfo(drupal_realpath($value->uri), PATHINFO_EXTENSION);
        $video->files->{$extension}->filename = $value->filename;
        $video->files->{$extension}->filepath = $value->uri;
        $video->files->{$extension}->filemime = file_get_mimetype($value->uri);
        $video->files->{$extension}->url = file_create_url($value->uri);
        $video->files->{$extension}->extension = $extension;
        $video->player = strtolower($extension);
      }
    else
      return FALSE;
  }

  public function changeStatus($vid, $status) {
    db_update('video_queue')->fields(array(
          'status' => $status,))
        ->condition('vid', $vid, '=')
        ->execute();
  }

}