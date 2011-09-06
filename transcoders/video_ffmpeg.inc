<?php

/*
 * @file
 * Transcoder class file to handle ffmpeg settings and conversions.
 *
 */

class video_ffmpeg implements transcoder_interface {

  // Naming for our radio options.  Makes it easy to extend our transcoders.
  protected $name = 'FFmpeg Binary';
  protected $value = 'video_ffmpeg';
  protected $toolkit;

  /**
   * @construct
   * load the PHPVideoToolkit() object
   */
  public function __construct() {
    module_load_include('php', 'video', 'libraries/phpvideotoolkit/phpvideotoolkit.php5');
    $this->toolkit = new PHPVideoToolkit(variable_get('video_ffmpeg_path', '/usr/bin/ffmpeg'), file_directory_temp() . '/');
  }

  /**
   * Get trasncoder class name
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#get_name()
   */
  public function get_name() {
    return $this->name;
  }

  /**
   * Get trasncoder calss values for creating instances.
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#get_value()
   */
  public function get_value() {
    return $this->value;
  }

  /**
   * Get help
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#get_help()
   */
  public function get_help() {
    return t('!ffmpeg sould be installed on your server to convert videos. Please see the !help.', array(
      '!ffmpeg' => l(t('FFmpeg'), 'http://ffmpeg.org/'),
      '!help' => l(t('documentation'), 'http://video.heidisoft.com/documentation/ffmpeg-installtion-scripts')
    ));
  }

  /**
   * Extract thumbnails from the video.
   * Interface Implementation
   * @see sites/all/modules/video/includes/transcoder_interface#get_thumbnails()
   * @global object $user Drupal use object.
   * @param object $video Video file object
   * @return stdClass 
   */
  public function get_thumbnails($video) {
    global $user;
    // Setup our thmbnail path.
    $videofile = file_load($video['fid']);
    $scheme = file_uri_scheme($videofile->uri);
    $video_thumbnail_path = variable_get('video_thumbnail_path', 'videos/thumbnails');
    // Get the file system directory.
    $schema_thumb_path = $scheme . '://' . $video_thumbnail_path . '/' . $video['fid'];
    file_prepare_directory($schema_thumb_path, FILE_CREATE_DIRECTORY);
    // Total thumbs to generate
    $total_thumbs = variable_get('video_thumbnail_count', 5);
    //get the actual video file path from the stream wrappers
    $videopath = drupal_realpath($videofile->uri);
    //get the playtime from the current transcoder
    $duration = $this->get_playtime($videopath);

    $files = NULL;
    for ($i = 1; $i <= $total_thumbs; $i++) {
      $seek = ($duration / $total_thumbs) * $i - 1;  //adding minus one to prevent seek times equaling the last second of the video
      $filename = file_munge_filename("video-thumbnail-" . $video['fid'] . "-$i.jpg", '', TRUE);
      $thumbnail_path = $schema_thumb_path . '/' . $filename;
      $thumbnail_real_path = drupal_realpath($thumbnail_path);
      //skip files already exists, this will save ffmpeg traffic
      if (!is_file($thumbnail_real_path)) {
        // if FFmpeg-php library installed use it
        if (class_exists('ffmpeg_movie')) {
          $movie = new ffmpeg_movie($videopath);
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
          $result = $this->toolkit->setInputFile($videopath);
          if (!$result) {
            //if there was an error then get it 
            $error_msg = t($this->toolkit->getLastError());
            watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
            $this->toolkit->reset();
            continue;
          }
          // create thuumbnail file
          $this->toolkit->extractFrame($seek);

          $result = $this->toolkit->setOutput(drupal_realpath($schema_thumb_path) . '/', $filename, PHPVideoToolkit::OVERWRITE_EXISTING);
          if (!$result) {
            //if there was an error then get it 
            $error_msg = t($this->toolkit->getLastError());
            watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
            $this->toolkit->reset();
            continue;
          }

          $result = $this->toolkit->execute(FALSE, TRUE);
          if ($result !== PHPVideoToolkit::RESULT_OK) {
            //if there was an error then get it
            $error_msg = t($this->toolkit->getLastError());
            watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
            $this->toolkit->reset();
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

  /**
   * Returns an array of available encoding & decoding codecs
   * Interface Implementation
   * @see sites/all/modules/video/includes/transcoder_interface#get_codecs()
   * @return array available audio/video encode/decode codecs. 
   */
  public function get_codecs() {
    //@todo : add to cache
    $info = $this->toolkit->getFFmpegInfo(FALSE);
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

  /**
   * Get the available muxing formats
   * Interface Implementation
   * @see sites/all/modules/video/includes/transcoder_interface#get_available_formats()
   * @param string $method
   * @return array formats available 
   */
  public function get_available_formats($method = false) {
    //@todo : add to cache
    $info = $this->toolkit->getFFmpegInfo(FALSE);
    $formats = array();
    switch ($method) {
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

  /**
   * Interface Implementation
   * @see sites/all/modules/video/includes/transcoder_interface#get_version()
   * @return string 
   */
  public function get_version() {
    $ffmpeg = $this->toolkit->getFFmpegInfo();
    if ($ffmpeg['ffmpeg-found'] == TRUE)
      return $ffmpeg['binary']['versions'];
    else
      return FALSE;
  }

  /**
   * Interface Implementation
   * @see sites/all/modules/video/includes/transcoder_interface#convert_video()
   * @param object video file object
   * @return object 
   */
  public function convert_video($video) {
    // set current video status to active.
    $this->change_status($video->vid, VIDEO_RENDERING_ACTIVE);
    // set output path for video files.
    $video_final_path = str_replace('original', '', drupal_dirname($video->uri)) . 'converted/' . $video->fid;
    if (!file_prepare_directory($video_final_path, FILE_CREATE_DIRECTORY)) {
      watchdog('transcoder', 'Video conversion failed.  Could not create the directory: ' . $video_final_path, array(), WATCHDOG_ERROR);
      return FALSE;
    }
    //get the actual video file path from the stream wrappers
    $video_real_path = drupal_realpath($video->uri);
    // process presets
    $presets = $video->presets;
    $converted_files = array();
    foreach ($presets as $name => $preset) {
      // get preset settings
      $settings = $preset['settings'];
      // file name and paths for the final output
      $filename = file_munge_filename(str_replace(' ', '_', pathinfo($video_real_path, PATHINFO_FILENAME) . ' ' . strtolower($name)) . '_' . time() . '.' . $settings['video_extension'], '');
      $video_final_file_path = $video_final_path . '/' . $filename;
      // get the actual video file path from the stream wrappers
      $video_final_file_real_path = drupal_realpath($video_final_file_path);
      // override with preset settings
      if (isset($settings['wxh']) && !empty($settings['wxh']) && variable_get('video_use_preset_wxh', FALSE)) {
        $video->dimensions = trim($settings['wxh']);
      }
      $dimension = explode('x', $video->dimensions);
      // get video meta informations
      $video_info = $this->get_video_info($video_real_path);
      // set input file
      $result = $this->toolkit->setInputFile($video_real_path);
      if (!$result) {
        // if there was an error then get it
        $error_msg = t($this->toolkit->getLastError());
        watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
        $this->toolkit->reset();
        continue;
      }
      // set max frame rate
      if (!empty($settings['max_frame_rate'])) {
        $result = $this->toolkit->setVideoFrameRate($settings['max_frame_rate']);
        if (!$result) {
          //if there was an error then get it
          $error_msg = t($this->toolkit->getLastError());
          watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
          $this->toolkit->reset();
          continue;
        }
      }
      // set video codecs
      $result = $this->toolkit->setVideoCodec($settings['video_codec'], FALSE);
      if (!$result) {
        //if there was an error then get it
        $error_msg = t($this->toolkit->getLastError());
        watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
        $this->toolkit->reset();
        continue;
      }

      // set video preset
      if ($settings['video_preset'] !== 'none') {
        $result = $this->toolkit->setVideoPreset($settings['video_preset'], FALSE);
        if (!$result) {
          //if there was an error then get it
          $error_msg = t($this->toolkit->getLastError());
          watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
          $this->toolkit->reset();
          continue;
        }
      }
      // set audio sample rate
      $settings['audio_sample_rate'] = (!empty($settings['audio_sample_rate'])) ? $settings['audio_sample_rate'] : $video_info['audio']['sample_rate'];

      if ($settings['audio_sample_rate'] < 1000) {
        $settings['audio_sample_rate'] *= 1000;
      }

      $settings['audio_sample_rate'] = min($settings['audio_sample_rate'], 44100);

      $result = $this->toolkit->setAudioSampleFrequency($settings['audio_sample_rate']);
      if (!$result) {
        // if there was an error then get it
        $error_msg = t($this->toolkit->getLastError());
        watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
        $this->toolkit->reset();
        continue;
      }
      // set audio codec
      $result = $this->toolkit->setAudioCodec($settings['audio_codec'], FALSE);
      if (!$result) {
        //if there was an error then get it 
        $error_msg = t($this->toolkit->getLastError());
        watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
        $this->toolkit->reset();
        continue;
      }
      // set audio channels
      $result = $this->toolkit->setAudioChannels($settings['audio_channels']);
      if (!$result) {
        //if there was an error then get it 
        $error_msg = t($this->toolkit->getLastError());
        watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
        $this->toolkit->reset();
        continue;
      }
      // set audio bit rate
      if (empty($settings['audio_bitrate'])) {
        $settings['audio_bitrate'] = 64;
      }

      if ($settings['audio_bitrate'] < 1000) {
        $settings['audio_bitrate'] *= 1000;
      }

      $result = $this->toolkit->setAudioBitRate($settings['audio_bitrate']);
      if (!$result) {
        //if there was an error then get it 
        $error_msg = t($this->toolkit->getLastError());
        watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
        $this->toolkit->reset();
        continue;
      }
      // set video bitrate
      if (empty($settings['video_bitrate'])) {
        $settings['video_bitrate'] = 200;
      }

      if ($settings['video_bitrate'] < 1000) {
        $settings['video_bitrate'] *= 1000;
      }

      $result = $this->toolkit->setVideoBitRate($settings['video_bitrate']);
      if (!$result) {
        //if there was an error then get it
        $error_msg = t($this->toolkit->getLastError());
        watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
        $this->toolkit->reset();
        continue;
      }
      // set video dimensions
      $result = $this->toolkit->setVideoDimensions($dimension[0], $dimension[1]);
      if (!$result) {
        //if there was an error then get it
        $error_msg = t($this->toolkit->getLastError());
        watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
        $this->toolkit->reset();
        continue;
      }
      // set watermarking videos if enabled
      if (isset($settings['video_enable_watermark']) && $settings['video_enable_watermark'] == TRUE) {
        // @todo vhook depricated so use http://ffmpeg.org/libavfilter.html
//        $file = file_load($settings['video_watermark_fid']);
//        $watermark_url = drupal_realpath($file->uri);
////        dsm($file);
//        $options = array(
//          'x-offset' => variable_get('video_warermarkx', 5),
//          'y-offset' => variable_get('video_warermarky', 5),
//          'position' => variable_get('video_warermark_position', 'bottom-right')
//        );
//        $result = $this->toolkit->addWatermark($watermark_url);
//        if (!$result) {
//          //if there was an error then get it
//          $error_msg = t($this->toolkit->getLastError());
//          dsm($this->toolkit->getCommands());
////          watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
//          $this->toolkit->reset();
//          continue;
//        }
      }
      // set out out file
      $result = $this->toolkit->setOutput(dirname($video_final_file_real_path) . '/', $filename, PHPVideoToolkit::OVERWRITE_EXISTING);
      if (!$result) {
        //if there was an error then get it 
        $error_msg = t($this->toolkit->getLastError());
        watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
        $this->toolkit->reset();
        continue;
      }
      // execute command
      $result = $this->toolkit->execute(FALSE, TRUE);
      if ($result !== PHPVideoToolkit::RESULT_OK) {
        //if there was an error then get it 
        $error_msg = t($this->toolkit->getLastError());
//        dsm($this->toolkit->getCommands());
        watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
        $this->toolkit->reset();
        continue;
      }

      $command_output = $this->toolkit->getLastOutput();
      //lets check to make sure our file exists, if not error out
      if (!file_exists($video_final_file_real_path) || !filesize($video_final_file_real_path)) {
        watchdog('transcoder', 'Video conversion failed for preset %preset.  FFmpeg reported the following output: ' . $command_output, array('%orig' => $video->uri, '%preset' => $name), WATCHDOG_ERROR);
        $this->change_status($video->vid, VIDEO_RENDERING_FAILED);
        return FALSE;
      }
      // Setup our converted video object
      $video->converted = new stdClass();
      $video->converted->vid = $video->vid;
      $video->converted->filename = $filename;
      $video->converted->uri = $video_final_file_path;
      $video->converted->filemime = file_get_mimetype($video_final_file_path);
      $video->converted->filesize = filesize($video_final_file_real_path);
      $video->converted->status = VIDEO_RENDERING_COMPLETE;
      $video->converted->preset = $name;
      $video->converted->completed = time();
      $converted_files[] = $video->converted;
    }

    // Update our video_queue table with the converted video information.
    if (!empty($converted_files)) {
      db_update('video_queue')
          ->fields(array(
            'status' => VIDEO_RENDERING_COMPLETE,
            'completed' => time(),
            'data' => serialize($converted_files)))
          ->condition('vid', $video->converted->vid, '=')
          ->execute();
      watchdog('transcoder', 'Successfully converted %orig to %dest', array('%orig' => $video->uri, '%dest' => $video->converted->uri), WATCHDOG_INFO);
      return $converted_files;
    }
    else
      return FALSE;
  }

  /**
   * Get some information from the video file
   */
  public function get_video_info($video) {
    $video_info = $this->toolkit->getFileInfo($video);
    return $video_info;
  }

  /**
   * Return the playtime seconds of a video
   */
  public function get_playtime($video) {
    $video_info = $this->get_video_info($video);
    return $video_info['duration']['seconds'];
  }

  /*
   * Return the dimensions of a video
   */

  public function get_dimensions($video) {
    $video_info = $this->get_video_info($video);
    $res = array('width' => 0, 'height' => 0);
    // Get dimensions
    $res['width'] = $video_info['video']['dimensions']['width'] ? $video_info['video']['dimensions']['width'] : NULL;
    $res['height'] = $video_info['video']['dimensions']['height'] ? $video_info['video']['dimensions']['height'] : NULL;

    return $res;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#admin_settings()
   */
  public function admin_settings() {
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

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#admin_settings_validate()
   */
  public function admin_settings_validate($form, &$form_state) {
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

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#create_job()
   */
  public function create_job($video, $nid) {
    return db_insert('video_queue')
        ->fields(array(
          'fid' => $video['fid'],
          'nid' => $nid,
          'status' => VIDEO_RENDERING_PENDING,
          'dimensions' => $video['dimensions'],
        ))
        ->execute();
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#delete_job()
   */
  public function delete_job($video) {
    $video = (object) $video;
    if (!$video = $this->load_job($video->fid))
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
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#load_job()
   */
  public function load_job($fid) {
    $job = null;
    $job = db_query("SELECT f.*, vf.vid, vf.nid, vf.dimensions, vf.data, vf.status as video_status
      FROM {video_queue} vf LEFT JOIN {file_managed} f ON vf.fid = f.fid WHERE f.fid=vf.fid AND f.fid = :fid", array(':fid' => $fid))
        ->fetch();
    if (!empty($job))
      return $job;
    else
      return FALSE;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#load_job_queue()
   */
  public function load_job_queue() {
    $total_videos = variable_get('video_ffmpeg_instances', 5);
    $videos = array();
    $result = db_query_range('SELECT f.*, vf.vid, vf.nid, vf.dimensions, vf.status as video_status
      FROM {video_queue} vf LEFT JOIN {file_managed} f ON vf.fid = f.fid
      WHERE vf.status = :vstatus AND f.status = :fstatus ORDER BY f.timestamp', 0, $total_videos, array(':vstatus' => VIDEO_RENDERING_PENDING, ':fstatus' => FILE_STATUS_PERMANENT));
    foreach ($result as $row) {
      $videos[] = $row;
    }
    return $videos;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#load_completed_job()
   */
  public function load_completed_job(&$video) {
    $file = $this->load_job($video->fid);
    $data = unserialize($file->data);
    if (!empty($data))
      foreach ($data as $value) {
        $extension = pathinfo(drupal_realpath($value->uri), PATHINFO_EXTENSION);
        $video->files->{$extension}->filename = $value->filename;
        $video->files->{$extension}->filepath = $value->uri;
        $video->files->{$extension}->filemime = file_get_mimetype($value->uri);
        $video->files->{$extension}->url = file_create_url($value->uri);
        $video->files->{$extension}->extension = $extension;
        $video->player = strtolower($extension);
      }
    else
      return FALSE;
  }

  /**
   * Change the status of the file.
   *
   * @param (int) $vid
   * @param (int) $status
   */
  public function change_status($vid, $status) {
    db_update('video_queue')->fields(array(
          'status' => $status,))
        ->condition('vid', $vid, '=')
        ->execute();
  }

  /*
   * Function determines the dimensions you want and compares with the actual wxh of the video.
   *
   * If they are not exact or the aspect ratio does not match, we then figure out how much padding
   * we should add.  We will either add a black bar on the top/bottom or on the left/right.
   *
   * @TODO I need to look more at this function.  I don't really like the guess work here.  Need to implement
   * a better way to check the end WxH.  Maybe compare the final resolution to our defaults?  I don't think
   * that just checking to make sure the final number is even is accurate enough.
   */

  public function dimensions($video) {
    //lets setup our dimensions.  Make sure our aspect ratio matches the dimensions to be used, if not lets add black bars.
    $aspect_ratio = _video_aspect_ratio(drupal_realpath($video->uri));
    $ratio = $aspect_ratio['ratio'];
    $width = $aspect_ratio ['width'];
    $height = $aspect_ratio['height'];

    $wxh = explode('x', $video->dimensions);
    $output_width = $wxh[0];
    $output_height = $wxh[1];
    $output_ratio = number_format($output_width / $output_height, 4);

    if ($output_ratio != $ratio && $width && $height) {
      $options = array();
      // Figure out our black bar padding.
      if ($ratio < $output_width / $output_height) {
        $end_width = $output_height * $ratio;
        $end_height = $output_height;
      }
      else {
        $end_height = $output_width / $ratio;
        $end_width = $output_width;
      }

      // We need to get back to an even resolution and maybe compare with our defaults?
      // @TODO Make this more exact on actual video dimensions instead of making sure the wxh are even numbers

      if ($end_width == $output_width) {
        // We need to pad the top/bottom of the video
        $padding = round($output_height - $end_height);
        $pad1 = $pad2 = floor($padding / 2);
        if ($pad1 % 2 !== 0) {
          $pad1++;
          $pad2--;
        }
        if (variable_get('video_ffmpeg_pad_method', 0)) {
          $options[] = '-vf "pad=' . round($output_width) . ':' . round($output_height) . ':0:' . $pad1 . '"';
        }
        else {
          $options[] = '-padtop ' . $pad1;
          $options[] = '-padbottom ' . $pad2;
        }
      }
      else {
        // We are padding the left/right of the video.
        $padding = round($output_width - $end_width);
        $pad1 = $pad2 = floor($padding / 2);  //@todo does padding need to be an even number?
        if ($pad1 % 2 !== 0) {
          $pad1++;
          $pad2--;
        }
        if (variable_get('video_ffmpeg_pad_method', 0)) {
          $options[] = '-vf "pad=' . round($output_width) . ':' . round($output_height) . ':' . $pad1 . ':0"';
        }
        else {
          $options[] = '-padleft ' . $pad1;
          $options[] = '-padright ' . $pad2;
        }
      }

      $end_width = round($end_width) % 2 !== 0 ? round($end_width) + 1 : round($end_width);
      $end_height = round($end_height) % 2 !== 0 ? round($end_height) + 1 : round($end_height);
      //add our size to the beginning to make sure it hits our -s
      array_unshift($options, $end_width . 'x' . $end_height);
      return implode(' ', $options);
    }
    else {
      return $video->dimensions;
    }
  }

}

?>
