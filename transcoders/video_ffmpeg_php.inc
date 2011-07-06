<?php

/*
 * @file
 * Transcoder class file to handle ffmpeg settings and conversions.
 */

// Make sure that the parent class is included.
include_once 'video_ffmpeg.inc';

class video_ffmpeg_php extends video_ffmpeg implements transcoder_interface {

  // Naming for our radio options. Makes it easy to extend our transcoders.
  private $name  = 'FFMPEG-PHP (Use ffmpeg-php when possible)';
  private $value = 'video_ffmpeg_php';

  /**
   * Constructor. Just call the parent.
   */
  public function __construct() {
    parent::__construct();
  }

  /**
   * Generate video thumbs.
   *
   * @todo
   *   Use php-ffmpeg API instead of run_command().
   */
  public function generate_thumbnails($video) {
    global $user;
    // Setup our thmbnail path.
    $video_thumb_path = variable_get('video_thumb_path', 'videos/thumbnails');
    // Get the file system directory.
    // @todo : get the field file system settings to this
    $schema_thumb_path = file_default_scheme() . '://' . $video_thumb_path . '/' . $video['fid'];
    file_prepare_directory($schema_thumb_path, FILE_CREATE_DIRECTORY);
    // Total thumbs to generate
    $total_thumbs = variable_get('video_thumbs', 5);
    $videofile = file_load($video['fid']);
    //get the actual video file path from the stream wrappers
    $videopath = drupal_realpath($videofile->uri);
    //get the playtime from the current transcoder
    $duration = $this->get_playtime($videopath);

    $files = NULL;
    for ($i = 1; $i <= $total_thumbs; $i++) {
      $seek = ($duration / $total_thumbs) * $i - 1;  //adding minus one to prevent seek times equaling the last second of the video
      $filename = file_munge_filename("video-thumb-" . $video['fid'] . "-$i.jpg", '', TRUE);
      $thumbfile = $schema_thumb_path . '/' . $filename;
      //skip files already exists, this will save ffmpeg traffic
      if (!is_file(drupal_realpath($thumbfile))) {
        // Use PHP-FFMPEG
        $movie  = new ffmpeg_movie($videopath);
        $frames = $movie->getFrameCount();
        $fps    = $movie->getFrameRate();

        // Get the right frame number
        $framenumber = (int) $seek * $fps;
        if ($framenumber > $frames) {
          $framenumber = $frames;
        }

        // Get the frame and create thumb file
        $frame = $movie->getFrame($framenumber);
        $thumb = $frame->toGDImage();
        imagejpeg($thumb, drupal_realpath($thumbfile));

        if (!file_exists(drupal_realpath($thumbfile))) {
          $error_param = array('%file' => $thumbfile, '%cmd' => $command, '%out' => $command_output);
          $error_msg = t("Error generating thumbnail for video: generated file %file does not exist.<br />Command Executed:<br />%cmd<br />Command Output:<br />%out", $error_param);
          // Log the error message.
          watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
          continue;
        }
      }
      // Begin building the file object.
      // @TODO : use file_munge_filename()
      $file = new stdClass();
      $file->uid = $user->uid;
      $file->status = 0;
      $file->filename = trim($filename);
      $file->uri = $thumbfile;
      $file->filemime = file_get_mimetype($filename);
      $file->filesize = filesize(drupal_realpath($thumbfile));
      $file->timestamp = time();
      $files[] = $file;
    }
    return $files;
  }

  /**
   * Return the playtime seconds of a video
   */
  public function get_playtime($video) {
    $movie = new ffmpeg_movie($video);
    return $movie->getDuration();
  }

  /**
   * Return the dimensions of a video
   */
  public function get_dimensions($video) {
    $movie         = new ffmpeg_movie($video);
    $res['width']  = $movie->getFrameWidth();
    $res['height'] = $movie->getFrameHeight();
    return $res;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#get_name()
   */
  public function get_name() {
    return $this->name;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#get_value()
   */
  public function get_value() {
    return $this->value;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#get_help()
   */
  public function get_help() {
    return l(t('FFMPEG-PHP Online Manual'), 'http://ffmpeg-php.sourceforge.net/');
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#admin_settings()
   */
  public function admin_settings() {
    return parent::admin_settings();
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#admin_settings_validate()
   */
  public function admin_settings_validate($form, &$form_state) {
    return;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#create_job()
   */
  public function create_job($video, $nid) {
    return parent::create_job($video, $nid);
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#delete_job()
   */
  public function delete_job($video) {
    return parent::delete_job($video);
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#load_job()
   */
  public function load_job($fid) {
    return parent::load_job($fid);
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#load_job_queue()
   */
  public function load_job_queue() {
    return parent::load_job_queue();
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#load_completed_job()
   */
  public function load_completed_job(&$video) {
    return parent::load_completed_job($video);
  }

}
