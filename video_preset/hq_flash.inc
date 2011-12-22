<?php
class hq_flash extends video_preset_base implements video_preset_interface {

  protected $name = 'Flash';
  protected $value = 'hq_flash';

  public function __construct() {

  }

  public function get_name() {
    return $this->name;
  }

  public function get_help() {
    return t('Creates files that play in any Flash player, including older versions.');
  }

  public function get_value() {
    return $this->value;
  }

  public function getExtension() {
    return 'flv';
  }
}
