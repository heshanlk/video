<?php

/*
 * @file
 * Class file to handle video conversion using ffmpeg and webservices.
 *
 */

class Conversion {

  protected $transcoder;

  public function __construct() {
    module_load_include('inc', 'video', '/includes/transcoder');
    $this->transcoder = new Transcoder;
  }

  /**
   * Our main function to call when converting queued up jobs.
   */
  public function runQueue() {
    if ($videos = $this->loadJobQueue()) {
      foreach ($videos as $video) {
        $this->process($video);
      }
      //clear cache once completed the conversion to update the file paths
      cache_clear_all();
    }
  }

  /**
   * Select videos from our queue
   *
   * @return
   *   An array containing all the videos to be proccessed.
   */
  public function loadJobQueue() {
    return $this->transcoder->loadJobQueue();
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
    }
    else {
      $video_object = $this->loadJob($video);
      $return = $this->render($video_object);
    }
    return $return;
  }

  private function render($video) {
    if (!is_object($video)) {
      watchdog('transcoder', 'Video object is not present', array(), WATCHDOG_ERROR);
      return FALSE;
    }
    // Make sure this video is pending or do nothing.
    if ($video->video_status != VIDEO_RENDERING_INQUEUE) {
      return $this->transcoder->convertVideo($video);
    }
    else {
      $status = array(
        VIDEO_RENDERING_INQUEUE => 'in queue',
        VIDEO_RENDERING_COMPLETE => 'completed',
        VIDEO_RENDERING_FAILED => 'failed'
      );
      watchdog('transcoder', 'Video conversion has been !status. You should add video to the queue. Please check the re-queue to enable the video conversion.', array('!status' => $status[$video->video_status]), WATCHDOG_WARNING);
      return FALSE;
    }
  }

  /**
   * Load a converted video based on the file id ($fid)
   *
   * @todo: Need to figure something out here for multiple files (HTML 5)
   * @param $fid
   *   Integer of the file id to be loaded.
   */
  public function loadCompletedJob($video) {
    return $this->transcoder->loadCompletedJob($video);
  }

  public function createJob($video, $nid) {
    return $this->transcoder->createJob($video, $nid);
  }

  public function deleteJob($video) {
    return $this->transcoder->deleteJob($video);
  }

  public function changeStatus($vid, $status) {
    return $this->transcoder->changeStatus($vid, $status);
  }

  /**
   * Load a file based on the file id ($fid)
   *
   * @param $fid
   *   Integer of the file id to be loaded.
   */
  public function loadJob($fid) {
    return $this->transcoder->loadJob($fid);
  }

}

?>