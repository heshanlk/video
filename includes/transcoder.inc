<?php
//$Id$
/*
 * @file
 * Class file used to wrap the trancoder functions.
 * 
 * //@todo Add Better commenting.
 */

class video_transcoder {
  public $transcoder;
  
  public function __construct() {
    $this->transcoder = variable_get('vid_convertor', 'ffmpeg');
    module_load_include('inc', 'video', '/plugins/' . $this->transcoder);
  }
  
  public function generate_thumbnails($file) {
    $function = $this->transcoder.'_auto_thumbnail';
    if (function_exists($function)) {
      $thumbs = $function($file);
      return $thumbs;
    }
    else {
      drupal_set_message(t('Transcoder not configured properly'), 'error');
      return false;
    }
  }
  
  public function convert_video(&$job) {
    module_load_include('inc', 'video', '/includes/ffmpeg');
    $ffmpeg = new video_ffmpeg;
    $ffmpeg->convert($job);
  }
}