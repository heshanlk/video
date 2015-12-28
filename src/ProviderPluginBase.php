<?php

/**
 * @file
 * Contains Drupal\video\ProviderPluginBase
 */

namespace Drupal\video;

use Drupal\Core\Plugin\ContainerFactoryPluginInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * A base for the provider plugins.
 */
abstract class ProviderPluginBase implements ProviderPluginInterface, ContainerFactoryPluginInterface {

  /**
   * File object to handle
   *
   * @var Drupal\file\Entity\File $file
   */
  protected $file;

  /**
   * Additional metadata for the embedded video object
   *
   * @var array
   */
  protected $metadata = array();

  /**
   * Create a plugin with the given input.
   *
   * @param string $configuration
   *   The configuration of the plugin.
   * @param \GuzzleHttp\ClientInterface $http_client
   *    An HTTP client.
   *
   * @throws \Exception
   */
  public function __construct($configuration) {
    $this->file = $configuration['file'];
    $this->metadata = $configuration['metadata'];
  }

  /**
   * Get the ID of the video.
   *
   * @return string
   *   The video ID.
   */
  protected function getVideoFile() {
    return $this->file;
  }

  /**
   * Get the input which caused this plugin to be selected.
   *
   * @return string
   *   The raw input from the user.
   */
  protected function getVideoMetadata() {
    return $this->metadata;
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container, array $configuration, $plugin_id, $plugin_definition) {
    return new static($configuration);
  }

}
