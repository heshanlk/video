<?php
/**
 * @file
 * Contains \Drupal\video_transcode\Annotation\Transcoder.
 */

namespace Drupal\video_transcode\Annotation;

use Drupal\Component\Annotation\Plugin;

/**
 * Defines a transcoder annotation object.
 *
 * Plugin Namespace: Plugin\video\Transcoder
 *
 * @see \Drupal\video_transcode\Plugin\TranscoderManager
 * @see plugin_api
 *
 * @Annotation
 */
class Transcoder extends Plugin {

  /**
   * The plugin ID.
   *
   * @var string
   */
  public $id;

  /**
   * The label of the transcoder.
   *
   * @var \Drupal\Core\Annotation\Translation
   *
   * @ingroup plugin_translatable
   */
  public $label;
  
  /**
   * A brief description of the transcoder.
   *
   * @ingroup plugin_translatable
   *
   * @var \Drupal\Core\Annotation\Translation (optional)
   */
  public $description = '';
  
  /**
   * The type of the service, external service or a locally installed service like Ffmpeg.
   *
   * @var boolean
   */
  public $isExternal;
  
}