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
  protected $logcommands = false;

  public function __construct() {
    $this->params['audiobitrate'] = variable_get('video_ffmpeg_helper_auto_cvr_audio_bitrate', $this->audio_bitrate);
    $this->params['videobitrate'] = variable_get('video_ffmpeg_helper_auto_cvr_video_bitrate', $this->video_bitrate);
    //@todo: move this to the actual widget and save in video_files table.
    $this->params['size'] = variable_get('video_ffmpeg_width', $this->video_width) . 'x' . variable_get('video_ffmpeg_height', $this->video_height);
    $this->params['command'] = variable_get('video_ffmpeg_helper_auto_cvr_options', $this->command);
    $this->params['cmd_path'] = variable_get('video_transcoder_path', $this->ffmpeg);
    $this->params['thumb_command'] = variable_get('video_ffmpeg_thumbnailer_options', $this->thumb_command);
    $this->nice = variable_get('video_ffmpeg_nice_enable', false) ? 'nice -n 19' : '';
    $this->params['videoext'] = variable_get('video_ffmpeg_ext', $this->video_ext);
    $this->params['enable_faststart'] = variable_get('video_ffmpeg_enable_faststart', 0);
    $this->params['faststart_cmd'] = variable_get('video_ffmpeg_faststart_cmd', '/usr/bin/qt-faststart');
    $this->logcommands = variable_get('video_ffmpeg_log_commands', true);
  }

  /**
   * Run the specified command
   * 
   * The nice prefix is automatically added.
   * The command is logged if the settings specify that all commands should be logged.
   * The command and error are logged if the command results in an error
   * 
   * @param string $command
   * @param string $output Output of the command
   * @param string $purpose Purpose of the command. This is logged.
   * @param bool $ignoreoutputfilenotfound Whether to the output file not found error. Useful for input file information.
   */
  private function run_command($command, &$output, $purpose, $ignoreoutputfilenotfound = false) {
    $output = '';
    $command = $this->nice .' '. $command .' 2>&1';
    
    if ($this->logcommands) {
      watchdog('video_ffmpeg', 'Executing ffmpeg command for !purpose: <pre>@command</pre>', array('@command' => $command, '!purpose' => $purpose), WATCHDOG_DEBUG);
    }
    
    $return_var = 0;
    
    ob_start();
    passthru($command, &$return_var);
    $output = ob_get_contents();
    ob_end_clean();
    
    // Returnvar 1 means input file not found. This is normal for information calls.
    if ($return_var != 0 && ($return_var != 1 || !$ignoreoutputfilenotfound)) {
      watchdog('video_ffmpeg',
      	'Error executing ffmpeg command for !purpose:<br/><pre>@command</pre>Output:<br/><pre>@output</pre>',
        array('@command' => $command, '@output' => trim($output), '!purpose' => $purpose),
        WATCHDOG_ERROR);
      
      return false;
    }
    
    return true;
  }

  public function generate_thumbnails($video) {
    global $user;
    
    $final_thumb_path = video_thumb_path($video);
    $total_thumbs = variable_get('video_thumbs', 5);

    $files = NULL;
    for ($i = 1; $i <= $total_thumbs; $i++) {
      $filename = '/video-thumb-for-' . $video['fid'] . '-'. $i .'.jpg';
      $thumbfile = $final_thumb_path . $filename;
      
      //skip files already exists, this will save ffmpeg traffic
      if (!is_file($thumbfile)) {
        if (!isset($duration)) {
          $duration = $this->get_playtime($video['filepath']);
        }
        $seek = ($duration / $total_thumbs) * $i - 1;  //adding minus one to prevent seek times equaling the last second of the video
        //setup the command to be passed to the transcoder.
        $command = $this->params['cmd_path'] .' '. strtr($this->params['thumb_command'], array('!videofile' => escapeshellarg($video['filepath']), '!seek' => $seek, '!thumbfile' => $thumbfile));

        // Generate the thumbnail from the video.
        $command_output = '';
        $this->run_command($command, $command_output, t('generating thumbnails'));
        
        // don't consider zero-byte files a success
        $exists = file_exists($thumbfile);
        if (!$exists || filesize($thumbfile) == 0) {
          $params = array('%file' => $thumbfile, '%video' => $video['filename']);
          
          if ($exists) {
            $error_msg = 'Error generating thumbnail for video %video: generated file %file is empty. This problem may be caused by a broken video file. The video reports that its length is @duration seconds. If this is wrong, please recreate the video and try again.';
            $params['@duration'] = $duration;
            unlink($thumbfile);
          }
          else {
            $error_msg = 'Error generating thumbnail for video %video: generated file %file does not exist.';
          }
          
          // Log the error message and break. Other thumbnails probably will not be generated as well.
          watchdog('video_ffmpeg', $error_msg, $params, WATCHDOG_ERROR);
          drupal_set_message(t($error_msg, $params), 'error');
          break;
        }
      }
      
      // Begin building the file object.
      // @TODO : use file_munge_filename()
      $file = new stdClass();
      $file->uid = $user->uid;
      $file->status = FILE_STATUS_TEMPORARY;
      $file->filename = $filename;
      $file->filepath = $thumbfile;
      $file->filemime = 'image/jpeg';
      $file->filesize = filesize($thumbfile);
      $file->timestamp = time();
      $files[] = $file;
    }
    return $files;
  }

  public function convert_video($video) {
    // This will update our current video status to active.
    $this->change_status($video->vid, VIDEO_RENDERING_ACTIVE);

    // Get the converted file object
    //we are going to move our video to an "original" folder
    //we are going to transcode the video to the "converted" folder
    // @TODO This about getting the correct path from the filefield if they active it
    $files = file_create_path();
    $original = $files . '/videos/original';
    $converted = $files . '/videos/converted';

    if (!field_file_check_directory($original, FILE_CREATE_DIRECTORY)) {
      watchdog('video_ffmpeg', 'Video conversion failed. Could not create directory %dir for storing original videos.', array('%dir' => $original), WATCHDOG_ERROR);
      return false;
    }
    if (!field_file_check_directory($converted, FILE_CREATE_DIRECTORY)) {
      watchdog('video_ffmpeg', 'Video conversion failed. Could not create directory %dir for storing converted videos.', array('%dir' => $converted), WATCHDOG_ERROR);
      return false;
    }

    $original = $original . '/' . $video->filename;
    //lets move our video and then convert it.
    if (file_move($video, $original)) {
      // Update our filepath since we moved it
      $update = drupal_write_record('files', $video, 'fid');
      // process presets
      $presets = $video->presets;
      $converted_files = array();
      foreach ($presets as $presetname => $preset) {
        // reset converted file path
        $converted = $files . '/videos/converted';
        
        //update our filename after the move to maintain filename uniqueness.
        $converted = file_create_filename(str_replace(' ', '_', pathinfo($video->filepath, PATHINFO_FILENAME)) . '.' . $preset['extension'], $converted);

        //call our transcoder
        $dimensions = $this->dimensions($video);
        $dimention = explode('x', $dimensions);
        if ($this->params['enable_faststart'] && in_array($preset['extension'], array('mov', 'mp4'))) {
          $ffmpeg_output = file_directory_temp() . '/' . basename($converted);
        } else {
          $ffmpeg_output = $converted;
        }
        
        $command_output = '';
        
        // Setup our default command to be run.
        foreach ($preset['command'] as $command) {
          $command = strtr($command, array(
            '!cmd_path' => $this->params['cmd_path'],
            '!videofile' => escapeshellarg($video->filepath),
            '!audiobitrate' => $preset['audio_bitrate'],
            '!width' => $dimention[0],
            '!height' => $dimention[1],
            '!videobitrate' => $preset['video_bitrate'],
            '!convertfile' => escapeshellarg($ffmpeg_output),
          ));
          
          // Process our video
          if (!$this->run_command($command, $command_output, t('rendering preset %preset', array('%preset' => $presetname)))) {
            $this->change_status($video->vid, VIDEO_RENDERING_FAILED);
            return FALSE;
          }
        }

        if ($ffmpeg_output != $converted && file_exists($ffmpeg_output)) {
          // Because the transcoder_interface doesn't allow the run_command() to include the ability to pass
          // the command to be execute so we need to fudge the command to run qt-faststart.
          $command_result = $this->run_command($this->params['faststart_cmd'] .' '. $ffmpeg_output .' '. $converted, $command_output);

          // Delete the temporary output file.
          file_delete($ffmpeg_output);
          
          if (!$command_result) {
            $this->change_status($video->vid, VIDEO_RENDERING_FAILED);
            return FALSE;
          }
        }

        //lets check to make sure our file exists, if not error out
        if (!file_exists($converted) || !filesize($converted)) {
          watchdog('video_ffmpeg', 'Video conversion failed for preset %preset: result file was not found.', array('%preset' => $presetname), WATCHDOG_ERROR);
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
        $video->converted->preset = $presetname;
        $video->converted->completed = time();
        $converted_files[] = $video->converted;
      }

      // Update our video_files table with the converted video information.
      $result = db_query("UPDATE {video_files} SET status=%d, completed=%d, data='%s' WHERE vid=%d", $video->converted->status, $video->converted->completed, serialize($converted_files), $video->converted->vid);

      watchdog('video_ffmpeg', 'Successfully converted %orig to %dest', array('%orig' => $video->filepath, '%dest' => $video->converted->filepath), WATCHDOG_INFO);
      return TRUE;
    } else {
      watchdog('video_ffmpeg', 'Could not move video %orig to the original folder.', array('%orig' => $video->filepath), WATCHDOG_ERROR);
      $this->change_status($video->vid, VIDEO_RENDERING_FAILED);
      return FALSE;
    }
  }

  /**
   * Get some information from the video file
   */
  private function get_video_info($video) {
    $command = $this->params['cmd_path'] .' -i '. escapeshellarg($video);

    // Execute the command
    $output = '';
    if ($this->run_command($command, $output, t('retrieving video info'), true)) {
      return $output;
    }

    return NULL;
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
    $hmsmm = explode(':', $playtime);
    $tmp = explode('.', $hmsmm[2]);
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
    $form['video_ffmpeg_log_commands'] = array(
      '#type' => 'checkbox',
      '#title' => t('Log all executed commands to the Drupal log.'),
      '#default_value' => $this->logcommands,
      '#description' => t('Enable this option when debugging ffmpeg encoding to log all commands to the <a href="@dblog-page">Drupal log</a>. This may help with debugging ffmpeg problems. When this option is disabled, only errors will be logged.', array('@dblog-page' => url('admin/reports/dblog')))
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
      '#default_value' => variable_get('video_thumb_path', 'video_thumbs'),
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

  public function create_job($video) {
    if (empty($video['dimensions'])) {
      watchdog('video_ffmpeg', 'Tried to create ffmpeg job for video %video with empty dimensions value.', array('%video' => $video['fid']), WATCHDOG_ERROR);
      return FALSE;
    }
    
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
    $sql = db_query("SELECT data FROM {video_files} WHERE fid=%d", $video->fid);
    //we loop here as future development will include multiple video types (HTML 5)
    while ($row = db_fetch_object($sql)) {
      $data = unserialize($row->data);
      if (empty($data))
        continue;
      foreach ($data as $file) {
        if (file_exists($file->filepath))
          unlink($file->filepath);
      }
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
    $result = db_query_range('SELECT f.*, vf.vid, vf.nid, vf.dimensions, vf.status as video_status FROM {video_files} vf LEFT JOIN {files} f ON vf.fid = f.fid WHERE vf.status = %d AND f.status = %d ORDER BY f.timestamp', VIDEO_RENDERING_PENDING, FILE_STATUS_PERMANENT, 0, $total_videos);

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
  public function load_completed_job(&$video) {
    $result = db_fetch_object(db_query('SELECT * FROM {video_files} WHERE fid = %d', $video->fid));
    $data = unserialize($result->data);
    if (empty($data))
      return $video;
    foreach ($data as $value) {
      $extension = pathinfo($value->filepath, PATHINFO_EXTENSION);
      $video->files->{$extension}->filename = pathinfo($value->filepath, PATHINFO_FILENAME) . '.' . $extension;
      $video->files->{$extension}->filepath = $value->filepath;
      $video->files->{$extension}->url = file_create_url($value->filepath);
      $video->files->{$extension}->extension = $extension;
      $video->files->{$extension}->filemime = file_get_mimetype($value->filepath);
      $video->player = strtolower($extension);
    }
    return $video;
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

?>
