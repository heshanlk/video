<?php
//$Id$
/*
 * @file
 * Transcoder class file to handle ffmpeg settings and conversions.
 *
 */


class video_ffmpeg implements transcoder_interface {
  // Naming for our radio options.  Makes it easy to extend our transcoders.
  private $name = 'FFmpeg';
  private $value = 'video_ffmpeg';
  
  protected $params = array();
  protected $audio_bitrate = 64;
  protected $video_bitrate = 200;
  protected $video_width = 640;
  protected $video_height = 480;
  protected $command = '-y -i !videofile -f flv -ar 22050 -ab !audiobitrate -s !size -b !videobitrate -qscale 1 !convertfile';
  protected $thumb_command = '-i !videofile -an -y -f mjpeg -ss !seek -vframes 1 !thumbfile';
  protected $nice = 'nice -n 19';
  protected $ffmpeg = '/usr/bin/ffmpeg';

  public function __construct() {
    $this->params['audiobitrate'] = variable_get('video_ffmpeg_helper_auto_cvr_audio_bitrate', $this->audio_bitrate);
    $this->params['videobitrate'] = variable_get('video_ffmpeg_helper_auto_cvr_video_bitrate', $this->video_bitrate);
    //@todo: move this to the actual widget and save in video_files table.
    $this->params['size'] = variable_get('video_ffmpeg_width', $this->video_width) . 'x' . variable_get('video_ffmpeg_height', $this->video_height);
    $this->params['command'] = variable_get('video_ffmpeg_helper_auto_cvr_options', $this->command);
    $this->params['cmd_path'] = variable_get('video_transcoder_path', $this->ffmpeg);
    $this->params['thumb_command'] = variable_get('video_ffmpeg_thumbnailer_options', $this->thumb_command);
  }

  public function run_command($options) {
    $command = $this->nice .' '. $this->params['cmd_path'].' '.$options.'  2>&1';
    watchdog('video_ffmpeg', 'Executing command: '. $command, array(), WATCHDOG_DEBUG);
    ob_start();
    passthru($command, $command_return);
    $output = ob_get_contents();
    ob_end_clean();
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
    $dimensions = $this->get_dimensions($video);

    $files = NULL;
    for($i = 1; $i <= $total_thumbs; $i++) {
      $seek = ($duration/$total_thumbs) * $i -1;  //adding minus one to prevent seek times equaling the last second of the video
      $filename = "/video-thumb-for-$fid-$i.jpg";
      $thumbfile = $final_thumb_path . $filename;
      //skip files already exists, this will save ffmpeg traffic
      if (!is_file($thumbfile)) {
        //setup the command to be passed to the transcoder.
        $options = t($this->params['thumb_command'], array('!videofile' => $videofile, '!seek' => $seek, '!thumbfile' => $thumbfile));
        // Generate the thumbnail from the video.
        $command_output = $this->run_command($options);
        if (!file_exists($thumbfile)) {
          $error_param = array('%file' => $thumbfile, '%cmd' => $options, '%out' => $command_output);
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
  
  public function convert_video($video, $converted) {
    // Setup our command to be run.
    $command = t($this->params['command'], array(
      '!videofile' => $video->filepath,
      '!audiobitrate' => $this->params['audiobitrate'],
      '!size' => $video->dimensions,
      '!videobitrate' => $this->params['videobitrate'],
      '!convertfile' => $converted
    ));
    //Convert the video with our transcoder.
    return $this->run_command($command);
  }

  /**
   * Get some information from the video file
   */
  public function get_video_info($video) {
    static $command_ouput;
    if(!empty($command_output)) return $command_output;

    $file = escapeshellarg($video['filepath']);
    // Execute the command
    $options = ' -i ' . $file;
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
    $res = array('width' => 0,'height' => 0);
    // Get dimensions
    $regex = ereg ('[0-9]?[0-9][0-9][0-9]x[0-9][0-9][0-9][0-9]?', $ffmpeg_output, $regs );
    if(isset($regs[0])) {
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
    $form['video_transcoder_path'] = array(
      '#type' => 'textfield',
      '#title' => t('Path to Video Transcoder'),
      '#description' => t('Absolute path to ffmpeg.'),
      '#default_value' => variable_get('video_transcoder_path', '/usr/bin/ffmpeg'),
    );
    $form['no_of_video_thumbs'] = array(
      '#type' => 'textfield',
      '#title' => t('Number of thumbnails'),
      '#description' => t('Number of thumbnails to display from video.'),
      '#default_value' => variable_get('no_of_video_thumbs', 5),
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
        '#description' => t('Provide the options for the thumbnailer.  Available argument values are: ').'<ol><li>'.t('!videofile (the video file to thumbnail)').'<li>'.t('!thumbfile (a newly created temporary file to overwrite with the thumbnail)</ol>'),
        '#default_value' => variable_get('video_ffmpeg_thumbnailer_options', '-i !videofile -an -y -f mjpeg -ss !seek -vframes 1 !thumbfile'),
    );

    // Video conversion settings.
    $form['autoconv'] = array(
        '#type' => 'fieldset',
        '#title' => t('Video Conversion'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
        '#prefix' => '<div id="video_ffmpeg">',
        '#suffix' => '</div>'
    );
    $form['autoconv']['video_ffmpeg_width'] = array(
        '#type' => 'textfield',
        '#title' => t('Video Width'),
        '#size' => 10,
        '#maxlength' => 10,
        '#default_value' => variable_get('video_ffmpeg_width', 640),
    );
    $form['autoconv']['video_ffmpeg_height'] = array(
        '#type' => 'textfield',
        '#title' => t('Video height'),
        '#size' => 10,
        '#maxlength' => 10,
        '#default_value' => variable_get('video_ffmpeg_height', 480),
    );
    $form['autoconv']['video_ffmpeg_helper_auto_cvr_video_bitrate'] = array(
        '#type' => 'textfield',
        '#title' => t('Video bitrate'),
        '#description' => t('The video bitrate in bit/s of the converted video.'),
        '#size' => 10,
        '#maxlength' => 10,
        '#default_value' => variable_get('video_ffmpeg_helper_auto_cvr_video_bitrate', 200000),
    );
    $form['autoconv']['video_ffmpeg_helper_auto_cvr_audio_bitrate'] = array(
        '#type' => 'textfield',
        '#title' => t('Audio bitrate'),
        '#description' => t('The audio bitrate in bit/s of the converted video.'),
        '#size' => 10,
        '#maxlength' => 10,
        '#default_value' => variable_get('video_ffmpeg_helper_auto_cvr_audio_bitrate', 64000),
    );
    $form['autoconv']['advanced'] = array(
        '#type' => 'fieldset',
        '#title' => t('Advanced Settings'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE
    );
    $form['autoconv']['advanced']['video_ffmpeg_helper_auto_cvr_options'] = array(
        '#type' => 'textarea',
        '#title' => t('Video converter options'),
        '#description' => t('Provide the ffmpeg options to configure the video conversion.  Available argument values are: ').'<ul>'.
        '<li>'.t('!videofile (the video file to convert)').
        '<li>'.t('!convertfile (a newly created file to store the converted file)').
        '<li>'.t('!size (video resolution of the converted file)').
        '</ul>'.t('For further informations refer to the !ffmpegdoc', array('!ffmpegdoc' => l(t('Official FFMpeg documentation.'), 'http://ffmpeg.mplayerhq.hu/ffmpeg-doc.html', array('fragment' => TRUE)))),
        '#default_value' => variable_get('video_ffmpeg_helper_auto_cvr_options', '-y -i !videofile -f flv -ar 22050 -ab !audiobitrate -s !size -b !videobitrate -qscale 1 !convertfile'),
    );
    return $form;
  }
}
?>
