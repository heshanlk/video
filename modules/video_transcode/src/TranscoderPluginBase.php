<?php

/**
 * @file
 * Provides Drupal\video_transcode\TranscoderBase.
 */

namespace Drupal\video_transcode;

use Drupal\Component\Plugin\PluginBase;

class TranscoderBase extends PluginBase implements FlavorInterface {

  public function getName() {
    return $this->pluginDefinition['name'];
  }
  
}