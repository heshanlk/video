<?php

/*
 * @file
 * Class file to handle video conversion using ffmpeg and webservices.
 *
 */
defined('VIDEO_RENDERING_PENDING') ||
    define('VIDEO_RENDERING_PENDING', 1);
defined('VIDEO_RENDERING_ACTIVE') ||
    define('VIDEO_RENDERING_ACTIVE', 5);
defined('VIDEO_RENDERING_COMPLETE') ||
    define('VIDEO_RENDERING_COMPLETE', 10);
defined('VIDEO_RENDERING_FAILED') ||
    define('VIDEO_RENDERING_FAILED', 20);

class video_conversion {

  protected $transcoder;

  public function __construct() {
    module_load_include('inc', 'video', '/includes/transcoder');
    $this->transcoder = new video_transcoder;
  }

  /**
   * Our main function to call when converting queued up jobs.
   */
  public function run_queue() {
    if ($videos = $this->load_job_queue()) {
      foreach ($videos as $video) {
        $this->process($video);
      }
      //clear cache once completed the conversion to update the file paths
      cache_clear_all('*', 'cache_content', true);
    }
  }

  /**
   * Select videos from our queue
   *
   * @return
   *   An array containing all the videos to be proccessed.
   */
  private function load_job_queue() {
    // @TODO : allow only limited jobs to process
    return $this->transcoder->load_job_queue();
  }

  /**
   * Process the video through ffmpeg.
   *
   * @param $video
   *   This can either be the file object or the file id (fid)
   *
   * @return
   *   TRUE of FALSE if video was converted successfully.
   */
  public function process($video) {
    if (is_object($video) && isset($video->fid)) {
      $return = $this->render($video);
    } else {
      $video_object = $this->load_job($video);
      $return = $this->render($video_object);
    }
    return $return;
  }

  private function render($video) {
    if (!is_object($video)) {
      watchdog('video_conversion', 'Video object is not present', array(), WATCHDOG_ERROR);
      return FALSE;
    }
    // Make sure this video is pending or do nothing.
    if ($video->video_status == VIDEO_RENDERING_PENDING) {
      return $this->transcoder->convert_video($video);
    }
    return NULL;
  }

  /**
   * Load a converted video based on the file id ($fid)
   *
   * @todo: Need to figure something out here for multiple files (HTML 5)
   * @param $fid
   *   Integer of the file id to be loaded.
   */
  public function load_completed_job($video) {
    return $this->transcoder->load_completed_job($video);
  }

  public function create_job($video) {
    return $this->transcoder->create_job($video);
  }

  public function update_job($video) {
    return $this->transcoder->update_job($video);
  }

  public function delete_job($video) {
    return $this->transcoder->delete_job($video);
  }

  /**
   * Load a file based on the file id ($fid)
   *
   * @param $fid
   *   Integer of the file id to be loaded.
   */
  public function load_job($fid) {
    return $this->transcoder->load_job($fid);
  }

}

?>