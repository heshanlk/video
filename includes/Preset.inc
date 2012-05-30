<?php
/**
 * @file
 * Class file used to store video presets on the video.
 */

class Preset {
  private $preset;

  public function __construct($preset = NULL) {
    $this->preset = $preset;
    if (!isset($preset))
      $this->preset = variable_get('video_preset', array('FLV'));
  }

  /**
   * Get the preset properties
   * @return array $properties
   */
  public function properties() {
    // adding modules/video_ui to load this file even if UI module is disabled
    module_load_include('inc', 'video', 'modules/video_ui/video.preset');
    $presets = array();
    foreach ($this->preset as $preset_name) {
      // skip selecting disabled presets 0=>0
      if (!empty($preset_name)) {
        $presets[$preset_name] = video_get_preset($preset_name);
      }
    }
    return $presets;
  }
}
