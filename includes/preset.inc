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
    if (empty($presets)) {
      $this->presets = variable_get('vid_preset', array('hq_flash'));
    }
  }

  public function admin_settings() {
    $form = array();
    $presets = $this->getAllPresets();

    $options = array();
    $help = array();
    foreach ($presets as $preset) {
      $options[$preset->get_value()] = check_plain($preset->get_name());

      $helptext = $preset->get_help();
      if (!empty($helptext)) {
        $help[] = check_plain($preset->get_name()) .': '. $helptext;
      }
    }

    $form['vid_preset'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Video transcode presets'),
      '#options' => $options,
      '#default_value' => $this->presets,
      '#description' => theme('item_list', $help),
      '#required' => TRUE,
    );

    return $form;
  }

  public function getAllPresets() {
    $presets = array();
    $files = array();

    foreach (module_list() as $module) {
      $module_files = file_scan_directory(drupal_get_path('module', $module) .'/video_preset', '^.*\.inc$');
      $files = array_merge($files, $module_files);
    }

    foreach ($files as $file) {
      require_once $file->filename;

      $preset = new $file->name;
      $presets[$preset->get_value()] = $preset;
    }

    ksort($presets);

    return $presets;
  }

  public function properties() {
    $presets = $this->getAllPresets();
    $properties = array();

    foreach ($this->presets as $presetvalue) {
      if (isset($presets[$presetvalue])) {
        $properties[$presetvalue] = $presets[$presetvalue]->get_properties();
      }
    }

    return $properties;
  }
}

interface video_preset_interface {
  public function get_name();
  public function get_value();
  public function get_help();
  public function get_properties();
}
