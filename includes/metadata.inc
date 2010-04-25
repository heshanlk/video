<?php
//$Id$
/*
 * @file
 * Class file used to store metadata on the video.
 * 
 */

class video_metadata {
   
  
  public function admin_settings() {
    $form = array();
    $form['vid_metadata'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enable Metadata'),
      '#default_value' => variable_get('vid_metadata', FALSE),
      '#description' => t('Enables metadata for videos by using flvtool2.  It cuts FLV files and adds cue Points (onCuePoint).  If you are converting your files to FLV then this is highly recommended.'),
    );
    $form['video_metadata_path'] = array(
      '#type' => 'textfield',
      '#title' => t('Path to FLVTool2'),
      '#description' => t('Absolute path to flvtool2.'),
      '#default_value' => variable_get('video_metadata_path', '/usr/bin/flvtool2'),
    );
    return $form;
  } 
}