<?php

/**
 * @file
 * Provides Drupal\video_transcode\TranscoderBase.
 */

namespace Drupal\video_transcode;

use Drupal\Component\Plugin\PluginBase;

abstract class TranscoderBase extends PluginBase implements TranscoderInterface {

  public function getName() {
    return $this->pluginDefinition['name'];
  }
  
}