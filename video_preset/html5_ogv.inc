<?php
class html5_ogv extends video_preset_base implements video_preset {
  protected $name = 'HTML5 Ogg Theora / Vorbis';
  protected $value = 'html5_ogv';

  public function __construct() {

  }

  public function get_name() {
    return $this->name;
  }

  public function get_help() {
    return t('Creates files that natively play in Firefox using HTML5.');
  }

  public function get_value() {
    return $this->value;
  }

  public function getExtension() {
    return 'ogv';
  }
}
