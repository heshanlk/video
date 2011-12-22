<?php
class video_preset {
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

abstract class video_preset_base implements video_preset_interface {
  private $settings = NULL;
  
  public function getSettings() {
    if ($this->settings == NULL) {
      $transcoder = variable_get('vid_convertor', 'video_ffmpeg');
      $settings = db_result(db_query('SELECT data FROM {video_preset_settings} WHERE preset = "%s" AND transcoder = "%s"', $this->get_value(), $transcoder));
      
      if ($settings) {
        $this->settings = unserialize($settings);
      }
      else {
        $this->settings = array();
      }
    }
    
    return $this->settings;
  }

  public function setSettings(array $settings) {
    $transcoder = variable_get('vid_convertor', 'video_ffmpeg');
    $this->settings = $settings;
    
    db_query('REPLACE INTO {video_preset_settings} SET data = "%s", preset = "%s", transcoder = "%s"', serialize($settings), $this->get_value(), $transcoder);
  }
}

interface video_preset_interface {
  public function get_name();
  public function get_value();
  public function get_help();
  public function getExtension();
  public function getSettings();
  public function setSettings(array $settings);
}
