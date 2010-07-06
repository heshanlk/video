<?php
//$Id$
/*
 * @file
 * Class file used to store metadata on the video.
 * 
 */

class video_metadata {
  protected $params = array();
  protected $nice = 'nice -n 19';
  protected $meta_command = '-UP';
  protected $meta_command_path = '/usr/bin/flvtool2';
  
  public function __construct() {
    $this->params['cmd_path'] = variable_get('video_metadata_path', $this->meta_command_path);
  }
  
  public function run_command($options) {
    $command = $this->nice .' '. $this->params['cmd_path'].' '.$options.'  2>&1';
    watchdog('video_metadata', 'Executing command: '. $command, array(), WATCHDOG_DEBUG);
    ob_start();
    passthru($command, $command_return);
    $output = ob_get_contents();
    ob_end_clean();
    return $output;
  }
  
  public function process($video) {
    $command_output = $this->run_command($this->meta_command .' '. $video);
    return $command_output;
  }
   
  
  public function admin_settings() {
    $form = array();
    $form['video_metadata'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enable Metadata'),
      '#default_value' => variable_get('video_metadata', FALSE),
      '#description' => t('Enables metadata for videos by using flvtool2.  It cuts FLV files and adds cue Points (onCuePoint).  If you are converting your files to FLV then this is highly recommended.'),
    );
    $form['video_metadata_path'] = array(
      '#type' => 'textfield',
      '#title' => t('Path to FLVTool2'),
      '#description' => t('Absolute path to flvtool2.'),
      '#default_value' => variable_get('video_metadata_path', $this->meta_command_path),
    );
    $form['video_metadata_dimensions'] = array(
      '#type' => 'textarea',
      '#title' => t('Selectable Dimensions when uploading videos.'),
      '#description' => t('Enter one dimension per line as Video Resolutions.  Each resolution must be in the form of WxH where W=Width and H=Height in pixels.  Example dimensions are 1280x720.'),
      '#default_value' => variable_get("video_metadata_dimensions", video_default_dimensions()),
    );
    return $form;
  } 
}