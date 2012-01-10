<?php

/*
 * @file
 * Class file used to store video in the filesyetm as CDN or local.
 *
 */

class video_filesystem {

  private $filesystem;

  public function __construct($filesystem = null) {
    //get our configured transcoder.
    if (!isset($filesystem))
      $filesystem = variable_get('vid_filesystem', 'drupal');
    if (!module_load_include('inc', 'video', '/filesystem/' . $filesystem)) {
      $modules = module_list();
      foreach ($modules as $module) {
        $mobule_files = array();
        $module_path = drupal_get_path('module', $module) . '/filesystem';
        $mobule_files = file_scan_directory($module_path, '^.*\.inc$');
        if (is_array($mobule_files)) {
          foreach ($mobule_files as $file) {
            if ($file->name == $filesystem)
              require_once $file->filename;
          }
        }
      }
    }
    if (class_exists($filesystem)) {
      $this->filesystem = new $filesystem;
    } else {
      drupal_set_message(t('The file system is not configured properly.'), 'error');
    }
  }

  public function load_file(&$video) {
    return $this->filesystem->load_file($video);
  }

  public function admin_settings() {
    $fs = variable_get('vid_filesystem', 'drupal');
    $downloads = variable_get('file_downloads', FILE_DOWNLOADS_PUBLIC);

    if ($downloads == FILE_DOWNLOADS_PRIVATE && empty($_POST['vid_filesystem'])) {
      drupal_set_message(t('Storing videos in the Drupal file system is not supported when using <a href="@filesystem">private downloads</a>.', array('@filesystem' => url('admin/settings/file-system'))), $fs == 'drupal' ? 'error' : 'warn');
    }

    $form = array();
    $options = $this->_filesystem();
    $form['vid_filesystem'] = array(
      '#type' => 'radios',
      '#title' => t('Video file system'),
      '#default_value' => variable_get('vid_filesystem', 'drupal'),
      '#options' => $options['radios'],
      '#description' => t('!list', array('!list' => theme('item_list', $options['help']))),
      '#prefix' => '<div id="filesystem-radios">',
      '#suffix' => '</div>',
    );
    $form = $form + $options['admin_settings'];
    return $form;
  }

  private function _filesystem() {
    $files = array();
    // Lets find our transcoder classes and build our radio options
    // We do this by scanning our transcoders folder
    $form = array('radios' => array(), 'help' => array(), 'admin_settings' => array());
    $path = drupal_get_path('module', 'video') . '/filesystem';
    $files = file_scan_directory($path, '^.*\.inc$');
    // check inside sub modules
    $modules = module_list();
    foreach ($modules as $module) {
      $mobule_files = array();
      $module_path = drupal_get_path('module', $module) . '/filesystem';
      $mobule_files = file_scan_directory($module_path, '^.*\.inc$');
      $files = array_merge($files, $mobule_files);
    }

    foreach ($files as $file) {
      if (!module_load_include('inc', 'video', '/filesystem/' . $file->name))
        require_once $file->filename;
      $focus = new $file->name;
      $form['radios'][$focus->get_value()] = $focus->get_name();
      $form['help'][] = $focus->get_help();
      // creating div for each option
      $form['video_' . $focus->get_value() . '_start'] = array(
        'video_' . $focus->get_value() . '_start' => array(
          '#type' => 'markup',
          '#value' => '<div id="' . $focus->get_value() . '">',
        ),
      );
      $form['video_' . $focus->get_value() . '_end'] = array(
        'video_' . $focus->get_value() . '_end' => array(
          '#type' => 'markup',
          '#value' => '</div>',
        ),
      );

      $form['admin_settings'] = $form['admin_settings'] + $form['video_' . $focus->get_value() . '_start'] + $focus->admin_settings() + $form['video_' . $focus->get_value() . '_end'];
    }
    return $form;
  }

  public function admin_settings_validate(&$form, &$form_state) {
    return $this->filesystem->admin_settings_validate($form, $form_state);
  }

}

interface filesystem_interface {
  public function load_file($video);

  public function get_value();
  
  public function get_name();

  public function get_help();

  public function admin_settings();

  public function admin_settings_validate($form, &$form_state);
}