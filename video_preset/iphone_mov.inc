<?php
class iphone_mov extends video_preset_base implements video_preset_interface {
  protected $name = 'iPhone MOV';
  protected $value = 'iphone_mov';

  public function __construct() {

  }

  public function get_name() {
    return $this->name;
  }

  public function get_help() {
    return t('Creates files that natively play on the iPhone using HTML5.');
  }

  public function get_value() {
    return $this->value;
  }

  public function getExtension() {
    return 'mov';
  }
}
