<?php
/**
 * @file
 * Definition of drupal.
 */

// Register this file as a CTools plugin
$plugin = array();

class drupal implements video_filesystem {
  protected $params = array();
  protected $name = 'Drupal file system';
  protected $value = 'drupal';

  public function __construct() {
  }

  public function load_file(stdClass $video) {
  }

  public function get_name() {
    return $this->name;
  }

  public function get_help() {
    return l(t('Drupal file system'), 'admin/settings/file-system');
  }

  public function get_value() {
    return $this->value;
  }

  public function admin_settings(&$form_state) {
    return array();
  }

  public function admin_settings_validate($form, &$form_state) {
  }

  public function onpreconvert(stdClass $video) {
    return TRUE;
  }

  public function onpostconvert(stdClass $video) {
    return TRUE;
  }

  public function delete_video(stdClass $video) {
    // The original file is deleted by the filefield module

    // Delete the locally converted videos
    if (!empty($video->data)) {
      foreach ($video->data as $file) {
        file_delete($file->filepath);
      }
    }

    // Delete our video thumbnails and folder.
    $thumb_folder = video_thumb_path($video, FALSE);

    // Recursively delete our folder and files.
    if (is_dir($thumb_folder)) {
      rmdirr($thumb_folder);

      // Delete any entries in the files table that may refer to the above path.
      db_query('DELETE FROM {files} WHERE filepath LIKE "%b%%"', strtr(db_escape_string($thumb_folder), array('%' => '\%', '_' => '\_')));
    }
  }
}
