<?php
//$Id$
/*
 * @file
 * Class file used to wrap the transcoder functions.
 * 
 * @todo need more commenting
 */

class video_transcoder {
  private $transcoder;
  
  public function __construct() {
    $transcoder = variable_get('vid_convertor', 'video_ffmpeg');
    module_load_include('inc', 'video', '/transcoders/' . $transcoder);
    if(class_exists($transcoder)) {
      $this->transcoder = new $transcoder;
    }
    else {
      drupal_set_message(t('Transcoder not configured properly'), 'error');
    }
  }
  
  public function admin_settings() {
    drupal_add_js(drupal_get_path('module', 'video'). '/js/admin.js');
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
  
  public function generate_thumbnails($file) {
    $thumbs = $this->transcoder->thumbnails($file);
    return $thumbs;
  }
  
  public function convert_video(&$job) {
    $this->transcoder->convert($job);
    // If they are using metadata.
    if (variable_get('video_metadata', FALSE)) {
      module_load_include('inc', 'video', '/includes/metadata');
      $metadata = new video_metadata;
      $metadata->process($job->convfile);
    }
  }
  
  private function _transcoders() {
    // Lets find our transcoder classes and build our radio options
    // We do this by scanning our transcoders folder
    $form = array('radios' => array(), 'help' => array(), 'admin_settings' => array());
    $path = drupal_get_path('module', 'video') .'/transcoders';
    $files = file_scan_directory($path, '^.*\.inc$');
    foreach($files as $file) {
      module_load_include('inc', 'video', '/transcoders/' . $file->name);
      $focus = new $file->name;
      $form['radios'][$focus->get_value()] = $focus->get_name();
      $form['help'][] = $focus->get_help();
      $form['admin_settings'] = $form['admin_settings'] + $focus->admin_settings();      
    }
    return $form;
  }
}

interface transcoder_interface {
  public function get_name();
  public function get_value();
  public function get_help();
  public function admin_settings();
  public function thumbnails($video);
  //public function convert();
}