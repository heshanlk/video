<?php

/*
 * @file
 * Class file used to store filesystem on the video.
 *
 */
class drupal extends video_filesystem {

  protected $params = array();
  protected $name = 'Drupal File System';
  protected $value = 'drupal';

  public function __construct() {
    
  }

  public function save_file($video) {

  }

  public function prepare_file($video) {

  }

  public function load_file(&$video) {
    
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/filesystem_interface#get_name()
   */
  public function get_name() {
    return $this->name;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/filesystem_interface#get_help()
   */
  public function get_help() {
    return t('Drupal filesystem', array());
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/filesystem_interface#get_value()
   */
  public function get_value() {
    return $this->value;
  }

  public function run_command($options) {
    return;
  }

  public function admin_settings() {
    $form = array();
    return $form;
  }

  public function admin_settings_validate($form, $form_state) {
    return;
  }

}

?>
