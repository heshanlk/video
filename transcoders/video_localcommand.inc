<?php
// Register this file as a CTools plugin
$plugin = array();

/*
 * @file
 * Transcoder class file to handle local command settings and conversions.
 */
class video_localcommand implements video_transcoder {
  private $name = 'Locally Installed Transcoder (FFmpeg/Handbreke/Mcoder)';
  private $value = 'video_localcommand';

  protected $usenice;
  protected $thumbcmdoptions;
  protected $faststartcmd;
  protected $flvtoolcmd;
  protected $cmdpath;
  protected $logcommands;

  public function __construct() {
    $this->usenice = PHP_OS == 'Linux' && variable_get('video_localcommand_nice_enable', FALSE);
    $this->thumbcmdoptions = variable_get('video_localcommand_thumbnailer_options', '-i !videofile -an -y -f mjpeg -ss !seek -vframes 1 !thumbfile');
    $this->faststartcmd = variable_get('video_localcommand_qtfaststart_cmd', '/usr/bin/qt-faststart');
    $this->flvtoolcmd = variable_get('video_flvtool2_path', '/usr/bin/flvtool2');
    $this->cmdpath = variable_get('video_transcoder_path', '/usr/bin/ffmpeg');
    $this->logcommands = (bool)variable_get('video_localcommand_log_commands', TRUE);
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
  private function run_command($command, &$output, $purpose = NULL, $ignoreoutputfilenotfound = FALSE) {
    $output = '';
    $command = $command .' 2>&1';

    if ($this->usenice) {
      $command = 'nice -n 19 '. $command;
    }

    $purposetext = !empty($purpose) ? (' '. t('for') .' '. $purpose) : '';

    if ($this->logcommands) {
      watchdog('video_command', 'Executing command!purposetext: <pre>@command</pre>', array('@command' => $command, '!purposetext' => (!empty($purpose) ? ' for '. $purpose : '')), WATCHDOG_DEBUG);
    }

    $return_var = 0;

    ob_start();
    passthru($command, $return_var);
    $output = ob_get_clean();

    // Returnvar 1 means input file not found. This is normal for information calls.
    if ($return_var != 0 && ($return_var != 1 || !$ignoreoutputfilenotfound)) {
      watchdog('video_command',
        'Error executing command!purposetext:<br/><pre>@command</pre>Output:<br/><pre>@output</pre>',
        array('@command' => $command, '@output' => trim($output), '!purposetext' => $purposetext),
        WATCHDOG_ERROR);
      return FALSE;
    }

    return TRUE;
  }

  public function generate_thumbnails(stdClass $video) {
    global $user;

    $final_thumb_path = video_thumb_path($video);
    $total_thumbs = variable_get('video_thumbs', 5);

    $files = NULL;
    for ($i = 1; $i <= $total_thumbs; $i++) {
      $filename = '/video-thumb-for-'. $video->fid .'-'. $i .'.jpg';
      $thumbfile = $final_thumb_path . $filename;

      // Skip files already exists, this will save time
      if (!is_file($thumbfile)) {
        if (!isset($duration)) {
          $duration = $this->get_playtime($video->filepath);
        }
        $seek = floor($duration / $total_thumbs) * $i - 1;  // Adding minus one to prevent seek times equaling the last second of the video
        // Setup the command to be passed to the transcoder.
        $command = $this->cmdpath .' '. strtr($this->thumbcmdoptions, array('!videofile' => escapeshellarg($video->filepath), '!seek' => $seek, '!thumbfile' => $thumbfile));
        // Generate the thumbnail from the video.
        $command_output = '';
        $this->run_command($command, $command_output, t('generating thumbnails'));

        // don't consider zero-byte files a success
        $exists = file_exists($thumbfile);
        if (!$exists || filesize($thumbfile) == 0) {
          $params = array('%file' => $thumbfile, '%video' => $video->filename);

          if ($exists) {
            $error_msg = 'Error generating thumbnail for video %video: generated file %file is empty. This problem may be caused by a broken video file. The video reports that its length is @duration seconds. If this is wrong, please recreate the video and try again.';
            $params['@duration'] = $duration;
            unlink($thumbfile);
          }
          else {
            $error_msg = 'Error generating thumbnail for video %video: generated file %file does not exist.';
          }

          // Log the error message and break. Other thumbnails probably will not be generated as well.
          watchdog('video_command', $error_msg, $params, WATCHDOG_ERROR);
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

  public function convert_video(stdClass $video) {
    // This will update our current video status to active.
    $this->change_status($video, VIDEO_RENDERING_ACTIVE);

    $converteddir = dirname(dirname($video->filepath)) .'/converted';

    // Check if $converteddir is an absolute path, we need this later
    $converteddirabsolute = FALSE;
    if ($converteddir[0] == '/') {
      $converteddirabsolute = TRUE;
    }
    elseif (substr($converteddir, 1, 2) == ':\\') {
      $converteddirabsolute = TRUE;
    }
    else {
      $converteddirabsolute = variable_get('file_downloads', FILE_DOWNLOADS_PUBLIC) == FILE_DOWNLOADS_PRIVATE;
    }

    if (!field_file_check_directory($converteddir, FILE_CREATE_DIRECTORY)) {
      watchdog('video_command', 'Video conversion could not be started: Could not create directory %dir for storing converted videos.', array('%dir' => $converteddir), WATCHDOG_ERROR);
      return FALSE;
    }

    $sourcedimensions = $this->get_dimensions($video->filepath);
    if (empty($sourcedimensions)) {
      watchdog('video_command', 'Video conversion could not be started: Could not find file information for file %file.', array('%file' => $video->filepath), WATCHDOG_ERROR);
      return FALSE;
    }

    // Increase the database timeout to prevent database errors after a long upload
    _video_db_increase_timeout();

    // Get the placeholders for the transcoding command
    $dimensionparameters = $this->getDimensionParameters($video, $sourcedimensions);
    $parameters = array(
      '!cmd_path' => $this->cmdpath,
      '!videofile' => escapeshellarg(realpath($video->filepath)),
      '!convertfile' => NULL,
      '!width' => $dimensionparameters->width,
      '!height' => $dimensionparameters->height,
      '!paddingwidth' => $dimensionparameters->paddingwidth,
      '!paddingheight' => $dimensionparameters->paddingheight,
      '!paddingtop' => $dimensionparameters->paddingtop,
      '!paddingbottom' => $dimensionparameters->paddingbottom,
      '!paddingleft' => $dimensionparameters->paddingleft,
      '!paddingright' => $dimensionparameters->paddingright,
    );

    // Create a temporary directory and move to it
    $drupaldir = getcwd();
    $tmpdir = tempnam(file_directory_temp(), 'drupal-video-'. $video->fid);
    unlink($tmpdir);
    mkdir($tmpdir, 0777);
    chdir($tmpdir);

    $result = TRUE;

    // process presets
    $converted_files = array();
    foreach ($video->presets as $preset) {
      // Preset settings
      $settings = $preset->getSettings();

      // Update our filename after the move to maintain filename uniqueness.
      $outputfile = file_create_filename(str_replace(' ', '_', pathinfo($video->filepath, PATHINFO_FILENAME)) . $preset->filenamesuffix .'.'. $preset->extension, $converteddir);
      $outputfilefull = $converteddirabsolute ? $outputfile : $drupaldir .'/'. $outputfile;
      $parameters['!convertfile'] = escapeshellarg($outputfilefull);

      $command_output = '';
      // Setup our default command to be run.
      foreach ($settings['commands'] as $cmdnr => $command) {
        $command = strtr($command, $parameters);

        // Process our video
        if (!$this->run_command($command, $command_output, t('rendering preset %preset, command #@cmdnr', array('%preset' => $preset->name, '@cmdnr' => $cmdnr + 1)))) {
          $result = FALSE;
          break 2;
        }
      }

      // Run flvtool2, if required
      if (!empty($settings['useflvtool2'])) {
        $command = $this->flvtoolcmd .' -U '. escapeshellarg($outputfilefull);
        $this->run_command($command, $command_output, t('running flvtool2'));
      }

      // Run qt-faststart, if required
      // qt-faststart needs a temporary file
      if (!empty($settings['useqtfaststart'])) {
        $tmpfile = $outputfilefull .'-qt';
        $command = $this->faststartcmd .' '. escapeshellarg($outputfilefull) .' '. escapeshellarg($tmpfile);

        // qt-faststart does not return an error code when it doesn't generate an output file,
        // so also check if the output file has been generated.
        if ($this->run_command($command, $command_output, t('running qt-faststart')) && file_exists($tmpfile)) {
          file_delete($outputfilefull);
          rename($tmpfile, $outputfilefull);
        }
        else {
          file_delete($tmpfile);
        }
      }

      // Lets check to make sure our file exists, if not error out
      if (!file_exists($outputfilefull) || ($filesize = filesize($outputfilefull)) === 0) {
        watchdog('video_command', 'Video conversion failed for preset %preset: result file was not found.', array('%preset' => $preset->name), WATCHDOG_ERROR);
        $result = FALSE;
        break;
      }

      // Create result object
      $converted_files[] = $file = new stdClass();
      $file->vid = intval($video->vid);
      $file->filename = basename($outputfile);
      $file->filepath = $outputfile;
      $file->filemime = file_get_mimetype($outputfile);
      $file->filesize = $filesize;
      $file->preset = $preset->name;
    }

    chdir($drupaldir);
    rmdirr($tmpdir);

    // Update our video_files table with the converted video information.
    if ($result) {
      $video->status = VIDEO_RENDERING_COMPLETE;
      $video->completed = time();
      $video->data = $converted_files;
      $result = db_query('UPDATE {video_files} SET status = %d, completed = %d, data = "%s" WHERE vid = %d', $video->status, $video->completed, serialize($video->data), $video->vid);

      // Prepare the watchdog statement
      $destinationfiles = array();
      foreach ($converted_files as $file) {
        $destinationfiles[] = $file->filepath;
      }
      watchdog('video_command', 'Successfully converted %orig to !destination-files', array('%orig' => $video->filepath, '!destination-files' => implode(', ', $destinationfiles)), WATCHDOG_INFO);
    }
    else {
      // Remove files that have been created
      foreach ($converted_files as $file) {
        file_delete($file->filepath);
      }

      $this->change_status($video, VIDEO_RENDERING_FAILED);
    }

    return $result;
  }

  /**
   * Get some information from the video file
   */
  private function get_video_info($filepath) {
    $command = $this->cmdpath .' -i '. escapeshellarg($filepath);

    // Execute the command
    $output = '';
    if ($this->run_command($command, $output, t('retrieving video info'), TRUE)) {
      return $output;
    }

    return NULL;
  }

  /**
   * Return the playtime seconds of a video
   */
  public function get_playtime($filepath) {
    $output = $this->get_video_info($filepath);

    $match = array();
    if (preg_match('/Duration: ([0-9]{2}):([0-9]{2}):([0-9]{2})\.([0-9])/', $output, $match)) {
      $hours = $match[1];
      $minutes = $match[2];
      $seconds = $match[3];

      if ($match[4] >= 5) { // fractional seconds
        $seconds++;
      }

      return $seconds + ($hours * 3600) + ($minutes * 60);
    }

    return NULL;
  }

  /**
   * Return the dimensions of a video
   */
  public function get_dimensions($filepath) {
    $output = $this->get_video_info($filepath);
    $match = array();

    if (preg_match('#(\d{2,4})x(\d{2,4})#', $output, $match)) {
      return array(
        'width' => intval($match[1]),
        'height' => intval($match[2]),
      );
    }

    return NULL;
  }

  public function get_name() {
    return $this->name;
  }

  public function get_value() {
    return $this->value;
  }

  public function create_job(stdClass $video) {
    $video->status = VIDEO_RENDERING_PENDING;
    return db_query('INSERT INTO {video_files} (fid, status, dimensions) VALUES (%d, %d, "%s")', $video->fid, VIDEO_RENDERING_PENDING, $video->dimensions);
  }

  public function update_job(stdClass $video) {
    if (!$this->load_job($video->fid)) {
      return;
    }

    // Lets update our table to include the nid
    db_query('UPDATE {video_files} SET nid = %d WHERE fid=%d', $video->nid, $video->fid);
  }

  public function delete_job(stdClass $video) {
    db_query('DELETE FROM {video_files} WHERE fid = %d', $video->fid);
  }

  public function load_job($fid) {
    $job = db_fetch_object(db_query('SELECT f.fid, f.filename, f.filepath, f.filesize, f.filemime, vf.vid, vf.nid, vf.dimensions, vf.status, vf.data FROM {video_files} vf LEFT JOIN {files} f ON vf.fid = f.fid WHERE f.fid=vf.fid AND f.fid = %d', $fid));
    if (!empty($job)) {
      $job->data = unserialize($job->data);
      return $job;
    }

    return FALSE;
  }

  public function load_job_queue($num) {
    $videos = array();

    $result = db_query_range('SELECT f.fid, f.filename, f.filepath, f.filemime, f.filesize, vf.vid, vf.nid, vf.dimensions, vf.status, vf.data FROM {video_files} vf LEFT JOIN {files} f ON vf.fid = f.fid WHERE vf.status = %d AND f.status = %d ORDER BY f.timestamp', VIDEO_RENDERING_PENDING, FILE_STATUS_PERMANENT, 0, $num);
    while ($row = db_fetch_object($result)) {
      $row->data = unserialize($row->data);
      $videos[] = $row;
    }

    return $videos;
  }

  public function load_completed_job(stdClass $video) {
    $result = db_fetch_object(db_query('SELECT data FROM {video_files} WHERE fid = %d', $video->fid));
    foreach (unserialize($result->data) as $value) {
      $extension = pathinfo($value->filepath, PATHINFO_EXTENSION);
      $video->files->{$extension} = new stdClass();
      $video->files->{$extension}->filename = pathinfo($value->filepath, PATHINFO_FILENAME) .'.'. $extension;
      $video->files->{$extension}->filepath = $value->filepath;
      $video->files->{$extension}->url = file_create_url($value->filepath);
      $video->files->{$extension}->extension = $extension;
      $video->files->{$extension}->filemime = file_get_mimetype($value->filepath);
      $video->player = drupal_strtolower($extension);
    }
    return $video;
  }

  /**
   * Change the status of the file.
   */
  public function change_status(stdClass $video, $status) {
    $video->status = $status;
    db_query('UPDATE {video_files} SET status = %d WHERE vid = %d', $status, $video->vid);
  }

  /**
   * Function determines the dimensions you want and compares with the actual wxh of the video.
   *
   * If they are not exact or the aspect ratio does not match, we then figure out how much padding
   * we should add.  We will either add a black bar on the top/bottom or on the left/right.
   *
   * The output width and height are divisible by two
   */
  public function getDimensionParameters(stdClass $video, array $sourcedimensions) {
    $wxh = explode('x', $video->dimensions, 2);
    $outputwidth = intval($wxh[0]);
    $outputheight = intval($wxh[1]);
    $outputratio = _video_aspect_ratio($outputwidth, $outputheight);

    $options = new stdClass();
    $options->paddingleft = 0;
    $options->paddingright = 0;
    $options->paddingtop = 0;
    $options->paddingbottom = 0;
    $options->width = intval(round($outputwidth / 2) * 2);
    $options->height = intval(round($outputheight / 2) * 2);
    $options->paddingwidth = $options->width;
    $options->paddingheight = $options->height;

    $filewidth = $sourcedimensions['width'];
    $fileheight = $sourcedimensions['height'];
    $fileratio = _video_aspect_ratio($filewidth, $fileheight);

    // If the input ratio is not equal to the output ratio
    if ($outputratio != $fileratio && $filewidth && $fileheight) {
      if ($fileratio < $outputratio) {
        // The input file is narrower than the output file
        // Change the output width proportionally
        $options->width = intval(round($filewidth * $outputheight / $fileheight / 2) * 2);

        // Determine left and right padding
        $padding = $outputwidth - $options->width;
        $options->paddingleft = intval(floor($padding / 2));
        $options->paddingright = intval(ceil($padding / 2));
      }
      else {
        // The input file is wider than the output file
        // Change the output height proportionally
        $options->height = intval(round($fileheight * $outputwidth / $filewidth / 2) * 2);

        // Determine top and bottom padding
        $padding = $outputheight - $options->height;
        $options->paddingtop = intval(floor($padding / 2));
        $options->paddingbottom = intval(ceil($padding / 2));
      }
    }

    return $options;
  }

  public function get_help() {
    return l(t('FFmpeg Online Manual'), 'http://www.ffmpeg.org/');
  }

  public function admin_settings(&$form_state) {
    $form = array();
    $form['video_localcommand_start'] = array(
      '#type' => 'markup',
      '#value' => '<div id="video_localcommand">',
    );

    $form['video_transcoder_path'] = array(
      '#type' => 'textfield',
      '#title' => t('Path to transcoder executable'),
      '#description' => t('Absolute path to your transcoder, such as FFmpeg. Do not include parameters.'),
      '#default_value' => $this->cmdpath,
    );
    $form['video_thumbs'] = array(
      '#type' => 'textfield',
      '#title' => t('Number of thumbnails'),
      '#description' => t('Number of thumbnails to generate for videos.'),
      '#default_value' => variable_get('video_thumbs', 5),
    );
    $form['video_localcommand_nice_enable'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enable the use of nice when calling the command.'),
      '#default_value' => $this->usenice,
      '#description' => t('The nice command invokes a command with an altered scheduling priority.'),
      '#access' => PHP_OS == 'Linux',
    );
    $form['video_localcommand_log_commands'] = array(
      '#type' => 'checkbox',
      '#title' => t('Log all executed commands to the Drupal log.'),
      '#default_value' => $this->logcommands,
      '#description' => t('Enable this option when debugging local transcoding to log all commands to the <a href="@dblog-page">Drupal log</a>. This may help with debugging problems. When this option is disabled, only errors will be logged.', array('@dblog-page' => url('admin/reports/dblog')))
    );
    // Thumbnail Videos We need to put this stuff last.
    $form['autothumb'] = array(
      '#type' => 'fieldset',
      '#title' => t('Thumbnails'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form['autothumb']['video_thumb_path'] = array(
      '#type' => 'textfield',
      '#title' => t('Path to save thumbnails'),
      '#description' => t('Path to save video thumbnails extracted from the videos.'),
      '#default_value' => variable_get('video_thumb_path', 'video_thumbs'),
    );
    $form['autothumb']['video_localcommand_thumbnailer_options'] = array(
      '#type' => 'textarea',
      '#title' => t('Video thumbnailer options'),
      '#description' => t('Provide the options for the thumbnailer.  Available argument values are:') .
        theme('item_list', array(
          '<strong>!seek</strong>: '. t('the number of seconds into the video where the screenshot is taken'),
          '<strong>!videofile</strong>: '. t('the video file to create thumbnails for'),
          '<strong>!thumbfile</strong>: '. t('a newly created temporary file to overwrite with the thumbnail'),
        )),
      '#default_value' => $this->thumbcmdoptions,
      '#wysiwyg' => FALSE,
    );

    // Video conversion settings.
    $form['helpers'] = array(
      '#type' => 'fieldset',
      '#title' => t('Helper programs'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE
    );
    $form['helpers']['video_localcommand_qtfaststart_cmd'] = array(
      '#type' => 'textfield',
      '#title' => t('Path to qt-faststart'),
      '#default_value' => $this->faststartcmd,
      '#description' => t('When you enter the path to qt-faststart here, you can enable the qt-faststart tool for one or more presets.'),
    );
    $form['helpers']['video_flvtool2_path'] = array(
      '#type' => 'textfield',
      '#title' => t('Path to flvtool2'),
      '#default_value' => $this->flvtoolcmd,
      '#description' => t('When you enter the path to flvtool2 here, you can enable the flvtool2 tool for one or more presets.'),
    );

    $form['video_localcommand_end'] = array(
      '#type' => 'markup',
      '#value' => '</div>',
    );
    return $form;
  }

  public function admin_settings_validate($form, &$form_state) {
    $v =& $form_state['values'];

    if (!empty($v['video_transcoder_path'])) {
      if (!is_file($v['video_transcoder_path'])) {
        form_error($form['video_transcoder_path'], t('The file %file does not exist.', array('%file' => $v['video_transcoder_path'])));
      }
    }
    elseif ($v['vid_convertor'] == 'video_localcommand') {
      form_error($form['video_transcoder_path'], t('!name field is required.', array('!name' => $form['video_transcoder_path']['#title'])));
    }

    if (!empty($v['video_localcommand_qtfaststart_cmd'])) {
      if (!is_file($v['video_localcommand_qtfaststart_cmd'])) {
        form_error($form['helpers']['video_localcommand_qtfaststart_cmd'], t('The file %file does not exist.', array('%file' => $v['video_localcommand_qtfaststart_cmd'])));
      }
    }

    if (!empty($v['video_flvtool2_path'])) {
      if (!is_file($v['video_flvtool2_path'])) {
        form_error($form['helpers']['video_flvtool2_path'], t('The file %file does not exist.', array('%file' => $v['video_flvtool2_path'])));
      }
    }
  }

  public function preset_settings(&$form_state, video_preset $preset) {
    $form = array();

    $settings = $preset->getSettings();

    if (!isset($settings['commands'])) {
      $settings['commands'] = array('');
    }

    // Add an empty entry box
    $settings['commands'][] = '';

    $form['commands'] = array(
      '#tree' => TRUE,
    );

    foreach ($settings['commands'] as $k => $command) {
      $form['commands'][$k] = array(
        '#type' => 'textarea',
        '#title' => t('Command @num', array('@num' => $k + 1)),
        '#default_value' => $command,
        '#rows' => 2,
        '#required' => $k === 0,
        '#wysiwyg' => FALSE,
      );
    }

    if (!empty($this->flvtoolcmd)) {
      $form['useflvtool2'] = array(
        '#type' => 'checkbox',
        '#title' => t('Post-process the output with flvtool2'),
        '#default_value' => !empty($settings['useflvtool2']),
      );
    }

    if (!empty($this->faststartcmd)) {
      $form['useqtfaststart'] = array(
        '#type' => 'checkbox',
        '#title' => t('Post-process the output with qt-faststart'),
        '#default_value' => !empty($settings['useqtfaststart']),
      );
    }

    $form['help'] = array(
      '#type' => 'item',
      '#title' => t('Notes'),
      '#value' =>
        '<p>'. t('The commands are executed in a temporary directory that is removed after the conversion has completed.') .'<br/>'.
          t('If you are using FFmpeg to transcode large videos, consider adding <code>-nostats</code> to your command line(s) to reduce memory usage while transcoding.') .'<br/>'.
          t('Submit this form and revisit this page to add more commands.') .'</p>'.
        '<p>'. t('You can use the following replacement variables in your commands:') .'</p>'.
        theme('item_list', array(
          '<strong>!cmd_path</strong>: '. t('the path to your executable (currently: %executable).', array('%executable' => $this->cmdpath)),
          '<strong>!videofile</strong>: '. t('the escaped absolute path to the input video file.'),
          '<strong>!convertfile</strong>: '. t('the escaped absolute path to the output video file.'),
          '<strong>!width</strong>: '. t('the width of the output video file, excluding padding, for use with -s.'),
          '<strong>!height</strong>: '. t('the height of the output video file, excluding padding, for use with -s.'),
          '<strong>!paddingwidth</strong>: '. t('the width of the output video file, including padding, for use as first parameter of -vf pad.'),
          '<strong>!paddingheight</strong>: '. t('the height of the output video file, including padding, for use as second parameter of -vf pad.'),
          '<strong>!paddingtop</strong>: '. t('the padding added to the top of the output video file, for use with -padtop or as third parameter of -vf pad.'),
          '<strong>!paddingbottom</strong>: '. t('the padding added to the bottom of the output video file, for use with -padbottom.'),
          '<strong>!paddingleft</strong>: '. t('the padding added to the left of the output video file, for use with -padleft or as fourth parameter of -vf pad.'),
          '<strong>!paddingright</strong>: '. t('the padding added to the right of the output video file, for use with -padright.'),
        )),
    );

    return $form;
  }

  public function preset_settings_validate($form, &$form_state) {
    if (!empty($form_state['values']['commands'])) {
      foreach ($form_state['values']['commands'] as $k => $v) {
        if (empty($v)) {
          unset($form_state['values']['commands'][$k]);
        }
      }

      $form_state['values']['commands'] = array_values($form_state['values']['commands']);
    }

    if (!empty($form['useflvtool2'])) {
      $form['useflvtool2'] = TRUE;
    }

    if (!empty($form['useqtfaststart'])) {
      $form['useqtfaststart'] = TRUE;
    }
  }

  public function requirements() {
    $requirements = array();
    $t = get_t();

    if (strpos($this->cmdpath, ' -') !== FALSE) {
      $requirements['video_localcommand_args'] = array(
        'title' => $t('Video local command'),
        'description' => $t('It is no longer necessary to add command line arguments to the %setting setting. You can add command line arguments per preset.', array('%setting' => t('Path to transcoder executable'))),
        'value' => l($t('Change setting'), 'admin/settings/video/transcoders'),
        'severity' => REQUIREMENT_WARNING,
      );
    }
    elseif (!file_exists($this->cmdpath)) {
      $requirements['video_localcommand_args'] = array(
        'title' => $t('Video local command'),
        'description' => $t('The file %cmdpath can\'t be found. Please change your settings.', array('%cmdpath' => $this->cmdpath)),
        'value' => l($t('Change setting'), 'admin/settings/video/transcoders'),
        'severity' => REQUIREMENT_ERROR,
      );
    }
    elseif (!is_executable($this->cmdpath)) {
      $requirements['video_localcommand_args'] = array(
        'title' => $t('Video local command'),
        'description' => $t('The file %cmdpath is not an executable. Please change your settings.', array('%cmdpath' => $this->cmdpath)),
        'value' => l($t('Change setting'), 'admin/settings/video/transcoders'),
        'severity' => REQUIREMENT_ERROR,
      );
    }

    return $requirements;
  }

  /**
   * Implementation of get_original_path_by_converted_path().
   */
  public function get_original_path_by_converted_path($filepath) {
    // This is ugly, but there is no other way
    $videos = db_query('SELECT fid, data FROM {video_files} WHERE status = %d AND data LIKE "%%%s%%"', VIDEO_RENDERING_COMPLETE, $filepath);
    $fid = NULL;
    while ($video = db_fetch_object($videos)) {
      foreach (unserialize($video->data) as $file) {
        if ($file->filepath == $filepath) {
          $fid = $video->fid;
          break 2;
        }
      }
    }

    if ($fid == NULL) {
      return NULL;
    }

    return $this->load_job($fid);
  }
}
