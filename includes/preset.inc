<?php

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
    $form = array();
    $presets = video_preset_get_presets();
    if (count($presets) == 0) {
      $form['video_preset'] = array(
        '#markup' => t('No Preset were found. Please use the !create_link link to create
      a new Video Preset, or upload an existing Feature to your modules directory.',
            array('!create_link' => l(t('Create Video Preset'), 'admin/config/media/video/presets/add'))),
        '#prefix' => '<div id="preset-checkboxes">',
        '#suffix' => '</div>',
      );
      return $form;
    }
    $preset = array();
    foreach ($presets as $id => $value) {
      $preset[$id] = $value['name'] . ' ' . l(t('edit'), ('admin/config/media/video/presets/preset/' . $value['name']));
//      $help[] = $value['name'] . ' - ' . $value['description'] . ' ' . l(t('edit'), preset_get_preset_path('video', $value['name']));
    }

    $form['video_preset'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Video transcode presets'),
      '#options' => $preset,
      '#default_value' => variable_get('video_preset', array()),
      '#description' => t('Please use the !manage_link link to manage
       Video Presets. Use the !create_link link to create
      a new Video Preset, or upload an existing Feature to your modules directory.',
          array('!manage_link' => l(t('Manage Video Preset'), 'admin/config/media/video/presets'),
            '!create_link' => l(t('Create Video Preset'), 'admin/config/media/video/presets/add'))),
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