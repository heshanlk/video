<?php

//$Id$
/*
 * @file
 * Transcoder class file to handle ffmpeg_wrapper settings and conversions.
 * @todo : add one parent class for ffmpeg and ffmpeg_wrapper classes
 *
 */


class video_ffmpeg_wrapper implements transcoder_interface {

  private $name = 'FFmpeg Wrapper';
  private $value = 'video_ffmpeg_wrapper';
  private $video_ext = 'flv';
  protected $thumb_command = '-i !videofile -an -y -f mjpeg -ss !seek -vframes 1 !thumbfile';

  public function __construct() {
    $this->params['thumb_command'] = variable_get('video_ffmpeg_thumbnailer_options', $this->thumb_command);
  }

  public function run_command($options) {
    watchdog('ffmpeg_wrapper', 'Sending options: ' . $options, array(), WATCHDOG_DEBUG);
    $output = ffmpeg_wrapper_run_command($options);
    if (is_object($output) && ISSET($output->errors)) {
      return implode("\n", $output->errors);
    }
    return $output;
  }

  public function generate_thumbnails($video) {
    global $user;
    // Setup our thmbnail path.
    $video_thumb_path = variable_get('video_thumb_path', 'video_thumbs');
    $final_thumb_path = file_directory_path() . '/' . $video_thumb_path . '/' . $video['fid'];

    // Ensure the destination directory exists and is writable.
    $directories = explode('/', $final_thumb_path);
    // Get the file system directory.
    $file_system = file_directory_path();
    foreach ($directories as $directory) {
      $full_path = isset($full_path) ? $full_path . '/' . $directory : $directory;
      // Don't check directories outside the file system path.
      if (strpos($full_path, $file_system) === 0) {
        field_file_check_directory($full_path, FILE_CREATE_DIRECTORY);
      }
    }

    // Total thumbs to generate
    $total_thumbs = variable_get('no_of_video_thumbs', 5);
    $videofile = escapeshellarg($video['filepath']);
    //get the playtime from the current transcoder
    $duration = $this->get_playtime($video);

    $files = NULL;
    for ($i = 1; $i <= $total_thumbs; $i++) {
      $seek = ($duration / $total_thumbs) * $i - 1;
      $filename = "/video-thumb-for-" . $video['fid'] . "-$i.jpg";
      $thumbfile = $final_thumb_path . $filename;
      //skip files already exists, this will save ffmpeg traffic
      if (!is_file($thumbfile)) {
        //setup the command to be passed to the transcoder.
        $options = t($this->params['thumb_command'], array('!videofile' => $videofile, '!seek' => $seek, '!thumbfile' => $thumbfile));
        // Generate the thumbnail from the video.
        $command_output = $this->run_command($options);
        if (!file_exists($thumbfile)) {
          $error_param = array('%file' => $thumbfile, '%cmd' => $command, '%out' => $command_output);
          $error_msg = t("Error generating thumbnail for video: generated file %file does not exist.<br />Command Executed:<br />%cmd<br />Command Output:<br />%out", $error_param);
          // Log the error message.
          watchdog('video_transcoder', $error_msg, array(), WATCHDOG_ERROR);
          continue;
        }
      }
      // Begin building the file object.
      // @TODO : use file_munge_filename()
      $file = new stdClass();
      $file->uid = $user->uid;
      $file->status = FILE_STATUS_TEMPORARY;
      $file->filename = trim($filename);
      $file->filepath = $thumbfile;
      $file->filemime = file_get_mimetype($filename);
      $file->filesize = filesize($thumbfile);
      $file->timestamp = time();
      $files[] = $file;
    }
    return $files;
  }

  public function video_converted_extension() {
    global $conf;
    if (isset($conf['ffmpeg_output_type']) && !empty($conf['ffmpeg_output_type'])) {
      return $conf['ffmpeg_output_type'];
    }
    return $this->video_ext;
  }

  public function convert_video($video) {
    // This will update our current video status to active.
    $this->change_status($video->vid, VIDEO_RENDERING_ACTIVE);
    // Get the converted file object
    //we are going to move our video to an "original" folder
    //we are going to transcode the video to the "converted" folder
    $pathinfo = pathinfo($video->filepath);
    $original = $pathinfo['dirname'] . '/original';
    $converted = $pathinfo['dirname'] . '/converted';

    if (!field_file_check_directory($original, FILE_CREATE_DIRECTORY)) {
      watchdog('video_transcoder', 'Video conversion failed.  Could not create the directory: ' . $orginal, array(), WATCHDOG_ERROR);
      return false;
    }
    if (!field_file_check_directory($converted, FILE_CREATE_DIRECTORY)) {
      watchdog('video_transcoder', 'Video conversion failed.  Could not create the directory: ' . $converted, array(), WATCHDOG_ERROR);
      return false;
    }

    $original = $original . '/' . $video->filename;
    //lets move our video and then convert it.
    if (file_move($video, $original)) {
      //update our filename after the move to maintain filename uniqueness.
//        $converted = $converted .'/'. pathinfo($video->filepath, PATHINFO_FILENAME) .'.'. $this->video_extension();
      $converted = file_create_filename(str_replace(' ', '_', pathinfo($video->filepath, PATHINFO_FILENAME)) . '.' . $this->video_converted_extension(), $converted);
      // Update our filepath since we moved it
      $update = drupal_write_record('files', $video, 'fid');
      //call our transcoder
//        $command_output = $this->convert_video($video, $converted);
      $dimensions = $this->dimensions($video);
      // Setup our default command to be run.
      $ffmpeg_object = new stdClass();
      // check configuration are pass of then use global $conf
      if (empty($params)) {
        global $conf;
        $params = $conf;
      }

      // first error check, make sure that we can decode this kind of file
      if (!ffmpeg_wrapper_can_decode($video->filepath)) {
        $message = 'FFmpeg Wrapper can not decode this file: !file';
        $variables = array('!file' => l($video->filepath, file_create_url($video->filepath)));
        watchdog('video_render', $message, $variables, WATCHDOG_ERROR);
        return false;
      }

      // did the admin define a specific FFmpeg comand to run?
      //  we only run what the admin specified
      if ($params['ffmpeg_video_custom']) {
        $options[] = str_replace(array('%in_file', '%out_file'), array($video->filepath, $converted), $params['ffmpeg_video_custom_command']);
      }
      // build a standard configuration
      else {
        // build the ffmpeg command structure out
        $options = array();
        // input file
        $options[] = "-i '" . $video->filepath . "'";
        // build the watermark config
        if ($params['ffmpeg_video_wm'])
          $options[] = "-vhook '" . ffmpeg_wrapper_path_to_vhook('watermark.so') . " -f " . $params['ffmpeg_video_wm_file'] . "'";
        // build the audio config
        if ($params['ffmpeg_audio_advanced']) {
          // use a specifc codec?
          if ($params['ffmpeg_audio_acodec'])
            $options[] = '-acodec ' . $params['ffmpeg_audio_acodec'];
          // use a specific sample rate?
          if ($params['ffmpeg_audio_ar'])
            $options[] = '-ar ' . $params['ffmpeg_audio_ar'];
          // use a specific bit rate?
          if ($params['ffmpeg_audio_ab'])
            $options[] = '-ab ' . $params['ffmpeg_audio_ab'];
        }

        //custom sizing per video
        $options[] = '-s ' . $dimensions;

        // build the video config
        if ($params['ffmpeg_video_advanced']) {
          // is codec set?
          if ($params['ffmpeg_video_vcodec'])
            $options[] = '-vcodec ' . $params['ffmpeg_video_vcodec'];
          // is the bit rate set?
          if ($params['ffmpeg_video_br'])
            $options[] = '-b ' . $params['ffmpeg_video_br'];
          // is frame rate set?
          if ($params['ffmpeg_video_fps'])
            $options[] = '-r ' . $params['ffmpeg_video_fps'];
        }
        // implement truncating
        if ($params['ffmpeg_time_advanced'])
          $options[] = '-t ' . $params['ffmpeg_time'];
        // add the output file
        $options[] = "'" . $converted . "'";
      }
      $ffmpeg_object->command = implode(" ", $options);

      // run our command and return the output.
      $command_output = $this->run_command($ffmpeg_object->command);

      //lets check to make sure our file exists, if not error out
      if (!file_exists($converted) || !filesize($converted)) {
        watchdog('video_conversion', 'Video conversion failed.  FFMPEG reported the following output: ' . $command_output, array(), WATCHDOG_ERROR);
        $this->change_status($video->vid, VIDEO_RENDERING_FAILED);
        return FALSE;
      }
      // Setup our converted video object
      $video_info = pathinfo($converted);
      //update our converted video
      $video->converted = new stdClass();
      $video->converted->vid = $video->vid;
      $video->converted->filename = $video_info['basename'];
      $video->converted->filepath = $converted;
      $video->converted->filemime = file_get_mimetype($converted);
      $video->converted->filesize = filesize($converted);
      $video->converted->status = VIDEO_RENDERING_COMPLETE;
      $video->converted->completed = time();
      // Update our video_files table with the converted video information.
      $result = db_query("UPDATE {video_files} SET filename='%s', filepath='%s', filemime='%s', filesize=%d, status=%d, completed=%d WHERE vid=%d",
              $video->converted->filename, $video->converted->filepath, $video->converted->filemime, $video->converted->filesize, $video->converted->status, $video->converted->completed, $video->converted->vid);

      watchdog('video_conversion', 'Successfully converted %orig to %dest', array('%orig' => $video->filepath, '%dest' => $video->converted->filepath), WATCHDOG_INFO);
      return TRUE;
    } else {
      watchdog('video_conversion', 'Cound not move the video to the original folder.', array(), WATCHDOG_ERROR);
      $this->change_status($video->vid, VIDEO_RENDERING_FAILED);
      return FALSE;
    }
  }

  /**
   * Return the playtime seconds of a video
   */
  public function get_playtime($video) {
    $ffmpeg_output = ffmpeg_wrapper_file_data($video['filepath']);
    return $ffmpeg_output['duration'];
  }

  /**
   * Return the dimensions of a video
   */
  public function get_dimensions($video) {
    $ffmpeg_output = ffmpeg_wrapper_file_data($video);
    $res = array('width' => 0, 'height' => 0);
    if ($ffmpeg_output['video']['s']) {
      $dimensions = explode("x", $ffmpeg_output['video']['s']);
      $res['width'] = $dimensions[0] ? $dimensions[0] : NULL;
      $res['height'] = $dimensions[1] ? $dimensions[1] : NULL;
    }
    return $res;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#get_name()
   */
  public function get_name() {
    return $this->name;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#get_value()
   */
  public function get_value() {
    return $this->value;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#get_help()
   */
  public function get_help() {
    $help = l(t('FFMPEG Wrapper'), 'http://drupal.org/project/ffmpeg_wrapper');
    // If the module isn't loaded, show an error next to the link.
    if (!module_exists('ffmpeg_wrapper')) {
      $help .= ' <span class="error">' . t('You currently do not have this installed.') . '</span>';
    }
    return $help;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#admin_settings()
   */
  public function admin_settings() {
    global $conf;
    $form = array();
    if (!module_exists('ffmpeg_wrapper')) {
      $markup = t('You must download and install the !ffmpeg_wrapper to use the FFmpeg Wrapper as a transcoder.', array('!ffmpeg_wrapper' => l(t('FFmpeg Wrapper Module'), 'http://www.drupal.org/project/ffmpeg_wrapper')));
      $form['video_wrapper_info'] = array(
        '#type' => 'markup',
        '#value' => $markup,
        '#prefix' => '<div id="' . $this->value . '">',
        '#suffix' => '</div>',
      );
    } else {
      $form['video_ffmpeg_wrapper_start'] = array(
        '#type' => 'markup',
        '#value' => '<div id="video_ffmpeg_wrapper">',
      );
      // adding auto thumbnail and transcoder path from video_ffmpeg
      // @TODO : move these to a common area
      $form['ffmpeg_wrapper']['video_transcoder_path'] = array(
        '#type' => 'textfield',
        '#title' => t('Path to Video Transcoder'),
        '#description' => t('Absolute path to ffmpeg.'),
        '#default_value' => variable_get('video_transcoder_path', '/usr/bin/ffmpeg'),
      );
      $form['ffmpeg_wrapper']['no_of_video_thumbs'] = array(
        '#type' => 'textfield',
        '#title' => t('Number of thumbnails'),
        '#description' => t('Number of thumbnails to display from video.'),
        '#default_value' => variable_get('no_of_video_thumbs', 5),
      );
      $form['ffmpeg_wrapper']['video_ffmpeg_nice_enable'] = array(
        '#type' => 'checkbox',
        '#title' => t('Enable the use of nice when calling the ffmpeg command.'),
        '#default_value' => variable_get('video_ffmpeg_nice_enable', TRUE),
        '#description' => t('The nice command Invokes a command with an altered scheduling priority.  This option may not be available on windows machines, so disable it.')
      );
      // Thumbnail Videos We need to put this stuff last.
      $form['ffmpeg_wrapper']['autothumb'] = array(
        '#type' => 'fieldset',
        '#title' => t('Video Thumbnails'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
      );
      $form['ffmpeg_wrapper']['autothumb']['video_thumb_path'] = array(
        '#type' => 'textfield',
        '#title' => t('Path to save thumbnails'),
        '#description' => t('Path to save video thumbnails extracted from the videos.'),
        '#default_value' => variable_get('video_thumb_path', 'video_thumbs'),
      );
      $form['ffmpeg_wrapper']['autothumb']['advanced'] = array(
        '#type' => 'fieldset',
        '#title' => t('Advanced Settings'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE
      );
      $form['ffmpeg_wrapper']['autothumb']['advanced']['video_ffmpeg_thumbnailer_options'] = array(
        '#type' => 'textarea',
        '#title' => t('Video thumbnailer options'),
        '#description' => t('Provide the options for the thumbnailer.  Available argument values are: ') . '<ol><li>' . t('!videofile (the video file to thumbnail)') . '<li>' . t('!thumbfile (a newly created temporary file to overwrite with the thumbnail)</ol>'),
        '#default_value' => variable_get('video_ffmpeg_thumbnailer_options', '-i !videofile -an -y -f mjpeg -ss !seek -vframes 1 !thumbfile'),
      );

      if (module_exists('ffmpeg_wrapper_ui')) {
        $ffmpeg_admin_form = ffmpeg_wrapper_ui_configuration_form($conf);
      } else if (module_exists('ffmpeg_wrapper')) {
        $ffmpeg_admin_form = ffmpeg_wrapper_configuration_form($conf);
      }
      $form = array_merge($form, $ffmpeg_admin_form['ffmpeg_wrapper']);

      $form['video_ffmpeg_wrapper_end'] = array(
        '#type' => 'markup',
        '#value' => '</div>',
      );
    }
    return $form;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#admin_settings_validate()
   */
  public function admin_settings_validate($form, &$form_state) {
    return;
  }

  public function create_job($video) {
    return db_query("INSERT INTO {video_files} (fid, status, dimensions) VALUES (%d, %d, '%s')", $video['fid'], VIDEO_RENDERING_PENDING, $video['dimensions']);
  }

  public function update_job($video) {
    if (!$this->load_job($video['fid']))
      return;
    //lets update our table to include the nid
    db_query("UPDATE {video_files} SET nid=%d WHERE fid=%d", $video['nid'], $video['fid']);
  }

  public function delete_job($video) {
    if (!$this->load_job($video->fid))
      return;
    //lets get all our videos and unlink them
    $sql = db_query("SELECT filepath FROM {video_files} WHERE fid=%d", $video->fid);
    //we loop here as future development will include multiple video types (HTML 5)
    while ($row = db_fetch_object($sql)) {
      if (file_exists($row->filepath))
        unlink($row->filepath);
    }
    //now delete our rows.
    db_query('DELETE FROM {video_files} WHERE fid = %d', $video->fid);
  }

  public function load_job($fid) {
    $job = null;
    $result = db_query('SELECT f.*, vf.vid, vf.nid, vf.dimensions, vf.status as video_status FROM {video_files} vf LEFT JOIN {files} f ON vf.fid = f.fid WHERE f.fid=vf.fid AND f.fid = %d', $fid);
    $job = db_fetch_object($result);
    if (!empty($job))
      return $job;
    else
      return FALSE;
  }

  public function load_job_queue() {
    $total_videos = variable_get('video_ffmpeg_instances', 5);
    $videos = array();
    $result = db_query_range('SELECT f.*, vf.vid, vf.nid, vf.dimensions, vf.status as video_status FROM {video_files} vf LEFT JOIN {files} f ON vf.fid = f.fid WHERE vf.status = %d AND f.status = %d ORDER BY f.timestamp',
            VIDEO_RENDERING_PENDING, FILE_STATUS_PERMANENT, 0, $total_videos);

    while ($row = db_fetch_object($result)) {
      $videos[] = $row;
    }
    return $videos;
  }

  /**
   * @todo : replace with the load job method
   * @param <type> $video
   * @return <type>
   */
  public function load_completed_job($video) {
    $result = db_query('SELECT * FROM {video_files} WHERE fid = %d', $video->fid);
    return db_fetch_object($result);
  }

  /**
   * Change the status of the file.
   *
   * @param (int) $vid
   * @param (int) $status
   */
  public function change_status($vid, $status) {
    $result = db_query('UPDATE {video_files} SET status = %d WHERE vid = %d ', $status, $vid);
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
    $aspect_ratio = _video_aspect_ratio($video->filepath);
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
      } else {
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
        } else {
          $options[] = '-padtop ' . $pad1;
          $options[] = '-padbottom ' . $pad2;
        }
      } else {
        // We are padding the left/right of the video.
        $padding = round($output_width - $end_width);
        $pad1 = $pad2 = floor($padding / 2);  //@todo does padding need to be an even number?
        if ($pad1 % 2 !== 0) {
          $pad1++;
          $pad2--;
        }
        if (variable_get('video_ffmpeg_pad_method', 0)) {
          $options[] = '-vf "pad=' . round($output_width) . ':' . round($output_height) . ':' . $pad1 . ':0"';
        } else {
          $options[] = '-padleft ' . $pad1;
          $options[] = '-padright ' . $pad2;
        }
      }

      $end_width = round($end_width) % 2 !== 0 ? round($end_width) + 1 : round($end_width);
      $end_height = round($end_height) % 2 !== 0 ? round($end_height) + 1 : round($end_height);
      //add our size to the beginning to make sure it hits our -s
      array_unshift($options, $end_width . 'x' . $end_height);
      return implode(' ', $options);
    } else {
      return $video->dimensions;
    }
  }

}