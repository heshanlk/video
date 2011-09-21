<?php

/*
 * @file
 * Class file to handle video conversion using ffmpeg and webservices.
 *
 */

class Conversion {

  public function __construct() {
    
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
    $factory = new TranscoderAbstractionAbstractFactory();
    $transcoder = $factory->getProduct();
    return $transcoder->loadJobQueue();
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
      $video = $this->loadJob($video);
      $return = $this->render($video);
    }
    // clear the cache for the video id
    $cid = 'video:video:' . md5($video->fid);
    $cache = cache_get($cid);
    if (!empty($cache->data)) {
      cache_clear_all($cid, 'cache');
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
      $transcoder = new Transcoder;
      if ($job = $transcoder->executeConvertion($video)) {
        // rules integration
        if (module_exists('rules'))
          rules_invoke_event('video_success', node_load($video->nid));
        return $job;
      }
      else {
        // rules integration
        if (module_exists('rules'))
          rules_invoke_event('video_failed', node_load($video->nid));
        watchdog('transcoder', 'Video conversion has been failed!. Please check further logs for debugging.', array(), WATCHDOG_WARNING);
      }
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

  public function changeStatus($vid, $status) {
    $transcoder = new Transcoder;
    return $transcoder->changeStatus($vid, $status);
  }

  /**
   * Load a file based on the file id ($fid)
   *
   * @param $fid
   *   Integer of the file id to be loaded.
   */
  public function loadJob($fid) {
    $factory = new TranscoderAbstractionAbstractFactory();
    $transcoder = $factory->getProduct();
    $transcoder->setInput((array) file_load($fid));
    return $transcoder->loadJob();
  }

}

?>