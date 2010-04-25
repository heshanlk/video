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
  
  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#thumbnails($video)
   */
  public function thumbnails($video) {
    
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
    }
    else {
      $form['tranccoder']['no_of_video_thumbs'] = array(
        '#type' => 'textfield',
        '#title' => t('Number of thumbnails'),
        '#description' => t('Number of thumbnails to display from video.'),
        '#default_value' => variable_get('no_of_video_thumbs', 5),
      );
      $markup = t('You must edit the !ffmpeg_wrapper on the modules configuration page.', array('!ffmpeg_wrapper', l(t('FFmpeg Wrapper Settings'), 'admin/settings/ffmpeg_wrapper')));
    }
    $form['video_wrapper_info'] = array(
      '#type' => 'markup',
      '#value' => $markup,
      '#prefix' => '<div id="'. $this->value .'">',
      '#suffix' => '</div>',
    );
    return $form;
  }
}