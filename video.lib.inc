<?php
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
   * 
   * @param stdClass $video
   */
  public function load_file(stdClass $video);
  
  /**
   * @param stdClass $video
   * @return bool FALSE if an error occured while doing the operation, TRUE when no operation was needed or on success
   */
  public function onpreconvert(stdClass $video);
  
  /**
   * @param stdClass $video
   * @return bool FALSE if an error occured while doing the operation, TRUE when no operation was needed or on success
   */
  public function onpostconvert(stdClass $video);
  
  /**
   * Delete the video from the file system.
   *
   * @param stdClass $video
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
   * @param stdClass $video The video
   * @param int $status One of the VIDEO_RENDERING_* constants
   */
  public function change_status(stdClass $video, $status);
  public function generate_thumbnails(stdClass $video);
  
  /**
   * Convert the given video.
   * 
   * Make sure to update the provided video object with any information that
   * is required by the file system module after conversion.
   * 
   * @param stdClass $video
   * @return bool TRUE if the conversion was successful, FALSE otherwise
   */
  public function convert_video(stdClass $video);
}

abstract class video_preset_base implements video_preset {
  private $settings = NULL;

  public function getSettings() {
    if ($this->settings == NULL) {
      $transcoder = variable_get('vid_convertor', 'video_localcommand');
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
    $transcoder = variable_get('vid_convertor', 'video_localcommand');
    $this->settings = $settings;

    db_query('REPLACE INTO {video_preset_settings} SET data = "%s", preset = "%s", transcoder = "%s"', serialize($settings), $this->get_value(), $transcoder);
  }
}

interface video_preset {
  public function get_name();
  public function get_value();
  public function get_help();
  public function getExtension();
  public function getSettings();
  public function setSettings(array $settings);
}
