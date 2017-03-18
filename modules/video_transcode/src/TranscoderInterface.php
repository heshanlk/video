<?php

/**
 * @file
 * Provides Drupal\video_transcode\TranscoderInterface
 */

namespace Drupal\video_transcode;

use Drupal\Component\Plugin\PluginInspectionInterface;

/**
 * Defines an interface for transcoder plugins.
 */
interface TranscoderInterface extends PluginInspectionInterface {

  /**
   * Return the name of the transcoder plugin.
   *
   * @return string
   */
  public function getName();
  
}