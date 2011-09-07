<?php

/**
 * @file
 * Transcoder Abstract Factory classes
 */
interface TranscoderFactoryInterface {
  public function extractFrames();

  public function execute();

  public function getName();

  public function getValue();
}

abstract class TranscoderAbstractionFactory {

  protected $settings = array();
  protected $errors = array();

  protected function __construct() {
    
  }

  public function setInput($file, $input_frame_rate=0, $validate_decode_codecs = TRUE) {
    // @TODO : do some validation to check the file exists;
    if (!empty($file))
      $this->settings['input'] = $file;
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

  public function getDimensions() {
    $res['width'] = 640;
    $res['height'] = 350;
    return $res;
  }

  abstract public function adminSettings();

  abstract public function adminSettingsValidate($form, &$form_state);
}

class TranscoderAbstractionFactoryFfmpeg extends TranscoderAbstractionFactory implements TranscoderFactoryInterface {

  protected $transcoder = array();

  public function __construct() {
    module_load_include('php', 'video', 'libraries/phpvideotoolkit/phpvideotoolkit.php5');
    $this->transcoder = new PHPVideoToolkit(variable_get('video_ffmpeg_path', '/usr/bin/ffmpeg'), file_directory_temp() . '/');
    parent::__construct();
  }

  public function setInput($file, $input_frame_rate=0, $validate_decode_codecs = TRUE) {
    parent::setInput($file, $input_frame_rate = 0, $validate_decode_codecs = TRUE);
    $this->transcoder->setInputFile(drupal_realpath($file['uri']));
  }

  public function setOptions($options = NULL) {
    parent::setOptions();
  }

  public function setOutput($output_directory, $output_name, $overwrite_mode = FILE_EXISTS_REPLACE) {
    $this->transcoder->setOutput($output_directory, $output_name);
  }

  public function extractFrames() {
    global $user;
    // Get the file system directory.
    $schema_thumb_path = file_uri_scheme($this->settings['input']['uri']) . '://' . variable_get('video_thumbnail_path', 'videos/thumbnails') . '/' . $this->settings['input']['fid'];
    file_prepare_directory($schema_thumb_path, FILE_CREATE_DIRECTORY);
    // get the video file informations
    $file_info = $this->getFileInfo();
    // video duration
    $duration = $file_info['duration']['seconds'];
    // Total thumbs to generate
    $no_of_thumbnails = variable_get('video_thumbnail_count', 5);
    $files = NULL;
    for ($i = 1; $i <= $no_of_thumbnails; $i++) {
      $seek = ($duration / $no_of_thumbnails) * $i - 1;  //adding minus one to prevent seek times equaling the last second of the video
      $filename = file_munge_filename("video-thumbnail-" . $this->settings['input']['fid'] . "-$i.jpg", '', TRUE);
      $thumbnail_path = $schema_thumb_path . '/' . $filename;
      $thumbnail_real_path = drupal_realpath($thumbnail_path);
      //skip files already exists, this will save ffmpeg traffic
      if (!is_file($thumbnail_real_path)) {
        // if FFmpeg-php library installed use it
        if (class_exists('ffmpeg_movie')) {
          $movie = new ffmpeg_movie(drupal_realpath($this->settings['input']['uri']));
          $frames = $movie->getFrameCount();
          $fps = $movie->getFrameRate();
          // Get the right frame number
          $framenumber = (int) $seek * $fps;
          if ($framenumber > $frames) {
            $framenumber = $frames;
          }
          // Get the frame and create thumb file
          $frame = $movie->getFrame($framenumber);
          $thumb = $frame->toGDImage();
          // @todo use image_save() function for this.
          imagejpeg($thumb, $thumbnail_real_path);
        }
        else {
          // create thuumbnail file
          $this->transcoder->extractFrame($seek);

          $result = $this->transcoder->setOutput(drupal_realpath($schema_thumb_path) . '/', $filename, PHPVideoToolkit::OVERWRITE_EXISTING);
          if (!$result) {
            //if there was an error then get it 
            $error_msg = t($this->transcoder->getLastError());
            watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
            $this->transcoder->reset();
            continue;
          }
          // @todo think about how to move execute() method to parent class
          $result = $this->transcoder->execute(FALSE, TRUE);
          if ($result !== PHPVideoToolkit::RESULT_OK) {
            //if there was an error then get it
            $error_msg = t($this->transcoder->getLastError());
            watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
            $this->transcoder->reset();
            continue;
          }
        }

        if (!file_exists($thumbnail_real_path)) {
          $error_param = array('%file' => $thumbnail_path);
          $error_msg = t("Error generating thumbnail for video: generated file %file does not exist.", $error_param);
          // Log the error message.
          watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
          continue;
        }
      }
      // Begin building the file object.
      $file = new stdClass();
      $file->uid = $user->uid;
      $file->status = 0;
      $file->filename = trim($filename);
      $file->uri = $thumbnail_path;
      $file->filemime = file_get_mimetype($filename);
      $file->filesize = filesize($thumbnail_real_path);
      $file->timestamp = time();
      $files[] = $file;
    }
    return $files;
  }

  public function execute() {
    
  }

  protected function getFileInfo() {
    //@todo Add to cache
    $file = $this->settings['input']['uri'];
    $cid = 'video:file:' . md5($file);
    $cache = cache_get($cid);
    if (!empty($cache->data)) {
      return $cache->data;
    }
    else {
      $data = $this->transcoder->getFileInfo(drupal_realpath($file));
      cache_set($cid, $data);
      return $data;
    }
  }

  public function getCodecs() {
    $cid = 'video:transcoder:ffmpeg';
    $cache = cache_get($cid);
    if (!empty($cache->data)) {
      $info = $cache->data;
    }
    else {
      $info = $this->transcoder->getFFmpegInfo(FALSE);
      cache_set($cid, $info);
    }
    $available_codecs = $info['codecs'];
    $codecs = array('decode' => array(), 'encode' => array());
    foreach ($available_codecs as $key => $value) {
      $codecs['encode'][$key] = array();
      $codecs['decode'][$key] = array();
      foreach ($value as $codec_key => $codec) {
        if ($codec['encode']) {
          $codecs['encode'][$key][$codec_key] = $codec['fullname'];
        }
        if ($codec['decode']) {
          $codecs['decode'][$key][$codec_key] = $codec['fullname'];
        }
      }
    }
    return $codecs;
  }

  public function getAvailableFormats($type = false) {
    $cid = 'video:transcoder:ffmpeg';
    $cache = cache_get($cid);
    if (!empty($cache->data)) {
      $info = $cache->data;
    }
    else {
      $info = $this->transcoder->getFFmpegInfo(FALSE);
      cache_set($cid, $info);
    }
    $formats = array();
    switch ($type) {
      case false :
        return array_keys($info['formats']);
      case 'both' :
        foreach ($info['formats'] as $id => $data) {
          if ($data['mux'] === true && $data['demux'] === true) {
            $formats[$id] = $data['fullname'];
          }
        }
        break;
      case 'muxing' :
        foreach ($info['formats'] as $id => $data) {
          if ($data['mux'] === true) {
            $formats[$id] = $data['fullname'];
          }
        }
        break;
      case 'demuxing' :
        foreach ($info['formats'] as $id => $data) {
          if ($data['demux'] === true) {
            $formats[$id] = $data['fullname'];
          }
        }
        break;
    }
    return $formats;
  }

  public function getVersion() {
    $cid = 'video:transcoder:ffmpeg';
    $cache = cache_get($cid);
    if (!empty($cache->data)) {
      $info = $cache->data;
    }
    else {
      $info = $this->transcoder->getFFmpegInfo(FALSE);
      cache_set($cid, $info);
    }
    if ($info['ffmpeg-found'] == TRUE) {
      return $info['binary']['versions'];
    }
    else
      return FALSE;
  }

  public function getName() {
    return 'FFmpeg';
  }

  public function getValue() {
    return 'video_ffmpeg';
  }

  public function getDimensions() {
    $video_info = $this->getFileInfo();
    $res = array('width' => 0, 'height' => 0);
    // Get dimensions
    $res['width'] = $video_info['video']['dimensions']['width'] ? $video_info['video']['dimensions']['width'] : NULL;
    $res['height'] = $video_info['video']['dimensions']['height'] ? $video_info['video']['dimensions']['height'] : NULL;
    return $res;
  }

  public function adminSettings() {
    $form = array();
    $form = array();
    $form['video_ffmpeg_start'] = array(
      '#type' => 'markup',
      '#markup' => '<div id="video_ffmpeg">',
    );
    // FFMPEG
    $form['ffmpeg']['ffmpeg'] = array(
      '#type' => 'fieldset',
      '#title' => t('Path to FFmpeg executable'),
      '#collapsible' => FALSE,
      '#collapsed' => FALSE
    );
    $form['ffmpeg']['ffmpeg']['video_ffmpeg_path'] = array(
      '#type' => 'textfield',
      '#title' => t('FFMPEG'),
      '#description' => t('Absolute path to ffmpeg executable.'),
      '#default_value' => variable_get('video_ffmpeg_path', '/usr/bin/ffmpeg'),
    );
    // Thumbnail Videos We need to put this stuff last.
    $form['ffmpeg']['thumbnail'] = array(
      '#type' => 'fieldset',
      '#title' => t('Video thumbnails'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );
    $form['ffmpeg']['thumbnail']['video_thumbnail_path'] = array(
      '#type' => 'textfield',
      '#title' => t('Path to save thumbnails'),
      '#description' => t('Path to save video thumbnails extracted from the videos.'),
      '#default_value' => variable_get('video_thumbnail_path', 'videos/thumbnails'),
    );
    $form['ffmpeg']['thumbnail']['video_thumbnail_count'] = array(
      '#type' => 'textfield',
      '#title' => t('Number of thumbnails'),
      '#description' => t('Number of thumbnails to extract from video.'),
      '#default_value' => variable_get('video_thumbnail_count', 5),
    );
    $form['ffmpeg']['thumbnail']['video_thumb_save_all'] = array(
      '#type' => 'checkbox',
      '#title' => t('Save all thumbnails in {file_manged} table'),
      '#description' => t('Save all auto created thumbnails to the {file_managed} table. Change file status as PERMANENT'),
      '#default_value' => variable_get('video_thumb_save_all', FALSE),
    );
    $form['video_ffmpeg_end'] = array(
      '#type' => 'markup',
      '#markup' => '</div>',
    );
    return $form;
  }

  public function adminSettingsValidate($form, &$form_state) {
    $ffmpeg_path = $form_state['values']['video_ffmpeg_path'];
    if (!empty($ffmpeg_path)) {
      // we have to initialize PHP toolkit object again, if changed the path of executable
      $toolkit = new PHPVideoToolkit($ffmpeg_path);
      $ffmpeg = $toolkit->getFFmpegInfo(FALSE);
      if ($ffmpeg['ffmpeg-found'] == TRUE) {
        drupal_set_message(t('FFmpeg version !version found on your system.', array('!version' => $ffmpeg['binary']['versions'])), 'status');
      }
      else {
        drupal_set_message(t('FFmpeg not found on your system. To convert 
          videos and create thumbnails you have to install FFmpeg on your server. 
          For more informations please see the !documentation.', array('!documentation' => l('documentation', 'http://video.heidisoft.com/documentation/ffmpeg-installtion-scripts'))), 'error');
        form_set_error('video_ffmpeg_path' . t('FFmpeg not found on your system. To convert 
          videos and create thumbnails you have to install FFmpeg on your server. 
          For more informations please see the !documentation.', array('!documentation' => l('documentation', 'http://video.heidisoft.com/documentation/ffmpeg-installtion-scripts'))));
      }
    }
    return;
  }

}

class TranscoderAbstractionFactoryZencoder extends TranscoderAbstractionFactory implements TranscoderFactoryInterface {

  public function __construct() {
    parent::__construct();
  }

  public function setInput($file, $input_frame_rate=0, $validate_decode_codecs = TRUE) {
    parent::setInput();
  }

  public function setOptions($options = NULL) {
    parent::setOptions();
  }

  public function setOutput($output_directory, $output_name, $overwrite_mode = FILE_EXISTS_REPLACE) {
    parent::setOutput();
  }

  public function extractFrames() {
    
  }

  public function execute() {
    
  }

  public function getName() {
    return 'Zencoder';
  }

  public function getValue() {
    return 'video_zencoder';
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
