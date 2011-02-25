<?php

/*
 * @file
 * Class file used to store metadata on the video.
 *
 */
class html5_mp4 implements video_preset_interface {

  protected $name = 'HTML5 MP4';
  protected $value = 'html5_mp4';

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
//    return t('<b>HTML5 MP4 Video</b>', array());
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
      'extension' => 'mp4',
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
//        'HandBrakeCLI --preset "iPhone & iPod Touch" --vb !videobitrate --width !width --two-pass --turbo --optimize --input !videofile --output !convertfile'
        '!cmd_path -i !videofile -an -pass 1 -vcodec libx264 -vpre slow_firstpass -b 500k -threads 0 !convertfile',
        '!cmd_path -y -i !videofile -acodec libfaac -ab 128k -pass 2 -vcodec libx264 -vpre slow -b 500k -threads 0 !convertfile'
      )
    );
    return $properties;
  }

}

?>
