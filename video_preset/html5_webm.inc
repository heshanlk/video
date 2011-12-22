<?php
class html5_webm extends video_preset_base implements video_preset_interface {
  protected $name = 'HTML5 WebM';
  protected $value = 'html5_webm';

  public function __construct() {

  }

  public function get_name() {
    return $this->name;
  }

  public function get_help() {
    return t('Creates files that natively play in Google Chrome using HTML5.');
  }

  public function get_value() {
    return $this->value;
  }

  public function getExtension() {
    return 'webm';
  }
}
