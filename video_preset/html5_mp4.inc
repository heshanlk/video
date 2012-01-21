<?php
class html5_mp4 extends video_preset_base implements video_preset {
  protected $name = 'HTML5 MP4';
  protected $value = 'html5_mp4';

  public function __construct() {

  }

  public function get_name() {
    return $this->name;
  }

  public function get_help() {
    return t('Creates files that natively play in Internet Explorer 9+ and Google Chrome using HTML5 and in any other browser using Flash.');
  }

  public function get_value() {
    return $this->value;
  }

  public function getExtension() {
    return 'mp4';
  }
}
