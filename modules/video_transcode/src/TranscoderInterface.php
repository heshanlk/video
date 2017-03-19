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
   * Get the transcoded files.
   *
   * @return array
   *   Array of transcoded files.
   */
  public function getOutputFiles();
  
  /**
   * Get the video thumbnails.
   *
   * @return array
   *   Array of video thumbnails.
   */
  public function getVideoThumbnails();
  
  /**
   * Execute commands or create the video transcoding job.
   *
   * @return boolean
   *   true on success and false on failed.
   */
  public function createJob();
  
  /**
   * Cancel an active video transcoding job.
   *
   * @return boolean
   *   true on success and false on failed.
   */
  public function createJob();
  
  /**
   * Get the current transocde job details.
   *
   * @return array
   *   Current transcoding job details.
   */
  public function getJobDetails();
  
  /**
   * Get the current transocde job progress.
   *
   * @return array
   *   Current transcoding job progress.
   */
  public function getJobProgress();
  
  /**
   * Handle the transcoding job post processing callbacks.
   *
   * @return boolean
   *   true on success and false on failed.
   */
  public function processCallback();
  
  /**
   * Get the current version.
   *
   * @return string
   *  Get the current version.
   */
  public function getVersion();
}