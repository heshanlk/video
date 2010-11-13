<?php

//$Id$
/*
 * @file
 * Class file used to store metadata on the video.
 *
 */
class flash_hq implements video_preset_interface {

  protected $name = 'Flash HQ Video';
  protected $value = 'flash_hq';
  protected $width;
  protected $height;
  protected $video_ext = 'flv';

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
    return t('<b>Flash HQ Video</b><br> Dimention : HQ<br>Video Codec : AAC<br> Audio Codec : MP3', array());
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
      'width' => '640',
      'height' => '340',
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
    );
    return $properties;
  }

}

?>
