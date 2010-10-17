<?php

//$Id$
/*
 * @file
 * Class file used to store metadata on the video.
 *
 */
class flash_lq implements video_preset_interface {

  protected $name = 'Flash HQ Video Low quality';
  protected $value = 'flash_lq';

  public function __construct() {
    
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/metadata_interface#get_name()
   */
  public function get_name() {
    return $this->name;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/metadata_interface#get_help()
   */
  public function get_help() {
    return t('<b>Flash HQ Video Low quality</b><br> Dimention : HQ<br>Video Codec : AAC<br> Audio Codec : MP3', array());
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/metadata_interface#get_value()
   */
  public function get_value() {
    return $this->value;
  }

  public function admin_settings() {
    $form = array();
    $form['video_flvtool2_start'] = array(
      '#type' => 'markup',
      '#value' => '<div id="video_flvtool2">',
    );
    $form['video_metadata_path'] = array(
      '#type' => 'textfield',
      '#title' => t('Path to FLVTool2'),
      '#description' => t('Executable path to flvtool2.'),
      '#default_value' => variable_get('video_metadata_path', $this->meta_command_path),
    );
    $form['video_flvtool2_end'] = array(
      '#type' => 'markup',
      '#value' => '</div>',
    );
    return $form;
  }

  public function get_properties() {
    $properties = array(
      'extension' => 'mp4',
      'width' => '480',
      'height' => '360',
      'quality' => '',
      'speed' => '',
      'upscale' => '',
      'stretch' => '',
      'frame_rate' => '',
      'max_frame_rate' => '',
      'keyframe_interval' => '',
      'video_bitrate' => '',
      'bitrate_cap' => '',
      'buffer_size' => '',
      'h264_profile' => '',
      'h264_level' => '',
      'skip_video' => '',
      'audio_codec' => '',
      'audio_bitrate' => '',
      'audio_channels' => '',
      'audio_sample_rate' => '',
      'skip_audio' => '',
      'start_clip' => '',
      'clip_length' => '',
    );
    return $properties;
  }

}

?>
