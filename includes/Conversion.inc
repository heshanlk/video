<?php
/**
 * @file
 * Class file to handle video conversion using FFmpeg and webservices.
 */

class Conversion {
  /**
   * Our main function to call when converting queued up jobs.
   */
  public function runQueue() {
    $factory = new TranscoderAbstractionAbstractFactory();
    $transcoder = $factory->getProduct();
    if ($videos = $transcoder->loadJobQueue()) {
      foreach ($videos as $video) {
        $this->process($video);
      }
    }
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
  public function process(stdClass $video) {
    // Make sure this video is pending or do nothing.
    $result = FALSE;

    if ($video->video_status == VIDEO_RENDERING_INQUEUE || $video->video_status == VIDEO_RENDERING_PENDING) {
      $transcoder = new Transcoder();
      if ($result = $transcoder->executeConversion($video)) {
        // rules integration
        if (module_exists('rules') && $video->entity_type == 'node') {
          rules_invoke_event('video_success', node_load($video->entity_id));
        }
      }
      else {
        // rules integration
        if (module_exists('rules') && $video->entity_type == 'node') {
          rules_invoke_event('video_failed', node_load($video->entity_id));
        }
        watchdog('transcoder', 'Video conversion for %videofilename failed. Please check logs for debugging.', array('%videofilename' => $video->filename), WATCHDOG_ERROR);
      }
    }
    else {
      $status = array(
        VIDEO_RENDERING_ACTIVE => 'activated previously',
        VIDEO_RENDERING_COMPLETE => 'completed',
        VIDEO_RENDERING_FAILED => 'failed'
      );
      watchdog('transcoder', 'Video conversion has been @status. You should add video to the queue. Please check the re-queue to enable the video conversion.', array('@status' => $status[$video->video_status]), WATCHDOG_WARNING);
    }

    return $result;
  }
}
