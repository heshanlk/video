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
  public function getOutputFiles();
  public function getVideoThumbnails();
  public function createJob();
  public function getJobDetails();
  public function getJobProgress();
  public function getVersion();
}