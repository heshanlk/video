<?php

/*
 * @file
 * Class file used to wrap the transcoder functions.
 *
 * @todo need more commenting
 * - Add Metadata support
 * 
 */

class video_transcoder {

  private $transcoder;

  public function __construct($transcoder = null) {
    $this->transcoder = $this->get_instance($transcoder);
  }

  /**
   * Create the transcoder instance
   * @param <type> $transcoder 
   */
  private function get_instance($transcoder = null) {
    //get our configured transcoder.
    if (!isset($transcoder))
      $transcoder = variable_get('video_convertor', 'video_ffmpeg');
//    module_load_include('inc', 'video', '/transcoders/' . $transcoder);
    if (!module_load_include('inc', 'video', '/transcoders/' . $transcoder)) {
      $modules = module_list();
      $files = array();
      foreach ($modules as $module) {
        $module_path = drupal_get_path('module', $module) . '/transcoders';
        $inc_files = file_scan_directory($module_path, '/.*\.inc/');
        if (!empty($inc_files))
          $files[$module] = $inc_files;
      }
      // @TODO : add lazy load
      foreach ($files as $module => $_files) {
        foreach ($_files as $file) {
          if ($file->name == $transcoder)
            module_load_include('inc', $module, '/transcoders/' . $file->name);
        }
      }
    }

    if (class_exists($transcoder)) {
      $transcoder_instance = new $transcoder;
      $this->transcoder = $transcoder_instance;
      return $transcoder_instance;
    }
    else {
      drupal_set_message(t('The transcoder is not configured properly.'), 'error');
    }
  }

  public function get_thumbnails($video) {
    // Save thiumnails to the vide_thumbnails table
    $thumbnails = array();
    $vid = $video['fid'];
    $thumbs = $this->transcoder->get_thumbnails($video);
    foreach ($thumbs as $file) {
      // media module is altering file_manged table with type, so if we do not consider about is then we might get entity load issue.
      // #1015580
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

  public function convert_video(&$video) {
    global $user;
    // load the presets
    $video_preset = new video_preset();
    $presets = $video_preset->properties();
    // if no presets enabled then write an error log
    if (empty($presets)) {
      watchdog('transcoder', 'No preset enabled. Please !presets_message.', array('!presets_message' => l('enable or create new preset', 'admin/config/media/video/presets')), WATCHDOG_ERROR, 'admin/config/media/video/presets');
      return FALSE;
    }
    $video->presets = $presets;
    $output = $this->transcoder->convert_video($video);
    // add files to file_manged table and add reference to the file_usage table
    if (is_array($output)) {
      $this->clean_converted($video->vid);
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

  /**
   * Clean tables before convert videos.
   * @param type $vid
   */
  protected function clean_converted($vid) {
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
   * Administrator setting per transcoder
   * @return type 
   */
  public function admin_settings() {
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

  public function admin_settings_validate($form, &$form_state) {
    return $this->transcoder->admin_settings_validate($form, $form_state);
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
      $form['radios'][$focus->get_value()] = $focus->get_name();
      $form['help'][] = $focus->get_help();
      $form['admin_settings'] = $form['admin_settings'] + $focus->admin_settings();
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

  public function get_dimensions($video) {
    return $this->transcoder->get_dimensions($video);
  }

  public function create_job($video, $nid) {
    return $this->transcoder->create_job($video, $nid);
  }

  public function delete_job($video) {
    return $this->transcoder->delete_job($video);
  }

  public function change_status($vid, $status) {
    return $this->transcoder->change_status($vid, $status);
  }

  /**
   * Load a file based on the file id ($fid)
   *
   * @param $fid
   *   Integer of the file id to be loaded.
   */
  public function load_job($fid) {
    return $this->transcoder->load_job($fid);
  }

  public function load_job_queue() {
    return $this->transcoder->load_job_queue();
  }

  public function load_completed_job(&$video) {
    return $this->transcoder->load_completed_job($video);
  }

  public function get_codecs() {
    return $this->transcoder->get_codecs();
  }

  public function get_available_formats($method = false) {
    return $this->transcoder->get_available_formats($method);
  }

  public function get_version() {
    return $this->transcoder->get_version();
  }

  public function get_name() {
    return $this->transcoder->get_name();
  }

}

interface transcoder_interface {

  public function create_job($video, $nid);

  public function delete_job($video);

  public function load_job($fid);

  public function load_job_queue();

  public function load_completed_job(&$video);

  public function change_status($vid, $status);

  public function get_codecs();

  public function get_available_formats($method = false);

  public function get_version();

  public function get_thumbnails($video);

  public function convert_video($video);

  public function get_name();

  public function get_value();

  public function get_help();

  public function admin_settings();

  public function admin_settings_validate($form, &$form_state);
}