<?php

/**
 * @file
 * Contains \Drupal\video\Plugin\video\Provider\Vimeo.
 */

namespace Drupal\video\Plugin\video\Provider;

use Drupal\video\ProviderPluginBase;

/**
 * @VideoEmbeddableProvider(
 *   id = "vimeo",
 *   label = @Translation("Vimeo"),
 *   description = @Translation("Vimeo Video Provider"),
 *   regular_expressions = {
 *     "/^https?:\/\/(www\.)?vimeo.com\/(?<id>[0-9]*)$/",
 *   },
 *   mimetype = "video/vimeo",
 *   stream_wrapper = "vimeo"
 * )
 */

class Vimeo extends ProviderPluginBase {
  
  /**
   * {@inheritdoc}
   */
  public function renderEmbedCode($settings) {
    $file = $this->getVideoFile();
    $data = $this->getVideoMetadata();
    return [
      '#type' => 'html_tag',
      '#tag' => 'iframe',
      '#attributes' => [
        'width' => $settings['width'],
        'height' => $settings['height'],
        'frameborder' => '0',
        'allowfullscreen' => 'allowfullscreen',
        'src' => sprintf('https://player.vimeo.com/video/%s?autoplay=%d', $data['id'], $autoplay),
      ],
    ];
  }
}
