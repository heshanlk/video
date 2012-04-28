<?php
/**
 * @file
 * File containing Video module interfaces and classes
 */

interface video_plugin {
  public function get_value();
  public function get_name();
  public function get_help();
  public function admin_settings(&$form_state);
  public function admin_settings_validate($form, &$form_state);
}

interface video_filesystem extends video_plugin {
  /**
   * Process the given video in preparation for use on the
   * front end.
   */
  public function load_file(stdClass $video);

  /**
   * @return bool
   *   FALSE if an error occured while doing the operation, TRUE when no operation was needed or on success
   */
  public function onpreconvert(stdClass $video);

  /**
   * @return bool
   *   FALSE if an error occured while doing the operation, TRUE when no operation was needed or on success
   */
  public function onpostconvert(stdClass $video);

  /**
   * Delete the video from the file system.
   */
  public function delete_video(stdClass $video);
}

interface video_transcoder extends video_plugin {
  public function preset_settings(&$form_state, video_preset $preset);
  public function preset_settings_validate($form, &$form_state);

  public function create_job(stdClass $video);
  public function update_job(stdClass $video);
  public function delete_job(stdClass $video);
  public function load_job($fid);

  /**
   * Return at most $num files that need to be transcoded.
   *
   * @param int $num
   */
  public function load_job_queue($num);
  public function load_completed_job(stdClass $video);

  /**
   * Change the rendering status of the file.
   *
   * @param $video
   *   Video instance
   * @param $status
   *   One of the VIDEO_RENDERING_* constants
   */
  public function change_status(stdClass $video, $status);
  public function generate_thumbnails(stdClass $video);

  /**
   * Convert the given video.
   *
   * Make sure to update the provided video object with any information that
   * is required by the file system module after conversion.
   *
   * @return bool
   *   TRUE if the conversion was successful, FALSE otherwise
   */
  public function convert_video(stdClass $video);

  public function requirements();

  /**
   * Return the width and height of the given file.
   *
   * @param $filepath
   *   string containing the path to the video file.
   *
   * @return
   *   array containing keys width and height or NULL when
   *   the width and height can't be determined.
   */
  public function get_dimensions($filepath);

  /**
   * Helper method for video_file_download()
   *
   * Returns the path to the original video file based on the path of the
   * converted video file.
   *
   * @param $filepath
   *   The path to the converted file, this path must have been processed by
   *   file_create_path().
   * @return
   *   Path to the original video, NULL if not found.
   */
  public function get_original_path_by_converted_path($filepath);
}

class video_preset {
  public $id = 0;
  public $name = '';
  public $help = '';
  public $extension = '';
  public $filenamesuffix = '';

  /**
   * Settings for the current transcoder
   *
   * @var array
   */
  private $settings = NULL;

  public function __construct(stdClass $dbdata) {
    $this->id = $dbdata->id;
    $this->name = $dbdata->name;
    $this->help = $dbdata->help;
    $this->extension = $dbdata->extension;
    $this->filenamesuffix = $dbdata->filenamesuffix;
  }

  public function getSettings() {
    if ($this->settings == NULL) {
      $transcoder = variable_get('vid_convertor', 'video_localcommand');
      $settings = db_result(db_query('SELECT data FROM {video_preset_settings} WHERE presetid = %d AND transcoder = "%s"', $this->id, $transcoder));

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
    $transcoder = variable_get('vid_convertor', 'video_localcommand');
    $this->settings = $settings;

    db_query('REPLACE INTO {video_preset_settings} SET data = "%s", presetid = %d, transcoder = "%s"', serialize($settings), $this->id, $transcoder);
  }
}
