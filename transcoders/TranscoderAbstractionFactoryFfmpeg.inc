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
  protected $realoutputdir = NULL;
  protected $realoutputname = NULL;

  public function __construct() {
    $this->transcoder = new PHPVideoToolkit(variable_get('video_ffmpeg_path', '/usr/bin/ffmpeg'), file_directory_temp() . '/');
    PHPVideoToolkit::$ffmpeg_info = $this->getCachedFFmpegInfo();
    parent::__construct();
  }

  public function setInput($file, $input_frame_rate=0, $validate_decode_codecs = TRUE) {
    parent::setInput($file, $input_frame_rate = 0, $validate_decode_codecs = TRUE);

    $srcuri = $this->settings['input']['uri'];
    $srcpath = drupal_realpath($srcuri);
    if (empty($srcpath)) {
      // If stored on a remote file system, such as S3, download the video to a temporary file.
      $srcpath = video_utility::createTemporaryLocalCopy($srcuri);
      if (empty($srcpath)) {
        watchdog('transcoder', 'Could not download @uri to a temporary file for transcoding.', array('@uri' => $srcuri), WATCHDOG_ERROR);
        return FALSE;
      }
    }

    $result = $this->transcoder->setInputFile($srcpath);
    if ($result !== PHPVideoToolkit::RESULT_OK) {
      watchdog('transcoder', 'Error set options @message', array('@message' => $this->transcoder->getLastError()), WATCHDOG_ERROR);
      $this->errors['input'] = $this->transcoder->getLastError();
      $this->transcoder->reset();
      return FALSE;
    }

    return TRUE;
  }

  public function setOptions(array $options) {
    // Reset the transcoder class keeping the input file info
    $this->transcoder->reset(TRUE);

    $video_info = $this->getFileInfo();
    foreach ($options as $key => $value) {
      if (empty($value) || $value === 'none') {
        continue;
      }

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
          $dimension = explode('x', trim($value));
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

    return TRUE;
  }

  public function setOutput($output_directory, $output_name, $overwrite_mode = FILE_EXISTS_REPLACE) {
    $this->realoutputdir = $output_directory;
    $this->realoutputname = $output_name;

    $tmpoutput = video_utility::createTempFile(video_utility::getExtension($output_name));
    $tmpoutputdir = dirname($tmpoutput);
    $tmpoutputname = basename($tmpoutput);

    parent::setOutput($tmpoutputdir, $tmpoutputname, $overwrite_mode);

    $result = $this->transcoder->setOutput($tmpoutputdir . '/', $tmpoutputname);
    if ($result !== PHPVideoToolkit::RESULT_OK) {
      watchdog('transcoder', 'Error set options @message', array('@message' => $this->transcoder->getLastError()), WATCHDOG_ERROR);
      $this->errors['output'] = $this->transcoder->getLastError();
      $this->transcoder->reset(true);
      return FALSE;
    }

    return TRUE;
  }

  public function extractFrames($destinationScheme, $format) {
    // When $job is null, we are viewing the thumbnails before the form has been submitted.
    $job = $this->loadJob();
    $fid = intval($this->settings['input']['fid']);

    // Get the file system directory.
    $dsturibase = $destinationScheme . '://' . variable_get('video_thumbnail_path', 'videos/thumbnails') . '/' . $fid . '/';
    file_prepare_directory($dsturibase, FILE_CREATE_DIRECTORY);
    $dstwrapper = file_stream_wrapper_get_instance_by_scheme($destinationScheme);

    // get the video file informations
    $file_info = $this->getFileInfo();
    $duration = floor($file_info['duration']['seconds']);
    $no_of_thumbnails = variable_get('video_thumbnail_count', 5);

    // Precaution for very short videos
    if ((2 * $no_of_thumbnails) > $duration) {
      $no_of_thumbnails = floor($duration / 2);
    }

    $thumbs = array();
    for ($i = 1; $i <= $no_of_thumbnails; $i++) {
      $seek = ceil($duration / ($no_of_thumbnails + 1) * $i);
      $filename = file_munge_filename('thumbnail-' . $fid . '_' . sprintf('%04d', $i) . '.' . $format, '', FALSE);
      $dsturi = $dsturibase . $filename;

      if (!file_exists($dsturi)) {
        // Create a temporary file that will be moved to the final URI later
        $dstpath = video_utility::createTempFile($format);

        $error = NULL;
        if (class_exists('ffmpeg_movie')) {
          $movie = new ffmpeg_movie($this->transcoder->getInputFile());
          $frames = $movie->getFrameCount();
          $fps = $movie->getFrameRate();
          $frame = $movie->getFrame(min($frames, (int) $seek * $fps));
          $thumb = $frame->toGDImage();
          $result = video_utility::imageSave($thumb, $dstpath, $format);
          if (!$result) {
            $error = t('Unknown FFmpeg-php error');
          }
        }
        else {
          $this->transcoder->extractFrame($seek);
          $result = $this->transcoder->setOutput(dirname($dstpath) . '/', basename($dstpath), PHPVideoToolkit::OVERWRITE_EXISTING);
          if ($result === PHPVideoToolkit::RESULT_OK) {
            $result = $this->transcoder->execute() === PHPVideoToolkit::RESULT_OK;
          }

          if (!$result) {
            $error = $this->transcoder->getLastError();
            $this->transcoder->reset(true);
          }
        }

        // Check if the extraction was successfull
        if ($error === NULL) {
          if (!file_exists($dstpath)) {
            $error = t('generated file %file does not exist', array('%file' => $dstpath));
          }
          elseif (filesize($dstpath) == 0) {
            $error = t('generated file %file is empty', array('%file' => $dstpath));
            drupal_unlink($dstpath);
          }
        }
        if ($error !== NULL) {
          form_set_error(NULL, t('Error generating thumbnail for video %videofilename: !error.', array('%videofilename' => $this->settings['input']['filename'], '!error' => $error)));
          watchdog('transcoder', 'Error generating thumbnail for video %videofilename: !error.', array('%videofilename' => $this->settings['input']['filename'], '!error' => $error), WATCHDOG_ERROR);
          continue;
        }

        // Move the file to the final URI
        copy($dstpath, $dsturi);
      }

      // Begin building the file object.
      $thumb = new stdClass();
      $thumb->status = 0;
      $thumb->filename = basename($dsturi);
      $thumb->uri = $dsturi;
      $thumb->filemime = $dstwrapper->getMimeType($dsturi);
      $thumbs[] = $thumb;
    }

    return !empty($thumbs) ? $thumbs : FALSE;
  }

  public function execute() {
    $tmpoutputpath = $this->settings['base_url'] . '/' . $this->settings['filename'];
    $result = $this->transcoder->execute();

    if ($result !== PHPVideoToolkit::RESULT_OK || !file_exists($tmpoutputpath) || ($filesize = filesize($tmpoutputpath)) == 0) {
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

    // Work-around for missing WebM support in file_get_mimetype().
    // See http://drupal.org/node/1347624 .
    $iswebm = substr($this->realoutputname, -5) === '.webm';

    $file_info = $this->getFileInfo();
    $realoutputuri = $this->realoutputdir . '/' . $this->realoutputname;
    copy($tmpoutputpath, $realoutputuri);
    drupal_unlink($tmpoutputpath);

    $output = new stdClass();
    $output->filename = $this->realoutputname;
    $output->uri = $realoutputuri;
    $output->filemime = $iswebm ? 'video/webm' : file_get_mimetype($realoutputuri);
    $output->filesize = $filesize;
    $output->timestamp = REQUEST_TIME;
    $output->jobid = NULL;
    $output->duration = floor($file_info['duration']['seconds']);

    return $output;
  }

  public function getFileInfo() {
    $file = $this->settings['input']['uri'];
    $cid = 'video:file:' . md5($file);
    $cache = cache_get($cid);
    if (!empty($cache->data)) {
      return $cache->data;
    }

    $data = $this->transcoder->getFileInfo();
    cache_set($cid, $data, self::INFO_CACHE, time() + 7 * 24 * 3600);
    return $data;
  }

  public function getCodecs() {
    $info = $this->getCachedFFmpegInfo();
    $codecs = array('decode' => array(), 'encode' => array());
    foreach ($info['codecs'] as $key => $value) {
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
    if ($info['ffmpeg-found']) {
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

  public function adminSettings() {
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
      '#collapsed' => FALSE,
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
      '#size' => 5,
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
      $toolkit = new PHPVideoToolkit($ffmpeg_path);
      PHPVideoToolkit::$ffmpeg_info = FALSE;
      $ffmpeg = $toolkit->getFFmpegInfo(FALSE);
      if (!$ffmpeg['ffmpeg-found'] || ($version = self::getVersionFromOutput($ffmpeg['raw'])) == NULL) {
        form_set_error('video_ffmpeg_path', t('FFmpeg not found at %path. To convert videos and create thumbnails you have to install FFmpeg on your server. For more information please see the !documentation.', array('%path' => $ffmpeg_path, '!documentation' => l(t('documentation'), 'http://video.heidisoft.com/documentation/ffmpeg-installtion-scripts'))));
      }
      elseif (empty($ffmpeg['codecs']['video']) || empty($ffmpeg['codecs']['audio'])) {
        form_set_error('video_ffmpeg_path', t('FFmpeg version %version was found, but no supported codecs could be found. Please check whether FFmpeg and all support libraries have been installed correctly.', array('%version' => $version)));
      }
      else {
        drupal_set_message(t('FFmpeg version %version found on your system.', array('%version' => $version)), 'status');
      }

      // Clear FFmpeg information when the path has changed.
      cache_clear_all(self::INFO_CID, self::INFO_CACHE);
    }
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

    $info = $this->transcoder->getFFmpegInfo(FALSE);
    cache_set(self::INFO_CID, $info, self::INFO_CACHE);
    return $info;
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
