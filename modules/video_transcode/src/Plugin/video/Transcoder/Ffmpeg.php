<?php
/**
 * @file
 * Contains \Drupal\video_transcode\Plugin\video\Transcoder\Ffmpeg.
 */

namespace Drupal\video_transcode\Plugin\video\Transcoder;

use Drupal\video_transcode\TranscoderBase;

/**
 * Provides Ffmpeg.
 *
 * @Transcoder(
 *   id = "ffmpeg",
 *   name = @Translation("FFmpeg"),
 *   isExternal = false
 * )
 */

class FFmpeg extends TranscoderBase {
  
  /**
   * {@inheritdoc}
   */
  public function getOutputFiles(){
    return [
      ['format' => 'mpeg4', 'url' => 'http://s3.amazonaws.com/bucket/video.mp4', 'id' => 1]
    ];
  }
  
  /**
   * {@inheritdoc}
   */
  public function getVideoThumbnails(){
    return [
      ['id' => 1, 'url' => 'http://s3.amazonaws.com/bucket/video/frame_0000.png']
    ];
  }
  
  
}