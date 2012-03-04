<?php
/**
 * @file
 * Transcoder Abstract Factory classes
 */

/**
 * Interface for transocder classes
 */
interface TranscoderFactoryInterface {

  public function extractFrames();

  public function execute();

  public function getName();

  public function isAvailable(&$errormsg);
}

/**
 * Abstract class for the transcoder classes to keep common methods
 */
abstract class TranscoderAbstractionFactory {

  protected $settings = array();
  protected $errors = array();

  protected function __construct() {
  }

  public function isAvailable(&$errormsg) {
    return TRUE;
  }

  /**
   * Set Input file to add input file in to settings variable
   */
  public function setInput($file, $input_frame_rate=0, $validate_decode_codecs = TRUE) {
    // @TODO : do some validation to check the file exists;
    if (!empty($file)) {
      if (empty($file['uri']))
        $file += (array) file_load($file['fid']);
      $this->settings['input'] = $file;
    }
    else
      drupal_set_message(t('Video file not found.'), 'error');
  }

  /**
   * Set options is to set transcoding settings before send to the transcoder.
   */
  public function setOptions($options = NULL) {
    if (is_array($options)) {
      foreach ($options as $key => $value) {
        $this->settings[$key] = $value;
      }
    }
  }

  /**
   * Set output file for transcoding, this would be the result file.
   */
  public function setOutput($output_directory, $output_name, $overwrite_mode = FILE_EXISTS_REPLACE) {
    // @TODO : do some validation to check the file exists
    if (count($output_directory) == 1)
      $this->settings['base_url'] = $output_directory;
    if (count($output_name) == 1)
      $this->settings['filename'] = $output_name;
    else
      $this->errors['output'] = 'Output file not found.';
  }

  /**
   * Get enabled and supporting codecs by the transcoder.
   */
  public function getCodecs() {
    $codecs = array(
      'encode' => array(
        'video' => array(
          'h264' => 'H.264 (default)',
          'vp8' => 'VP8',
          'theora' => 'Theora',
          'vp6' => 'VP6',
          'mpeg4' => 'MPEG-4',
          'wmv' => 'WMV'
        ),
        'audio' => array(
          'aac' => 'AAC (default for most cases)',
          'mp3' => 'MP3',
          'vorbis' => 'Vorbis (default for VP8 and Theora)',
          'wma' => 'WMA'
        )
      ),
      'decoding' => array()
    );
    return $codecs;
  }

  /**
   * Get available output file formats from the transcoder.
   */
  public function getAvailableFormats($type = FALSE) {
    return array(
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
      'wmv' => 'WMV'
    );
  }

  /**
   * Get the installed transcoder version.
   */
  public function getVersion() {
    return '1.0';
  }

  /**
   * Get file informations
   * @return
   *   Associative array with file informations like duration, dimensions
   */
  public function getFileInfo() {
    $file = array();
    $file['duration']['seconds'] = 100;
    $file['video']['dimensions'] = array('width' => 640, 'height' => 350);
    return $file;
  }

  /**
   * Get errors
   */
  public function getErrors() {
    return $this->errors;
  }

  /**
   * Check for errors if any
   */
  public function checkErrors() {
    return !empty($this->errors);
  }

  /**
   * Admin settings form for the transcoder
   */
  abstract public function adminSettings();

  /**
   * Validate admin settings. This will call when Drupal admin settings validate.
   */
  abstract public function adminSettingsValidate($form, &$form_state);

  /**
   * Create new transcoder job on database.
   */
  public function createJob($video, $entity_id, $entity_type = 'node') {
    return db_insert('video_queue')
            ->fields(array(
              'fid' => $video['fid'],
              'entity_id' => $entity_id,
              'entity_type' => $entity_type,
              'status' => VIDEO_RENDERING_PENDING,
              'dimensions' => $video['dimensions'],
            ))
            ->execute();
  }

  /**
   * Delete transcoder job and its details from database.
   */
  public function deleteJob($video) {
    $video = (object) $video;
    if (!$video = $this->loadJob())
      return;
    // converted output values
    $converted = unserialize($video->data);
    if (!empty($converted)) {
      foreach ($converted as $file) {
        if (file_exists(drupal_realpath($file->uri)))
          @drupal_unlink($file->uri);
      }
    }
    // now delete our rows.
    db_delete('video_queue')
        ->condition('fid', $video->fid)
        ->execute();
    // now delete our rows.
    db_delete('video_output')
        ->condition('original_fid', $video->fid)
        ->execute();
  }

  /**
   * Load transcoding job from the database.
   */
  public function loadJob() {
    $file = $this->settings['input'];
    $job = NULL;
    $job = db_query("SELECT f.*, vf.vid, vf.entity_id, vf.entity_type, vf.dimensions, vf.data, vf.status as video_status
      FROM {video_queue} vf LEFT JOIN {file_managed} f ON vf.fid = f.fid WHERE f.fid=vf.fid AND f.fid = :fid", array(':fid' => $file['fid']))
        ->fetch();
    if (!empty($job))
      return $job;
    else
      return FALSE;
  }

  /**
   * Load output from a transcoding job from the database.
   */
  public function loadOutput() {
    $fids = db_select('video_output', 'vo')
      ->fields('vo', array('output_fid'))
      ->condition('vo.original_fid', $this->settings['input']['fid'])
      ->execute()->fetchAllKeyed(0, 0);
    $files = file_load_multiple($fids);

    $videofiles = new stdClass();
    foreach ($files as $file) {
      $extension = drupal_strtolower(pathinfo($file->filename, PATHINFO_EXTENSION));
      $videofiles->{$extension} = $file;
    }
    return $videofiles;
  }

  /**
   * Select videos from our queue
   */
  public function loadJobQueue() {
    $total_videos = variable_get('video_ffmpeg_instances', 5);
    $videos = array();
    $result = db_query_range('SELECT f.*, vf.vid, vf.entity_id, vf.entity_type, vf.dimensions, vf.status as video_status
      FROM {video_queue} vf LEFT JOIN {file_managed} f ON vf.fid = f.fid
      WHERE vf.status = :vstatus AND f.status = :fstatus ORDER BY f.timestamp', 0, $total_videos, array(':vstatus' => VIDEO_RENDERING_PENDING, ':fstatus' => FILE_STATUS_PERMANENT));
    foreach ($result as $row) {
      $videos[] = $row;
    }
    return $videos;
  }

  /**
   * Process postback jobs
   */
  public function processPostback() {
  }
}

interface TranscoderAbstractFactoryInterface {
  public static function getProduct();
}

class TranscoderAbstractionAbstractFactory implements TranscoderAbstractFactoryInterface {
  public static function getProduct($transcoder = NULL) {
    // get our configured transcoder.
    if (!isset($transcoder))
      $transcoder = variable_get('video_convertor', 'TranscoderAbstractionFactoryZencoder');
    if (!module_load_include('inc', 'video', '/transcoders/' . $transcoder)) {
      $modules = module_list();
      $files = array();
      foreach ($modules as $module) {
        $module_path = drupal_get_path('module', $module) . '/transcoders';
        $inc_files = file_scan_directory($module_path, '/.*\.inc/');
        if (!empty($inc_files))
          $files[$module] = $inc_files;
      }
      foreach ($files as $module => $_files) {
        foreach ($_files as $file) {
          if ($file->name == $transcoder)
            module_load_include('inc', $module, '/transcoders/' . $file->name);
        }
      }
    }
    if (class_exists($transcoder)) {
      return new $transcoder;
    }
    else {
      drupal_set_message(t('The transcoder is not configured properly.'), 'error');
    }
  }
}
