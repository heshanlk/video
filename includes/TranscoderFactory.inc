<?php

/**
 * @file
 * Transcoder Abstract Factory classes
 */
interface TranscoderFactoryInterface {

  public function extractFrames();

  public function execute();

  public function getName();
}

abstract class TranscoderAbstractionFactory {

  protected $settings = array();
  protected $errors = array();

  protected function __construct() {
    
  }

  public function setInput($file, $input_frame_rate=0, $validate_decode_codecs = TRUE) {
    // @TODO : do some validation to check the file exists;
    if (!empty($file)) {
      if (empty($file['uri']))
        $file += (array) file_load($file['fid']);
      $this->settings['input'] = $file;
    }
    else
      drupal_set_message('Video file not found!', 'error');
  }

  public function setOptions($options = NULL) {
    if (is_array($options)) {
      foreach ($options as $key => $value) {
        $this->settings[$key] = $value;
      }
    }
  }

  public function setOutput($output_directory, $output_name, $overwrite_mode = FILE_EXISTS_REPLACE) {
    // @TODO : do some validation to check the file exists
    if (count($output_directory) == 1)
      $this->settings['base_url'] = $output_directory;
    if (count($output_name) == 1)
      $this->settings['filename'] = $output_name;
    else
      $this->errors['output'] = 'Output file not found.';
  }

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

  public function getAvailableFormats($type = false) {
    return array('FLV');
  }

  public function getVersion() {
    return '1.0';
  }

  public function getFileInfo() {
    $file = array();
    $file['duration']['seconds'] = 100;
    $file['video']['dimensions'] = array('width' => 640, 'height' => 350);
    return $res;
  }

  abstract public function adminSettings();

  abstract public function adminSettingsValidate($form, &$form_state);

  public function createJob($video, $nid) {
    return db_insert('video_queue')
            ->fields(array(
              'fid' => $video['fid'],
              'nid' => $nid,
              'status' => VIDEO_RENDERING_PENDING,
              'dimensions' => $video['dimensions'],
            ))
            ->execute();
  }

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
    //now delete our rows.
    db_delete('video_queue')
        ->condition('fid', $video->fid)
        ->execute();
    //now delete our rows.
    db_delete('video_output')
        ->condition('original_fid', $video->fid)
        ->execute();
  }

  public function loadJob() {
    $file = $this->settings['input'];
    $job = null;
    $job = db_query("SELECT f.*, vf.vid, vf.nid, vf.dimensions, vf.data, vf.status as video_status
      FROM {video_queue} vf LEFT JOIN {file_managed} f ON vf.fid = f.fid WHERE f.fid=vf.fid AND f.fid = :fid", array(':fid' => $file['fid']))
        ->fetch();
    if (!empty($job))
      return $job;
    else
      return FALSE;
  }

  public function loadCompletedJob() {
    $file = $this->settings['input'];
    $output = null;
    $output = db_query("SELECT vo.output_fid FROM {video_output} vo WHERE vo.original_fid = :fid", array(':fid' => $file['fid']))
        ->fetch();
    foreach ($output as $fid) {
      $file = file_load($fid);
      $extension = pathinfo(drupal_realpath($file->uri), PATHINFO_EXTENSION);
      $video->{$extension} = $file;
    }
    return $video;
  }

}

interface TranscoderAbstractFactoryInterface {

  public static function getProduct();
}

class TranscoderAbstractionAbstractFactory implements TranscoderAbstractFactoryInterface {

  public static function getProduct($transcoder = NULL) {
    //get our configured transcoder.
    if (!isset($transcoder))
      $transcoder = variable_get('video_convertor', 'video_ffmpeg');
    if (!module_load_include('inc', 'video', '/transcoders/' . $transcoder)) {
      $modules = module_list();
      $files = array();
      foreach ($modules as $module) {
        $module_path = drupal_get_path('module', $module) . '/transcoders';
        $inc_files = file_scan_directory($module_path, '/.*\.inc/');
        if (!empty($inc_files))
          $files[$module] = $inc_files;
      }
      // @TODO : add lazy load
      foreach ($files as $module => $_files) {
        foreach ($_files as $file) {
          if ($file->name == $transcoder)
            module_load_include('inc', $module, '/transcoders/' . $file->name);
        }
      }
    }
    if (class_exists($transcoder)) {
//      $factory = new $transcoder;
      $factory = new TranscoderAbstractionFactoryFfmpeg();
      return $factory;
    }
    else {
      drupal_set_message(t('The transcoder is not configured properly.'), 'error');
    }
  }

}

?>
