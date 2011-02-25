<?php

/*
 * @file
 * Class file used to store metadata on the video.
 *
 */
class html5_webm implements video_preset_interface {

  protected $name = 'HTML5 WebM';
  protected $value = 'html5_webm';

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
//    return t('<b>HTML5 WebM Video</b>', array());
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
      'extension' => 'webm',
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
        '!cmd_path -pass 1 -passlogfile !videofile -threads 16  -keyint_min 0 -g 250 -skip_threshold 0 -qmin 1 -qmax 51 -i !videofile -vcodec libvpx -b 204800 -s !widthx!height -aspect 4:3 -an -f webm -y NUL',
        '!cmd_path -y -pass 2 -passlogfile !videofile -threads 16  -keyint_min 0 -g 250 -skip_threshold 0 -qmin 1 -qmax 51 -i !videofile -vcodec libvpx -b 204800 -s !widthx!height -aspect 4:3 -acodec libvorbis -ac 2 -y !convertfile'
      )
    );
    return $properties;
  }

}

?>
