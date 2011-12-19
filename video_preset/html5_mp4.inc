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
    return t('Creates files that natively play in Internet Explorer 9+ and Google Chrome using HTML5 and in any other browser using Flash.', array());
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
        '!cmd_path -y -i !videofile -pass 1 -vcodec libx264 -b 500k -threads 0 -vpre slow_firstpass -an !convertfile',
        '!cmd_path -y -i !videofile -pass 2 -vcodec libx264 -b 500k -threads 0 -vpre slow -acodec libfaac -ab 128k !convertfile'
        //'!cmd_path -strict experimental -y -i !videofile -pass 1 -vcodec libx264 -b 500k -threads 0 -vpre slow_firstpass -an !convertfile',
        //'!cmd_path -strict experimental -y -i !videofile -pass 2 -vcodec libx264 -b 500k -threads 0 -vpre slow -acodec aac -ab 128k !convertfile'
      )
    );
    return $properties;
  }
}
