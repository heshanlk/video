<?php

/*
 * @file
 * Class file used to store video presets on the video.
 *
 */

class video_preset {

  private $presets;

  public function __construct($preset = null) {
    $this->presets = $preset;
    if (!isset($preset))
      $this->presets = variable_get('vid_preset', '');
//get our configured transcoder.
//    if (!isset($preset))
//      $preset = variable_get('vid_preset', 'flash_hq');
//    echo print_r($preset);
//    if (!module_load_include('inc', 'video', '/video_preset/' . $preset)) {
//      $modules = module_list();
//      foreach ($modules as $module) {
//        $mobule_files = array();
//        $module_path = drupal_get_path('module', $module) . '/video_preset';
//        $mobule_files = file_scan_directory($module_path, '^.*\.inc$');
//        if (is_array($mobule_files)) {
//          foreach ($mobule_files as $file) {
//            if ($file->name == $preset)
//              require_once $file->filename;
//          }
//        }
////
//      }
//    }
//    if (class_exists($preset)) {
//      $this->preset = new $preset;
//    } else {
//      drupal_set_message(t('The preset is not configured properly.'), 'error');
//    }
  }

  public function admin_settings() {
    $form = array();
    $options = $this->_preset();
    $form['vid_preset'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Video transcode presets'),
      '#options' => $options['radios'],
      '#default_value' => variable_get('vid_preset', array('status', 'promote')),
      '#description' => t('!list', array('!list' => theme('item_list', $options['help']))),
      '#prefix' => '<div id="preset-checkboxes">',
      '#suffix' => '</div>',
    );
//    $form = $form + $options['admin_settings'];
    return $form;
  }

  private function _preset($presets = null) {
// @TDOO : Observer will match in this case
    $files = array();
// Lets find our transcoder classes and build our radio options
// We do this by scanning our transcoders folder
    $form = array('radios' => array(), 'help' => array(), 'properties' => array());
    $path = drupal_get_path('module', 'video') . '/video_preset';
    $files = file_scan_directory($path, '^.*\.inc$');
// check inside sub modules
    $modules = module_list();
    foreach ($modules as $module) {
      $mobule_files = array();
      $module_path = drupal_get_path('module', $module) . '/video_preset';
      $mobule_files = file_scan_directory($module_path, '^.*\.inc$');
      $files = array_merge($files, $mobule_files);
    }

    foreach ($files as $file) {
      if (!module_load_include('inc', 'video', '/video_preset/' . $file->name))
        require_once $file->filename;
      $focus = new $file->name;
      $form['radios'][$focus->get_value()] = $focus->get_name();
      $form['help'][] = $focus->get_help();
//      $form['admin_settings'] = $form['admin_settings'] + $focus->admin_settings();
      if (is_array($presets) && !empty($presets[$focus->get_value()]))
        $form['properties'][$focus->get_value()] = $focus->get_properties();
//      echo $focus->get_value();
    }
    return $form;
  }

  public function properties() {
    $presets = $this->presets;
    $options = $this->_preset($presets);
    return $options['properties'];
  }

}

interface video_preset_interface {

  public function get_name();

  public function get_help();

  public function get_properties();
}