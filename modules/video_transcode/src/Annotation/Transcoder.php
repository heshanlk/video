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
   * The name of the transcoder.
   *
   * @var \Drupal\Core\Annotation\Translation
   *
   * @ingroup plugin_translatable
   */
  public $name;
  
  /**
   * The type of the service.
   *
   * @var boolean
   */
  public $isExternal;
  
  
  
}