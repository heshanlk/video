<?php

//$Id$
/*
 * @file
 * Class file used to store video presets on the video.
 *
 */

class video_preset {

  private $preset;

  public function __construct($preset = null) {
    $this->preset = $preset;
    if (!isset($preset))
      $this->preset = variable_get('video_preset', array());
  }

  /**
   * Show admin settings
   * @return array $form
   */
  public function admin_settings() {
    $presets = preset_get_presets('video');
    $preset = array();
    foreach ($presets as $id => $value) {
      $preset[$id] = $value['name'];
      $help[] = $value['name'] . ' - ' . $value['description'] . ' ' . l(t('edit'), preset_get_preset_path('video', $value['name']));
    }
    $form = array();
    $form['video_preset'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Video transcode presets'),
      '#options' => $preset,
      '#default_value' => variable_get('video_preset', array()),
      '#description' => t('!list', array('!list' => theme('item_list', array('items' => $help)))),
      '#prefix' => '<div id="preset-checkboxes">',
      '#suffix' => '</div>',
    );
//    $form = $form + $options['admin_settings'];
    return $form;
  }

  /**
   * Get the preset properties
   * @return array $properties
   */
  public function properties() {
    $preset = $this->preset;
    return $options['properties'];
    if (preset_exists('video', $preset))
      preset_get_settings($preset);
  }

}