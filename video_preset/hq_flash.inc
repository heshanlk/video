<?php

/*
 * @file
 * Class file used to store metadata on the video.
 *
 */
class hq_flash implements video_preset_interface {

  protected $name = 'Flash';
  protected $value = 'hq_flash';

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
//    return t('<b>High Video</b>', array());
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/metadata_interface#get_value()
   */
  public function get_value() {
    return $this->value;
  }

  public function get_properties() {
    $properties = array(
      'extension' => 'flv',
      'quality' => '',
      'speed' => '',
      'upscale' => '',
      'stretch' => '',
      'frame_rate' => '',
      'max_frame_rate' => '',
      'keyframe_interval' => '',
      'video_codec' => '',
      'video_bitrate' => '',
      'aspect_mode' => '',
      'bitrate_cap' => '',
      'buffer_size' => '',
      'h264_profile' => '',
      'h264_level' => '',
      'skip_video' => '',
      'audio_codec' => '',
      'audio_quality' => '',
      'audio_bitrate' => '',
      'audio_channels' => '',
      'audio_sample_rate' => '',
      'skip_audio' => '',
      'start_clip' => '',
      'clip_length' => '',
      'command' => array(
        '!cmd_path -i !videofile -s !widthx!height -r 15 -b 250 -ar 22050 -ab 48 !convertfile'
      )
    );
    return $properties;
  }

}

?>
