<?php

/*
 * @file
 * Class file used to wrap the transcoder functions.
 *
 * @todo need more commenting
 * - Add Metadata support
 * Add autoloading
 * # http://stackoverflow.com/questions/3343208/php-class-lazy-loading
 * # http://php.net/manual/en/language.oop5.autoload.php
 * 
 */

class video_transcoder {

  private $transcoder;

  public function __construct($transcoder = null) {
    $this->transcoder = $this->get_instance($transcoder);
  }

  /**
   *
   * @param <type> $transcoder 
   */
  private function get_instance($transcoder = null) {
    //get our configured transcoder.
    if (!isset($transcoder))
      $transcoder = variable_get('vid_convertor', 'video_ffmpeg');
//    module_load_include('inc', 'video', '/transcoders/' . $transcoder);
    if (!module_load_include('inc', 'video', '/transcoders/' . $transcoder)) {
      $modules = module_list();
      $files = array();
      foreach ($modules as $module) {
        $module_path = drupal_get_path('module', $module) . '/transcoders';
        $inc_files = file_scan_directory($module_path, '^.*\.inc$');
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
    } else {
      drupal_set_message(t('The transcoder is not configured properly.'), 'error');
    }
  }

  public function generate_thumbnails($video) {
    return $this->transcoder->generate_thumbnails($video);
  }

  public function convert_video(&$video) {
    module_load_include('inc', 'video', '/includes/preset');
    $video_preset = new video_preset();
    $presets = $video_preset->properties();
    $video->presets = $presets;
    $output = $this->transcoder->convert_video($video);
    // if successfully converted the video then update the status to publish
    if ($output)
    // Update our node id to published.  We do not do a node_load as it causes editing problems when saving.
      db_query("UPDATE {node} SET status=%d WHERE nid=%d", 1, $video->nid);

    // If they are using metadata.
    // @TODO : add meta data support
//    if (variable_get('video_metadata', FALSE)) {
//      module_load_include('inc', 'video', '/includes/metadata');
//      $metadata = new video_metadata;
//      $metadata->process($converted);
//    }
    return $output;
  }

  public function admin_settings() {
    $form = array();
    $options = $this->_transcoders();
    $form['vid_convertor'] = array(
      '#type' => 'radios',
      '#title' => t('Video transcoder'),
      '#default_value' => variable_get('vid_convertor', 'video_ffmpeg'),
      '#options' => $options['radios'],
      '#description' => t('Selecting a video transcoder will help you convert videos and generate thumbnails. !list', array('!list' => theme('item_list', $options['help']))),
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
    $path = drupal_get_path('module', 'video') . '/transcoders';
    $files = file_scan_directory($path, '^.*\.inc$');
    // check inside sub modules
    $modules = module_list();
    foreach ($modules as $module) {
      $mobule_files = array();
      $module_path = drupal_get_path('module', $module) . '/transcoders';
      $mobule_files = file_scan_directory($module_path, '^.*\.inc$');
      $files = array_merge($files, $mobule_files);
    }

    foreach ($files as $file) {
      if (!module_load_include('inc', 'video', '/transcoders/' . $file->name))
        require_once $file->filename;
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

  public function create_job($video) {
    return $this->transcoder->create_job($video);
  }

  public function update_job($video) {
    return $this->transcoder->update_job($video);
  }

  public function delete_job($video) {
    return $this->transcoder->delete_job($video);
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

}

interface transcoder_interface {

  public function create_job($video);

  public function update_job($video);

  public function delete_job($video);

  public function load_job($fid);

  public function load_job_queue();

  public function load_completed_job(&$video);

  public function change_status($vid, $status);

  public function generate_thumbnails($video);

  public function convert_video($video);

  public function get_name();

  public function get_value();

  public function get_help();

  public function admin_settings();

  public function admin_settings_validate($form, &$form_state);
}