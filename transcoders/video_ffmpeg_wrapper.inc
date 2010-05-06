<?php
//$Id$
/*
 * @file
 * Transcoder class file to handle ffmpeg_wrapper settings and conversions.
 *
 */


class video_ffmpeg_wrapper implements transcoder_interface {
  private $name = 'FFmpeg Wrapper';
  private $value = 'video_ffmpeg_wrapper';

  protected $thumb_command = '-i !videofile -an -y -f mjpeg -ss !seek -vframes 1 !thumbfile';

  public function __construct() {
    $this->params['thumb_command'] = variable_get('video_ffmpeg_thumbnailer_options', $this->thumb_command);
  }

  public function run_command($options) {
    $output = ffmpeg_wrapper_run_command($options);
    return $output;
  }

  public function generate_thumbnails($video) {
    global $user;
    // Setup our thmbnail path.
    $video_thumb_path = variable_get('video_thumb_path', 'video_thumbs');
    $final_thumb_path = file_directory_path(). '/' . $video_thumb_path . '/' . $video['fid'];

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
    for($i = 1; $i <= $total_thumbs; $i++) {
      $seek = ($duration/$total_thumbs) * $i;
      $filename = "/video-thumb-for-$fid-$i.jpg";
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
          watchdog('video_transcoder',$error_msg, array(), WATCHDOG_ERROR);
          continue;
        }
      }
      // Begin building the file object.
      // @TODO : use file_munge_filename()
      $file = new stdClass();
      $file->uid      = $user->uid;
      $file->status   = FILE_STATUS_TEMPORARY;
      $file->filename = trim($filename);
      $file->filepath = $thumbfile;
      $file->filemime = file_get_mimetype($filename);
      $file->filesize = filesize($thumbfile);
      $file->timestamp = time();
      $files[] = $file;
    }
    return $files;
  }

  public function convert_video(&$job) {
    $ffmpeg_object = new stdClass();
    // check configuration are pass of then use global $conf
    if (empty($params)) {
      global $conf;
      $params = $conf;
    }

    $input_file = getcwd() .'/'. $job->filepath;
    // first error check, make sure that we can decode this kind of file
    if (!ffmpeg_wrapper_can_decode($input_file)) {
      $message = 'FFmpeg Wrapper can not decode this file: !file';
      $variables = array('!file' => l($input_file, file_create_url($input_file)));
      watchdog('video_render', $message, $variables, WATCHDOG_ERROR);
      return false;
    }
    // build the output file path if we don't have one. Use the output type as the extension.
    $output_file = file_create_filename(basename($input_file) .'.'. $params['ffmpeg_output_type'], ($output_file_path ? $output_file_path : dirname($input_file)));

    // did the admin define a specific FFmpeg comand to run?
    //  we only run what the admin specified
    if ($params['ffmpeg_video_custom']) {
      $options[] = str_replace(array('%in_file', '%out_file'), array($input_file, $output_file), $params['ffmpeg_video_custom_command']);
    }
    // build a standard configuration
    else {
      // build the ffmpeg command structure out
      $options = array();
      // input file
      $options[] = "-i '". $input_file ."'";
      // build the watermark config
      if ($params['ffmpeg_video_wm']) {
        $options[] = "-vhook '". ffmpeg_wrapper_path_to_vhook('watermark.so') ." -f ". $params['ffmpeg_video_wm_file'] ."'";
      }
      // build the audio config
      if ($params['ffmpeg_audio_advanced']) {
        // use a specifc codec?
        if ($params['ffmpeg_audio_acodec']) {
          $options[] =  '-acodec '. $params['ffmpeg_audio_acodec'];
        }
        // use a specific sample rate?
        if ($params['ffmpeg_audio_ar'] ) {
          $options[] = '-ar '. $params['ffmpeg_audio_ar'];
        }
        // use a specific bit rate?
        if ($params['ffmpeg_audio_ab']) {
          $options[] = '-ab '. $params['ffmpeg_audio_ab'];
        }
      }

      // build the video config
      if ($params['ffmpeg_video_advanced']) {
        // is codec set?
        if ($params['ffmpeg_video_vcodec']) {
          $options[] = '-vcodec '. $params['ffmpeg_video_vcodec'];
        }
        // is frame size set?
        if ($params['ffmpeg_video_size']) {
          $options[] = '-s '. $params[$params['ffmpeg_video_size'] == 'other' ? 'ffmpeg_video_size_other' : 'ffmpeg_video_size'];
        }
        // is the bit rate set?
        if ($params['ffmpeg_video_br']) {
          $options[] = '-b '. $params['ffmpeg_video_br'];
        }
        // is frame rate set?
        if ($params['ffmpeg_video_fps']) {
          $options[] = '-r '. $params['ffmpeg_video_fps'];
        }
      }
      // implement truncating
      if ($params['ffmpeg_time_advanced']) {
        $options[] = '-t '. $params['ffmpeg_time'];
      }
      // add the output file
      $options[] = "'". $output_file ."'";
    }
    $ffmpeg_object->command = implode(" ", $options);

    // run ffmpeg with error checking
    if (!$success = $this->run_command($ffmpeg_object->command)) {
      watchdog('video_conversion', 'video conversion failed. ffmpeg reported the following output: ' . $success);
      return false;
    }

    // successful convert, make a note in the log
    $message = 'FFmpeg converted this file: @file';
    $message .= '<br />'. 'FFmpeg ran this command: <br /><pre> !command </pre>';
    $variables = array('@file' => $output_file, '!command' =>  $ffmpeg_object->command);
    watchdog('video_transcoder', $message, $variables, WATCHDOG_NOTICE);

    $ffmpeg_object->output_file = $output_file;

    if (!file_exists($output_file) || !filesize($output_file)) {
      watchdog('video_transcoder', 'Video conversion failed. FFmpeg reported the following output: ' . $command_output);
    }
    else {
      $file_name = basename($output_file);
      $file = new stdClass();
      $file->uid      = $job->uid;
      $file->status   = FILE_STATUS_PERMANENT;
      $file->filename = basename($file_name);
      $file->filepath = $output_file;
      $file->filemime = file_get_mimetype($file_name);
      $file->filesize = filesize($output_file);
      $file->timestamp = time();
      $job->converted = $file;
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
    if(!module_exists('ffmpeg_wrapper')) {
      $help .= ' <span class="error">'.t('You currently do not have this installed.').'</span>';
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
      '#prefix' => '<div id="'. $this->value .'">',
      '#suffix' => '</div>',
      );
    }
    else {
      $form['video_ffmpeg_wrapper_start'] = array(
        '#type' => 'markup',
        '#value' => '<div id="video_ffmpeg_wrapper">',
      );
      if(module_exists('ffmpeg_wrapper_ui')) {
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
}