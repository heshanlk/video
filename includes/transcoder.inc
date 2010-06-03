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
    //get our configured transcoder.
    $transcoder = variable_get('vid_convertor', 'video_ffmpeg');
    module_load_include('inc', 'video', '/transcoders/' . $transcoder);
    if(class_exists($transcoder)) {
      $this->transcoder = new $transcoder;
    }
    else {
      drupal_set_message(t('The transcoder is not configured properly.'), 'error');
    }
  }

  public function generate_thumbnails($video) {
    return $this->transcoder->generate_thumbnails($video);
  }

  public function convert_video($video, $converted, $dimensions) {
    $output = $this->transcoder->convert_video($video, $converted, $dimensions);
    // If they are using metadata.
    if (variable_get('video_metadata', FALSE)) {
      module_load_include('inc', 'video', '/includes/metadata');
      $metadata = new video_metadata;
      $metadata->process($converted);
    }
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
    //we need to move our video/thumbnail fieldsets to the bottom of our form as they are used for each trancoder
    $autothumb = $form['admin_settings']['autothumb'];
    $autoconv = $form['admin_settings']['autoconv'];
    unset($form['admin_settings']['autothumb'], $form['admin_settings']['autoconv']);
    $form['admin_settings']['autothumb'] = $autothumb;
    $form['admin_settings']['autoconv'] = $autoconv;
    return $form;
  }

  public function get_dimensions($video) {
    return $this->transcoder->get_dimensions($video);
  }

  public function video_converted_extension() {
    return $this->transcoder->video_converted_extension();
  }
}

interface transcoder_interface {
  public function run_command($command);
  public function generate_thumbnails($video);
  public function convert_video($video, $converted, $dimensions);
  public function get_playtime($video);
  public function get_name();
  public function get_value();
  public function get_help();
  public function admin_settings();
}