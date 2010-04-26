<?php
//$Id$
/*
 * @file
 * Class file used to store metadata on the video.
 * 
 */

class video_metadata {
  protected $meta_command = '-UP';
  protected $meta_command_path = '/usr/bin/flvtool2';
  
  public function process($video) {
    $command_path = variable_get('video_metadata_path', $this->meta_command_path);
    $command = $command_path ." ". $this->meta_command ." '".$video."'";
    $command_output = shell_exec($command .' 2>&1');
    //log our command for debugging.
    watchdog('video_conversion', 'Executing command: '.$command, array(), WATCHDOG_DEBUG);
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
    return $form;
  } 
}