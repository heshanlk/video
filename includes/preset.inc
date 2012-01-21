<?php
class video_preset_api {
  private $presets;

  public function __construct() {
    $this->presets = variable_get('vid_preset', array('hq_flash'));
  }

  public function getEnabledPresets() {
    return $this->getPresets(TRUE);
  }

  public function getAllPresets() {
    return $this->getPresets();
  }

  private function getPresets($enabled = FALSE) {
    $presets = array();
    $files = array();

    foreach (module_list() as $module) {
      $module_files = file_scan_directory(drupal_get_path('module', $module) .'/video_preset', '^.*\.inc$');
      $files = array_merge($files, $module_files);
    }

    foreach ($files as $file) {
      require_once $file->filename;

      $preset = new $file->name;
      if (!$enabled || in_array($preset->get_value(), $this->presets, TRUE)) {
        $presets[$preset->get_value()] = $preset;
      }
    }

    ksort($presets);

    return $presets;
  }
}
