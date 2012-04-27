<?php
/**
 * @file
 * Transcoder Abstract Factory classes
 */

/**
 * Interface for transocder classes
 */
interface TranscoderFactoryInterface {
  /**
   * Extract frames from the current video.
   *
   * The thumbnails are be saved to the given scheme. The files are not
   * be saved to the database, this will be done by the caller.
   * The uid, filesize and timestamp properties are not set by this method.
   *
   * @param $destinationScheme
   *   A valid stream wrapper scheme
   * @param $format
   *   png or jpg
   *
   * @return
   *   array of file objects, FALSE on error
   */
  public function extractFrames($destinationScheme, $format);

  public function execute();

  public function getName();

  public function getValue();

  public function isAvailable(&$errormsg);

  public function getDimensions();
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
  public function setOptions(array $options) {
    foreach ($options as $key => $value) {
      $this->settings[$key] = $value;
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
    return NULL;
  }

  public function getDimensions() {
    $i = $this->getFileInfo();

    if (empty($i) || empty($i['video']['dimensions']['width']) || empty($i['video']['dimensions']['height'])) {
      return NULL;
    }

    return array(
      'width' => intval($i['video']['dimensions']['width']),
      'height' => intval($i['video']['dimensions']['height']),
    );
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
   * Create new transcoder job in the database.
   */
  public function createJob($fid, $dimensions, $entity_id, $entity_type, $field_name, $langcode, $delta) {
    $video = new stdClass();
    $video->fid = intval($fid);
    $video->entity_id = intval($entity_id);
    $video->entity_type = $entity_type;
    $video->status = VIDEO_RENDERING_PENDING;
    $video->dimensions = $dimensions;
    $video->data = array(
      'field_name' => $field_name,
      'langcode' => $langcode,
      'delta' => intval($delta),
    );

    return drupal_write_record('video_queue', $video) === SAVED_NEW ? $video : FALSE;
  }

  /**
   * Delete transcoder job and its details from database.
   *
   * @todo improve this method: not everything is deleted.
   */
  public function deleteJob() {
    if (!$video = $this->loadJob()) {
      return FALSE;
    }

    db_delete('video_queue')->condition('fid', $video->fid)->execute();
    db_delete('video_output')->condition('original_fid', $video->fid)->execute();
    return TRUE;
  }

  /**
   * Load transcoding job from the database.
   */
  public function loadJob($id = NULL, $field = 'fid') {
    if ($id == NULL) {
      $id = $this->settings['input']['fid'];
    }

    $job = db_query('SELECT vf.*, f.*, vf.status as video_status FROM {video_queue} vf LEFT JOIN {file_managed} f ON vf.fid = f.fid WHERE f.fid=vf.fid AND vf.' . $field . ' = :id', array(':id' => $id))->fetch();
    if (empty($job)) {
      return FALSE;
    }

    $job->data = empty($job->data) ? NULL : unserialize($job->data);

    return $job;
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
    $result = db_query_range('SELECT vf.*, f.*, vf.status as video_status FROM {video_queue} vf LEFT JOIN {file_managed} f ON vf.fid = f.fid WHERE vf.status = :vstatus AND f.status = :fstatus ORDER BY f.timestamp', 0, $total_videos, array(':vstatus' => VIDEO_RENDERING_PENDING, ':fstatus' => FILE_STATUS_PERMANENT));
    foreach ($result as $row) {
      $row->data = empty($row->data) ? NULL : unserialize($row->data);
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
  /**
   * @return
   *   TranscoderFactoryInterface
   */
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
