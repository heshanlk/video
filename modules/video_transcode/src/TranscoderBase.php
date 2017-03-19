<?php

/**
 * @file
 * Provides Drupal\video_transcode\TranscoderBase.
 */

namespace Drupal\video_transcode;

use Drupal\Core\Plugin\ContainerFactoryPluginInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;
use GuzzleHttp\ClientInterface;
use Drupal\Component\Plugin\PluginBase;

abstract class TranscoderBase extends PluginBase implements TranscoderInterface, ContainerFactoryPluginInterface {

  /**
   * File object to transcode
   *
   * @var Drupal\file\Entity\File $file
   */
  protected $file;
  
  /**
   * An http client.
   *
   * @var \GuzzleHttp\ClientInterface
   */
  protected $httpClient;
  
  
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
  public function __construct($configuration, ClientInterface $http_client) {
    $this->file = $configuration['file'];
    $this->httpClient = $http_client;
  }
  
  /**
   * Get the Plugin label.
   *
   * @return string
   * The name of the plugin.
   */
  public function getLabel() {
    return $this->pluginDefinition['label'];
  }
  
  /**
   * Get the ID of the video.
   *
   * @return string
   *   The video ID.
   */
  protected function getInputFile() {
    return $this->file;
  }
  
  /**
   * Get the transcoder supported codecs.
   *
   * @return array
   *   Array of supported codes.
   */
  public function getAvailableCodecs(){
    return [
      'encode' => [
        'video' => [
          'h264' => 'H.264',
          'vp8' => 'VP8',
          'theora' => 'Theora',
          'vp6' => 'VP6',
          'mpeg4' => 'MPEG-4',
          'wmv' => 'WMV',
        ],
        'audio' => [
          'aac' => 'AAC',
          'mp3' => 'MP3',
          'vorbis' => 'Vorbis',
          'wma' => 'WMA',
        ]
      ],
      'decode' => [],
    ];
  }
  
  /**
   * Get the transcoder supported formats.
   *
   * @return array
   *   Array of supported formats.
   */
  public function getAvailableFormats($type = FALSE){
    return [
      '3g2' => '3G2',
      '3gp' => '3GP',
      '3gp2' => '3GP2',
      '3gpp' => '3GPP',
      '3gpp2' => '3GPP2',
      'aac' => 'AAC',
      'f4a' => 'F4A',
      'f4b' => 'F4B',
      'f4v' => 'F4V',
      'flv' => 'FLV',
      'm4a' => 'M4A',
      'm4b' => 'M4B',
      'm4r' => 'M4R',
      'm4v' => 'M4V',
      'mov' => 'MOV',
      'mp3' => 'MP3',
      'mp4' => 'MP4',
      'oga' => 'OGA',
      'ogg' => 'OGG',
      'ogv' => 'OGV',
      'ogx' => 'OGX',
      'ts' => 'TS',
      'webm' => 'WebM',
      'wma' => 'WMA',
      'wmv' => 'WMV',
    ];
  }
  
  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container, array $configuration, $plugin_id, $plugin_definition) {
    return new static($configuration, $container->get('http_client'));
  }
  
}