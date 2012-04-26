<?php
/**
 * @file
 * This file will be used to keep all utility functions data structures.
 */

/**
 * Helper functions for the Video module.
 *
 * These functions are put in a utility class so they will only be
 * auto-loaded when needed.
 */
class video_utility {
  /**
   * Retrieve a single entity
   */
  public static function loadEntity($type, $id) {
    $entities = entity_load($type, array($id));
    if (empty($entities)) {
      return NULL;
    }

    return reset($entities);
  }

  /**
   * Finds a stream wrapper using the output URL.
   *
   * Examples of output URLs:
   * - http://bucket.s3.amazonaws.com/dir/file.ext
   *   output: s3://file.ext
   * - http://yoursite/sites/yoursite/files/dir/file.ext
   *   output: public://dir/file.ext
   *
   * If no suitable stream wrapper is found, returns NULL.
   *
   * @todo optimize: find the wrapperbase -> wrapper mapping just once.
   */
  public static function urlToUri($url) {
    $uri = NULL;

    $mapping = self::getStreamBaseMapping();
    foreach ($mapping as $wrapperscheme => $wrapperbase) {
      $wrapperbaselength = drupal_strlen($wrapperbase);

      if (strncmp($wrapperbase, $url, $wrapperbaselength) === 0) {
        $uri = $wrapperscheme . '://' . drupal_substr($url, $wrapperbaselength);
        break;
      }
    }

    return $uri;
  }

  private static $streamBaseMapping = NULL;
  private static function getStreamBaseMapping() {
    if (self::$streamBaseMapping != NULL) {
      return self::$streamBaseMapping;
    }

    self::$streamBaseMapping = array();

    foreach (file_get_stream_wrappers() as $wrapperscheme => $wrapperinfo) {
      $wrapper = file_stream_wrapper_get_instance_by_scheme($wrapperscheme);
      self::$streamBaseMapping[$wrapperscheme] = $wrapper->getExternalUrl();
    }

    return self::$streamBaseMapping;
  }

  public static function getExtension($filename) {
    return pathinfo($filename, PATHINFO_EXTENSION);
  }

  public static function imageSave($resource, $path, $type) {
    switch ($type) {
      case 'png':
        return imagepng($resource, $path);

      case 'jpg':
        return imagejpeg($resource, $path);
    }

    throw new Exception('Unknown type "' . $type . '"');
  }

  private static $tmpfilesdirs;
  public static function createTempFile($extension = '') {
    if (empty(self::$tmpfilesdirs)) {
      self::$tmpfilesdirs = array();
      drupal_register_shutdown_function(array('video_utility', 'cleanTemporaryFilesDirs'));
    }

    $tmpnam = tempnam(file_directory_temp(), 'drupal-video-');

    if (!empty($extension)) {
      drupal_unlink($tmpnam);
      $tmpnam = $tmpnam . '.' . $extension;
    }

    self::$tmpfilesdirs[] = $tmpnam;
    return $tmpnam;
  }

  public static function createTempDir() {
    $dirname = self::createTempFile();
    drupal_unlink($dirname);
    drupal_mkdir($dirname);
    return $dirname;
  }

  public static function cleanTemporaryFilesDirs() {
    if (!empty(self::$tmpfilesdirs)) {
      foreach (self::$tmpfilesdirs as $tmpfiledir) {
        self::unlink($tmpfiledir);
      }
    }
  }

  private static $temporaryCopies;
  public static function createTemporaryLocalCopy($uri) {
    if (empty(self::$temporaryCopies)) {
      self::$temporaryCopies = array();
    }
    elseif (isset(self::$temporaryCopies[$uri])) {
      return self::$temporaryCopies[$uri];
    }

    // Try to fetch the file using http. This is much faster in the case of S3 via the Amazon S3 module
    $url = file_create_url($uri);
    if ($url != $uri && ($copy = self::createTemporaryLocalCopy($url))) {
      self::$temporaryCopies[$uri] = $copy;
      return $copy;
    }

    $copy = self::createTempFile(self::getExtension($uri));
    if (copy($uri, $copy)) {
      self::$temporaryCopies[$uri] = $copy;
      return $copy;
    }

    return FALSE;
  }

  public static function unlink($uri) {
    if (is_file($uri)) {
      drupal_unlink($uri);
    }
    elseif (is_dir($uri)) {
      $contents = scandir($uri);
      foreach ($objs as $obj) {
        if ($obj != '.' && $obj != '..') {
          self::unlink($uri . '/' . $obj);
        }
      }
      drupal_rmdir($uri);
    }
  }

  public static function objectToArray($data) {
    if (is_array($data) || is_object($data)) {
      $result = array();
      foreach ($data as $key => $value) {
        $result[$key] = self::objectToArray($value);
      }
      return $result;
    }
    return $data;
  }

  public static function validateDimensionList($str) {
    return preg_match('#^([1-9]\d{0,3}x[1-9]\d{0,3})(\n[1-9]\d{0,3}x[1-9]\d{0,3})*$#', $str) > 0;
  }

  public static function getDimensions() {
    $dims = explode("\n", str_replace("\r", '', variable_get('video_metadata_dimensions', self::getDefaultDimensions())));
    return array_combine($dims, $dims);
  }

  public static function getDefaultDimensions() {
    return "176x144\n352x288\n704x576\n1408x1152\n128x96\n160x120\n320x240\n640x480\n800x600\n1024x768\n1600x1200\n2048x1024\n1280x1024\n2560x2048\n5120x4096\n852x480\n1366x768\n1600x1024\n1920x1200\n2560x1600\n3200x2048\n3840x2400\n6400x4096\n7680x4800\n320x200\n640x350\n852x480\n1280x720\n1920x1080\n640x360\n320x180";
  }

  /**
   * Return our list of video extensions and their associated player.
   */
  public static function getVideoExtensions() {
    return array(
      'divx' => 'video_play_divx',
      'mkv' => 'video_play_divx',
      'mov' => 'video_play_quicktime',
      '3gp' => 'video_play_quicktime',
      '3g2' => 'video_play_quicktime',
      'mp4' => 'video_play_quicktime',
      'm4v' => 'video_play_quicktime',
      'rm' => 'video_play_realmedia',
      'f4v' => 'video_play_flv',
      'flv' => 'video_play_flv',
      'swf' => 'video_play_flash',
      'dir' => 'video_play_dcr',
      'dcr' => 'video_play_dcr',
      'asf' => 'video_play_windowsmedia',
      'wmv' => 'video_play_windowsmedia',
      'avi' => 'video_play_windowsmedia',
      'mpg' => 'video_play_windowsmedia',
      'mpeg' => 'video_play_windowsmedia',
      'ogg' => 'video_play_theora',
      'ogv' => 'video_play_theora',
      'webm' => 'video_play_theora'
    );
  }

  /**
   * Return our supported video players.
   */
  public static function getVideoPlayers() {
    return array(
      'video_play_html5' => t('HTML5 Player'),
      'video_play_divx' => t('Divx Player'),
      'video_play_quicktime' => t('Quicktime'),
      'video_play_realmedia' => t('Real Media Player'),
      'video_play_flv' => t('FLV Flash Players'),
      'video_play_flash' => t('SWF Flash Player'),
      'video_play_dcr' => t('Director/Shockwave'),
      'video_play_windowsmedia' => t('Windows Media Player'),
      'video_play_theora' => t('Theora Player'),
    );
  }
}
