<?php

/*
 * @file
 * Transcoder class file to handle ffmpeg settings and conversions.
 *
 */

class video_ffmpeg implements transcoder_interface {

  // Naming for our radio options.  Makes it easy to extend our transcoders.
  private $name = 'Locally Installed Transcoders (FFMPEG/Handbreke/Mcoder)';
  private $value = 'video_ffmpeg';
  protected $params = array();
  protected $audio_bitrate = 64;
  protected $video_bitrate = 200;
  protected $video_width = 640;
  protected $video_height = 480;
  protected $command = '-y -i !videofile -f flv -ar 22050 -ab !audiobitrate -s !size -b !videobitrate -qscale 1 !convertfile';
  protected $thumb_command = '-i !videofile -an -y -f mjpeg -ss !seek -vframes 1 !thumbfile';
  protected $ffmpeg = '/usr/bin/ffmpeg';
  protected $nice;
  protected $video_ext = 'flv';

  public function __construct() {
    $this->params['audiobitrate'] = variable_get('video_ffmpeg_helper_auto_cvr_audio_bitrate', $this->audio_bitrate);
    $this->params['videobitrate'] = variable_get('video_ffmpeg_helper_auto_cvr_video_bitrate', $this->video_bitrate);
    //@todo: move this to the actual widget and save in video_files table.
    $this->params['size'] = variable_get('video_ffmpeg_width', $this->video_width) . 'x' . variable_get('video_ffmpeg_height', $this->video_height);
    $this->params['command'] = variable_get('video_ffmpeg_helper_auto_cvr_options', $this->command);
    $this->params['cmd_path'] = variable_get('video_transcoder_path', $this->ffmpeg);
    $this->params['thumb_command'] = variable_get('video_ffmpeg_thumbnailer_options', $this->thumb_command);
    $this->nice = variable_get('video_ffmpeg_nice_enable', false) ? 'nice -n 19 ' : '';
    $this->params['videoext'] = variable_get('video_ffmpeg_ext', $this->video_ext);
    $this->params['enable_faststart'] = variable_get('video_ffmpeg_enable_faststart', 0);
    $this->params['faststart_cmd'] = variable_get('video_ffmpeg_faststart_cmd', '/usr/bin/qt-faststart');
  }

  public function run_command($options) {
//    $command = $this->nice . ' ' . $this->params['cmd_path'] . ' ' . $options . '  2>&1';
    $command = $this->nice . $options . '  2>&1';
    watchdog('transcoder', 'Executing command: ' . $command, array(), WATCHDOG_DEBUG);
    ob_start();
    passthru($command, $command_return);
    $output = ob_get_contents();
    ob_end_clean();
    return $output;
  }

  public function generate_thumbnails($video) {
    global $user;
    // Setup our thmbnail path.
    $video_thumb_path = variable_get('video_thumb_path', 'videos/thumbnails');
    // Get the file system directory.
    $schema_thumb_path = file_default_scheme() . '://' . $video_thumb_path . '/' . $video['fid'];
    file_prepare_directory($schema_thumb_path, FILE_CREATE_DIRECTORY);
    // Total thumbs to generate
    $total_thumbs = variable_get('video_thumbs', 5);
    $videofile = file_load($video['fid']);
    //get the actual video file path from the stream wrappers
    $videopath = drupal_realpath($videofile->uri);
    //get the playtime from the current transcoder
    $duration = $this->get_playtime($videopath);

    $files = NULL;
    for ($i = 1; $i <= $total_thumbs; $i++) {
      $seek = ($duration / $total_thumbs) * $i - 1;  //adding minus one to prevent seek times equaling the last second of the video
      $filename = file_munge_filename("video-thumb-for-" . $video['fid'] . "-$i.jpg", '', TRUE);
      $thumbfile = $schema_thumb_path . '/' . $filename;
      //skip files already exists, this will save ffmpeg traffic
      if (!is_file(drupal_realpath($thumbfile))) {
        //setup the command to be passed to the transcoder.
        $options = $this->params['cmd_path'] . ' ' . t($this->params['thumb_command'], array('!videofile' => $videopath, '!seek' => $seek, '!thumbfile' => drupal_realpath($thumbfile)));
        // Generate the thumbnail from the video.
        $command_output = $this->run_command($options);
        if (!file_exists(drupal_realpath($thumbfile))) {
          $error_param = array('%file' => $thumbfile, '%cmd' => $options, '%out' => $command_output);
          $error_msg = t("Error generating thumbnail for video: generated file %file does not exist.<br />Command Executed:<br />%cmd<br />Command Output:<br />%out", $error_param);
          // Log the error message.
          watchdog('transcoder', $error_msg, array(), WATCHDOG_ERROR);
          continue;
        }
      }
      // Begin building the file object.
      // @TODO : use file_munge_filename()
      $file = new stdClass();
      $file->uid = $user->uid;
      $file->status = 0;
      $file->filename = trim($filename);
      $file->uri = $thumbfile;
      $file->filemime = file_get_mimetype($filename);
      $file->filesize = filesize(drupal_realpath($thumbfile));
      $file->timestamp = time();
      $files[] = $file;
    }
    return $files;
  }

  public function convert_video($video) {
    // This will update our current video status to active.
//    $this->change_status($video->vid, VIDEO_RENDERING_ACTIVE);
    // get the paths so tokens will compatible with this
    // @todo : add best method to get existing file path and add converted there
    $target = str_replace('original', '', drupal_dirname($video->uri));
    $converted = $target . 'converted/' . $video->fid;
    if (!file_prepare_directory($converted, FILE_CREATE_DIRECTORY)) {
      watchdog('transcoder', 'Video conversion failed.  Could not create the directory: ' . $converted, array(), WATCHDOG_ERROR);
      return false;
    }
    //get the actual video file path from the stream wrappers
    $original_video_path = drupal_realpath($video->uri);
    // process presets
    $presets = $video->presets;
    $converted_files = array();
    foreach ($presets as $name => $preset) {
      $settings = $preset['settings'];
      // override with preset settings
      if (isset($settings['width']) && isset($settings['height'])) {
        $video->dimensions = $settings['width'] . 'x' . $settings['height'];
      }
      $converted .= '/' . file_munge_filename(str_replace(' ', '_', pathinfo($original_video_path, PATHINFO_FILENAME)) . '.' . $settings['video_extension'], $settings['video_extension']);
      //get the actual video file path from the stream wrappers
      $converted_video_path = drupal_realpath($converted);
      $dimensions = $this->dimensions($video);
      $dimention = explode('x', $dimensions);
      if ($this->params['enable_faststart'] && in_array($settings['video_extension'], array('mov', 'mp4'))) {
        $ffmpeg_output = file_directory_temp() . '/' . basename($converted_video_path);
      } else {
        $ffmpeg_output = $converted_video_path;
      }
      // Setup our default command to be run.
      $command = strtr($settings['cli_code'], array(
            '!cmd_path' => $this->params['cmd_path'],
            '!videofile' => '"' . $original_video_path . '"',
            '!audiobitrate' => $settings['audio_bitrate'],
            '!width' => $dimention[0],
            '!height' => $dimention[1],
            '!videobitrate' => $settings['video_bitrate'],
            '!convertfile' => '"' . $ffmpeg_output . '"',
          ));
      // process our video
      $command_output = $this->run_command($command);


      if ($ffmpeg_output != $converted_video_path && file_exists($ffmpeg_output)) {
        // Because the transcoder_interface doesn't allow the run_command() to include the ability to pass
        // the command to be execute so we need to fudge the command to run qt-faststart.
        $cmd_path = $this->params['cmd_path'];
        $this->params['cmd_path'] = $this->params['faststart_cmd'];
        $command_output .= $this->run_command($ffmpeg_output . ' ' . $converted_video_path, $verbose);
        $this->params['cmd_path'] = $cmd_path;

        // Delete the temporary output file.
        drupal_unlink($ffmpeg_output);
      }

      //lets check to make sure our file exists, if not error out
      if (!file_exists($converted_video_path) || !filesize($converted_video_path)) {
        watchdog('transcoder', 'Video conversion failed for preset %preset.  FFMPEG reported the following output: ' . $command_output, array('%orig' => $video->uri, '%preset' => $name), WATCHDOG_ERROR);
        $this->change_status($video->vid, VIDEO_RENDERING_FAILED);
        return FALSE;
      }
      // Setup our converted video object
      $video_info = pathinfo($converted_video_path);
      //update our converted video
      $video->converted = new stdClass();
      $video->converted->vid = $video->vid;
      $video->converted->filename = $video_info['basename'];
      $video->converted->uri = $converted;
      $video->converted->filemime = file_get_mimetype($converted);
      $video->converted->filesize = filesize($converted);
      $video->converted->status = VIDEO_RENDERING_COMPLETE;
      $video->converted->preset = $name;
      $video->converted->completed = time();
      $converted_files[] = $video->converted;
    }
    // Update our video_files table with the converted video information.
    db_update('video_files')
        ->fields(array(
          'status' => VIDEO_RENDERING_COMPLETE,
          'completed' => time(),
          'data' => serialize($converted_files)))
        ->condition('vid', $video->converted->vid, '=')
        ->execute();
    watchdog('transcoder', 'Successfully converted %orig to %dest', array('%orig' => $video->uri, '%dest' => $video->converted->uri), WATCHDOG_INFO);
    return TRUE;
  }

  /**
   * Get some information from the video file
   */
  public function get_video_info($video) {
    static $command_ouput;
    if (!empty($command_output))
      return $command_output;

    $file = escapeshellarg($video);
    // Execute the command
    $options = $this->params['cmd_path'] . ' -i ' . $file;
    $command_output = $this->run_command($options);
    return $command_output;
  }

  /**
   * Return the playtime seconds of a video
   */
  public function get_playtime($video) {
    $ffmpeg_output = $this->get_video_info($video);
    // Get playtime
    $pattern = '/Duration: ([0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9])/';
    preg_match_all($pattern, $ffmpeg_output, $matches, PREG_PATTERN_ORDER);
    $playtime = $matches[1][0];
    // ffmpeg return length as 00:00:31.1 Let's get playtime from that
    $hmsmm = explode(":", $playtime);
    $tmp = explode(".", $hmsmm[2]);
    $seconds = $tmp[0];
    $hours = $hmsmm[0];
    $minutes = $hmsmm[1];
    return $seconds + ($hours * 3600) + ($minutes * 60);
  }

  /*
   * Return the dimensions of a video
   */

  public function get_dimensions($video) {
    $ffmpeg_output = $this->get_video_info($video);
    $res = array('width' => 0, 'height' => 0);
    // Get dimensions
    $regex = ereg('[0-9]?[0-9][0-9][0-9]x[0-9][0-9][0-9][0-9]?', $ffmpeg_output, $regs);
    if (isset($regs[0])) {
      $dimensions = explode("x", $regs[0]);
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
    return l(t('FFMPEG Online Manual'), 'http://www.ffmpeg.org/');
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#admin_settings()
   */
  public function admin_settings() {
    $form = array();
    $form['video_ffmpeg_start'] = array(
      '#type' => 'markup',
      '#value' => '<div id="video_ffmpeg">',
    );

    $form['video_transcoder_path'] = array(
      '#type' => 'textfield',
      '#title' => t('Path to Video Transcoder'),
      '#description' => t('Absolute path to ffmpeg.'),
      '#default_value' => variable_get('video_transcoder_path', '/usr/bin/ffmpeg'),
    );
    $form['video_thumbs'] = array(
      '#type' => 'textfield',
      '#title' => t('Number of thumbnails'),
      '#description' => t('Number of thumbnails to display from video.'),
      '#default_value' => variable_get('video_thumbs', 5),
    );
    $form['video_ffmpeg_nice_enable'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enable the use of nice when calling the ffmpeg command.'),
      '#default_value' => variable_get('video_ffmpeg_nice_enable', TRUE),
      '#description' => t('The nice command Invokes a command with an altered scheduling priority.  This option may not be available on windows machines, so disable it.')
    );
    // Thumbnail Videos We need to put this stuff last.
    $form['autothumb'] = array(
      '#type' => 'fieldset',
      '#title' => t('Video Thumbnails'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form['autothumb']['video_thumb_path'] = array(
      '#type' => 'textfield',
      '#title' => t('Path to save thumbnails'),
      '#description' => t('Path to save video thumbnails extracted from the videos.'),
      '#default_value' => variable_get('video_thumb_path', 'videos/thumbnails'),
    );
    $form['autothumb']['advanced'] = array(
      '#type' => 'fieldset',
      '#title' => t('Advanced Settings'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE
    );
    $form['autothumb']['advanced']['video_ffmpeg_thumbnailer_options'] = array(
      '#type' => 'textarea',
      '#title' => t('Video thumbnailer options'),
      '#description' => t('Provide the options for the thumbnailer.  Available argument values are: ') . '<ol><li>' . t('!videofile (the video file to thumbnail)') . '<li>' . t('!thumbfile (a newly created temporary file to overwrite with the thumbnail)</ol>'),
      '#default_value' => variable_get('video_ffmpeg_thumbnailer_options', '-i !videofile -an -y -f mjpeg -ss !seek -vframes 1 !thumbfile'),
    );

    // Video conversion settings.
    $form['autoconv'] = array(
      '#type' => 'fieldset',
      '#title' => t('Video Conversion'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE
    );
    $form['autoconv']['video_ffmpeg_enable_faststart'] = array(
      '#type' => 'checkbox',
      '#title' => t('Process mov/mp4 videos with qt-faststart'),
      '#default_value' => variable_get('video_ffmpeg_enable_faststart', 0),
    );
    $form['autoconv']['video_ffmpeg_faststart_cmd'] = array(
      '#type' => 'textfield',
      '#title' => t('Path to qt-faststart'),
      '#default_value' => variable_get('video_ffmpeg_faststart_cmd', '/usr/bin/qt-faststart'),
    );

    $form['autoconv']['video_ffmpeg_pad_method'] = array(
      '#type' => 'radios',
      '#title' => t('FFMPeg Padding method'),
      '#default_value' => variable_get('video_ffmpeg_pad_method', 0),
      '#options' => array(
        0 => t('Use -padtop, -padbottom, -padleft, -padright for padding'),
        1 => t('Use -vf "pad:w:h:x:y:c" for padding'),
      ),
    );

    $form['video_ffmpeg_end'] = array(
      '#type' => 'markup',
      '#value' => '</div>',
    );
    return $form;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#admin_settings_validate()
   */
  public function admin_settings_validate($form, &$form_state) {
    return;
  }

  /**
   * Interface Implementations
   * @see sites/all/modules/video/includes/transcoder_interface#create_job()
   */
  public function create_job($video, $nid) {
    return db_insert('video_files')
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
        if (file_exists($file->filepath))
          @unlink($file->filepath);
      }
    }
    //now delete our rows.
    db_delete('video_files')
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
      FROM {video_files} vf LEFT JOIN {file_managed} f ON vf.fid = f.fid WHERE f.fid=vf.fid AND f.fid = :fid",
                array(':fid' => $fid))
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
//    return;
    $total_videos = variable_get('video_ffmpeg_instances', 5);
    $videos = array();
    $result = db_query_range('SELECT f.*, vf.vid, vf.nid, vf.dimensions, vf.status as video_status
      FROM {video_files} vf LEFT JOIN {file_managed} f ON vf.fid = f.fid
      WHERE vf.status = :vstatus AND f.status = :fstatus ORDER BY f.timestamp',
            0, $total_videos, array(':vstatus' => VIDEO_RENDERING_PENDING, ':fstatus' => FILE_STATUS_PERMANENT));
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
        $video->files->{$extension}->uri = $value->uri;
        $video->files->{$extension}->url = file_create_url($value->uri);
        $video->files->{$extension}->extension = $extension;
        $video->player = strtolower($extension);
      }
  }

  /**
   * Change the status of the file.
   *
   * @param (int) $vid
   * @param (int) $status
   */
  public function change_status($vid, $status) {
    db_update('video_files')->fields(array(
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

?>
