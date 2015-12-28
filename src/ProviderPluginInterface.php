<?php

/**
 * @file
 * Contains Drupal\video\ProviderInterface.
 */

namespace Drupal\video;

/**
 * Providers an interface for embed providers.
 */
interface ProviderPluginInterface {
  
  /**
   * Render embed code.
   *
   * @param string $settings
   *   The settings of the video player.
   *
   * @return mixed
   *   A renderable array of the embed code.
   */
  public function renderEmbedCode($settings);

}