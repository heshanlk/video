<?php
//$Id$
/*
 * @file
 * Transcoder class file to handle ffmpeg_wrapper settings and conversions.
 *
 */


class video_ffmpeg_wrapper implements transcoder_interface {
  private $name = 'FFmpeg Wrapper';
  private $value = 'video_ffmpeg_wrapper';
  
  
  public function run_command($command) {
    $output = ffmpeg_wrapper_run_command($command, $error_check = true, $path = '');
    return $output;
  }
   
  /**
   * Return the playtime seconds of a video
   */
  function get_playtime($video) {
    $ffmpeg_output = ffmpeg_wrapper_file_data($video['filepath']);
    return $ffmpeg_output['duration'];
  }  

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#get_name()
   */
  public function get_name() {
    return $this->name;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#get_value()
   */
  public function get_value() {
    return $this->value;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#get_help()
   */
  public function get_help() {
    $help = l(t('FFMPEG Wrapper'), 'http://drupal.org/project/ffmpeg_wrapper');
    // If the module isn't loaded, show an error next to the link.
    if(!module_exists('ffmpeg_wrapper')) {
      $help .= ' <span class="error">'.t('You currently do not have this installed.').'</span>';
    }
    return $help;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#admin_settings()
   */
  public function admin_settings() {
    $form = array();
    if (!module_exists('ffmpeg_wrapper')) {
      $markup = t('You must download and install the !ffmpeg_wrapper to use the FFmpeg Wrapper as a transcoder.', array('!ffmpeg_wrapper' => l(t('FFmpeg Wrapper Module'), 'http://www.drupal.org/project/ffmpeg_wrapper')));
      $form['video_wrapper_info'] = array(
      '#type' => 'markup',
      '#value' => $markup,
      '#prefix' => '<div id="'. $this->value .'">',
      '#suffix' => '</div>',
      );
    }
    else {
      $form['video_ffmpeg_wrapper_start'] = array(
        '#type' => 'markup',
        '#value' => '<div id="video_ffmpeg_wrapper">',
      );
      $ffmpeg_admin_form = ffmpeg_wrapper_admin();
      $form = array_merge($form, $ffmpeg_admin_form['ffmpeg_wrapper']);
      $form['video_ffmpeg_wrapper_end'] = array(
        '#type' => 'markup',
        '#value' => '</div>',
      );
    } 
    return $form;
  }
}