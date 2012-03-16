<?php
/**
 * @file
 * File containing class TranscoderAbstractionFactoryFfmpeg
 */

/**
 * Class that handles FFmpeg transcoding.
 */
class TranscoderAbstractionFactoryFfmpeg extends TranscoderAbstractionFactory implements TranscoderFactoryInterface {
  const INFO_CID = 'video:transcoder:ffmpeg';
  const INFO_CACHE = 'cache';

  /**
   * @var PHPVideoToolkit
   */
  protected $transcoder = NULL;

  public function __construct() {
    module_load_include('php', 'video', 'libraries/phpvideotoolkit/phpvideotoolkit.php5');
    $this->transcoder = new PHPVideoToolkit(variable_get('video_ffmpeg_path', '/usr/bin/ffmpeg'), file_directory_temp() . '/');
    PHPVideoToolkit::$ffmpeg_info = $this->getCachedFFmpegInfo();
    parent::__construct();
  }

  public function setInput($file, $input_frame_rate=0, $validate_decode_codecs = TRUE) {
    parent::setInput($file, $input_frame_rate = 0, $validate_decode_codecs = TRUE);
    $result = $this->transcoder->setInputFile(drupal_realpath($this->settings['input']['uri']));
    if ($result !== PHPVideoToolkit::RESULT_OK) {
      watchdog('transcoder', 'Error set options @message', array('@message' => $this->transcoder->getLastError()), WATCHDOG_ERROR);
      $this->errors['input'] = $this->transcoder->getLastError();
      $this->transcoder->reset();
      return FALSE;
    }
  }

  public function setOptions($options = NULL) {
    // Reset the transcoder class keeping the input file info
    $this->transcoder->reset(true);

    $video_info = $this->getFileInfo();
    foreach ($options as $key => $value) {
      if (empty($value) || $value === 'none')
        continue;
      $result = TRUE;
      switch ($key) {
        case 'max_frame_rate':
          $result = $this->transcoder->setVideoFrameRate($value);
          break;
        case 'video_codec':
          $result = $this->transcoder->setVideoCodec($value);
          break;
        case 'video_preset':
          $result = $this->transcoder->setVideoPreset($value);
          break;
        case 'audio_sample_rate':
          $value = (!empty($value)) ? $value : $video_info['audio']['sample_rate'];
          if ($value < 1000) {
            $value *= 1000;
          }
          $value = min($value, 44100);
          $result = $this->transcoder->setAudioSampleFrequency($value);
          break;
        case 'audio_codec':
          $result = $this->transcoder->setAudioCodec($value);
          break;
        case 'audio_channels':
          $result = $this->transcoder->setAudioChannels($value);
          break;
        case 'audio_bitrate':
          if (empty($value)) {
            $value = 64;
          }

          if ($value < 1000) {
            $value *= 1000;
          }
          $result = $this->transcoder->setAudioBitRate($value);
          break;
        case 'video_bitrate':
          if (empty($value)) {
            $value = 200;
          }

          $result = $this->transcoder->setVideoBitRate($value);
          break;
        case 'wxh':
          if (isset($value) && !empty($value) && variable_get('video_use_preset_wxh', FALSE)) {
            $dimensions = trim($value);
          }
          else {
            $dimensions = $this->settings['input']['dimensions'];
          }
          $dimension = explode('x', $dimensions);
          $result = $this->transcoder->setVideoDimensions($dimension[0], $dimension[1]);
          break;
      }
      if ($result !== PHPVideoToolkit::RESULT_OK) {
        watchdog('transcoder', 'Error set options @message', array('@message' => $this->transcoder->getLastError()), WATCHDOG_ERROR);
        $this->errors['options'] = $this->transcoder->getLastError();
        $this->transcoder->reset(true);
        return FALSE;
      }
    }
  }

  public function setOutput($output_directory, $output_name, $overwrite_mode = FILE_EXISTS_REPLACE) {
    parent::setOutput($output_directory, $output_name, $overwrite_mode);
    $result = $this->transcoder->setOutput(drupal_realpath($output_directory) . '/', $output_name);
    if ($result !== PHPVideoToolkit::RESULT_OK) {
      watchdog('transcoder', 'Error set options @message', array('@message' => $this->transcoder->getLastError()), WATCHDOG_ERROR);
      $this->errors['output'] = $this->transcoder->getLastError();
      $this->transcoder->reset(true);
      return FALSE;
    }
  }

  public function extractFrames() {
    global $user;
    // Get the file system directory.
    $schema_thumb_path = file_uri_scheme($this->settings['input']['uri']) . '://' . variable_get('video_thumbnail_path', 'videos/thumbnails') . '/' . $this->settings['input']['fid'];
    file_prepare_directory($schema_thumb_path, FILE_CREATE_DIRECTORY);
    // get the video file informations
    $file_info = $this->getFileInfo();
    // video duration
    $duration = floor($file_info['duration']['seconds']);
    // Total thumbs to generate
    $no_of_thumbnails = variable_get('video_thumbnail_count', 5);
    $files = NULL;

    // Precaution for very short videos
    if ((2 * $no_of_thumbnails) > $duration) {
      $no_of_thumbnails = floor($duration / 2);
    }

    $debugline = 'Video ' . $this->settings['input']['uri'] . ': duration = ' . $duration . ', frames = ';

    for ($i = 1; $i <= $no_of_thumbnails; $i++) {
      $seek = ceil($duration / ($no_of_thumbnails + 1) * $i);
      $debugline .= $seek . ', ';
      $filename = file_munge_filename('thumbnail-' . $this->settings['input']['fid'] . '_' . sprintf("%04d", $i) . '.png', '', TRUE);
      $thumbnail_path = $schema_thumb_path . '/' . $filename;
      $thumbnail_real_path = drupal_realpath($thumbnail_path);
      // Skip files already exists, this will save ffmpeg traffic
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
            // If there was an error then get it
            watchdog('transcoder', $this->transcoder->getLastError(), array(), WATCHDOG_ERROR);
            $this->transcoder->reset(true);
            continue;
          }
          // @todo think about how to move execute() method to parent class
          $result = $this->transcoder->execute();
          if ($result !== PHPVideoToolkit::RESULT_OK) {
            watchdog('transcoder', $this->transcoder->getLastError(), array(), WATCHDOG_ERROR);
            $this->transcoder->reset(true);
            continue;
          }
        }

        if (!file_exists($thumbnail_real_path)) {
          watchdog('transcoder', 'Error generating thumbnail for video: generated file %file does not exist.', array('%file' => $thumbnail_path), WATCHDOG_ERROR);
          continue;
        }
      }
      if (file_exists($thumbnail_real_path)) {
        // Begin building the file object.
        $file = new stdClass();
        $file->uid = $user->uid;
        $file->status = 0;
        $file->filename = $filename;
        $file->uri = $thumbnail_path;
        $file->filemime = 'image/png';
        $file->filesize = filesize($thumbnail_real_path);
        $file->timestamp = time();
        $files[] = $file;
      }
    }
    watchdog('video', $debugline, array(), WATCHDOG_DEBUG);
    return !empty($files) ? $files : FALSE;
  }

  public function execute() {
    $result = $this->transcoder->execute();
    if ($result !== PHPVideoToolkit::RESULT_OK) {
      $errorlist = $this->transcoder->getErrors();
      $_commandoutput = $this->transcoder->getCommandOutput();
      $commandoutput = array();
      foreach ($_commandoutput as $cmd) {
        $commandoutput[] = '<pre>' . check_plain($cmd['command']) . '</pre><pre>' . check_plain($cmd['output']) . '</p>';
      }

      watchdog('transcoder', 'FFmpeg failed to transcode %video. !errorlist !commandlist', array(
        '%video' => $this->settings['input']['filename'],
        '!errorlist' => theme('item_list', array('type' => 'ol', 'items' => $errorlist, 'title' => t('Reported errors'))),
        '!commandlist' => theme('item_list', array('type' => 'ol', 'items' => $commandoutput, 'title' => t('Executed commands and output')))
      ), WATCHDOG_ERROR);
      $this->errors['execute'] = $errorlist;
      $this->transcoder->reset(true);
      return FALSE;
    }

    $command_output = $this->transcoder->getLastOutput();
    $output_file = $this->settings['base_url'] . '/' . $this->settings['filename'];
    $output_file_rp = drupal_realpath($output_file);
    if (!file_exists($output_file_rp) || ($filesize = filesize($output_file_rp)) == 0) {
      watchdog('transcoder', 'Video conversion has failed! FFmpeg reported the following output: !output', array('!output' => $command_output), WATCHDOG_ERROR);
      return FALSE;
    }

    // Work-around for missing WebM support in file_get_mimetype().
    // See http://drupal.org/node/1347624 .
    $iswebm = substr($this->settings['filename'], -5) === '.webm';

    $file_info = $this->getFileInfo();

    $output = new stdClass();
    $output->filename = $this->settings['filename'];
    $output->uri = $output_file;
    $output->filemime = $iswebm ? 'video/webm' : file_get_mimetype($output_file);
    $output->filesize = $filesize;
    $output->timestamp = time();
    $output->jobid = NULL;
    $output->duration = floor($file_info['duration']['seconds']);

    return $output;
  }

  public function getFileInfo() {
    // @todo Add to cache
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
    $info = $this->getCachedFFmpegInfo();
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

  public function getAvailableFormats($type = FALSE) {
    $info = $this->getCachedFFmpegInfo();
    $formats = array();
    switch ($type) {
      case FALSE:
        return array_keys($info['formats']);
      case 'both' :
        foreach ($info['formats'] as $id => $data) {
          if ($data['mux'] === TRUE && $data['demux'] === TRUE) {
            $formats[$id] = $data['fullname'];
          }
        }
        break;
      case 'muxing' :
        foreach ($info['formats'] as $id => $data) {
          if ($data['mux'] === TRUE) {
            $formats[$id] = $data['fullname'];
          }
        }
        break;
      case 'demuxing' :
        foreach ($info['formats'] as $id => $data) {
          if ($data['demux'] === TRUE) {
            $formats[$id] = $data['fullname'];
          }
        }
        break;
    }

    if (isset($formats['ogg']) && !isset($formats['ogv'])) {
      $formats['ogv'] = $formats['ogg'];
      unset($formats['ogg']);
      asort($formats);
    }

    return $formats;
  }

  public function getVersion() {
    $info = $this->getCachedFFmpegInfo();
    if ($info['ffmpeg-found'] == TRUE) {
      return self::getVersionFromOutput($info['raw']);
    }

    return FALSE;
  }

  public function getName() {
    return 'FFmpeg';
  }

  public function getValue() {
    return 'TranscoderAbstractionFactoryFfmpeg';
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
      '#markup' => '<div id="' . $this->getValue() . '">',
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
      '#description' => t('Absolute path to FFmpeg executable.') . t('When you install a new FFmpeg version, please <a href="@performance-page">clear the caches</a> to let Drupal detect the updated codec support.', array('@performance-page' => url('admin/config/development/performance'))),
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
      '#description' => t('Path to save video thumbnails extracted from videos.'),
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
      '#title' => t('Save all thumbnails in {file_managed} table'),
      '#description' => t('Save all auto-created thumbnails to the {file_managed} table. Change file status as PERMANENT'),
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
    if (!empty($ffmpeg_path) && $ffmpeg_path != variable_get('video_ffmpeg_path')) {
      // we have to initialize PHP toolkit object again, if changed the path of executable
      $toolkit = new PHPVideoToolkit($ffmpeg_path);
      $ffmpeg = $toolkit->getFFmpegInfo(FALSE);
      if ($ffmpeg['ffmpeg-found'] == TRUE) {
        $version = self::getVersionFromOutput($ffmpeg['raw']);
        drupal_set_message(t('FFmpeg version %version found on your system.', array('%version' => $version)), 'status');
      }
      else {
        drupal_set_message(t('FFmpeg not found on your system. To convert videos and create thumbnails you have to install FFmpeg on your server. For more information please see the !documentation.', array('!documentation' => l(t('documentation'), 'http://video.heidisoft.com/documentation/ffmpeg-installtion-scripts'))), 'error');
        form_set_error('video_ffmpeg_path', t('FFmpeg not found on your system. To convert videos and create thumbnails you have to install FFmpeg on your server. For more information please see the !documentation.', array('!documentation' => l(t('documentation'), 'http://video.heidisoft.com/documentation/ffmpeg-installtion-scripts'))));
      }

      // Clear FFmpeg information when the path has changed.
      cache_clear_all(self::INFO_CID, self::INFO_CACHE);
    }
    return;
  }

  /**
   * Returns a cached copy of PHPVideoToolkit::getFFmpegInfo()
   *
   * @return
   *   array of FFmpeg installation information.
   */
  private function getCachedFFmpegInfo() {
    $cache = cache_get(self::INFO_CID, self::INFO_CACHE);
    if (!empty($cache->data)) {
      return $cache->data;
    }
    else {
      $info = $this->transcoder->getFFmpegInfo(FALSE);
      cache_set(self::INFO_CID, $info, self::INFO_CACHE);
      return $info;
    }
  }

  /**
   * Returns the FFmpeg version string from an output string.
   *
   * FFmpeg returns its version in the output of almost any call.
   *
   * Used instead of PHPVideoToolkit::getVersion(), because PHPVideoToolkit
   * has not been updated and does not support git versions.
   *
   * @param
   *   string containing output of ffmpeg -version
   * @return
   *   string containing version of FFmpeg
   */
  public static function getVersionFromOutput($output) {
    $matches = array();

    // Git check outs
    $pattern = '/ffmpeg version N-\d+-g([a-f0-9]+)/';
    if (preg_match($pattern, $output, $matches)) {
      return 'git: ' . $matches[1];
    }

    // Git check outs
    $pattern = '/ffmpeg version git-(\d{4}-\d{2}-\d{2}-[a-f0-9]+)/';
    if (preg_match($pattern, $output, $matches)) {
      return 'git: ' . $matches[1];
    }

    // Release
    $pattern = '/ffmpeg version ([0-9.]+)/i';
    if (preg_match($pattern, $output, $matches)) {
      return $matches[1];
    }

    // Fallback to unrecognized string
    $pattern = '/ffmpeg version ([^\n]+)/i';
    if (preg_match($pattern, $output, $matches)) {
      $version = $matches[1];
      if (($pos = strpos($version, ' Copyright')) !== FALSE) {
        $version = drupal_substr($version, 0, $pos);
      }
      return t('unrecognized: !version', array('!version' => $version));
    }

    return NULL;
  }
}
